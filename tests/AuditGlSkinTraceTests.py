#!/usr/bin/env python3
"""CPU-only regression cases for captured skinning input validation."""

from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from audit_gl_skin_trace import SkinAuditor


class SkinTraceTests(unittest.TestCase):
    def setUp(self):
        self.audit = SkinAuditor()
        self.call("glShaderSource", shader=1, string=[
            "layout(location=3) in vec4 aBoneWeights;\n"
            "layout(location=4) in vec4 aBoneIndices;\n"
            "uniform mat4 uBones[72];"])
        self.call("glAttachShader", program=3, shader=1)
        self.call("glLinkProgram", program=3)
        self.call("glUseProgram", program=3)
        self.call("glGetUniformLocation", result=4, program=3, name="uBones[0]")
        self.call("glUniformMatrix4fv", location=4, count=14, transpose=False,
                  value=[1 if i % 5 == 0 else 0 for i in range(16)] * 14)
        self.call("glBindVertexArray", array=5)
        self.call("glBindBuffer", target=0x8892, buffer=8)
        self.upload()
        for index, type_, offset in ((3, 0x1406, 0), (4, 0x1401, 16)):
            self.call("glVertexAttribPointer", index=index, size=4, type=type_,
                      stride=20, pointer=offset, normalized=False)
            self.call("glEnableVertexAttribArray", index=index)

    def call(self, api, result=None, **args):
        self.audit.consume((self.audit.call + 1, api, list(args.items()), result, 0))

    def upload(self, bones=(13, 0, 0, 0), weights=(1, 0, 0, 0)):
        data = struct.pack("<4f4B", *weights, *bones)
        self.call("glBufferData", target=0x8892, size=len(data), data=data)

    def draw(self):
        self.call("glDrawArrays", first=0, count=1)

    def test_valid_and_cached(self):
        self.draw()
        self.draw()
        self.assertEqual(self.audit.errors, [])
        self.assertEqual(self.audit.maximum_bone, 13)
        self.assertEqual(self.audit.skin_vertices, 1)
        self.assertEqual(self.audit.skin_draws, 2)

    def test_orphaning_rechecks_changed_storage(self):
        self.draw()
        self.upload(bones=(72, 0, 0, 0))
        self.draw()
        self.assertEqual(self.audit.skin_vertices, 2)
        self.assertIn("invalid bone indices", self.audit.errors[0]["message"])

    def test_zero_weight_lane_still_checked(self):
        self.upload(bones=(0, 255, 0, 0))
        self.draw()
        self.assertTrue(self.audit.errors)

    def test_nan_weight(self):
        self.upload(weights=(float("nan"), 0, 0, 0))
        self.draw()
        self.assertIn("invalid bone weights", self.audit.errors[0]["message"])

    def test_weight_sum(self):
        self.upload(weights=(0.1, 0, 0, 0))
        self.draw()
        self.assertTrue(self.audit.errors)

    def test_uninitialized_storage(self):
        self.call("glBufferData", target=0x8892, size=20, data=0)
        with self.assertRaisesRegex(ValueError, "uninitialized"):
            self.draw()

    def test_cached_vertex_does_not_hide_disabled_attribute(self):
        self.draw()
        self.call("glDisableVertexAttribArray", index=4)
        with self.assertRaisesRegex(ValueError, "enabled"):
            self.draw()

    def test_cached_vertex_does_not_hide_normalization_change(self):
        self.draw()
        self.call("glVertexAttribPointer", index=4, size=4, type=0x1401,
                  stride=20, pointer=16, normalized=True)
        with self.assertRaisesRegex(ValueError, "unnormalized"):
            self.draw()

    def test_actual_element_selection(self):
        self.upload(bones=(255, 0, 0, 0))
        good = struct.pack("<4f4B", 1, 0, 0, 0, 5, 0, 0, 0)
        bad = bytes(self.audit.buffers[8])
        self.call("glBufferData", target=0x8892, size=40, data=bad + good)
        self.call("glBindBuffer", target=0x8893, buffer=9)
        self.call("glBufferData", target=0x8893, size=2, data=struct.pack("<H", 1))
        self.call("glDrawElements", count=1, type=0x1403, indices=0)
        self.assertEqual(self.audit.errors, [])
        self.assertEqual(self.audit.maximum_bone, 5)

    def test_unknown_skin_layout_rejected(self):
        self.call("glShaderSource", shader=1, string=["uniform mat4 uBones[72];"])
        with self.assertRaisesRegex(ValueError, "unsupported skin shader"):
            self.call("glLinkProgram", program=3)

    def test_missing_palette(self):
        self.audit.uploaded_bones[3].clear()
        self.draw()
        self.assertIn("without a finite upload", self.audit.errors[0]["message"])

    def test_nonfinite_palette_after_cached_draw(self):
        self.draw()
        self.call("glUniformMatrix4fv", location=4, count=1, transpose=False,
                  value=[float("nan")] * 16)
        self.draw()
        self.assertTrue(any("without a finite upload" in e["message"] for e in self.audit.errors))

    def test_partial_upload_cannot_cover_missing_bone(self):
        self.audit.uploaded_bones[3].clear()
        self.call("glUniformMatrix4fv", location=4, count=1, transpose=False, value=[0] * 16)
        self.draw()
        self.assertIn("[13]", self.audit.errors[0]["message"])

    def test_bad_upload_length_rejected(self):
        with self.assertRaisesRegex(ValueError, "data length"):
            self.call("glUniformMatrix4fv", location=4, count=14, transpose=False, value=[0] * 16)


if __name__ == "__main__":
    unittest.main()
