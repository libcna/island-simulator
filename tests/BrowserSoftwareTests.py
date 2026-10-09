#!/usr/bin/env python3
"""CPU-only browser gate checks; no HTTP server, browser or graphics is launched."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import run_browser_software as gate
from gpu_incident_qualification import contained_command


class BrowserSoftwareTests(unittest.TestCase):
    def test_environment_forces_software_and_removes_displays_and_stale_scope(self):
        with patch.dict(os.environ, {"DISPLAY": ":0", "WAYLAND_DISPLAY": "wayland-0",
                                     "XAUTHORITY": "/private", "DRI_PRIME": "1",
                                     "ISLAND_WEB_SMOKE_SCOPE": "coast", "GALLIUM_DRIVER": "radeonsi"}):
            environment = gate.browser_environment("chrome", "full", Path("/evidence"))
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "DRI_PRIME", "ISLAND_WEB_SMOKE_SCOPE"):
            self.assertNotIn(key, environment)
        self.assertEqual(environment["LIBGL_ALWAYS_SOFTWARE"], "1")
        self.assertEqual(environment["GALLIUM_DRIVER"], "llvmpipe")
        self.assertEqual(environment["LP_NUM_THREADS"], "8")
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")

    def test_focused_scope_is_explicit(self):
        self.assertEqual(gate.browser_environment("chrome", "terrain", Path("/evidence"))[
            "ISLAND_WEB_SMOKE_SCOPE"], "terrain")

    def test_litter_scope_preserves_supervision_and_driver_policy(self):
        environment = gate.browser_environment("chrome", "litter", Path("/evidence"))
        self.assertEqual(environment["ISLAND_WEB_SMOKE_SCOPE"], "litter")
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")
        self.assertEqual(environment["GALLIUM_DRIVER"], "llvmpipe")
        self.assertEqual(gate.browser_command(ROOT, ROOT, Path("/node"), "chrome", "litter"),
                         ["/node", str(ROOT / "tools/run_web_smoke.mjs"), str(ROOT)])

    def test_exact_browser_command_and_invalid_scope(self):
        self.assertEqual(gate.browser_command(Path("/project"), Path("/build"), Path("/node"),
                                              "firefox", "full"),
                         ["/node", "/project/tools/run_firefox_web_smoke.mjs", "/build"])
        for browser, scope in (("firefox", "terrain"), ("chrome", "invalid"), ("invalid", "full")):
            with self.subTest(browser=browser, scope=scope), self.assertRaises(ValueError):
                gate.browser_command(ROOT, ROOT, Path("/node"), browser, scope)

    def test_device_policy_contains_no_host_device_bind(self):
        command = contained_command(["node", "fixture"], private_devices=True)
        self.assertIn("--unshare-pid", command)
        self.assertIn("--die-with-parent", command)
        self.assertIn("--dev", command)
        self.assertNotIn("--dev-bind", command)

    def node_fixture(self, environment, *, contained, body=""):
        node = shutil.which("node")
        self.assertIsNotNone(node, "Node is required for CPU-only browser containment tests")
        module = (ROOT / "tools/browser_containment.mjs").as_uri()
        command = [node, "--input-type=module", "-e",
                   f"import {{requireBrowserContainment,recordBrowserProcess}} from {json.dumps(module)}; "
                   "requireBrowserContainment('chrome'); " + body]
        if contained:
            command = contained_command(command, private_devices=True)
        return subprocess.run(command, env=environment, capture_output=True, text=True, timeout=5)

    def test_guard_rejects_uncontained_even_with_marker_environment(self):
        with tempfile.TemporaryDirectory(prefix="island-browser-guard-") as temporary:
            evidence = Path(temporary)
            environment = gate.browser_environment("chrome", "full", evidence)
            result = self.node_fixture(environment, contained=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("requires tools/run_browser_software.py", result.stderr)
            self.assertFalse((evidence / "browser-worker.json").exists())

    def test_private_namespace_guard_and_attribution(self):
        with tempfile.TemporaryDirectory(prefix="island-browser-guard-") as temporary:
            evidence = Path(temporary)
            environment = gate.browser_environment("chrome", "full", evidence)
            result = self.node_fixture(environment, contained=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            record = json.loads((evidence / "browser-worker.json").read_text())
            self.assertNotEqual(record["outerPid"], record["namespacePid"])
            self.assertEqual(record["namespacePid"], 2)
            self.assertTrue(record["privateGpuDevices"])

    def test_guard_rejects_driver_override_in_namespace(self):
        with tempfile.TemporaryDirectory(prefix="island-browser-guard-") as temporary:
            environment = gate.browser_environment("chrome", "full", Path(temporary))
            environment["GALLIUM_DRIVER"] = "radeonsi"
            result = self.node_fixture(environment, contained=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("requires tools/run_browser_software.py", result.stderr)

    def test_child_pid_mapping_matches_childs_own_procfs_identity(self):
        with tempfile.TemporaryDirectory(prefix="island-browser-guard-") as temporary:
            environment = gate.browser_environment("chrome", "full", Path(temporary))
            child_code = ("import os,json,time; print(json.dumps({'outerPid': int(os.readlink('/proc/self')), "
                          "'namespacePid': os.getpid()}),flush=True); time.sleep(10)")
            body = ("import {spawn} from 'node:child_process'; import {once} from 'node:events'; "
                    f"const child=spawn({json.dumps(sys.executable)},['-c',{json.dumps(child_code)}]); "
                    "try { const [data]=await once(child.stdout,'data'); const expected=JSON.parse(data); "
                    "const actual=recordBrowserProcess(child,'chrome'); "
                    "const display=recordBrowserProcess(child,'Xvfb','display-process.json'); "
                    "if(display.outerPid!==actual.outerPid) throw new Error('display PID mismatch'); "
                    "if(actual.outerPid!==expected.outerPid || actual.namespacePid!==expected.namespacePid) "
                    "throw new Error('child PID mismatch'); } finally { child.kill(); await once(child,'exit'); }")
            result = self.node_fixture(environment, contained=True, body=body)
            self.assertEqual(result.returncode, 0, result.stderr)
            identity = json.loads((Path(temporary) / "browser-process.json").read_text())
            self.assertNotEqual(identity["outerPid"], identity["namespacePid"])
            display = json.loads((Path(temporary) / "display-process.json").read_text())
            self.assertEqual(display["outerPid"], identity["outerPid"])
            self.assertEqual(identity["browser"], "chrome", "display record must not overwrite browser evidence")

    def test_process_evidence_filename_cannot_escape_run_directory(self):
        node = shutil.which("node")
        module = (ROOT / "tools/browser_containment.mjs").as_uri()
        result = subprocess.run([node, "--input-type=module", "-e",
            f"import {{recordBrowserProcess}} from {json.dumps(module)}; "
            "import assert from 'node:assert/strict'; "
            "assert.throws(()=>recordBrowserProcess({},'Xvfb','../outside.json'), /Unsupported/);"],
            capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_missing_child_attribution_fails_closed(self):
        with tempfile.TemporaryDirectory(prefix="island-browser-guard-") as temporary:
            environment = gate.browser_environment("chrome", "full", Path(temporary))
            result = self.node_fixture(environment, contained=True,
                                       body="recordBrowserProcess({pid:-1},'chrome');")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Cannot attribute", result.stderr)


if __name__ == "__main__":
    unittest.main()
