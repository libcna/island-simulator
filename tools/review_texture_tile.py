#!/usr/bin/env python3
"""Read-only P6 texture inspection; metrics are evidence, not a seamlessness certificate."""

import argparse
import json
import math
from pathlib import Path

from validate_capture import PpmImage, read_ppm


def inspect_tile(image: PpmImage) -> dict:
    width, height = image.width, image.height
    if not 2 <= width <= 4096 or not 2 <= height <= 4096:
        raise ValueError("tile dimensions must be between 2 and 4096")
    if len(image.pixels) != width * height * 3:
        raise ValueError("tile requires an exact RGB8 payload")

    def differences(axis, first, second):
        count = height if axis == "x" else width
        total = squared = 0
        for along in range(count):
            a = (along * width + first if axis == "x" else first * width + along) * 3
            b = (along * width + second if axis == "x" else second * width + along) * 3
            for channel in range(3):
                delta = abs(image.pixels[a + channel] - image.pixels[b + channel])
                total += delta
                squared += delta * delta
        return total / (count * 3), math.sqrt(squared / (count * 3))

    axes = {}
    for axis, extent in (("x", width), ("y", height)):
        seam_mae, seam_rms = differences(axis, extent - 1, 0)
        # Uniform deterministic interior cuts, with both near-border cuts included. Comparing
        # only edge texels to zero would falsely reject ordinary continuous noisy materials.
        positions = sorted({round(index * (extent - 2) / 63) for index in range(64)})
        interior = [differences(axis, position, position + 1)[0] for position in positions]
        average = sum(interior) / len(interior)
        axes[axis] = {
            "seamMeanAbsoluteRgb8Difference": seam_mae,
            "seamRootMeanSquareRgb8Difference": seam_rms,
            "interiorMeanAbsoluteRgb8Difference": average,
            "seamToInteriorMeanRatio": seam_mae / average if average else None,
            "interiorCutsBelowSeam": sum(value < seam_mae for value in interior),
            "interiorCutCount": len(interior),
        }

    mip_bytes = 0
    mip_levels = 0
    w, h = width, height
    while True:
        mip_bytes += w * h * 4
        mip_levels += 1
        if w == 1 and h == 1:
            break
        w, h = max(1, w // 2), max(1, h // 2)
    return {
        "scope": "offline RGB8 discontinuity review, not runtime or artistic acceptance",
        "width": width,
        "height": height,
        "powerOfTwo": width & (width - 1) == 0 and height & (height - 1) == 0,
        "rgba8BaseBytes": width * height * 4,
        "rgba8FullMipBytes": mip_bytes,
        "fullMipLevelCount": mip_levels,
        "axes": axes,
        "limitations": "Low edge error cannot prove visual tiling, feature continuity, albedo "
                        "correctness or suitable world scale. Inspect a repeated sheet in addition.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="RGB8 P6 PPM; convert PNG without resampling")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        # Bound decoding input as well as the later scan. No GPU or image decoder is needed.
        if args.image.stat().st_size > 4096 * 4096 * 3 + 4096:
            raise ValueError("input exceeds the bounded texture review size")
        report = inspect_tile(read_ppm(args.image))
        report["input"] = str(args.image)
        encoded = json.dumps(report, indent=2, allow_nan=False) + "\n"
        if args.output:
            args.output.write_text(encoded)
        print(encoded, end="")
    except (ValueError, OSError) as error:
        parser.exit(1, f"Texture review failed: {error}\n")


if __name__ == "__main__":
    main()
