#!/usr/bin/env python3
"""CPU-only checks for the incident trace-state inspector."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from inspect_gl_trace_state import Inspector


class TraceStateTests(unittest.TestCase):
    def setUp(self):
        self.inspector = Inspector()
        self.call("glShaderSource", shader=1, string=["uniform sampler2D image;"])
        self.call("glAttachShader", program=3, shader=1)
        self.call("glLinkProgram", program=3)
        self.call("glGetProgramiv", program=3, pname=0x8B82, params=[1])
        self.call("glUseProgram", program=3)
        self.call("glGetUniformLocation", result=2, program=3, name="image")
        self.call("glUniform1i", location=2, v0=1)
        self.call("glActiveTexture", texture=0x84C1)
        self.call("glBindTexture", target=0x0DE1, texture=8)
        self.image(0, 4, 4)
        self.parameter(0x2801, 0x2601)

    def image(self, level, width, height, image_format=0x1908):
        self.call("glTexImage2D", target=0x0DE1, level=level, width=width, height=height,
                  border=0, internalformat=image_format)

    def parameter(self, name, value):
        self.call("glTexParameteri", target=0x0DE1, pname=name, param=value)

    def call(self, api, result=None, **args):
        self.inspector.consume((self.inspector.call + 1, api, list(args.items()), result, 0))

    def test_valid_program_sampler(self):
        self.call("glDrawElements")
        self.assertEqual(self.inspector.errors, [])
        self.assertEqual(self.inspector.sampled[(3, 8)], 1)

    def test_absent_texture(self):
        self.call("glUniform1i", location=2, v0=5)
        self.call("glDrawElements")
        self.assertIn("without a base-level image", self.inspector.errors[0]["message"])

    def test_missing_mipmap(self):
        self.parameter(0x2801, 0x2703)
        self.call("glDrawElements")
        self.assertIn("mip level 1", self.inspector.errors[0]["message"])

    def test_complete_non_power_of_two_chain(self):
        self.parameter(0x2801, 0x2703)
        for level, size in enumerate(((7, 3), (3, 1), (1, 1))):
            self.image(level, *size)
        self.call("glDrawElements")
        self.assertEqual(self.inspector.errors, [])

    def test_sampler_override_requires_mips(self):
        self.call("glBindSampler", unit=1, sampler=9)
        self.call("glDrawElements")  # Default sampler minification requires mips.
        self.assertTrue(self.inspector.errors)

    def test_max_level_zero_with_mip_sampler_is_complete(self):
        self.parameter(0x813D, 0)
        self.call("glBindSampler", unit=1, sampler=9)
        self.call("glDrawElements")
        self.assertEqual(self.inspector.errors, [])

    def test_sampler_override_disables_mips(self):
        self.parameter(0x2801, 0x2703)
        self.call("glBindSampler", unit=1, sampler=9)
        self.call("glSamplerParameteri", sampler=9, pname=0x2801, param=0x2601)
        self.call("glDrawElements")
        self.assertEqual(self.inspector.errors, [])

    def test_deleted_sampler_restores_texture_parameters(self):
        self.parameter(0x2801, 0x2703)
        self.call("glBindSampler", unit=1, sampler=9)
        self.call("glSamplerParameteri", sampler=9, pname=0x2801, param=0x2601)
        self.call("glDeleteSamplers", samplers=[9])
        self.call("glDrawElements")
        self.assertTrue(self.inspector.errors)

    def test_nonzero_base_and_bounded_chain(self):
        self.parameter(0x813C, 2)
        self.parameter(0x813D, 3)
        self.parameter(0x2801, 0x2703)
        self.image(2, 8, 4)
        self.image(3, 4, 2)
        self.call("glDrawElements")
        self.assertEqual(self.inspector.errors, [])

    def test_lod_clamp_does_not_replace_mip_completeness(self):
        self.parameter(0x2801, 0x2703)
        self.parameter(0x813B, 0)  # GL_TEXTURE_MAX_LOD is not GL_TEXTURE_MAX_LEVEL.
        self.call("glDrawElements")
        self.assertIn("mip level 1", self.inspector.errors[0]["message"])

    def test_invalid_level_range(self):
        self.parameter(0x813C, 2)
        self.parameter(0x813D, 1)
        self.call("glDrawElements")
        self.assertIn("invalid base/max", self.inspector.errors[0]["message"])

    def test_absent_nonzero_base_image(self):
        self.parameter(0x813C, 2)
        self.call("glDrawElements")
        self.assertIn("base-level image (level 2)", self.inspector.errors[0]["message"])

    def test_wrong_mip_size_or_format(self):
        for width, image_format in ((3, 0x1908), (2, 0x1907)):
            with self.subTest(width=width, image_format=image_format):
                self.inspector.errors.clear()
                self.parameter(0x2801, 0x2703)
                self.image(1, width, 2, image_format)
                self.image(2, 1, 1)
                self.call("glDrawElements")
                self.assertIn("mip level 1", self.inspector.errors[0]["message"])

    def test_texture_reuse_resets_parameters(self):
        self.parameter(0x813D, 0)
        self.call("glDeleteTextures", textures=[8])
        self.call("glBindTexture", target=0x0DE1, texture=8)
        self.image(0, 4, 4)
        self.call("glDrawElements")
        self.assertTrue(self.inspector.errors)

    def test_unsupported_storage_fails_closed(self):
        with self.assertRaisesRegex(ValueError, "unsupported texture state"):
            self.call("glTexStorage2D")

    def test_texture_feedback(self):
        self.call("glBindFramebuffer", target=0x8D40, framebuffer=5)
        self.call("glFramebufferTexture2D", target=0x8D40, attachment=0x8CE0, texture=8)
        self.call("glDrawElements")
        self.assertIn("draw framebuffer", self.inspector.errors[0]["message"])

    def test_read_framebuffer_is_not_draw_feedback(self):
        self.call("glBindFramebuffer", target=0x8CA8, framebuffer=5)
        self.call("glFramebufferTexture2D", target=0x8CA8, attachment=0x8CE0, texture=8)
        self.call("glDrawElements")
        self.assertEqual(self.inspector.errors, [])

    def test_deleted_texture_name_cannot_reuse_old_image(self):
        self.call("glDeleteTextures", textures=[8])
        self.call("glBindTexture", target=0x0DE1, texture=8)
        self.call("glDrawElements")
        self.assertTrue(self.inspector.errors)

    def test_link_failure(self):
        self.call("glLinkProgram", program=3)
        self.call("glGetProgramiv", program=3, pname=0x8B82, params=[0])
        self.call("glDrawElements")
        self.assertIn("did not link", self.inspector.errors[0]["message"])

    def test_unsupported_sampler_is_not_silently_accepted(self):
        self.call("glShaderSource", shader=1, string=["uniform samplerCube sky;"])
        with self.assertRaisesRegex(ValueError, "unsupported sampler"):
            self.call("glLinkProgram", program=3)

    def test_setup_and_teardown_not_counted_as_midstream_deletions(self):
        self.call("glDeleteBuffers", buffers=[1])
        self.call("glXSwapBuffers")
        self.call("glDeleteBuffers", buffers=[2])
        self.call("glXSwapBuffers")
        self.call("glDeleteBuffers", buffers=[3])
        self.assertEqual(self.inspector.report()["deletion_calls_between_first_and_last_swap"], 1)


if __name__ == "__main__":
    unittest.main()
