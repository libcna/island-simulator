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
    def test_flight_review_cpu_regressions_and_scope(self):
        self.assertIn("flight", gate.SCOPES)
        node = shutil.which("node")
        self.assertIsNotNone(node)
        result = subprocess.run([node,str(ROOT / "tests/BrowserFlightReviewTests.mjs")],
                                capture_output=True,text=True,timeout=10)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('await runFlightReview(sessionId)',source)
        self.assertIn('&& (!focusedFlightCheck || flightReview?.passed)',source)
        self.assertIn('["flight", ["--input-diagnostics"]]',(ROOT / "tools/web_smoke.html.in").read_text())
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

    def test_fullscreen_scope_keeps_containment_and_real_state_checks(self):
        self.assertIn("fullscreen", gate.SCOPES)
        environment = gate.browser_environment("chrome", "fullscreen", Path("/evidence"))
        self.assertEqual(environment["ISLAND_WEB_SMOKE_SCOPE"], "fullscreen")
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('document.fullscreenElement', source)
        self.assertIn('await requireState(true, cycle+1)', source)
        self.assertIn('await requireState(false, cycle+1)', source)
        self.assertIn('state.active === active && confirmed === occurrence', source)
        self.assertIn('await key("keyDown", true)', source)
        game = (ROOT / "game/src/IslandGame.cpp").read_text()
        self.assertIn('Keys::F9, Keys::F10, Keys::J,', game)

    def test_material_scope_preserves_software_supervision(self):
        environment = gate.browser_environment("chrome", "materials", Path("/evidence"))
        self.assertEqual(environment["ISLAND_WEB_SMOKE_SCOPE"], "materials")
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")
        self.assertEqual(environment["GALLIUM_DRIVER"], "llvmpipe")
        self.assertEqual(gate.browser_command(ROOT, ROOT, Path("/node"), "chrome", "materials"),
                         ["/node", str(ROOT / "tools/run_web_smoke.mjs"), str(ROOT)])
        with self.assertRaises(ValueError):
            gate.browser_command(ROOT, ROOT, Path("/node"), "firefox", "materials")
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedMaterialsCheck ? ["litter", "grass", "bay-ground", "wet-sand"]', source)

    def test_climb_scope_requires_full_cycle_and_held_summit(self):
        self.assertIn("monkey-climb", gate.SCOPES)
        self.assertIn('["monkey-climb", ["--viewpoint", "monkey-climb"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        self.assertEqual(gate.browser_environment("chrome", "monkey-climb", Path("/evidence"))
                         ["ISLAND_BROWSER_SUPERVISED"], "chrome")
        times = [0, 5, 10.1, 18.5, 24, 29.1, 38.9, 0]
        heights = [5, 5.2, 5.3, 7.3, 6.4, 5.3, 5.01, 5]
        log = "\n".join(f"monkey climb stage={i} subject=3 tree={187 if i<7 else -1} "
            f"time={t} active={int(i<7)} centre-y={heights[i]}" for i,t in enumerate(times))
        held = "\nmonkey climb held summit presented after eight cycle stages"
        report = {"passed": True, "views": [{"mode": "monkey-climb", "result": {"output": log+held}}]}
        self.assertEqual(len(gate.validate_climb_report(report)), 8)
        for invalid in [{}, {"passed": True, "views": []}, {"passed": False, "views": report["views"]},
                        {"passed": True, "views": [{"mode": "monkey-climb", "result": {"output": log}}]},
                        {"passed": True, "views": [{"mode": "monkey-climb", "result": {"output": held}}]}]:
            with self.assertRaises(RuntimeError):
                gate.validate_climb_report(invalid)

    def test_hut_scope_and_report_require_material_draw_and_live_context(self):
        self.assertIn("hut",gate.SCOPES)
        for mode in ("hut","hut-material"):
            self.assertIn(f'["{mode}", ["--viewpoint", "{mode}", "--debug-overlay"]]',
                          (ROOT / "tools/web_smoke.html.in").read_text())
        def report():
            return {"passed":True,"views":[{"mode":mode,"passed":True,"glError":0,"contextLost":False,
                "result":{"output":f"hut beam-aligned weathered grain and filtered thatch fibres\nhut 1 draw, {7 if mode == 'hut' else 6} contact patches\nhut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes"}}
                for mode in ("hut","hut-material")]}
        self.assertEqual(gate.validate_hut_report(report()),["hut","hut-material"])
        missing_texture=report()
        missing_texture["views"][0]["result"]["output"]=missing_texture["views"][0]["result"]["output"].split("\nhut palm fibre")[0]
        with self.assertRaises(RuntimeError): gate.validate_hut_report(missing_texture)
        for index,wrong_count in ((0,6),(1,7),(1,0)):
            bad=report()
            bad["views"][index]["result"]["output"] = (
                "hut beam-aligned weathered grain and filtered thatch fibres\n"
                f"hut 1 draw, {wrong_count} contact patches\n"
                "hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes")
            with self.assertRaises(RuntimeError): gate.validate_hut_report(bad)
        for key,value in (("passed",False),("glError",1282),("contextLost",True),
                          ("result",{"output":"hut 0 draw, 0 contact patches"})):
            bad=report();bad["views"][1][key]=value
            with self.subTest(key=key),self.assertRaises(RuntimeError): gate.validate_hut_report(bad)
        for text in ("", "hut 1 draw, 7 contact patches", "hut beam-aligned weathered grain and filtered thatch fibres",
                     report()["views"][0]["result"]["output"]+"\nhut material fallback: failed"):
            bad=report();bad["views"][0]["result"]["output"]=text
            with self.assertRaises(RuntimeError): gate.validate_hut_report(bad)
        missing=report();missing["views"].pop()
        with self.assertRaises(RuntimeError): gate.validate_hut_report(missing)

    def test_snake_scope_requires_both_views_and_visible_population(self):
        self.assertIn("snakes", gate.SCOPES)
        environment = gate.browser_environment("chrome", "snakes", Path("/evidence"))
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")
        self.assertEqual(environment["ISLAND_WEB_SMOKE_SCOPE"], "snakes")
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedSnakeCheck ? ["snakes", "snake-head"]', source)
        self.assertIn('snakeMotionFrames.length === 4', source)
        self.assertIn('snakes (?:[1-9]|[1-4][0-9]|50)', source)
        for mode in ("snakes", "snake-head"):
            self.assertIn(f'["{mode}", ["--viewpoint", "{mode}"]]',
                          (ROOT / "tools/web_smoke.html.in").read_text())

    def test_snake_report_rejects_frozen_missing_and_invalid_motion(self):
        log='\n'.join(f"snake runtime sample frame={i*8} head={i*0.2},2,0 speed=0.5 visible=1" for i in range(1,5))
        def report(text):
            return {"passed":True,"views":[{"mode":mode,"result":{"output":text}}
                                          for mode in ("snakes","snake-head")]}
        self.assertEqual(len(gate.validate_snake_report(report(log))),2)
        for invalid in ("",log.replace("frame=16","frame=8"),log.replace("speed=0.5","speed=nan"),
                        log.replace("visible=1","visible=0"),log.replace("head=0.2,2,0","head=nan,2,0"),
                        '\n'.join(f"snake runtime sample frame={i*8} head=1,2,0 speed=0 visible=1" for i in range(1,5))):
            with self.assertRaises(RuntimeError): gate.validate_snake_report(report(invalid))
        missing=report(log);missing["views"].pop()
        with self.assertRaises(RuntimeError): gate.validate_snake_report(missing)

    def test_monkey_scope_retains_supervision_and_intermediate_visual_samples(self):
        self.assertIn("monkeys", gate.SCOPES)
        environment = gate.browser_environment("chrome", "monkeys", Path("/evidence"))
        self.assertEqual(environment["ISLAND_WEB_SMOKE_SCOPE"], "monkeys")
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")
        with self.assertRaises(ValueError):
            gate.browser_command(ROOT, ROOT, Path("/node"), "firefox", "monkeys")
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedMonkeyCheck ? ["monkeys"]', source)
        self.assertIn('mode === "boars" || mode === "monkeys"', source)
        self.assertIn('pose < 4', source)
        self.assertIn('++frames >= 8', source)
        self.assertIn('monkeyMotionFrames.push', source)
        self.assertIn('motionReview, monkeyMotionFrames,', source)

    def test_goat_scope_requires_coat_population_images_and_live_context(self):
        self.assertIn("goats", gate.SCOPES)
        self.assertEqual(gate.browser_environment("chrome", "goats", Path("/evidence"))
                         ["ISLAND_BROWSER_SUPERVISED"], "chrome")
        self.assertIn('["goats", ["--viewpoint", "goats"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedGoatCheck ? ["goats"]', source)
        self.assertIn('goatMotionFrames.push(frame)', source)
        self.assertIn('goatMotionFrames.length === 4', source)
        def report():
            return {"passed": True, "views": [{"mode": "goats", "passed": True,
                "glError": 0, "contextLost": False,
                "goatMotionFrames": [f"pose-{i}.png" for i in range(4)],
                "result": {"output": "goats 3/6 visible, 2 draw;\n"
                    "goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes"}}]}
        self.assertEqual(len(gate.validate_goat_report(report())), 4)
        for key, value in (("mode", "monkeys"), ("passed", False), ("glError", 1282),
                           ("contextLost", True), ("goatMotionFrames", []),
                           ("goatMotionFrames", ["same.png"] * 4)):
            bad = report(); bad["views"][0][key] = value
            with self.subTest(key=key), self.assertRaises(RuntimeError):
                gate.validate_goat_report(bad)
        for old, new in (("3/6", "0/6"), ("2 draw", "1 draw"),
                         ("goat fur loaded", "missing"), ("9 mips", "1 mips")):
            bad = report()
            bad["views"][0]["result"]["output"] = bad["views"][0]["result"]["output"].replace(old, new)
            with self.assertRaises(RuntimeError): gate.validate_goat_report(bad)
        for flag in ("error", "exited"):
            bad = report(); bad["views"][0]["result"][flag] = True
            with self.assertRaises(RuntimeError): gate.validate_goat_report(bad)
        for bad in ({"passed": False, "views": report()["views"]}, {"passed": True, "views": []}):
            with self.assertRaises(RuntimeError): gate.validate_goat_report(bad)

    def test_spider_scope_requires_material_shadows_lighting_and_live_frames(self):
        self.assertIn("spiders", gate.SCOPES)
        self.assertIn('["spiders", ["--viewpoint", "spiders"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedSpiderCheck ? ["spiders"]', source)
        self.assertIn('spiderMotionFrames.push(frame)', source)
        def report():
            return {"passed": True, "views": [{"mode": "spiders", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "intervals": [16.7]*60, "spiderMotionFrames": [f"pose-{i}.png" for i in range(4)],
                "result": {"output": "spider render proof: spiders 1/200 visible, 2 draw; contact-shadows=1\n"
                    "spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes\n"
                    "spider lighting ground-transmission=0 body-transmission=0 body-tint=0.46,0.53,0.62\n"}}]}
        self.assertEqual(len(gate.validate_spider_report(report())), 4)
        for key, value in (("mode", "goats"), ("passed", False), ("glError", 1282),
                           ("contextLost", True), ("renderer", "AMD"), ("intervals", []),
                           ("intervals", [float('nan')]*60), ("intervals", [0]*60),
                           ("spiderMotionFrames", []), ("spiderMotionFrames", ["same.png"]*4),
                           ("spiderMotionFrames", ["../escape.png", "a.png", "b.png", "c.png"])):
            bad=report(); bad["views"][0][key]=value
            with self.subTest(key=key), self.assertRaises(RuntimeError): gate.validate_spider_report(bad)
        for old, new in (("1/200", "0/200"), ("2 draw", "1 draw"), ("contact-shadows=1", "contact-shadows=0"),
                         ("setae loaded", "missing"), ("body-tint=0.46", "body-tint=0.25"),
                         ("spider lighting", "absent")):
            bad=report(); result=bad["views"][0]["result"]
            result["output"]=result["output"].replace(old,new)
            with self.assertRaises(RuntimeError): gate.validate_spider_report(bad)
        for flag in ("error", "exited"):
            bad=report(); bad["views"][0]["result"][flag]=True
            with self.assertRaises(RuntimeError): gate.validate_spider_report(bad)
        for marker in ("material fallback: missing", "GL_INVALID_OPERATION", "gl-error 1282"):
            bad=report(); bad["views"][0]["result"]["output"] += marker
            with self.assertRaises(RuntimeError): gate.validate_spider_report(bad)
        for bad in ({"passed": False, "views": report()["views"]}, {"passed": True, "views": []}):
            with self.assertRaises(RuntimeError): gate.validate_spider_report(bad)

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
