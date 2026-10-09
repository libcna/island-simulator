#!/usr/bin/env python3
"""Inspect captured skinning inputs offline; never replay GL or access a GPU.

Supports the explicit-layout, buffer-backed skin shader in the preserved incident trace.
Unknown skin input formats fail closed. This cannot qualify hardware or driver behaviour.
"""

import argparse
import json
import math
from pathlib import Path
import re
import struct
import subprocess

from audit_gl_vertex_trace import Auditor, TraceUnpickler


class SkinAuditor(Auditor):
    def __init__(self):
        super().__init__()
        self.sources = {}
        self.attachments = {}
        self.program = 0
        self.skin_programs = {}
        self.normalized = {}
        self.checked = set()
        self.skin_draws = 0
        self.skin_vertices = 0
        self.maximum_bone = -1
        self.bone_locations = {}
        self.uploaded_bones = {}
        self.vertex_bones = {}
        self.palette_uploads = 0
        self.matrix_values = 0

    def consume(self, call):
        self.call, name, pairs, result, _ = call
        a = dict(pairs)
        if name == "glShaderSource":
            self.sources[a["shader"]] = "".join(a["string"])
        elif name == "glAttachShader":
            self.attachments.setdefault(a["program"], []).append(a["shader"])
        elif name == "glLinkProgram":
            self.skin_programs.pop(a["program"], None)
            self.uploaded_bones[a["program"]] = set()
            self.bone_locations[a["program"]] = {}
            for shader in self.attachments.get(a["program"], []):
                source = self.sources[shader]
                if "uBones" not in source:
                    continue
                palette = re.search(r"uniform\s+mat4\s+uBones\[(\d+)\]", source)
                indices = re.search(r"layout\(location=(\d+)\) in vec4 aBoneIndices;", source)
                weights = re.search(r"layout\(location=(\d+)\) in vec4 aBoneWeights;", source)
                if not all((palette, indices, weights)):
                    raise ValueError("unsupported skin shader declaration")
                self.skin_programs[a["program"]] = tuple(
                    int(match[1]) for match in (palette, indices, weights))
        elif name == "glUseProgram":
            self.program = a["program"]
        elif name == "glGetUniformLocation" and result >= 0:
            match = re.fullmatch(r"uBones(?:\[(\d+)\])?", a["name"])
            if match:
                self.bone_locations.setdefault(a["program"], {})[result] = int(match[1] or 0)
        elif name == "glUniformMatrix4fv" \
                and a["location"] in self.bone_locations.get(self.program, {}):
            first = self.bone_locations[self.program][a["location"]]
            count = a["count"]
            values = a["value"]
            palette = self.skin_programs[self.program][0]
            if count < 0 or first + count > palette or len(values) != count * 16:
                raise ValueError("invalid bone matrix upload range or data length")
            self.palette_uploads += 1
            self.matrix_values += len(values)
            for index in range(count):
                bone = first + index
                if all(math.isfinite(v) for v in values[index * 16:(index + 1) * 16]):
                    self.uploaded_bones[self.program].add(bone)
                else:
                    self.uploaded_bones[self.program].discard(bone)
                    self.fail(f"non-finite bone matrix {bone}")
        elif name in ("glVertexAttribPointer", "glVertexAttribIPointer"):
            self.normalized[(self.vao, a["index"])] = a.get("normalized", False)
        super().consume(call)

    def values(self, attribute, vertex):
        key = (self.vao, attribute)
        if attribute not in self.enabled.get(self.vao, set()) or self.divisors.get(key, 0):
            raise ValueError("skin attributes must be enabled and per-vertex")
        buffer, components, type_, stride, offset = self.attributes[key]
        if components != 4 or type_ not in (0x1401, 0x1406) or self.normalized.get(key):
            raise ValueError("unsupported skin attribute encoding")
        code, width = ("B", 1) if type_ == 0x1401 else ("f", 4)
        start = offset + vertex * (stride or width * 4)
        end = start + width * 4
        if start < 0 or end > len(self.buffers[buffer]):
            raise ValueError("skin attribute fetch exceeds storage")
        if not all(self.initialized[buffer][start:end]):
            raise ValueError("skin attribute bytes are uninitialized")
        return struct.unpack_from("<4" + code, self.buffers[buffer], start)

    def draw(self, name, a):
        super().draw(name, a)
        if self.program not in self.skin_programs or not a["count"]:
            return
        if not a.get("instancecount", a.get("primcount", 1)):
            return
        self.skin_draws += 1
        palette, indices_attribute, weights_attribute = self.skin_programs[self.program]
        for attribute in (indices_attribute, weights_attribute):
            key = (self.vao, attribute)
            if attribute not in self.enabled.get(self.vao, set()) \
                    or self.divisors.get(key, 0) or self.normalized.get(key):
                raise ValueError("skin inputs must be enabled, unnormalized and per-vertex")
        attributes = tuple(self.attributes[(self.vao, n)] for n in
                           (indices_attribute, weights_attribute))
        generations = tuple(self.generations[attr[0]] for attr in attributes)
        if "Elements" in name:
            index_buffer = self.element_buffers[self.vao]
            width, code = {0x1401: (1, "B"), 0x1403: (2, "H"), 0x1405: (4, "I")}[a["type"]]
            start = int(a["indices"])
            end = start + a["count"] * width
            if start < 0 or end > len(self.buffers[index_buffer]):
                raise ValueError("skin index fetch exceeds storage")
            if not all(self.initialized[index_buffer][start:end]):
                raise ValueError("skin element indices are uninitialized")
            vertices = {v[0] + a.get("basevertex", 0) for v in struct.iter_unpack(
                "<" + code, memoryview(self.buffers[index_buffer])[start:end])}
        else:
            vertices = range(a["first"], a["first"] + a["count"])
        required_bones = set()
        for vertex in vertices:
            key = (self.program, attributes, generations, vertex)
            if key in self.checked:
                required_bones.update(self.vertex_bones.get(key, ()))
                continue
            self.checked.add(key)
            indices = self.values(indices_attribute, vertex)
            weights = self.values(weights_attribute, vertex)
            self.skin_vertices += 1
            # Check all four lanes, even zero-weight/conditionally unused ones.
            if any(not math.isfinite(v) or v != int(v) or not 0 <= v < palette
                   for v in indices):
                self.fail(f"invalid bone indices at vertex {vertex}: {indices}, palette {palette}")
            else:
                self.maximum_bone = max(self.maximum_bone, int(max(indices)))
                self.vertex_bones[key] = tuple(int(v) for v in indices)
                required_bones.update(self.vertex_bones[key])
            if any(not math.isfinite(v) or v < 0 or v > 1 for v in weights) \
                    or not math.isclose(sum(weights), 1.0, abs_tol=1e-5):
                self.fail(f"invalid bone weights at vertex {vertex}: {weights}")
        missing = required_bones - self.uploaded_bones.get(self.program, set())
        if missing:
            self.fail(f"skin draw references bones without a finite upload: {sorted(missing)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    audit = SkinAuditor()
    process = subprocess.Popen(["apitrace", "pickle", str(args.trace)], stdout=subprocess.PIPE)
    try:
        while True:
            try:
                call = TraceUnpickler(process.stdout).load()
            except EOFError:
                break
            audit.consume(call)
        if process.wait() != 0:
            raise ValueError("trace decoding failed")
    except (KeyError, ValueError, struct.error) as error:
        audit.fail(f"audit incomplete: {error}")
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait()
        process.stdout.close()
    report = {"scope": "Offline fetched skin indices/weights, finite required palette uploads and vertex bounds; not shader output, driver safety, or hardware qualification",
              "frames": audit.frames, "draws": audit.draws, "skin_draws": audit.skin_draws,
              "distinct_skin_vertex_versions": audit.skin_vertices,
              "maximum_bone_index": audit.maximum_bone,
              "palette_uploads": audit.palette_uploads, "palette_float_values": audit.matrix_values,
              "errors": audit.errors}
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return int(bool(audit.errors) or not audit.skin_draws)


if __name__ == "__main__":
    raise SystemExit(main())
