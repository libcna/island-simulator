#!/usr/bin/env python3
"""Host-only, bounded PNG reduction matching ForestFloorPixels.cpp code-value filtering.

Pillow supplies PNG decoding/encoding, not resizing or colour management. Original artwork
must be retained; outputs belong in build directories. No graphics device is created.
"""

import argparse
import math
import os
from pathlib import Path
import tempfile

try:
    from PIL import Image
except ImportError as error:
    raise SystemExit("Material preparation requires host Python 3 and Pillow (python3-pil).") from error


def area_reduce(rgba: bytes, source_size: int, target_size: int) -> bytes:
    if not 2 <= source_size <= 2048 or len(rgba) != source_size * source_size * 4:
        raise ValueError("Source requires an exact 2..2048 square RGBA8 payload")
    if not 2 <= target_size <= 1024 or target_size & (target_size - 1):
        raise ValueError("Target must be a power of two in 2..1024")
    if any(alpha != 255 for alpha in rgba[3::4]):
        raise ValueError("Material source must be opaque")
    # Keep operation order aligned with ForestFloorPixels::PrepareTexture. Python float
    # and the C++ reference use double here; channels are positive, so floor(x+.5)
    # matches lround. Do not substitute a decoder's generic BOX/LANCZOS resize.
    scale = source_size / target_size
    output = bytearray(target_size * target_size * 3)
    spans = []
    for x in range(target_size):
        left, right = x * scale, (x + 1) * scale
        spans.append([(sx, min(right, sx + 1.0) - max(left, float(sx)))
                      for sx in range(math.floor(left), min(source_size, math.ceil(right)))])
    for y in range(target_size):
        for x in range(target_size):
            sums = [0.0, 0.0, 0.0]
            for sy, dy in spans[y]:
                for sx, dx in spans[x]:
                    at = (sy * source_size + sx) * 4
                    for channel in range(3):
                        sums[channel] += rgba[at + channel] * dx * dy
            at = (y * target_size + x) * 3
            for channel in range(3):
                output[at + channel] = math.floor(min(255.0, max(0.0,
                    sums[channel] / (scale * scale))) + 0.5)
    return bytes(output)


def prepare(source: Path, output: Path, size: int) -> dict:
    if source.resolve() == output.resolve():
        raise ValueError("Prepared texture cannot overwrite original artwork")
    if output.suffix.lower() != ".png":
        raise ValueError("Prepared material output must have a .png suffix")
    if not 33 <= source.stat().st_size <= 8 * 1024 * 1024:
        raise ValueError("PNG source must be 33 bytes..8 MiB")
    with source.open("rb") as stream:
        header = stream.read(26)
    if header[:16] != b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR" or header[24:26] not in (b"\x08\x02", b"\x08\x06"):
        raise ValueError("Material source requires an 8-bit RGB/RGBA PNG IHDR")
    width, height = int.from_bytes(header[16:20], "big"), int.from_bytes(header[20:24], "big")
    if width != height or not 2 <= width <= 2048:
        raise ValueError("PNG material must be square and 2..2048 pixels wide before decoding")
    with Image.open(source) as image:
        if image.format != "PNG" or image.width != image.height or not 2 <= image.width <= 2048:
            raise ValueError("PNG material must be square and 2..2048 pixels wide")
        if getattr(image, "n_frames", 1) != 1:
            raise ValueError("Animated PNG is not a static material")
        rgb = area_reduce(image.convert("RGBA").tobytes(), image.width, size)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=output.name + ".", suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            # A fresh Image carries no input metadata or timestamps. Compression changes
            # encoded size only; decoded output bytes are tested against the C++ oracle.
            with Image.frombytes("RGB", (size, size), rgb) as prepared:
                prepared.save(stream, format="PNG", compress_level=9, optimize=False)
        temporary.chmod(0o644)
        os.replace(temporary, output)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {"source_bytes": source.stat().st_size, "output_bytes": output.stat().st_size,
            "size": size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--size", type=int, required=True)
    args = parser.parse_args()
    try:
        result = prepare(args.source, args.output, args.size)
        print(f"Material prepared: {args.output}, {result['size']} square, "
              f"{result['source_bytes']} -> {result['output_bytes']} bytes; original unchanged")
    except (OSError, ValueError) as error:
        parser.exit(1, f"Material preparation failed: {error}\n")


if __name__ == "__main__":
    main()
