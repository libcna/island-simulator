#!/usr/bin/env python3
"""Exercise the offline C++ material renderer without starting CNA or a graphics device."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXECUTABLE = Path(sys.argv.pop(1)).resolve()
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from validate_capture import read_ppm


class ForestFloorPreviewTests(unittest.TestCase):
    def test_uniform_colour_survives_sampling_mips_and_output(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "source.ppm"
            target = Path(directory) / "preview.ppm"
            pixel = bytes([59,123,207])
            source.write_bytes(b"P6\r\n# fixture\r\n3 3\r\n255\r\n" + pixel * 9)
            result = subprocess.run([EXECUTABLE, source, target], capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            image = read_ppm(target)
            self.assertEqual((image.width, image.height), (1024,1024))
            self.assertEqual(image.pixels, pixel * (1024*1024))

    def test_invalid_sources_never_create_output(self):
        fixtures = [b"P3\n2 2\n255\n", b"P6\n2049 2049\n255\n",
                    b"P6\n2x 2\n255\n", b"P6\n2 2\n256\n",
                    b"P6\n2 2\n255\n" + bytes(11),
                    b"P6\n2 2\n255\n" + bytes(13),
                    b"P6\n2 3\n255\n" + bytes(18)]
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "source.ppm"
            target = Path(directory) / "preview.ppm"
            for payload in fixtures:
                with self.subTest(payload=payload):
                    source.write_bytes(payload)
                    result = subprocess.run([EXECUTABLE, source, target], capture_output=True, timeout=10)
                    self.assertEqual(result.returncode, 1)
                    self.assertFalse(target.exists())

    def test_source_cannot_be_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "source.ppm"
            payload = b"P6\n2 2\n255\n" + bytes(12)
            source.write_bytes(payload)
            result = subprocess.run([EXECUTABLE, source, source], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(source.read_bytes(), payload)


if __name__ == "__main__":
    unittest.main()
