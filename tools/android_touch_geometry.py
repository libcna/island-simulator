#!/usr/bin/env python3
"""Map smoke-test gestures through CNA's aspect-fit logical viewport, without adb."""
import argparse
from pathlib import Path
import re
from typing import NamedTuple


class TouchGeometry(NamedTuple):
    selector_x: int
    selector_y: int
    movement_x: int
    movement_low_y: int
    movement_high_y: int
    movement_bottom_y: int


def geometry(window_width, window_height, logical_width, logical_height):
    for extent in (window_width, window_height, logical_width, logical_height):
        if type(extent) is not int or not 16 <= extent <= 65536:
            raise ValueError("window and logical extents must be integers in [16, 65536]")
    if window_width * logical_height <= window_height * logical_width:
        width, height = window_width, window_width * logical_height // logical_width
    else:
        width, height = window_height * logical_width // logical_height, window_height
    if width < 16 or height < 16:
        raise ValueError("aspect-fit content is too small for touch gestures")
    left, top = (window_width - width) // 2, (window_height - height) // 2
    radius = max(1.0, logical_height * 0.06)
    # Match TouchControls::SensitivitySelectorLayout; use content fractions for the movement
    # zone so pillarbox/letterbox margins can never become the gesture's starting point.
    points = TouchGeometry(
        left + int((logical_width - radius * 1.4) * width / logical_width),
        top + int(radius * 1.4 * height / logical_height),
        left + width // 4,
        top + height * 76 // 100,
        top + height // 6,
        top + height * 5 // 6,
    )
    if not left <= points.selector_x < left + width or not top <= points.selector_y < top + height:
        raise ValueError("logical selector does not fit the content viewport")
    return points


def from_log(text):
    window = re.findall(r"Window size: ([1-9][0-9]*)x([1-9][0-9]*)\b", text)
    logical = re.findall(r"viewport ([1-9][0-9]*)x([1-9][0-9]*) logical backbuffer\b", text)
    if not window or not logical:
        raise ValueError("missing window/logical viewport diagnostics for Android touch input")
    return geometry(*(int(value) for value in (*window[-1], *logical[-1])))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    arguments = parser.parse_args()
    try:
        print(*from_log(arguments.log.read_text(errors="replace")))
    except (OSError, ValueError) as error:
        parser.exit(1, f"Android touch geometry: {error}\n")
