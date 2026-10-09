#!/usr/bin/env python3
"""Exercise the incident supervisor with synthetic journal data and CPU-only child processes."""

from __future__ import annotations

import contextlib
import fcntl
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

MODULE_PATH = Path(__file__).resolve().parent.parent / "tools/gpu_incident_qualification.py"
SPEC = importlib.util.spec_from_file_location("gpu_qualification", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
GPU = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GPU)


def record(cursor="initial", message="Linux kernel baseline"):
    return {"_TRANSPORT": "kernel", "_BOOT_ID": "test-boot", "__CURSOR": cursor,
            "__REALTIME_TIMESTAMP": "1791414767000000", "MESSAGE": message}


def process_state(path: Path):
    """A process may disappear after procfs open but before read, returning ESRCH, not ENOENT."""
    try:
        return path.read_text().rsplit(")", 1)[1].split()[0]
    except (FileNotFoundError, ProcessLookupError):
        return None


class SyntheticJournal:
    def __init__(self, marker: Path, mode="clean"):
        self.marker = marker
        self.mode = mode
        self.fault_sent = False

    def latest(self):
        if self.mode == "no-access" or (self.marker.exists() and self.mode == "lost-access"):
            raise GPU.MonitorError("no current kernel record")
        if self.marker.exists() and self.mode == "lost-cursor":
            return record("missing")
        if self.marker.exists() and self.mode == "interrupt":
            os.kill(os.getpid(), signal.SIGTERM)
        return record()

    def read(self, *_arguments):
        if self.marker.exists() and self.mode == "fault" and not self.fault_sent:
            self.fault_sent = True
            return [record("fault", "amdgpu: [gfxhub] page fault at address 0x7000")]
        return []


class QualificationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="island-gpu-monitor-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.marker = self.root / "started.json"
        self.runner = self.root / "private-runner"
        self.runner.write_text('#!/bin/sh\nshift\nexec "$@"\n', encoding="utf-8")
        self.runner.chmod(0o700)
        self.game = self.root / "island-simulator"
        self.write_game()

    def write_game(self, *, persistent=False, exit_code=0, diagnostic="", orphan=False,
                   detached=False):
        child_marker = self.root / "child.json"
        child_code = ("import os,signal,time; from pathlib import Path; "
                      "signal.signal(signal.SIGTERM, signal.SIG_IGN); "
                      f"Path({str(child_marker)!r}).write_text(os.readlink('/proc/self')); "
                      "time.sleep(30)")
        self.game.write_text(
            f"#!{sys.executable}\n"
            "import json, os, signal, subprocess, sys, time\n"
            "from pathlib import Path\n"
            "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
            f"child = subprocess.Popen([sys.executable, '-c', {child_code!r}], "
            f"start_new_session={detached!r})\n"
            f"while not Path({str(child_marker)!r}).exists(): time.sleep(0.001)\n"
            f"Path({str(self.marker)!r}).write_text(json.dumps({{'parent': int(os.readlink('/proc/self')), "
            f"'child': int(Path({str(child_marker)!r}).read_text()), 'argv': sys.argv}}))\n"
            f"print({diagnostic!r}, flush=True)\n"
            + ("time.sleep(30)\n" if persistent else "" if orphan else "child.kill()\nchild.wait()\n")
            + f"sys.exit({exit_code})\n", encoding="utf-8")
        self.game.chmod(0o700)

    def run_case(self, mode="clean", timeout=4.0):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return GPU.qualify(self.root, "benchmark-jungle", self.runner,
                               journal=SyntheticJournal(self.marker, mode),
                               timeout=timeout, poll_interval=0.02)

    def assert_owned_processes_stopped(self):
        pids = json.loads(self.marker.read_text())
        for key in ("parent", "child"):
            path = Path(f"/proc/{pids[key]}/stat")
            for _ in range(100):
                state = process_state(path)
                if state is None or state == "Z":  # Reaping belongs to init after the synthetic parent dies.
                    break
                time.sleep(0.01)
            else:
                os.kill(pids[key], signal.SIGKILL)
                self.fail(f"owned {key} process survived supervisor shutdown")

    def test_no_journal_access_never_starts_workload(self):
        self.assertEqual(self.run_case("no-access"), 3)
        self.assertFalse(self.marker.exists())

    def test_missing_containment_never_starts_workload(self):
        with patch.object(GPU.shutil, "which", return_value=None):
            self.assertEqual(self.run_case(), 3)
        self.assertFalse(self.marker.exists())

    def test_detached_child_dies_after_normal_initial_exit(self):
        self.write_game(orphan=True, detached=True)
        self.assertEqual(self.run_case(), 0)
        self.assert_owned_processes_stopped()

    def test_detached_child_dies_after_fault(self):
        self.write_game(persistent=True, detached=True)
        self.assertEqual(self.run_case("fault"), 4)
        self.assert_owned_processes_stopped()

    def test_detached_child_dies_after_timeout(self):
        self.write_game(persistent=True, detached=True)
        self.assertEqual(self.run_case(timeout=0.5), 4)
        self.assert_owned_processes_stopped()

    def test_detached_child_dies_after_interruption(self):
        self.write_game(persistent=True, detached=True)
        self.assertEqual(self.run_case("interrupt"), 4)
        self.assert_owned_processes_stopped()

    def test_unrelated_process_survives_contained_cleanup(self):
        sentinel = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(30)"],
                                    start_new_session=True)
        try:
            self.write_game(persistent=True, detached=True)
            self.assertEqual(self.run_case("fault"), 4)
            self.assert_owned_processes_stopped()
            self.assertIsNone(sentinel.poll())
        finally:
            sentinel.kill()
            sentinel.wait(timeout=2)

    def test_double_fork_and_setsid_descendant_cannot_escape(self):
        self.game.write_text(
            f"#!{sys.executable}\n"
            "import json,os,signal,time\nfrom pathlib import Path\n"
            "parent = int(os.readlink('/proc/self'))\n"
            "child = os.fork()\n"
            "if child == 0:\n"
            "    os.setsid()\n"
            "    if os.fork() != 0: os._exit(0)\n"
            "    signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
            f"    Path({str(self.marker)!r}).write_text(json.dumps({{'parent': parent, "
            "'child': int(os.readlink('/proc/self'))}))\n"
            "    time.sleep(30)\n"
            "    os._exit(0)\n"
            "os.waitpid(child, 0)\n"
            f"while not Path({str(self.marker)!r}).exists(): time.sleep(0.001)\n",
            encoding="utf-8")
        self.assertEqual(self.run_case(), 0)
        self.assert_owned_processes_stopped()

    def test_supervisor_sigkill_stops_detached_descendants(self):
        self.write_game(persistent=True, detached=True)
        launcher_file = self.root / "launcher.pid"
        code = ("import subprocess,time; from pathlib import Path; "
                f"p=subprocess.Popen({GPU.contained_command([str(self.game)])!r}, "
                "start_new_session=True); "
                f"Path({str(launcher_file)!r}).write_text(str(p.pid)); p.wait()")
        supervisor = subprocess.Popen([sys.executable, "-c", code], start_new_session=True,
                                      stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            deadline = time.monotonic() + 4
            while not self.marker.exists() and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertTrue(self.marker.exists(), "contained CPU fixture did not start")
            supervisor.kill()  # No Python finally block or wrapper trap can run.
            supervisor.wait(timeout=2)
            self.assert_owned_processes_stopped()
            launcher = Path(f"/proc/{int(launcher_file.read_text())}/stat")
            self.assertIn(process_state(launcher), (None, "Z"))
        finally:
            if supervisor.poll() is None:
                supervisor.kill()
                supervisor.wait(timeout=2)

    def test_clean_run_preserves_exact_command_and_kernel_baseline(self):
        self.assertEqual(self.run_case(), 0)
        metadata_path, = (self.root / "gpu-incident-evidence").glob("*/run-metadata.json")
        metadata = json.loads(metadata_path.read_text())
        self.assertEqual(metadata["kernel_baseline"], record())
        self.assertEqual(metadata["command"][-2:], ["--viewpoint", "jungle"])
        self.assertEqual(metadata["status"], 0)
        artifacts = metadata["artifacts_before_launch"]
        self.assertEqual(artifacts["game"]["sha256"], hashlib.sha256(self.game.read_bytes()).hexdigest())
        self.assertEqual(artifacts["game"]["size_bytes"], self.game.stat().st_size)
        self.assertEqual(artifacts["game"]["path"], str(self.game.resolve()))
        self.assertEqual(artifacts["private_display_runner"]["sha256"],
                         hashlib.sha256(self.runner.read_bytes()).hexdigest())
        self.assertIsNone(artifacts["cmake_cache"])
        self.assert_owned_processes_stopped()

    def test_cache_identity_is_recorded_when_available(self):
        cache = self.root / "CMakeCache.txt"
        cache.write_text("CNA_GRAPHICS_RENDERER:STRING=OPENGL33\n", encoding="utf-8")
        self.assertEqual(self.run_case(), 0)
        metadata_path, = (self.root / "gpu-incident-evidence").glob("*/run-metadata.json")
        metadata = json.loads(metadata_path.read_text())
        self.assertEqual(metadata["artifacts_before_launch"]["cmake_cache"]["sha256"],
                         hashlib.sha256(cache.read_bytes()).hexdigest())
        self.assert_owned_processes_stopped()

    def test_unreadable_artifact_prevents_launch(self):
        with patch.object(GPU, "artifact_identity", side_effect=PermissionError("not readable")):
            self.assertEqual(self.run_case(), 3)
        self.assertFalse(self.marker.exists())

    def test_changed_artifact_prevents_launch(self):
        with patch.object(GPU, "artifact_identity", side_effect=GPU.MonitorError("changed")):
            self.assertEqual(self.run_case(), 3)
        self.assertFalse(self.marker.exists())

    def test_live_fault_kills_entire_group_and_latches_even_if_final_delta_empty(self):
        self.write_game(persistent=True)
        self.assertEqual(self.run_case("fault"), 4)
        self.assert_owned_processes_stopped()
        stop = self.root / "gpu-incident-evidence/QUALIFICATION_STOP.json"
        self.assertIn("GPU fault", stop.read_text())
        self.assertEqual(self.run_case(), 3)
        self.assertEqual(len(list(stop.parent.glob("*/run-metadata.json"))), 1)

    def test_lost_monitor_access_stops_workload(self):
        self.write_game(persistent=True)
        self.assertEqual(self.run_case("lost-access"), 4)
        self.assert_owned_processes_stopped()

    def test_lost_cursor_stops_workload(self):
        self.write_game(persistent=True)
        self.assertEqual(self.run_case("lost-cursor"), 4)
        self.assert_owned_processes_stopped()

    def test_timeout_stops_workload(self):
        self.write_game(persistent=True)
        self.assertEqual(self.run_case(timeout=0.2), 4)
        self.assert_owned_processes_stopped()

    def test_interruption_stops_workload(self):
        self.write_game(persistent=True)
        self.assertEqual(self.run_case("interrupt"), 4)
        self.assert_owned_processes_stopped()

    def test_exited_wrapper_cannot_leave_a_child_running(self):
        self.write_game(orphan=True)
        self.assertEqual(self.run_case(), 0)
        self.assert_owned_processes_stopped()

    def test_second_invocation_cannot_start_concurrently(self):
        with (self.root / "gpu-qualification.lock").open("w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.assertEqual(self.run_case(), 3)
        self.assertFalse(self.marker.exists())

    def test_failed_workload_is_not_qualified(self):
        self.write_game(exit_code=7)
        self.assertEqual(self.run_case(), 7)

    def test_high_severity_gl_diagnostic_is_not_qualified(self):
        self.write_game(diagnostic="island-simulator: gl-debug frame=1 severity=high message=bad")
        self.assertEqual(self.run_case(), 4)

    def test_invalid_workload_never_starts(self):
        with self.assertRaises(ValueError):
            GPU.qualify(self.root, "capture-../../invalid", self.runner)
        self.assertFalse(self.marker.exists())


class PreflightTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="island-preflight-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.dump_root = self.root / "devcoredump"
        self.dump_root.mkdir()
        self.stop = self.root / "gpu-incident-evidence/QUALIFICATION_STOP.json"
        self.stop.parent.mkdir()
        self.original = '{"evidence":"preserved/run", "failure":"GPU fault"}\n'
        self.stop.write_text(self.original)

    def inspect(self, mode="clean"):
        with patch.object(GPU.subprocess, "Popen", side_effect=AssertionError("unexpected launch")):
            result = GPU.preflight(self.root, dump_root=self.dump_root,
                                   journal=SyntheticJournal(self.root / "unused", mode))
        self.assertFalse(result["workload_launched"])
        self.assertFalse(result["hardware_run_authorized"])
        self.assertEqual(result["hardware_qualification"], "not-performed")
        self.assertFalse((self.root / "gpu-qualification.lock").exists())
        self.assertEqual(list(self.stop.parent.iterdir()), [self.stop] if self.stop.exists() else [])
        return result

    def test_active_latch_is_preserved_and_no_workload_or_payload_is_opened(self):
        entry = self.dump_root / "devcd3"
        entry.mkdir()
        (entry / "data").write_text("must not be read")
        (self.dump_root / "disabled").write_text("0")
        original_open = Path.open

        def guard_payload(path, *args, **kwargs):
            if path == entry / "data":
                raise AssertionError("preflight must not read device dump payloads")
            return original_open(path, *args, **kwargs)

        with patch.object(Path, "open", guard_payload):
            result = self.inspect()
        self.assertEqual(result["errors"], [])
        self.assertEqual(result["stop_latch"], "active")
        self.assertEqual(result["device_dump_entries"], ["devcd3"])
        self.assertEqual(result["journal_access"], "verified")
        self.assertEqual(result["kernel_latest_record"], record())
        self.assertEqual(self.stop.read_text(), self.original)

    def test_absent_latch_never_authorizes_hardware(self):
        self.stop.unlink()
        result = self.inspect()
        self.assertEqual(result["stop_latch"], "absent-not-authorization")
        self.assertEqual(result["errors"], [])

    def test_invalid_latch_is_reported_and_not_replaced(self):
        for payload in ("{broken", "[]", '{}', '{"failure":0,"evidence":"x"}'):
            with self.subTest(payload=payload):
                self.stop.write_text(payload)
                result = self.inspect()
                self.assertEqual(result["stop_latch"], "unreadable-or-invalid")
                self.assertTrue(result["errors"])
                self.assertEqual(self.stop.read_text(), payload)

    def test_journal_permission_failure_does_not_trigger_a_workload(self):
        result = self.inspect("no-access")
        self.assertEqual(result["journal_access"], "unverified")
        self.assertTrue(result["errors"])

    def test_absent_dump_interface_is_explicit(self):
        self.dump_root.rmdir()
        result = self.inspect()
        self.assertEqual(result["device_dump_interface"], "absent")
        self.assertEqual(result["device_dump_entries"], [])

    def test_dump_directory_access_failure_is_not_reported_as_empty(self):
        with patch.object(Path, "iterdir", side_effect=PermissionError("denied")):
            result = GPU.preflight(self.root, dump_root=self.dump_root,
                                   journal=SyntheticJournal(self.root / "unused"))
        self.assertIsNone(result["device_dump_entries"])
        self.assertTrue(result["errors"])

    def test_cli_dispatch_cannot_enter_qualification(self):
        for errors, status in (([], 0), (["unreadable"], 3)):
            with self.subTest(errors=errors), patch.object(
                    GPU.sys, "argv", ["supervisor", str(self.root), "preflight"]), \
                    patch.object(GPU, "preflight", return_value={"errors": errors}), \
                    patch.object(GPU, "qualify", side_effect=AssertionError("unexpected qualification")), \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(GPU.main(), status)


class ArtifactIdentityTests(unittest.TestCase):
    def test_in_place_modification_during_hash_is_rejected(self):
        with tempfile.TemporaryDirectory(prefix="island-artifact-test-") as directory:
            artifact = Path(directory) / "game"
            artifact.write_bytes(b"original")
            digest = hashlib.sha256(b"original")

            def modify(_stream):
                artifact.write_bytes(b"different executable")
                return digest.hexdigest()

            with patch.object(GPU, "sha256_stream", side_effect=modify):
                with self.assertRaises(GPU.MonitorError):
                    GPU.artifact_identity(artifact)

    def test_atomic_replacement_during_hash_is_rejected(self):
        with tempfile.TemporaryDirectory(prefix="island-artifact-test-") as directory:
            artifact = Path(directory) / "game"
            artifact.write_bytes(b"original")
            replacement = Path(directory) / "replacement"
            replacement.write_bytes(b"replacement")

            def replace(_stream):
                replacement.replace(artifact)
                return hashlib.sha256(b"original").hexdigest()

            with patch.object(GPU, "sha256_stream", side_effect=replace):
                with self.assertRaises(GPU.MonitorError):
                    GPU.artifact_identity(artifact)


class ProcessStateTests(unittest.TestCase):
    def test_exit_during_open_or_read_is_stopped(self):
        for error in (FileNotFoundError(), ProcessLookupError()):
            with self.subTest(error=type(error).__name__), patch.object(Path, "read_text", side_effect=error):
                self.assertIsNone(process_state(Path("/proc/test/stat")))

    def test_permission_failure_is_not_mistaken_for_exit(self):
        with patch.object(Path, "read_text", side_effect=PermissionError()):
            with self.assertRaises(PermissionError):
                process_state(Path("/proc/test/stat"))

    def test_parenthesized_process_name_does_not_change_state(self):
        with patch.object(Path, "read_text", return_value="123 (test) child) S 1 2 3"):
            self.assertEqual(process_state(Path("/proc/test/stat")), "S")


class JournalTests(unittest.TestCase):
    def test_json_whitespace_is_irrelevant(self):
        with patch.object(GPU.subprocess, "run", return_value=subprocess.CompletedProcess(
                [], 0, json.dumps(record(), separators=(", ", ": ")) + "\n", "")):
            self.assertEqual(GPU.Journal().latest(), record())

    def test_empty_malformed_and_non_kernel_records_are_rejected(self):
        for output in ("", "-- No entries --", "{}", "[]", '{"_TRANSPORT":"stdout"}'):
            with self.subTest(output=output), patch.object(
                    GPU.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, output, "")):
                with self.assertRaises(GPU.MonitorError):
                    GPU.Journal().latest()

    def test_read_failure_and_timeout_are_rejected(self):
        with patch.object(GPU.subprocess, "run", return_value=subprocess.CompletedProcess(
                [], 1, "", "Permission denied")):
            with self.assertRaises(GPU.MonitorError):
                GPU.Journal().latest()
        with patch.object(GPU.subprocess, "run", side_effect=subprocess.TimeoutExpired("journalctl", 3)):
            with self.assertRaises(GPU.MonitorError):
                GPU.Journal().latest()


if __name__ == "__main__":
    unittest.main()
