#!/usr/bin/env python3
"""Compare the host PNG pipeline with the unchanged C++ runtime area filter, without graphics."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = Path(sys.argv.pop(1)).resolve()
sys.path.insert(0, str(ROOT / "tools"))
from prepare_material_texture import Image, area_reduce, prepare
from validate_capture import read_ppm


class PreparedMaterialTests(unittest.TestCase):
    def compare(self, image, target):
        with tempfile.TemporaryDirectory(prefix="island-material-test-") as directory:
            base = Path(directory)
            source, output = base / "source.png", base / "prepared.png"
            image.save(source)
            original = source.read_bytes()
            prepare(source, output, target)
            encoded = output.read_bytes()
            with Image.open(output) as result:
                self.assertEqual(result.size, (target, target))
                actual = result.convert("RGB").tobytes()
            rgb = image.convert("RGB").tobytes()
            ppm = base / "source.ppm"
            ppm.write_bytes(f"P6\n{image.width} {image.height}\n255\n".encode() + rgb)
            reference = base / "reference.ppm"
            run = subprocess.run([EXECUTABLE, "--reduce", ppm, reference, str(target)],
                                 capture_output=True, timeout=30)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(actual, read_ppm(reference).pixels)
            # Re-decoding an already reduced PNG must not change the base pixels. The
            # runtime then builds exactly the same lower mips from this identical base.
            rgba = b"".join(actual[i:i+3] + b"\xff" for i in range(0, len(actual), 3))
            self.assertEqual(area_reduce(rgba, target, target), actual)
            prepare(source, output, target)
            self.assertEqual(output.read_bytes(), encoded, "same-host encoding is nondeterministic")
            self.assertEqual(source.read_bytes(), original, "original image was modified")
            self.assertEqual(list(base.glob("*.tmp")), [])

    def test_noninteger_scales_and_constant_fields(self):
        for source, target in ((3,2), (7,4), (11,8), (2,8), (8,2)):
            for constant in (True,False):
                with self.subTest(source=source, target=target, constant=constant):
                    pixels = bytes((59,123,207,255)) * (source*source) if constant else bytes(
                        c for i in range(source*source)
                        for c in ((i*47)%256, (i*103+9)%256, (i*199+27)%256, 255))
                    with Image.frombytes("RGBA", (source,source), pixels) as image:
                        self.compare(image, target)

    def test_actual_artwork_matches_cpp_pixel_for_pixel(self):
        for name, size in (("forest-floor-albedo-v1.png",512), ("leaf-lamina-v1.png",256),
                           ("monkey-fur-albedo-v2.png",512), ("goat-fur-albedo-v1.png",256),
                           ("boar-bristle-albedo-v2.png",256)):
            with self.subTest(name=name), Image.open(ROOT / "assets/art-studies" / name) as image:
                self.compare(image, size)

    def test_invalid_inputs_preserve_existing_output(self):
        with tempfile.TemporaryDirectory(prefix="island-material-test-") as directory:
            base = Path(directory)
            source, output = base / "source.png", base / "prepared.png"
            for mode, dimensions, colour, size in (
                ("RGB",(2,3),(4,5,6),2), ("RGBA",(2,2),(4,5,6,254),2),
                ("RGB",(2049,2),(4,5,6),2), ("RGB",(2,2),(4,5,6),3),
                ("RGB",(2,2),(4,5,6),2048), ("L",(2,2),255,2)):
                with self.subTest(mode=mode, dimensions=dimensions, size=size):
                    with Image.new(mode, dimensions, colour) as image:
                        image.save(source)
                    output.write_bytes(b"previous-good-build")
                    with self.assertRaises(ValueError): prepare(source,output,size)
                    self.assertEqual(output.read_bytes(), b"previous-good-build")
                    self.assertEqual(list(base.glob("*.tmp")), [])
            source.write_bytes(b"invalid PNG" * 10)
            with self.assertRaises(ValueError): prepare(source,output,2)
            with self.assertRaises(ValueError): prepare(source,source,2)
            self.assertEqual(source.read_bytes(), b"invalid PNG" * 10)


if __name__ == "__main__":
    unittest.main()
