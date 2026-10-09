#!/usr/bin/env python3

"""Exercise island-simulator command-line failures before graphics initialization."""

from __future__ import annotations

import os
import json
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class InvalidInvocation:
    name: str
    arguments: tuple[str, ...]
    expected_error: str


def run_case(executable: Path, case: InvalidInvocation, log_directory: Path) -> None:
    existing_logs = set(log_directory.glob("island-runtime-*.log"))
    environment = os.environ.copy()
    environment["ISLAND_RUNTIME_LOG_DIRECTORY"] = str(log_directory)
    result = subprocess.run(
        [str(executable), *case.arguments],
        capture_output=True,
        check=False,
        env=environment,
        text=True,
        timeout=5.0,
    )
    if result.returncode != 2:
        raise RuntimeError(
            f"{case.name}: expected exit code 2, got {result.returncode}; "
            f"stdout={result.stdout!r}, stderr={result.stderr!r}"
        )
    if case.expected_error not in result.stderr:
        raise RuntimeError(
            f"{case.name}: expected stderr to contain {case.expected_error!r}; "
            f"stderr={result.stderr!r}"
        )
    if result.stdout:
        raise RuntimeError(
            f"{case.name}: validation failure unexpectedly wrote stdout={result.stdout!r}"
        )

    new_logs = set(log_directory.glob("island-runtime-*.log")) - existing_logs
    if len(new_logs) != 1:
        raise RuntimeError(
            f"{case.name}: expected one persistent runtime log, found {len(new_logs)}"
        )
    log_path = new_logs.pop()
    log_text = log_path.read_text(encoding="utf-8")
    if "island-simulator: runtime-start pid=" not in log_text:
        raise RuntimeError(f"{case.name}: runtime-start record missing from {log_path}")
    expected_command = " ".join(f'"{value}"' for value in (str(executable), *case.arguments))
    if f"command-line={expected_command}" not in log_text:
        raise RuntimeError(
            f"{case.name}: exact command line missing from {log_path}; log={log_text!r}"
        )


def run_namespace_attribution(executable: Path, log_directory: Path) -> None:
    """Exec preserves the fixture's PID; invalid arguments exit before graphics startup."""
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
    from gpu_incident_qualification import contained_command

    identity = log_directory / "expected-identity.json"
    code = ("import json,os; from pathlib import Path; "
            f"Path({str(identity)!r}).write_text(json.dumps({{"
            "'pid': int(os.readlink('/proc/self')), 'namespace': os.getpid()})); "
            f"os.execv({str(executable)!r}, [{str(executable)!r}, '--not-an-island-option'])")
    environment = os.environ.copy()
    environment["ISLAND_RUNTIME_LOG_DIRECTORY"] = str(log_directory)
    result = subprocess.run(contained_command([sys.executable, "-c", code]), env=environment,
                            capture_output=True, text=True, timeout=5)
    if result.returncode != 2:
        raise RuntimeError(f"namespace attribution fixture failed: {result.stderr}")
    expected = json.loads(identity.read_text())
    if expected["pid"] == expected["namespace"] or expected["namespace"] != 2:
        raise RuntimeError(f"PID namespace was not established: {expected}")
    match = re.search(r"runtime-start pid=(\d+) main-tid=(\d+) namespace-pid=(\d+) "
                      r"namespace-main-tid=(\d+)", result.stderr)
    required = (expected["pid"], expected["pid"], expected["namespace"], expected["namespace"])
    if match is None or tuple(map(int, match.groups())) != required:
        raise RuntimeError(f"kernel PID/TID attribution mismatch: {result.stderr}")
    if not (log_directory / f"island-runtime-{expected['pid']}.log").is_file():
        raise RuntimeError("runtime log filename does not contain the outer PID")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: ValidateCommandLineTests.py <island-simulator>", file=sys.stderr)
        return 2

    executable = Path(sys.argv[1]).resolve()
    if not executable.is_file():
        print(f"command-line validation: executable not found: {executable}", file=sys.stderr)
        return 2

    cases = (
        InvalidInvocation("bush camera parsed before missing capture path",
                          ("--viewpoint", "bush", "--capture"),
                          "island-simulator: --capture requires a path"),
        InvalidInvocation("ground broadleaf camera parsed before missing capture path",
                          ("--viewpoint", "ground-broadleaf", "--capture"),
                          "island-simulator: --capture requires a path"),
        InvalidInvocation("canopy camera without inspection", ("--viewpoint", "canopy"),
                          "canopy viewpoint requires --canopy-lod-inspection"),
        InvalidInvocation("canopy capture without inspection", ("--viewpoint", "canopy", "--capture", "unused.ppm"),
                          "canopy viewpoint requires --canopy-lod-inspection"),
        *(InvalidInvocation(
            "invalid canopy inspection: " + repr(arguments), arguments,
            "--canopy-lod-inspection requires unique X_MM Z_MM near|far") for arguments in (
                ("--canopy-lod-inspection",),
                ("--canopy-lod-inspection", "1", "2"),
                ("--canopy-lod-inspection", "x", "2", "near"),
                ("--canopy-lod-inspection", "1.2", "2", "near"),
                ("--canopy-lod-inspection", "99999999999999999999", "2", "near"),
                ("--canopy-lod-inspection", "500001", "2", "near"),
                ("--canopy-lod-inspection", "1", "2", "invalid"),
                ("--canopy-lod-inspection", "1", "2", "near",
                 "--canopy-lod-inspection", "1", "2", "far"),
            )),
        *(InvalidInvocation(
            "canopy inspection mode conflict: " + repr(arguments),
            ("--canopy-lod-inspection", "-87120", "-15369", "near", *arguments),
            "canopy LOD inspection requires --capture and a fixed viewpoint") for arguments in (
                (), ("--capture", "unused.ppm"), ("--viewpoint", "east"),
                ("--capture", "unused.ppm", "--viewpoint", "east", "--benchmark", "unused.json"),
                ("--capture", "unused.ppm", "--viewpoint", "east", "--debug-overlay"),
                ("--capture", "unused.ppm", "--viewpoint", "traversal"),
            )),
        InvalidInvocation(
            "unknown argument",
            ("--not-an-island-option",),
            "island-simulator: unknown argument '--not-an-island-option'",
        ),
        InvalidInvocation(
            "missing viewpoint value",
            ("--viewpoint",),
            "island-simulator: --viewpoint requires overview, beach, jungle",
        ),
        InvalidInvocation(
            "unknown viewpoint",
            ("--viewpoint", "volcano"),
            "island-simulator: unknown viewpoint 'volcano'",
        ),
        InvalidInvocation(
            "missing capture path",
            ("--capture",),
            "island-simulator: --capture requires a path",
        ),
        InvalidInvocation(
            "missing benchmark path",
            ("--benchmark",),
            "island-simulator: --benchmark requires a JSON output path",
        ),
        InvalidInvocation(
            "missing startup-report path",
            ("--startup-report",),
            "island-simulator: --startup-report requires a JSON output path",
        ),
        InvalidInvocation(
            "benchmark and debug conflict",
            ("--benchmark", "unused.json", "--debug-overlay"),
            "island-simulator: --benchmark cannot be combined with smoke, capture, or debug modes",
        ),
        InvalidInvocation(
            "traversal without benchmark",
            ("--viewpoint", "traversal"),
            "island-simulator: the traversal viewpoint requires --benchmark",
        ),
    )
    try:
        with tempfile.TemporaryDirectory(prefix="island-command-line-diagnostics-") as directory:
            log_directory = Path(directory)
            for case in cases:
                run_case(executable, case, log_directory)
            if sys.platform.startswith("linux"):
                run_namespace_attribution(executable, log_directory)
    except (OSError, subprocess.SubprocessError, RuntimeError) as error:
        print(f"command-line validation failed: {error}", file=sys.stderr)
        return 1

    print(f"command-line validation passed ({len(cases)} invalid invocations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
