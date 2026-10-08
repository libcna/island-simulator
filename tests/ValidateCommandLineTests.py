#!/usr/bin/env python3

"""Exercise island-simulator command-line failures before graphics initialization."""

from __future__ import annotations

import os
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


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: ValidateCommandLineTests.py <island-simulator>", file=sys.stderr)
        return 2

    executable = Path(sys.argv[1]).resolve()
    if not executable.is_file():
        print(f"command-line validation: executable not found: {executable}", file=sys.stderr)
        return 2

    cases = (
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
    except (OSError, subprocess.SubprocessError, RuntimeError) as error:
        print(f"command-line validation failed: {error}", file=sys.stderr)
        return 1

    print(f"command-line validation passed ({len(cases)} invalid invocations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
