#!/usr/bin/env python3
"""Validate deterministic, integrity-checked Web release packaging."""

from __future__ import annotations

import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


EXPECTED_CONTENT_TYPES = {
    "island-simulator.html": "text/html; charset=utf-8",
    "island-simulator.js": "text/javascript; charset=utf-8",
    "island-simulator.wasm": "application/wasm",
}


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def run_packager(script: Path, build_directory: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(script), str(build_directory)],
        check=False,
        capture_output=True,
        text=True,
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: ValidateWebPackageTests.py /path/to/package_web_release.sh", file=sys.stderr)
        return 2

    script = Path(sys.argv[1]).resolve()
    if not script.is_file():
        print(f"packager does not exist: {script}", file=sys.stderr)
        return 1

    payloads = {
        "island-simulator.html": b"<!doctype html><script src='island-simulator.js'></script>\n",
        "island-simulator.js": b"globalThis.islandPackageTest = true;\n" * 32,
        "island-simulator.wasm": b"\0asm\x01\0\0\0" + bytes(range(256)) * 12,
    }

    with tempfile.TemporaryDirectory(prefix="island-web-package-test-") as temporary:
        build_directory = Path(temporary)
        for name, payload in payloads.items():
            (build_directory / name).write_bytes(payload)

        first = run_packager(script, build_directory)
        if first.returncode != 0:
            print(first.stdout, file=sys.stderr)
            print(first.stderr, file=sys.stderr)
            return 1

        output_directory = build_directory / "web-release"
        manifest_path = output_directory / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if manifest.get("schema") != 1 or manifest.get("contentEncoding") != "gzip":
            raise AssertionError("manifest schema or encoding is incorrect")

        entries = {entry["name"]: entry for entry in manifest.get("files", [])}
        if set(entries) != set(payloads):
            raise AssertionError("manifest does not contain exactly the three Web artifacts")

        total_bytes = 0
        total_gzip_bytes = 0
        deterministic_gzip: dict[str, bytes] = {}
        for name, payload in payloads.items():
            packaged = (output_directory / name).read_bytes()
            compressed = (output_directory / f"{name}.gz").read_bytes()
            entry = entries[name]
            if packaged != payload or gzip.decompress(compressed) != payload:
                raise AssertionError(f"packaged content does not round-trip: {name}")
            if entry["contentType"] != EXPECTED_CONTENT_TYPES[name]:
                raise AssertionError(f"content type is incorrect: {name}")
            if entry["bytes"] != len(payload) or entry["gzipBytes"] != len(compressed):
                raise AssertionError(f"manifest size is incorrect: {name}")
            if entry["sha256"] != sha256(payload):
                raise AssertionError(f"manifest hash is incorrect: {name}")
            total_bytes += len(payload)
            total_gzip_bytes += len(compressed)
            deterministic_gzip[name] = compressed

        if manifest["totalBytes"] != total_bytes:
            raise AssertionError("manifest uncompressed total is incorrect")
        if manifest["totalGzipBytes"] != total_gzip_bytes:
            raise AssertionError("manifest gzip total is incorrect")
        if not 0.0 < manifest["transferReductionPercent"] < 100.0:
            raise AssertionError("manifest transfer reduction is outside the valid range")

        second = run_packager(script, build_directory)
        if second.returncode != 0:
            raise AssertionError(f"second packaging run failed: {second.stderr}")
        for name, expected in deterministic_gzip.items():
            if (output_directory / f"{name}.gz").read_bytes() != expected:
                raise AssertionError(f"gzip output is not deterministic: {name}")

        missing_directory = build_directory / "missing"
        missing_directory.mkdir()
        (missing_directory / "island-simulator.html").write_bytes(b"html")
        (missing_directory / "island-simulator.js").write_bytes(b"js")
        missing = run_packager(script, missing_directory)
        if missing.returncode == 0 or "missing release artifact" not in missing.stderr:
            raise AssertionError("missing artifact did not fail with a useful diagnostic")
        if (missing_directory / "web-release").exists():
            raise AssertionError("missing-artifact preflight left a partial output directory")

    print("web package validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
