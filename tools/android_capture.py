#!/usr/bin/env python3
"""Wait for a non-degenerate Android presentation, not merely a nonempty PNG."""
import io
import json
from pathlib import Path
import subprocess
import sys
import time

from PIL import Image
from validate_capture import PpmImage, validate_capture, profile_for_arguments


def validate_png(data, profile="scene"):
    with Image.open(io.BytesIO(data)) as source:
        source.load()
        image = source.convert("RGB")
        return validate_capture(PpmImage(image.width, image.height, image.tobytes()), profile=profile)


def capture(adb, output, timeout=30, profile="scene"):
    deadline = time.monotonic() + timeout
    attempts = 0
    while time.monotonic() < deadline:
        attempts += 1
        result = subprocess.run([adb, "exec-out", "screencap", "-p"],
                                capture_output=True, check=True,
                                timeout=max(0.1, min(10, deadline - time.monotonic())))
        output.write_bytes(result.stdout)
        try:
            report = validate_png(result.stdout, profile=profile)
        except (OSError, ValueError) as error:
            report = {"valid": False, "errors": [str(error)]}
        report["attempts"] = attempts
        output.with_suffix(".validation.json").write_text(json.dumps(report, indent=2) + "\n")
        if report["valid"]:
            print(f"android capture: presented scene accepted after {attempts} attempt(s)")
            return
        time.sleep(min(1, max(0, deadline - time.monotonic())))
    raise RuntimeError(f"no usable Android presentation within {timeout} seconds: {output}")


if __name__ == "__main__":
    try:
        arguments = sys.argv[3].split(",") if len(sys.argv) > 3 else []
        capture(sys.argv[1], Path(sys.argv[2]), profile=profile_for_arguments(arguments))
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"android capture failed: {error}", file=sys.stderr)
        sys.exit(1)
