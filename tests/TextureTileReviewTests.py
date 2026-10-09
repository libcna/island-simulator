#!/usr/bin/env python3
"""Deterministic offline texture-review arithmetic; no renderer or image-library dependency."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from review_texture_tile import inspect_tile
from validate_capture import PpmImage


class TextureTileReviewTests(unittest.TestCase):
    def test_flat_tile_is_finite_and_does_not_invent_a_ratio(self):
        report = inspect_tile(PpmImage(4, 4, bytes([71, 93, 52]) * 16))
        self.assertTrue(report["powerOfTwo"])
        self.assertEqual(report["rgba8BaseBytes"], 64)
        self.assertEqual(report["rgba8FullMipBytes"], 84)
        self.assertEqual(report["fullMipLevelCount"], 3)
        for axis in report["axes"].values():
            self.assertEqual(axis["seamMeanAbsoluteRgb8Difference"], 0)
            self.assertIsNone(axis["seamToInteriorMeanRatio"])

    def test_directional_ramp_reports_only_its_discontinuous_axis(self):
        row = bytes([0] * 3 + [10] * 3 + [20] * 3 + [30] * 3)
        report = inspect_tile(PpmImage(4, 3, row * 3))
        x = report["axes"]["x"]
        self.assertEqual(x["seamMeanAbsoluteRgb8Difference"], 30)
        self.assertEqual(x["seamRootMeanSquareRgb8Difference"], 30)
        self.assertEqual(x["seamToInteriorMeanRatio"], 3)
        self.assertEqual(x["interiorCutsBelowSeam"], 3)
        self.assertEqual(report["axes"]["y"]["seamMeanAbsoluteRgb8Difference"], 0)
        self.assertFalse(report["powerOfTwo"])
        self.assertEqual(report["rgba8FullMipBytes"], 48 + 8 + 4)

    def test_vertical_ramp_and_three_channels(self):
        pixels = b"".join(bytes([step, step * 2, 0]) * 2 for step in (0, 10, 20))
        report = inspect_tile(PpmImage(2, 3, pixels))
        self.assertEqual(report["axes"]["x"]["seamMeanAbsoluteRgb8Difference"], 0)
        self.assertEqual(report["axes"]["y"]["seamMeanAbsoluteRgb8Difference"], 20)
        self.assertEqual(report["axes"]["y"]["seamToInteriorMeanRatio"], 2)

    def test_reject_invalid_dimensions_and_payloads(self):
        for image in (PpmImage(1, 2, b""), PpmImage(4097, 2, b""),
                      PpmImage(2, -1, b""), PpmImage(2, 2, b""),
                      PpmImage(2, 2, bytes(13))):
            with self.subTest(image=image):
                with self.assertRaises(ValueError):
                    inspect_tile(image)


if __name__ == "__main__":
    unittest.main()
