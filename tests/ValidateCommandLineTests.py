#!/usr/bin/env python3

"""Exercise island-simulator command-line failures before graphics initialization."""

from __future__ import annotations

import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class InvalidInvocation:
    name: str
    arguments: tuple[str, ...]
    expected_error: str


def run_case(executable: Path, case: InvalidInvocation) -> None:
    result = subprocess.run(
        [str(executable), *case.arguments],
        capture_output=True,
        check=False,
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
        for case in cases:
            run_case(executable, case)
    except (OSError, subprocess.SubprocessError, RuntimeError) as error:
        print(f"command-line validation failed: {error}", file=sys.stderr)
        return 1

    print(f"command-line validation passed ({len(cases)} invalid invocations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
