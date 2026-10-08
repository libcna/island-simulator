#!/usr/bin/env python3
"""Run one GPU workload with mandatory kernel monitoring and preserved incident evidence."""

from __future__ import annotations

import datetime
import fcntl
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

FAULT = re.compile(
    r"amdgpu.*(page fault|timeout|GPU reset|VM fault|VM_L2_PROTECTION_FAULT)"
    r"|gfxhub.*page fault|ring gfx.*timeout|GPU reset|VRAM is lost", re.IGNORECASE
)
GL_ERROR = re.compile(r"gl-error |gl-debug .*severity=high\b")
CAPTURE_VIEWPOINTS = frozenset(
    "overview beach jungle east west grass ground litter contact movement palms water current "
    "distant props driftwood reef fronds birds fish shark monkeys shipwreck crabs butterflies turtles"
    .split()
)


class MonitorError(RuntimeError):
    """The kernel's state cannot be established reliably."""


def utc_now() -> str:
    return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="milliseconds")


class Journal:
    def __init__(self, executable: str = "journalctl", timeout: float = 3.0):
        self.executable = executable
        self.timeout = timeout

    def read(self, *arguments: str) -> list[dict]:
        try:
            result = subprocess.run(
                [self.executable, "-k", "--no-pager", "-o", "json", *arguments],
                capture_output=True, text=True, timeout=self.timeout, check=False,
            )
        except (OSError, subprocess.SubprocessError) as error:
            raise MonitorError(f"kernel journal read failed: {error}") from error
        if result.returncode != 0:
            raise MonitorError(f"journalctl exited {result.returncode}: {result.stderr.strip()}")
        records = []
        for line in result.stdout.splitlines():
            if not line.strip():
                continue
            try:
                record = json.loads(line)
            except (ValueError, TypeError) as error:
                raise MonitorError("kernel journal returned invalid JSON") from error
            if not isinstance(record, dict) or record.get("_TRANSPORT") != "kernel":
                raise MonitorError("kernel journal returned a non-kernel record")
            if not all(isinstance(record.get(key), str) and record[key]
                       for key in ("__CURSOR", "__REALTIME_TIMESTAMP", "_BOOT_ID", "MESSAGE")):
                raise MonitorError("kernel journal record is missing attribution fields")
            records.append(record)
        return records

    def latest(self) -> dict:
        records = self.read("-n", "1")
        if len(records) != 1:
            raise MonitorError("this account cannot read a current kernel journal record")
        return records[0]


def stop_process_group(process: subprocess.Popen) -> None:
    """Stop owned clients, including children left behind by an exited wrapper."""
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=1.0)
    except subprocess.TimeoutExpired:
        pass
    # A wrapper exiting does not prove its children exited. Signal the entire group again.
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=1.0)
    except subprocess.TimeoutExpired as error:
        raise MonitorError("owned workload did not terminate after SIGKILL") from error


def workload_arguments(game: Path, workload: str, evidence: Path) -> list[str]:
    if workload == "smoke":
        return [str(game), "--smoke-test"]
    if workload in ("benchmark-jungle", "benchmark-traversal"):
        return [str(game), "--benchmark", str(evidence / f"{workload}.json"),
                "--viewpoint", workload.removeprefix("benchmark-")]
    if workload.startswith("capture-") and workload[8:] in CAPTURE_VIEWPOINTS:
        return [str(game), "--capture", str(evidence / f"{workload}.ppm"),
                "--viewpoint", workload[8:]]
    raise ValueError(f"unsupported workload: {workload}")


def qualify(build: Path, workload: str, runner: Path, *, journal: Journal | None = None,
            timeout: float = 120.0, poll_interval: float = 0.2) -> int:
    if not build.is_dir():
        raise ValueError(f"build directory is missing: {build}")
    # Two invocations against the same build must not accidentally recreate parallel stress.
    with (build / "gpu-qualification.lock").open("a", encoding="utf-8") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print("GPU qualification REFUSED: another qualification owns this build.", file=sys.stderr)
            return 3
        return qualify_locked(build, workload, runner, journal=journal, timeout=timeout,
                              poll_interval=poll_interval)


def qualify_locked(build: Path, workload: str, runner: Path, *, journal: Journal | None,
                   timeout: float, poll_interval: float) -> int:
    build = build.resolve()
    game = build / "island-simulator"
    evidence_root = build / "gpu-incident-evidence"
    stop_file = evidence_root / "QUALIFICATION_STOP.json"
    journal = journal or Journal()
    workload_arguments(game, workload, evidence_root)
    for executable in (game, runner):
        if not executable.is_file() or not os.access(executable, os.X_OK):
            raise ValueError(f"executable is missing: {executable}")
    if stop_file.exists():
        print(f"GPU qualification REFUSED: review the prior failure in {stop_file}", file=sys.stderr)
        return 3
    try:
        baseline = journal.latest()
    except MonitorError as error:
        print(f"GPU qualification REFUSED: {error}", file=sys.stderr)
        return 3

    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    evidence = evidence_root / f"{stamp}-{os.getpid()}-{workload}"
    logs = evidence / "runtime-logs"
    logs.mkdir(parents=True)
    command = [str(runner), "--exec", *workload_arguments(game, workload, evidence)]
    metadata = {"started_utc": utc_now(), "kernel_baseline": baseline, "command": command,
                "serialization": "one-game-process", "timeout_seconds": timeout}
    metadata_path = evidence / "run-metadata.json"
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    environment = os.environ.copy()
    environment.update(ISLAND_RUNTIME_LOG_DIRECTORY=str(logs), ISLAND_GPU_DIAGNOSTICS="1",
                       MESA_DEBUG="context", CNA_WAYLAND_TEST_KEEP_LOG=str(evidence / "compositor.log"))
    print(f"GPU qualification: one workload; evidence: {evidence}", flush=True)
    process = None
    cursor = baseline["__CURSOR"]
    failure = ""
    status = 0
    interrupted = 0
    old_handlers = {}

    def interrupted_handler(number, _frame):
        nonlocal interrupted
        interrupted = number

    def collect(stream) -> None:
        nonlocal cursor
        # journalctl can silently succeed without records after permission loss. Probe access
        # separately because an empty delta alone cannot distinguish that from a quiet kernel.
        latest = journal.latest()
        if latest["_BOOT_ID"] != baseline["_BOOT_ID"]:
            raise MonitorError("kernel boot identity changed during qualification")
        previous_cursor = cursor
        records = journal.read("--after-cursor", cursor)
        for record in records:
            stream.write(json.dumps(record) + "\n")
            stream.flush()
            cursor = record["__CURSOR"]
            if record["_BOOT_ID"] != baseline["_BOOT_ID"]:
                raise MonitorError("kernel delta belongs to a different boot")
            if FAULT.search(record["MESSAGE"]):
                raise MonitorError(f"GPU fault detected: {record['MESSAGE']}")
        if latest["__CURSOR"] != previous_cursor and not any(
                item["__CURSOR"] == latest["__CURSOR"] for item in records):
            raise MonitorError("kernel cursor continuity was lost")

    try:
        for number in (signal.SIGINT, signal.SIGTERM):
            old_handlers[number] = signal.signal(number, interrupted_handler)
        with (evidence / "workload.log").open("w", encoding="utf-8") as output, \
                (evidence / "kernel-live.jsonl").open("w", encoding="utf-8") as kernel:
            process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT,
                                       env=environment, start_new_session=True)
            metadata["workload_process_group"] = process.pid
            metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
            deadline = time.monotonic() + timeout
            while True:
                collect(kernel)
                if interrupted:
                    raise MonitorError(f"qualification interrupted by signal {interrupted}")
                if process.poll() is not None:
                    break
                if time.monotonic() >= deadline:
                    raise MonitorError("workload exceeded its time limit")
                time.sleep(poll_interval)
            status = process.returncode
            stop_process_group(process)
            process = None
            collect(kernel)
            if status:
                failure = f"workload exited with status {status}"
            for path in [evidence / "workload.log", *logs.glob("*.log")]:
                if GL_ERROR.search(path.read_text(encoding="utf-8", errors="replace")):
                    failure = f"OpenGL error or high-severity diagnostic in {path.name}"
                    status = 4
                    break
    except (MonitorError, OSError) as error:
        failure = str(error)
        status = 4
    finally:
        if process is not None:
            try:
                stop_process_group(process)
            except MonitorError as error:
                failure = f"{failure}; {error}"
                status = 4
        for number, handler in old_handlers.items():
            signal.signal(number, handler)

    # A later empty scan must never erase a fault already detected by the live monitor.
    try:
        final_records = journal.read("--after-cursor", baseline["__CURSOR"])
        (evidence / "kernel-after.jsonl").write_text(
            "".join(json.dumps(record) + "\n" for record in final_records), encoding="utf-8")
        if any(FAULT.search(record["MESSAGE"]) for record in final_records):
            failure = failure or "GPU fault in final kernel delta"
            status = 4
    except MonitorError as error:
        failure = failure or str(error)
        status = 4
    metadata.update(finished_utc=utc_now(), status=status, failure=failure)
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    if failure:
        stop_file.write_text(json.dumps({"evidence": str(evidence), "failure": failure}, indent=2)
                             + "\n", encoding="utf-8")
        print(f"GPU qualification FAILED: {failure}; further runs are locked out.", file=sys.stderr)
        return status if 0 < status < 126 else 4
    print("GPU qualification passed: no new GPU faults, timeouts, resets, or crashes.")
    return 0


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <build-directory> <workload>", file=sys.stderr)
        return 2
    root = Path(__file__).resolve().parent.parent
    runner = root / "build/dependencies/cna-next/tools/platform/run_gpu_tests_private.sh"
    try:
        return qualify(Path(sys.argv[1]), sys.argv[2], runner)
    except (ValueError, OSError) as error:
        print(f"GPU qualification REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
