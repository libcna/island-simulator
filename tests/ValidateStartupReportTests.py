#!/usr/bin/env python3

import json
import math
from pathlib import Path
import sys


TIMING_FIELDS = (
    "world_generation",
    "framework_initialization",
    "gpu_content_creation",
    "post_content_to_first_draw",
    "first_draw",
    "total",
)


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: ValidateStartupReportTests.py <startup-report.json>")

    report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    if report.get("schema_version") != 1:
        raise AssertionError("startup report has an unsupported schema")
    if report.get("renderer") != "OPENGL33" or report.get("viewpoint") != "overview":
        raise AssertionError("startup report does not describe the qualified OpenGL overview")
    if report.get("resolution") != {"width": 1280, "height": 720}:
        raise AssertionError("startup report has an unexpected render resolution")

    audio = report.get("audio", {})
    if audio.get("attempted") or audio.get("available"):
        raise AssertionError("the deterministic smoke report must not initialize audio")
    if audio.get("deferred_for_gesture") or not audio.get("skipped_for_smoke_test"):
        raise AssertionError("the native smoke report has inconsistent audio startup state")

    timing = report.get("timing_ms", {})
    values = []
    for field in TIMING_FIELDS:
        value = timing.get(field)
        if not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0.0:
            raise AssertionError(f"startup timing {field} is missing, negative, or non-finite")
        values.append(float(value))
    if timing["world_generation"] <= 0.0 or timing["gpu_content_creation"] <= 0.0:
        raise AssertionError("startup report did not measure world and GPU-content work")
    if timing["first_draw"] <= 0.0 or timing["total"] <= 0.0:
        raise AssertionError("startup report did not measure the first completed draw")
    component_sum = sum(values[:-1])
    if timing["total"] + 0.01 < component_sum:
        raise AssertionError("startup total is shorter than its measured component intervals")
    if timing["total"] > 60_000.0:
        raise AssertionError("startup exceeded the one-minute smoke-test safety budget")

    print("startup report validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
