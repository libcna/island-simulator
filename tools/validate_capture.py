#!/usr/bin/env python3

"""Validate that a deterministic PPM capture contains a non-degenerate rendered scene."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any


MINIMUM_WIDTH = 640
MINIMUM_HEIGHT = 360
MINIMUM_LUMINANCE_DEVIATION = 18.0
MINIMUM_QUANTIZED_COLOR_BINS = 64
MINIMUM_SPATIAL_CONTRAST = 30.0
PROFILES = ("scene", "shaded-detail", "close-natural-detail")


def profile_for_viewpoint(viewpoint: str) -> str:
    """Return only explicitly reviewed profiles; never infer an exception from failed pixels."""
    if viewpoint == "litter":
        return "shaded-detail"
    if viewpoint in ("spiders-bright", "ants", "cicadas", "beetles"):
        return "close-natural-detail"
    return "scene"


def profile_for_arguments(arguments: list[str]) -> str:
    """Match the game's last explicit camera selection, including --overview."""
    viewpoint = ""
    for index, argument in enumerate(arguments):
        if argument == "--overview":
            viewpoint = "overview"
        elif argument == "--viewpoint" and index + 1 < len(arguments):
            viewpoint = arguments[index + 1]
    return profile_for_viewpoint(viewpoint)


class CaptureError(ValueError):
    """Raised when a capture is not a supported complete P6 PPM image."""


@dataclass(frozen=True)
class PpmImage:
    width: int
    height: int
    pixels: bytes


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate a deterministic island-simulator P6 PPM capture."
    )
    parser.add_argument("capture", type=Path, help="P6 PPM capture to validate.")
    parser.add_argument("--expect-width", type=int, help="Required capture width.")
    parser.add_argument("--expect-height", type=int, help="Required capture height.")
    parser.add_argument("--profile", choices=PROFILES, default="scene")
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional path for a machine-readable JSON validation report.",
    )
    return parser.parse_args()


def _next_header_token(data: bytes, cursor: int) -> tuple[bytes, int]:
    while cursor < len(data):
        if data[cursor] in b" \t\r\n":
            cursor += 1
            continue
        if data[cursor] == ord("#"):
            line_end = data.find(b"\n", cursor)
            if line_end < 0:
                raise CaptureError("unterminated PPM header comment")
            cursor = line_end + 1
            continue
        break
    if cursor >= len(data):
        raise CaptureError("truncated PPM header")

    start = cursor
    while cursor < len(data) and data[cursor] not in b" \t\r\n#":
        cursor += 1
    if start == cursor:
        raise CaptureError("empty PPM header token")
    return data[start:cursor], cursor


def read_ppm(path: Path) -> PpmImage:
    try:
        data = path.read_bytes()
    except OSError as error:
        raise CaptureError(f"cannot read capture: {error}") from error

    cursor = 0
    tokens: list[bytes] = []
    for _ in range(4):
        token, cursor = _next_header_token(data, cursor)
        tokens.append(token)

    if tokens[0] != b"P6":
        raise CaptureError("capture must use binary P6 PPM encoding")
    try:
        width = int(tokens[1])
        height = int(tokens[2])
        maximum = int(tokens[3])
    except ValueError as error:
        raise CaptureError("PPM dimensions and maximum value must be integers") from error
    if width <= 0 or height <= 0:
        raise CaptureError("PPM dimensions must be positive")
    if maximum != 255:
        raise CaptureError("capture must use 8-bit PPM channels with maximum value 255")
    if cursor >= len(data) or data[cursor] not in b" \t\r\n":
        raise CaptureError("PPM header must end with whitespace")
    if data[cursor:cursor + 2] == b"\r\n":
        cursor += 2
    else:
        cursor += 1

    expected_bytes = width * height * 3
    pixels = data[cursor:]
    if len(pixels) != expected_bytes:
        raise CaptureError(
            f"PPM payload has {len(pixels)} bytes; expected {expected_bytes}"
        )
    return PpmImage(width=width, height=height, pixels=pixels)


def analyze_capture(image: PpmImage) -> dict[str, Any]:
    pixel_count = image.width * image.height
    sums = [0, 0, 0]
    minima = [255, 255, 255]
    maxima = [0, 0, 0]
    luminance_sum = 0.0
    luminance_square_sum = 0.0
    dark_pixels = 0
    bright_pixels = 0
    quantized_colors: set[int] = set()

    grid_columns = 8
    grid_rows = 4
    region_sums = [[0, 0, 0, 0] for _ in range(grid_columns * grid_rows)]
    pixels = image.pixels
    for pixel_index in range(pixel_count):
        offset = pixel_index * 3
        red = pixels[offset]
        green = pixels[offset + 1]
        blue = pixels[offset + 2]
        channels = (red, green, blue)
        for channel in range(3):
            value = channels[channel]
            sums[channel] += value
            minima[channel] = min(minima[channel], value)
            maxima[channel] = max(maxima[channel], value)

        luminance = (54.0 * red + 183.0 * green + 19.0 * blue) / 256.0
        luminance_sum += luminance
        luminance_square_sum += luminance * luminance
        dark_pixels += int(luminance < 16.0)
        bright_pixels += int(luminance > 239.0)
        quantized_colors.add((red >> 4) << 8 | (green >> 4) << 4 | (blue >> 4))

        x = pixel_index % image.width
        y = pixel_index // image.width
        region_x = min(grid_columns - 1, x * grid_columns // image.width)
        region_y = min(grid_rows - 1, y * grid_rows // image.height)
        region = region_sums[region_y * grid_columns + region_x]
        region[0] += red
        region[1] += green
        region[2] += blue
        region[3] += 1

    mean = [value / pixel_count for value in sums]
    luminance_mean = luminance_sum / pixel_count
    luminance_variance = max(
        0.0, luminance_square_sum / pixel_count - luminance_mean * luminance_mean
    )
    region_means = [
        (region[0] / region[3], region[1] / region[3], region[2] / region[3])
        for region in region_sums
    ]
    spatial_contrast = 0.0
    for first_index, first in enumerate(region_means):
        for second in region_means[first_index + 1:]:
            distance = math.sqrt(sum((first[i] - second[i]) ** 2 for i in range(3)))
            spatial_contrast = max(spatial_contrast, distance)

    return {
        "width": image.width,
        "height": image.height,
        "pixelCount": pixel_count,
        "sha256": hashlib.sha256(image.pixels).hexdigest(),
        "channelMean": [round(value, 4) for value in mean],
        "channelMinimum": minima,
        "channelMaximum": maxima,
        "luminanceMean": round(luminance_mean, 4),
        "luminanceStandardDeviation": round(math.sqrt(luminance_variance), 4),
        "darkPixelFraction": round(dark_pixels / pixel_count, 6),
        "brightPixelFraction": round(bright_pixels / pixel_count, 6),
        "quantizedColorBins": len(quantized_colors),
        "spatialContrast": round(spatial_contrast, 4),
    }


def validate_capture(image: PpmImage, expected_width: int | None = None,
                     expected_height: int | None = None, *, profile: str = "scene") -> dict[str, Any]:
    if profile not in PROFILES:
        raise ValueError(f"Unknown capture profile: {profile}")
    metrics = analyze_capture(image)
    # Shaded close-ups must retain at least 25% relative variation and a hard noise
    # floor. This is stricter for bright/washed-out pictures, not a blanket bypass.
    minimum_deviation = (max(12.0, metrics["luminanceMean"] * 0.25)
                         if profile == "shaded-detail"
                         else max(10.0, metrics["luminanceMean"] * 0.20)
                         if profile == "close-natural-detail"
                         else MINIMUM_LUMINANCE_DEVIATION)
    minimum_color_bins = 40 if profile == "close-natural-detail" else MINIMUM_QUANTIZED_COLOR_BINS
    minimum_spatial_contrast = 15.0 if profile == "close-natural-detail" else MINIMUM_SPATIAL_CONTRAST
    errors: list[str] = []
    if image.width < MINIMUM_WIDTH or image.height < MINIMUM_HEIGHT:
        errors.append(
            f"capture is smaller than {MINIMUM_WIDTH}x{MINIMUM_HEIGHT}: "
            f"{image.width}x{image.height}"
        )
    if expected_width is not None and image.width != expected_width:
        errors.append(f"capture width is {image.width}; expected {expected_width}")
    if expected_height is not None and image.height != expected_height:
        errors.append(f"capture height is {image.height}; expected {expected_height}")
    if metrics["luminanceStandardDeviation"] < minimum_deviation:
        errors.append("capture has insufficient luminance variation")
    if metrics["quantizedColorBins"] < minimum_color_bins:
        errors.append("capture has too few quantized colour bins")
    if metrics["spatialContrast"] < minimum_spatial_contrast:
        errors.append("capture has insufficient large-scale spatial contrast")
    if not 24.0 <= metrics["luminanceMean"] <= 232.0:
        errors.append("capture mean luminance is outside the usable presentation range")
    if metrics["darkPixelFraction"] > 0.92:
        errors.append("capture is overwhelmingly dark")
    if metrics["brightPixelFraction"] > 0.92:
        errors.append("capture is overwhelmingly bright")

    return {
        "schemaVersion": 1,
        "profile": profile,
        "valid": not errors,
        "metrics": metrics,
        "thresholds": {
            "minimumWidth": MINIMUM_WIDTH,
            "minimumHeight": MINIMUM_HEIGHT,
            "minimumLuminanceStandardDeviation": minimum_deviation,
            "minimumRelativeLuminanceDeviation": 0.25 if profile == "shaded-detail" else None,
            "minimumQuantizedColorBins": minimum_color_bins,
            "minimumSpatialContrast": minimum_spatial_contrast,
        },
        "errors": errors,
    }


def main() -> int:
    arguments = parse_arguments()
    try:
        image = read_ppm(arguments.capture)
        report = validate_capture(image, arguments.expect_width, arguments.expect_height,
                                  profile=arguments.profile)
    except CaptureError as error:
        report = {
            "schemaVersion": 1,
            "valid": False,
            "metrics": {},
            "thresholds": {},
            "errors": [str(error)],
        }

    if arguments.output is not None:
        try:
            arguments.output.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
        except OSError as error:
            print(f"island-simulator: cannot write capture report: {error}", file=sys.stderr)
            return 1

    if report["valid"]:
        metrics = report["metrics"]
        print(
            "island-simulator: capture validation passed: "
            f"{metrics['width']}x{metrics['height']}, "
            f"{metrics['quantizedColorBins']} colour bins, "
            f"luminance deviation {metrics['luminanceStandardDeviation']:.2f}, "
            f"spatial contrast {metrics['spatialContrast']:.2f}"
        )
        return 0

    print("island-simulator: capture validation failed", file=sys.stderr)
    for error in report["errors"]:
        print(f"  - {error}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
