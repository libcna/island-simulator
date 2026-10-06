#!/usr/bin/env python3

"""Regression tests for the deterministic P6 capture validator."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

from tools.validate_capture import CaptureError, PpmImage, read_ppm, validate_capture  # noqa: E402


class CaptureValidatorTests(unittest.TestCase):
    def test_reads_comment_and_crlf_header(self) -> None:
        pixels = bytes((10, 20, 30, 40, 50, 60))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.ppm"
            path.write_bytes(b"P6\r\n# generated\r\n2 1\r\n255\r\n" + pixels)
            image = read_ppm(path)
        self.assertEqual((image.width, image.height), (2, 1))
        self.assertEqual(image.pixels, pixels)

    def test_rejects_non_binary_and_truncated_captures(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.ppm"
            path.write_bytes(b"P3\n1 1\n255\n0 0 0")
            with self.assertRaisesRegex(CaptureError, "binary P6"):
                read_ppm(path)
            path.write_bytes(b"P6\n2 1\n255\n\x00\x00\x00")
            with self.assertRaisesRegex(CaptureError, "payload"):
                read_ppm(path)

    def test_rejects_solid_and_dimension_mismatched_images(self) -> None:
        width = 640
        height = 360
        image = PpmImage(width, height, bytes((80, 120, 160)) * width * height)
        report = validate_capture(image, expected_width=1280, expected_height=720)
        self.assertFalse(report["valid"])
        self.assertIn("capture width is 640; expected 1280", report["errors"])
        self.assertIn("capture height is 360; expected 720", report["errors"])
        self.assertIn("capture has insufficient luminance variation", report["errors"])
        self.assertIn("capture has too few quantized colour bins", report["errors"])
        self.assertIn("capture has insufficient large-scale spatial contrast", report["errors"])


if __name__ == "__main__":
    unittest.main()
