#!/usr/bin/env python3
"""Audit vertex/index fetch bounds in a local apitrace without executing any GL calls.

This checks the buffer-backed draw forms used by island-simulator. It is not a complete GL
validator: textures, shader semantics, GPU synchronization, and driver command streams are outside
its scope. All enabled attributes are checked conservatively, including shader-inactive ones.
"""

import argparse
import json
import pickle
import struct
import subprocess
import sys
from pathlib import Path


class TraceUnpickler(pickle.Unpickler):
    def find_class(self, module, name):
        if (module, name) == ("unpickle", "Pointer"):
            return int
        raise pickle.UnpicklingError(f"unsupported trace object {module}.{name}")


class Auditor:
    def __init__(self):
        self.vao = 0
        self.array_buffer = 0
        self.element_buffers = {}
        self.attributes = {}
        self.enabled = {}
        self.divisors = {}
        self.buffers = {}
        self.initialized = {}
        self.generations = {}
        self.index_cache = {}
        self.draws = 0
        self.fetches = 0
        self.frames = 0
        self.errors = []
        self.call = 0

    def fail(self, message):
        if len(self.errors) < 100:
            self.errors.append({"call": self.call, "frame": self.frames, "message": message})

    def buffer_binding(self, target):
        if target == 0x8892:
            return self.array_buffer
        if target == 0x8893:
            return self.element_buffers.get(self.vao, 0)
        raise ValueError(f"unsupported buffer target {target:#x}")

    def consume(self, call):
        self.call, name, pairs, _return_value, _flags = call
        args = dict(pairs)
        if name == "glXSwapBuffers":
            self.frames += 1
        elif name == "glBindVertexArray":
            self.vao = args["array"]
        elif name == "glBindBuffer":
            if args["target"] == 0x8892:
                self.array_buffer = args["buffer"]
            elif args["target"] == 0x8893:
                self.element_buffers[self.vao] = args["buffer"]
        elif name in ("glBufferData", "glBufferSubData"):
            if args["target"] not in (0x8892, 0x8893):
                return
            buffer = self.buffer_binding(args["target"])
            size = args["size"]
            data = args["data"]
            if name == "glBufferData":
                self.buffers[buffer] = bytearray(size)
                self.initialized[buffer] = bytearray(size)
            storage = self.buffers[buffer]
            offset = args.get("offset", 0)
            if offset < 0 or offset + size > len(storage):
                self.fail(f"buffer {buffer} upload exceeds allocation")
            elif isinstance(data, bytes):
                if len(data) != size:
                    self.fail(f"buffer {buffer} upload trace blob size mismatch")
                else:
                    storage[offset:offset + size] = data
                    self.initialized[buffer][offset:offset + size] = bytes([1]) * size
            self.generations[buffer] = self.generations.get(buffer, 0) + 1
        elif name in ("glVertexAttribPointer", "glVertexAttribIPointer"):
            key = (self.vao, args["index"])
            self.attributes[key] = (self.array_buffer, args["size"], args["type"],
                                    args["stride"], int(args["pointer"]))
        elif name == "glEnableVertexAttribArray":
            self.enabled.setdefault(self.vao, set()).add(args["index"])
        elif name == "glDisableVertexAttribArray":
            self.enabled.setdefault(self.vao, set()).discard(args["index"])
        elif name == "glVertexAttribDivisor":
            self.divisors[(self.vao, args["index"])] = args["divisor"]
        elif name.startswith("glDraw") and name not in ("glDrawBuffer", "glDrawBuffers"):
            self.draw(name, args)
        elif name.startswith(("glMapBuffer", "glNamedBuffer", "glBindVertexBuffer",
                              "glVertexAttribFormat", "glVertexAttribIFormat")) \
                or name in ("glBufferStorage", "glCopyBufferSubData", "glPrimitiveRestartIndex"):
            raise ValueError(f"unsupported buffer/attribute mutation {name}; audit is incomplete")

    def draw(self, name, args):
        supported = {"glDrawElements", "glDrawElementsBaseVertex", "glDrawElementsInstanced",
                     "glDrawElementsInstancedBaseVertex", "glDrawArrays", "glDrawArraysInstanced"}
        if name not in supported:
            raise ValueError(f"unsupported draw API {name}; audit is incomplete")
        self.draws += 1
        count = args["count"]
        instances = args.get("instancecount", args.get("primcount", 1))
        if count == 0 or instances == 0:
            return
        if "Elements" in name:
            buffer = self.element_buffers.get(self.vao, 0)
            data = self.buffers.get(buffer)
            if data is None:
                self.fail(f"draw uses missing index buffer {buffer}")
                return
            formats = {0x1401: (1, "B"), 0x1403: (2, "H"), 0x1405: (4, "I")}
            width, code = formats[args["type"]]
            offset = int(args["indices"])
            end = offset + width * count
            if offset < 0 or end > len(data):
                self.fail(f"index fetch {offset}:{end} exceeds buffer {buffer} size {len(data)}")
                return
            key = (buffer, self.generations[buffer], offset, count, code)
            if key not in self.index_cache:
                if not all(self.initialized[buffer][offset:end]):
                    raise ValueError("index values are not fully initialized in the trace")
                indices = [item[0] for item in struct.iter_unpack("<" + code, memoryview(data)[offset:end])]
                self.index_cache[key] = min(indices), max(indices)
            minimum, maximum = self.index_cache[key]
            minimum += args.get("basevertex", 0)
            maximum += args.get("basevertex", 0)
            if minimum < 0:
                self.fail("negative effective vertex index")
        else:
            maximum = args["first"] + count - 1
        widths = {0x1400: 1, 0x1401: 1, 0x1402: 2, 0x1403: 2, 0x1404: 4,
                  0x1405: 4, 0x1406: 4, 0x140A: 8, 0x140B: 2}
        for attribute in self.enabled.get(self.vao, set()):
            key = (self.vao, attribute)
            buffer, components, type_, stride, offset = self.attributes[key]
            element_size = widths[type_] * components
            stride = stride or element_size
            divisor = self.divisors.get(key, 0)
            last = (instances - 1) // divisor if divisor else maximum
            end = offset + last * stride + element_size
            storage = self.buffers.get(buffer)
            self.fetches += 1
            if storage is None or offset < 0 or end > len(storage):
                capacity = len(storage) if storage is not None else 0
                self.fail(f"VAO {self.vao} attribute {attribute}, buffer {buffer}: "
                          f"fetch ends at {end}, capacity {capacity}, divisor {divisor}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    audit = Auditor()
    process = subprocess.Popen(["apitrace", "pickle", str(args.trace)], stdout=subprocess.PIPE)
    try:
        while True:
            try:
                # apitrace writes independent pickles, each with its own memo.
                call = TraceUnpickler(process.stdout).load()
            except EOFError:
                break
            audit.consume(call)
        if process.wait() != 0:
            raise RuntimeError("apitrace could not decode the complete trace")
    except (KeyError, ValueError, RuntimeError, pickle.UnpicklingError) as error:
        audit.fail(f"audit incomplete: {error}")
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait()
        process.stdout.close()
    report = {"trace": str(args.trace), "draws": audit.draws, "frames": audit.frames,
              "attribute_fetch_ranges": audit.fetches, "errors": audit.errors,
              "scope": "CPU audit of enabled vertex/index fetch bounds only; no GL execution"}
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 1 if audit.errors or not audit.draws else 0


if __name__ == "__main__":
    sys.exit(main())
