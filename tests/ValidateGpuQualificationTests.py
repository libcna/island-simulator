#!/usr/bin/env python3
"""Exercise the incident supervisor with synthetic journal data and CPU-only child processes."""

from __future__ import annotations

import contextlib
import fcntl
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

    def write_game(self, *, persistent=False, exit_code=0, diagnostic="", orphan=False):
        self.game.write_text(
            f"#!{sys.executable}\n"
            "import json, os, signal, subprocess, sys, time\n"
            "from pathlib import Path\n"
            "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
            "child = subprocess.Popen([sys.executable, '-c', "
            "'import signal,time; signal.signal(signal.SIGTERM, signal.SIG_IGN); time.sleep(30)'])\n"
            f"Path({str(self.marker)!r}).write_text(json.dumps({{'parent': os.getpid(), "
            "'child': child.pid, 'argv': sys.argv}))\n"
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
                try:
                    state = path.read_text().split(")", 1)[1].split()[0]
                except FileNotFoundError:
                    break
                if state == "Z":  # Reaping belongs to init after the synthetic parent dies.
                    break
                time.sleep(0.01)
            else:
                os.kill(pids[key], signal.SIGKILL)
                self.fail(f"owned {key} process survived supervisor shutdown")

    def test_no_journal_access_never_starts_workload(self):
        self.assertEqual(self.run_case("no-access"), 3)
        self.assertFalse(self.marker.exists())

    def test_clean_run_preserves_exact_command_and_kernel_baseline(self):
        self.assertEqual(self.run_case(), 0)
        metadata_path, = (self.root / "gpu-incident-evidence").glob("*/run-metadata.json")
        metadata = json.loads(metadata_path.read_text())
        self.assertEqual(metadata["kernel_baseline"], record())
        self.assertEqual(metadata["command"][-2:], ["--viewpoint", "jungle"])
        self.assertEqual(metadata["status"], 0)
        self.assert_owned_processes_stopped()

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
