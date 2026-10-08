#!/usr/bin/env python3
"""Regression checks for the offline incident trace auditor; no GL or display is used."""

import importlib.util
from pathlib import Path
import struct
import unittest

SPEC = importlib.util.spec_from_file_location(
    "audit_gl_vertex_trace", Path(__file__).resolve().parent.parent / "tools/audit_gl_vertex_trace.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class TraceAuditTests(unittest.TestCase):
    def setUp(self):
        self.audit = MODULE.Auditor()
        self.call("glBindVertexArray", array=1)
        self.call("glBindBuffer", target=0x8892, buffer=1)
        self.call("glBufferData", target=0x8892, size=36, data=bytes(36))
        self.call("glVertexAttribPointer", index=0, size=3, type=0x1406, stride=12, pointer=0)
        self.call("glEnableVertexAttribArray", index=0)
        self.call("glBindBuffer", target=0x8893, buffer=2)
        self.call("glBufferData", target=0x8893, size=6, data=struct.pack("<3H", 0, 1, 2))

    def call(self, name, **args):
        self.audit.consume((self.audit.call + 1, name, list(args.items()), None, 0))

    def draw(self, **kwargs):
        args = {"count": 3, "type": 0x1403, "indices": 0}
        args.update(kwargs)
        self.call("glDrawElements", **args)

    def test_valid_boundary_draw(self):
        self.draw()
        self.assertEqual(self.audit.errors, [])
        self.assertEqual(self.audit.fetches, 1)

    def test_bad_index_value_exceeds_vertex_storage(self):
        self.call("glBufferSubData", target=0x8893, offset=4, size=2, data=struct.pack("<H", 3))
        self.draw()
        self.assertIn("capacity 36", self.audit.errors[0]["message"])

    def test_index_range_exceeds_allocation(self):
        self.draw(indices=2)
        self.assertIn("index fetch", self.audit.errors[0]["message"])

    def test_instance_divisor_and_nonzero_offset(self):
        self.call("glBindBuffer", target=0x8892, buffer=3)
        self.call("glBufferData", target=0x8892, size=32, data=bytes(32))
        self.call("glVertexAttribPointer", index=1, size=4, type=0x1406, stride=16, pointer=16)
        self.call("glEnableVertexAttribArray", index=1)
        self.call("glVertexAttribDivisor", index=1, divisor=2)
        self.call("glDrawElementsInstanced", count=3, type=0x1403, indices=0, primcount=2)
        self.assertEqual(self.audit.errors, [])
        self.call("glDrawElementsInstanced", count=3, type=0x1403, indices=0, primcount=3)
        self.assertIn("divisor 2", self.audit.errors[0]["message"])

    def test_index_cache_invalidates_after_upload(self):
        self.draw()
        self.call("glBufferData", target=0x8893, size=6, data=struct.pack("<3H", 0, 1, 9))
        self.draw()
        self.assertTrue(self.audit.errors)

    def test_attribute_offset_exceeds_allocation(self):
        self.call("glVertexAttribPointer", index=0, size=3, type=0x1406, stride=12, pointer=4)
        self.draw()
        self.assertIn("fetch ends at 40", self.audit.errors[0]["message"])

    def test_unknown_draw_cannot_silently_pass(self):
        with self.assertRaises(ValueError):
            self.call("glDrawElementsIndirect")

    def test_missing_index_contents_cannot_silently_pass(self):
        self.call("glBufferData", target=0x8893, size=6, data=0)
        with self.assertRaises(ValueError):
            self.draw()


if __name__ == "__main__":
    unittest.main()
