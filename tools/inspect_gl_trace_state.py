#!/usr/bin/env python3
"""Inspect an apitrace offline; never execute/replay its GL commands.

Report allocation churn and deletion timing, and check the simple sampler2D/program path used
by this game's trace. This is not a general OpenGL validator or proof of hardware safety.
Shader arithmetic, driver storage renaming, synchronization, and context ownership are not checked.
"""

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import subprocess

from audit_gl_vertex_trace import TraceUnpickler


class Inspector:
    def __init__(self):
        self.frame = 0
        self.call = 0
        self.program = 0
        self.sources = {}
        self.attachments = {}
        self.samplers = {}
        self.locations = {}
        self.uniforms = {}
        self.linked = set()
        self.unit = 0
        self.textures = {}
        self.images = {}
        self.texture_parameters = {}
        self.sampler_parameters = {}
        self.bound_samplers = {}
        self.sampling_configurations = Counter()
        self.framebuffer = 0
        self.read_framebuffer = 0
        self.framebuffer_textures = {}
        self.draws = Counter()
        self.sampled = Counter()
        self.allocations = Counter()
        self.allocation_bytes = 0
        self.deletions = []
        self.errors = []
        self.compile_checks = 0
        self.link_checks = 0

    def fail(self, message):
        if len(self.errors) < 100:
            self.errors.append({"call": self.call, "frame": self.frame, "message": message})

    def check_texture(self, unit, texture):
        """Check mutable normalized-color 2D mip chains; do not infer driver storage safety."""
        parameters = self.texture_parameters.get(texture, {})
        base = parameters.get(0x813C, 0)  # GL_TEXTURE_BASE_LEVEL
        maximum = parameters.get(0x813D, 1000)  # GL_TEXTURE_MAX_LEVEL
        sampler = self.bound_samplers.get(unit, 0)
        sampling = self.sampler_parameters.get(sampler, {}) if sampler else parameters
        min_filter = sampling.get(0x2801, 0x2702)  # Default NEAREST_MIPMAP_LINEAR
        if min_filter not in (0x2600, 0x2601, 0x2700, 0x2701, 0x2702, 0x2703):
            raise ValueError(f"unsupported minification filter: {min_filter}")
        if sampling.get(0x884C, 0) != 0:
            raise ValueError("comparison sampling is outside the normalized-color audit")
        self.sampling_configurations[(texture, sampler, base, maximum, min_filter)] += 1
        if base < 0 or maximum < base:
            self.fail(f"texture {texture} has invalid base/max level range")
            return
        image = self.images.get((texture, base))
        if not texture or image is None or min(image[:2]) <= 0:
            self.fail(f"unit {unit} without a base-level image (level {base})")
            return
        width, height, image_format = image
        # The incident's sampled images are normalized color, not integer/depth/compressed.
        if image_format not in (0x1907, 0x1908, 0x8051, 0x8058):
            raise ValueError(f"unsupported sampled internal format: {image_format}")
        last = base if min_filter in (0x2600, 0x2601) else min(
            maximum, base + max(width, height).bit_length() - 1)
        for level in range(base, last + 1):
            expected = (max(1, width >> (level - base)),
                        max(1, height >> (level - base)), image_format)
            if self.images.get((texture, level)) != expected:
                self.fail(f"texture {texture} has missing/mismatched mip level {level}")
                return

    def consume(self, call):
        self.call, name, pairs, result, _flags = call
        a = dict(pairs)
        if name == "glXSwapBuffers":
            self.frame += 1
        elif name == "glShaderSource":
            self.sources[a["shader"]] = "".join(a["string"])
        elif name == "glAttachShader":
            self.attachments.setdefault(a["program"], []).append(a["shader"])
        elif name == "glLinkProgram":
            program = a["program"]
            self.linked.discard(program)
            declarations = set()
            for shader in self.attachments[program]:
                source = re.sub(r"/\*.*?\*/|//[^\n]*", "", self.sources[shader], flags=re.S)
                for declaration in re.findall(r"uniform\s+[^;]+;", source):
                    if "sampler" not in declaration:
                        continue
                    match = re.fullmatch(r"uniform\s+sampler2D\s+(\w+)\s*;", declaration)
                    if not match:
                        raise ValueError(f"unsupported sampler declaration: {declaration}")
                    declarations.add(match[1])
            self.samplers[program] = sorted(declarations)
            self.uniforms[program] = {}
            self.locations[program] = {}
        elif name == "glGetShaderiv" and a["pname"] == 0x8B81:
            self.compile_checks += 1
            if a["params"] != [1]:
                self.fail(f"shader {a['shader']} did not compile")
        elif name == "glGetProgramiv" and a["pname"] == 0x8B82:
            self.link_checks += 1
            if a["params"] == [1]:
                self.linked.add(a["program"])
            else:
                self.fail(f"program {a['program']} did not link")
        elif name == "glUseProgram":
            self.program = a["program"]
        elif name == "glGetUniformLocation":
            self.locations.setdefault(a["program"], {})[a["name"]] = result
        elif name == "glUniform1i" and a["location"] >= 0:
            self.uniforms.setdefault(self.program, {})[a["location"]] = a["v0"]
        elif name == "glActiveTexture":
            self.unit = a["texture"] - 0x84C0
        elif name == "glBindTexture":
            if a["target"] != 0x0DE1:
                raise ValueError("only the trace's GL_TEXTURE_2D path is supported")
            self.textures[self.unit] = a["texture"]
        elif name == "glTexImage2D":
            if a["target"] != 0x0DE1 or a["border"] != 0:
                raise ValueError("only borderless mutable GL_TEXTURE_2D images are supported")
            self.images[(self.textures[self.unit], a["level"])] = (
                a["width"], a["height"], a["internalformat"])
        elif name in ("glTexParameteri", "glTexParameterf", "glSamplerParameteri", "glSamplerParameterf"):
            if name.startswith("glTex"):
                if a["target"] != 0x0DE1:
                    raise ValueError("unsupported texture parameter target")
                parameters = self.texture_parameters.setdefault(self.textures[self.unit], {})
            else:
                parameters = self.sampler_parameters.setdefault(a["sampler"], {})
            if a["pname"] in (0x813C, 0x813D, 0x2801, 0x884C):
                if int(a["param"]) != a["param"]:
                    raise ValueError("nonintegral texture completeness parameter")
                parameters[a["pname"]] = int(a["param"])
        elif name == "glBindSampler":
            self.bound_samplers[a["unit"]] = a["sampler"]
        elif name == "glGenSamplers":
            for sampler in a["samplers"]:
                self.sampler_parameters[sampler] = {}
        elif name.startswith(("glTexImage", "glTexStorage", "glCompressedTex", "glCopyTexImage",
                              "glGenerateMipmap", "glTexture", "glBindTextures", "glBindSamplers",
                              "glTexParameter", "glSamplerParameter")):
            raise ValueError(f"unsupported texture state operation: {name}")
        elif name == "glBindFramebuffer":
            if a["target"] in (0x8D40, 0x8CA9):
                self.framebuffer = a["framebuffer"]
            if a["target"] in (0x8D40, 0x8CA8):
                self.read_framebuffer = a["framebuffer"]
        elif name == "glGenFramebuffers":
            for framebuffer in a["framebuffers"]:
                self.framebuffer_textures[framebuffer] = {}
        elif name in ("glFramebufferTexture2D", "glFramebufferRenderbuffer"):
            framebuffer = self.read_framebuffer if a["target"] == 0x8CA8 else self.framebuffer
            self.framebuffer_textures.setdefault(framebuffer, {})[a["attachment"]] = a.get("texture", 0)
        elif name == "glBufferData":
            self.allocations[(a["size"], isinstance(a["data"], bytes))] += 1
            self.allocation_bytes += a["size"]
        elif name.startswith("glDelete") and name != "glDeleteShader":
            # Deleting an in-flight GL object is not by itself a use-after-free. Keep facts,
            # not a simplistic lifetime verdict. The driver must retain pending GPU storage.
            self.deletions.append({"call": self.call, "frame": self.frame, "api": name})
            if name == "glDeleteFramebuffers":
                if self.framebuffer in a["framebuffers"]:
                    self.framebuffer = 0
                if self.read_framebuffer in a["framebuffers"]:
                    self.read_framebuffer = 0
            elif name == "glDeleteTextures":
                for texture in a["textures"]:
                    self.texture_parameters.pop(texture, None)
                for unit, texture in self.textures.items():
                    if texture in a["textures"]:
                        self.textures[unit] = 0
                for key in list(self.images):
                    if key[0] in a["textures"]:
                        del self.images[key]
            elif name == "glDeleteSamplers":
                for sampler in a["samplers"]:
                    self.sampler_parameters.pop(sampler, None)
                for unit, sampler in self.bound_samplers.items():
                    if sampler in a["samplers"]:
                        self.bound_samplers[unit] = 0
        elif name.startswith("glDraw") and name not in ("glDrawBuffer", "glDrawBuffers"):
            self.draws[self.program] += 1
            if self.program not in self.linked:
                self.fail(f"no observed successful link for draw program {self.program}")
                return
            for sampler in self.samplers.get(self.program, []):
                location = self.locations[self.program].get(sampler)
                if location is None:
                    raise ValueError(f"sampler activity unknown: {self.program}/{sampler}")
                if location == -1:
                    continue
                unit = self.uniforms[self.program].get(location, 0)
                texture = self.textures.get(unit, 0)
                self.check_texture(unit, texture)
                if texture and texture in self.framebuffer_textures.get(self.framebuffer, {}).values():
                    self.fail(f"sampled texture {texture} is also attached to the draw framebuffer")
                self.sampled[(self.program, texture)] += 1

    def report(self):
        return {
            "scope": "Offline program links, mutable normalized-color sampler2D mip completeness with sampler overrides, allocation/deletion inventory; not texel contents, lifetimes, shader UB, or hardware qualification",
            "frames": self.frame,
            "draws": sum(self.draws.values()),
            "draws_by_program": dict(sorted(self.draws.items())),
            "shader_compile_checks": self.compile_checks,
            "program_link_checks": self.link_checks,
            "sampled_program_texture_pairs": [
                {"program": p, "texture": t, "draws": n}
                for (p, t), n in sorted(self.sampled.items())],
            "sampling_configurations": [
                {"texture": t, "sampler": s, "base_level": b, "max_level": m,
                 "min_filter": f, "draws": n}
                for (t, s, b, m, f), n in sorted(self.sampling_configurations.items())],
            "buffer_data_calls": sum(self.allocations.values()),
            "buffer_data_requested_bytes": self.allocation_bytes,
            "most_frequent_allocations": [
                {"bytes": size, "initial_data": data, "calls": count}
                for (size, data), count in self.allocations.most_common(8)],
            "first_non_shader_deletion": self.deletions[0] if self.deletions else None,
            "deletion_calls_between_first_and_last_swap": sum(0 < d["frame"] < self.frame for d in self.deletions),
            "errors": self.errors,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    inspector = Inspector()
    process = subprocess.Popen(["apitrace", "pickle", str(args.trace)], stdout=subprocess.PIPE)
    try:
        while True:
            try:
                call = TraceUnpickler(process.stdout).load()
            except EOFError:
                break
            inspector.consume(call)
        if process.wait() != 0:
            raise ValueError("trace decoding failed")
    except (ValueError, KeyError) as error:
        inspector.fail(f"inspection incomplete: {error}")
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait()
        process.stdout.close()
    report = inspector.report()
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 1 if inspector.errors or not inspector.draws else 0


if __name__ == "__main__":
    raise SystemExit(main())
