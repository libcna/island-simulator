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
    def test_deer_scope_requires_continuous_neck_nose_and_fitted_spots(self):
        self.assertIn("deer", gate.SCOPES)
        anatomy = ("near deer use vertex-blended ellipsoid light, curved tapered necks, "
                   "articulated hocks, fitted muzzles, and smooth antler beams, with continuous "
                   "five-ring necks, separate nasal pads, and surface-fitted oval coat spots")
        markers = "\n".join((
            "four inland deer use long legs, pointed ears, spotted coats and one antlered silhouette",
            anatomy,
            "deer use separated chest and haunch masses, muscle-profiled upper legs, and normal-driven dorsal-to-belly coat tone",
            "deer QA camera frames one antlered clearing animal broadside",
            "deer 2/4 visible, 1 draw"))
        def report():
            return {"passed": True, "views": [{"mode": "deer", "passed": True,
                "glError": 0, "contextLost": False, "screenshot": "deer.png",
                "result": {"output": markers}}]}
        self.assertEqual(gate.validate_deer_report(report()), ("deer.png",))
        missing = report()
        missing["views"][0]["result"]["output"] = markers.replace(anatomy + "\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_deer_report(missing)

    def test_bunker_scope_requires_all_rooms_material_and_sealed_end(self):
        self.assertIn("bunker", gate.SCOPES)
        modes = ["bunker", "bunker-corridor", "bunker-computers", "bunker-reactor",
                 "bunker-service", "bunker-end"]
        page = (ROOT / "tools/web_smoke.html.in").read_text()
        for mode in modes:
            self.assertIn(f'["{mode}", ["--viewpoint", "{mode}", "--debug-overlay"]]', page)
        markers = "\n".join((
            "concealed inland bunker has an open rock portal, descending access corridor, computer room, service room, and fictional reactor-generator chamber",
            "bunker uses bounded baked light pools, luminous ceiling fixtures, active terminal screens, and a contained cyan generator core in two static draws",
            "bunker computer room has four desks and terminals, keyboard consoles, eight server cabinets, and a separate generator control bank",
            "bunker concrete and metal use portable world-space mottling, restrained age variation, and baked face shading",
            "bunker deep spine ends at a sealed maintenance bulkhead and the service branch has a complete lit shell",
        ))
        def report():
            return {"passed": True, "views": [{"mode": mode, "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": mode + ".png", "intervals": [16.0] * 12,
                "result": {"error": "", "exited": False,
                           "output": f"bunker QA camera {mode}\n{markers}"}}
                for mode in modes]}
        self.assertEqual(gate.validate_bunker_report(report()),
                         [mode + ".png" for mode in modes])
        missing = report()
        missing["views"][-1]["result"]["output"] = markers
        with self.assertRaises(RuntimeError):
            gate.validate_bunker_report(missing)
        fallback = report()
        fallback["views"][2]["result"]["output"] += "\nbunker material fallback: shader error"
        with self.assertRaises(RuntimeError):
            gate.validate_bunker_report(fallback)

    def test_helicopter_scope_requires_closed_rotor_blades_in_both_views(self):
        markers = "\n".join((
            "helicopter uses a smoothly tapered thirty-two-sided cockpit canopy, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane",
            "helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges",
            "helicopter rotor head adds four blade grips, pitch links, a mast cap, and muted yellow tip bands",
            "helicopter raised tapered engine cowling feeds two rear exhaust outlets behind the rotor mast",
            "helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber",
            "helicopter carries four outward-open framed door panels with glazing and handles",
            "helicopter cockpit uses four round gauges, paired articulated cyclic and collective controls, and four pedals",
            "helicopter instruments use recessed ticked dials, one artificial horizon, needle hubs, and framed multi-line radio displays",
            "helicopter open cabin uses restrained indirect light, a tapered instrument binnacle, and twin radio displays",
            "helicopter realism pass adds a rounded chin and tail-root fairing, twelve-sided six-station tail boom, maintenance seams, engine intakes, shaped cushions, harnesses, centre pedestal, and overhead console",
            "helicopter curved shell uses analytic per-vertex lighting across the canopy, cowling, tail boom, fairings, and cushions without added geometry",
            "helicopter cockpit uses a fuller nine-station longitudinal bubble and a progressively wrapped lower windscreen",
            "helicopter divided windscreen carries paired articulated parked wipers and restrained translucent sky glints",
            "parked main rotor uses three-span quadratic blade droop, progressive chord taper, and restrained tip sweep",
            "parked main rotor uses closed cambered airfoil sections with fine leading and trailing edges",
            "lateral tail-rotor shaft carries four closed swept blades with visible chord, thickness, and muted tip bands",
            "helicopter empennage uses a closed swept vertical fin and separate tapered horizontal stabilizers",
            "helicopter four open doorways use twin-rail tubular anti-slip entry steps instead of solid slabs",
            "helicopter cabin uses rounded closed pilot cushions on exposed metal frames and a seam-divided rear bench",
            "helicopter rotor mast uses concentric tubular shafts, a flattened round swashplate, and a rounded gearbox housing",
            "helicopter 2 draw",
        ))
        def view(mode):
            return {"mode": mode, "passed": True, "glError": 0, "contextLost": False,
                "renderer": "ANGLE SwiftShader", "screenshot": mode + ".png",
                "intervals": [16.7] * 60, "result": {"output": markers}}
        report = {"passed": True, "views": [view("helicopter"), view("helicopter-inside")]}
        self.assertEqual(gate.validate_helicopter_report(report),
                         ["helicopter.png", "helicopter-inside.png"])
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter cockpit uses four round gauges, paired articulated cyclic and collective controls, and four pedals\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter instruments use recessed ticked dials, one artificial horizon, needle hubs, and framed multi-line radio displays\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter open cabin uses restrained indirect light, a tapered instrument binnacle, and twin radio displays\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter raised tapered engine cowling feeds two rear exhaust outlets behind the rotor mast\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "parked main rotor uses three-span quadratic blade droop, progressive chord taper, and restrained tip sweep\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "parked main rotor uses closed cambered airfoil sections with fine leading and trailing edges\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "lateral tail-rotor shaft carries four closed swept blades with visible chord, thickness, and muted tip bands\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter empennage uses a closed swept vertical fin and separate tapered horizontal stabilizers\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter four open doorways use twin-rail tubular anti-slip entry steps instead of solid slabs\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter cabin uses rounded closed pilot cushions on exposed metal frames and a seam-divided rear bench\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)
        report["views"][1]["result"]["output"] = markers.replace(
            "helicopter rotor mast uses concentric tubular shafts, a flattened round swashplate, and a rounded gearbox housing\n", "")
        with self.assertRaises(RuntimeError):
            gate.validate_helicopter_report(report)

    def test_shipwreck_scope_requires_current_end_profile_two_frames_and_live_context(self):
        self.assertIn("shipwreck", gate.SCOPES)
        markers = "\n".join((
            "shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas",
            "shipwreck weathered wood follows per-piece board and beam UVs",
            "shipwreck restrained rusted iron straps, rivets, hinges and rudder braces",
            "shipwreck hull uses asymmetric rising sheer, curved keel, and irregular splintered break sections",
            "shipwreck beach broadside has true missing plank bays with exposed structural ribs",
            "shipwreck bow and stern use raked lower profiles with thick end timbers",
            "shipwreck missing plank bays expose eleven-centimetre inner edges",
            "shipwreck stern deckhouse uses separated broken roof planks and framed window bays",
            "shipwreck beach debris uses buried ends, restrained warp, unequal settling, and five genuinely split long planks",
            "shipwreck 20m broken hull, 12 debris pieces, one portable culled batch, 5712 triangles",
            "shipwreck visible, 12 debris, 1 draw",
        ))
        def report():
            return {"passed": True, "views": [{"mode": "shipwreck", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "shipwreck.png", "afterSimulationScreenshot": "shipwreck-after.png",
                "intervals": [16.7] * 60, "result": {"output": markers}}]}
        self.assertEqual(gate.validate_shipwreck_report(report()),
                         ("shipwreck.png", "shipwreck-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output=markers.replace(
                "shipwreck bow and stern use raked lower profiles with thick end timbers\n", "")),
            lambda value: value["views"][0]["result"].update(output=markers.replace(
                "shipwreck missing plank bays expose eleven-centimetre inner edges\n", "")),
            lambda value: value["views"][0]["result"].update(output=markers.replace(
                "shipwreck stern deckhouse uses separated broken roof planks and framed window bays\n", "")),
            lambda value: value["views"][0]["result"].update(output=markers.replace(
                "shipwreck beach debris uses buried ends, restrained warp, unequal settling, and five genuinely split long planks\n", "")),
            lambda value: value["views"][0]["result"].update(output=markers.replace(
                "shipwreck visible, 12 debris, 1 draw", "shipwreck culled, 12 debris, 0 draw")),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_shipwreck_report(invalid)

    def test_stream_scope_is_focused_and_checks_live_material(self):
        self.assertIn("stream", gate.SCOPES)
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedStreamCheck ? ["stream"]', source)
        self.assertIn("route-following downstream ripple and translucent current streaks", source)
        self.assertIn("stream water uses bank-edge sheen and broken shallow riffles", source)
        self.assertIn("stream water uses depth-graded colour and braided moving current threads", source)
        self.assertIn("stream channel uses shared low-frequency pools and pinches", source)
        self.assertIn("stream-bank stones", source)
        self.assertIn("stream terrain uses a compact incised profile and slope-limited exposed bank strata", source)
        self.assertIn('focusedFlightCheck || focusedStreamCheck || focusedShipwreckCheck || mode === "boars"', source)
        self.assertIn('["stream", ["--viewpoint", "stream", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        good={"passed":True,"views":[{"mode":"stream","passed":True,"glError":0,
            "contextLost":False,"renderer":"SwiftShader driver",
            "screenshot":"before.png","afterSimulationScreenshot":"after.png",
            "result":{"error":"","exited":False,"output":
                "route-following downstream ripple and translucent current streaks\n"
                "stream water uses bank-edge sheen and broken shallow riffles\n"
                "stream water uses depth-graded colour and braided moving current threads\n"
                "stream channel uses shared low-frequency pools and pinches\n"
                "stream-bank stones 14, non-solid existing rock variants beyond the carved bank\n"
                "stream terrain uses a compact incised profile and slope-limited exposed bank strata\n"
                "stream water first visible draw"}}]}
        self.assertEqual(gate.validate_stream_report(good),("before.png","after.png"))
        stale=json.loads(json.dumps(good))
        stale["views"][0]["result"]["output"]=stale["views"][0]["result"]["output"].replace(
            "stream channel uses shared low-frequency pools and pinches\n","")
        no_stones=json.loads(json.dumps(good))
        no_stones["views"][0]["result"]["output"]=no_stones["views"][0]["result"]["output"].replace(
            "stream-bank stones 14, non-solid existing rock variants beyond the carved bank\n","")
        for bad in ({},dict(good,passed=False),
                    {**good,"views":[{**good["views"][0],"glError":1282}]},
                    {**good,"views":[{**good["views"][0],"afterSimulationScreenshot":"before.png"}]},
                    stale,no_stones):
            with self.assertRaises(RuntimeError):gate.validate_stream_report(bad)

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
                "result":{"output":f"hut beam-aligned weathered grain and filtered thatch fibres\nhut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation\nhut four-pane muted glazing and braced plank door\nhut raised veranda uses full-height outer posts, split rails, fascia, and three steps\nhut enclosed slatted gable, framed attic light, and four knee-braced veranda supports\nhut wall boards meet edge-to-edge on all four sides and both gables without see-through gaps\nhut 1 draw, {7 if mode == 'hut' else 6} contact patches\nhut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes"}}
                for mode in ("hut","hut-material")]}
        self.assertEqual(gate.validate_hut_report(report()),["hut","hut-material"])
        missing_texture=report()
        missing_texture["views"][0]["result"]["output"]=missing_texture["views"][0]["result"]["output"].split("\nhut palm fibre")[0]
        with self.assertRaises(RuntimeError): gate.validate_hut_report(missing_texture)
        for index,wrong_count in ((0,6),(1,7),(1,0)):
            bad=report()
            bad["views"][index]["result"]["output"] = (
                "hut beam-aligned weathered grain and filtered thatch fibres\n"
                "hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation\n"
                "hut four-pane muted glazing and braced plank door\n"
                "hut raised veranda uses full-height outer posts, split rails, fascia, and three steps\n"
                "hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports\n"
                "hut wall boards meet edge-to-edge on all four sides and both gables without see-through gaps\n"
                f"hut 1 draw, {wrong_count} contact patches\n"
                "hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes")
            with self.assertRaises(RuntimeError): gate.validate_hut_report(bad)
        for key,value in (("passed",False),("glError",1282),("contextLost",True),
                          ("result",{"output":"hut 0 draw, 0 contact patches"})):
            bad=report();bad["views"][1][key]=value
            with self.subTest(key=key),self.assertRaises(RuntimeError): gate.validate_hut_report(bad)
        for text in ("", "hut 1 draw, 7 contact patches", "hut beam-aligned weathered grain and filtered thatch fibres",
                     "hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation",
                     "hut four-pane muted glazing and braced plank door",
                     "hut raised veranda uses full-height outer posts, split rails, fascia, and three steps",
                     "hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports",
                     "hut wall boards meet edge-to-edge on all four sides and both gables without see-through gaps",
                     report()["views"][0]["result"]["output"]+"\nhut material fallback: failed"):
            bad=report();bad["views"][0]["result"]["output"]=text
            with self.assertRaises(RuntimeError): gate.validate_hut_report(bad)
        missing=report();missing["views"].pop()
        with self.assertRaises(RuntimeError): gate.validate_hut_report(missing)

    def test_cave_scope_requires_current_exterior_interior_material(self):
        self.assertIn("cave", gate.SCOPES)
        page = (ROOT / "tools/web_smoke.html.in").read_text()
        for mode in ("cave", "cave-inside"):
            self.assertIn(f'["{mode}", ["--viewpoint", "{mode}", "--debug-overlay"]]', page)
        markers = (
            "cave rock first visible draw",
            "cave rock 2623 triangles in one culled draw; 20m walkable interior",
            "cave terrain shoulders use an irregular rock apron around the open corridor",
            "cave connected shell strips share smooth lighting normals while portal breaks stay sharp",
            "cave adds six high-clearance eroded ceiling pendants in the existing draw",
            "cave mouth is embedded in a rising inland mountain slope",
            "cave crown is concealed from above by blended earth and 371 irregular three-blade tufts in the existing draw",
            "cave rock loaded Content/structures/cave-rock.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored triplanar sampling")
        def report():
            return {"passed": True, "views": [{"mode": mode, "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": mode + ".png", "intervals": [16.0] * 60,
                "result": {"error": "", "exited": False, "output":
                    f"cave QA camera {'inside' if mode == 'cave-inside' else 'outside'}\n"
                    + "\n".join(markers)}} for mode in ("cave", "cave-inside")]}
        self.assertEqual(gate.validate_cave_report(report()), ["cave.png", "cave-inside.png"])
        for index in range(len(markers)):
            bad = report()
            bad["views"][0]["result"]["output"] = bad["views"][0]["result"]["output"].replace(markers[index], "")
            with self.subTest(marker=markers[index]), self.assertRaises(RuntimeError):
                gate.validate_cave_report(bad)
        for key, value in (("glError", 1282), ("contextLost", True),
                           ("intervals", [16.0] * 59), ("renderer", "hardware")):
            bad = report(); bad["views"][1][key] = value
            with self.subTest(key=key), self.assertRaises(RuntimeError):
                gate.validate_cave_report(bad)

    def test_turtle_scope_requires_current_anatomy_two_frames_and_live_context(self):
        self.assertIn("turtles", gate.SCOPES)
        self.assertIn('["turtles", ["--viewpoint", "turtles", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        marker = "near turtles use staggered scutes and closed tapered-volume paddles"
        silhouette = "near turtles use compact integrated heads, narrower hydrodynamic paddles, and restrained shell seams"
        surface = "near turtles use a twenty-by-sixteen carapace surface, eighteen-sided seven-ring heads, and irregular mottled shell and skin tones"
        def report():
            return {"passed": True, "views": [{"mode": "turtles", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "turtles.png", "afterSimulationScreenshot": "turtles-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": marker + "\n" + silhouette + "\n" + surface
                    + "\nturtles 1/5 visible (near 1, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_turtle_report(report()), ("turtles.png", "turtles-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output="turtles 1/5 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette + "\n" + surface
                + "\nturtles 0/5 visible (near 0, far 0), 0 draw"),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_turtle_report(invalid)

    def test_stingray_scope_requires_current_anatomy_and_live_context(self):
        self.assertIn("stingrays",gate.SCOPES)
        self.assertIn('["stingrays", ["--viewpoint", "stingrays", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        anatomy=("stingrays use broad undulating discs, raised eyes, dorsal spots, pale "
                 "undersides, and long tapered tails in one draw")
        profile=("near stingrays use a curved diamond disc, raised central body, wedge nose, "
                 "and restrained dark rim")
        geometry="stingray near/far geometry 846/18 vertices with a crossed-volume tapered tail"
        population="stingrays 5 agents glide, forage, and settle over shallow sandy seabed"
        camera="stingray QA camera follows one shallow-sand glide"
        def report():
            return {"passed":True,"views":[{"mode":"stingrays","passed":True,
                "glError":0,"contextLost":False,"renderer":"ANGLE SwiftShader",
                "screenshot":"stingrays.png","intervals":[16.7]*60,
                "result":{"output":anatomy+"\n"+profile+"\n"+geometry+"\n"+population+"\n"+camera
                    +"\nstingrays 1/5 visible (near 1, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_stingray_report(report()),"stingrays.png")
        for mutate in (
            lambda value:value.update(passed=False),
            lambda value:value["views"][0].update(glError=1282),
            lambda value:value["views"][0].update(contextLost=True),
            lambda value:value["views"][0].update(intervals=[16.7]*59),
            lambda value:value["views"][0]["result"].update(output=population),
            lambda value:value["views"][0]["result"].update(
                output=anatomy+"\n"+profile+"\n"+geometry+"\n"+population+"\n"+camera
                    +"\nstingrays 0/5 visible (near 0, far 0), 0 draw"),
        ):
            invalid=report();mutate(invalid)
            with self.assertRaises(RuntimeError):gate.validate_stingray_report(invalid)

    def test_octopus_scope_requires_current_profile_and_live_context(self):
        self.assertIn("octopuses", gate.SCOPES)
        marker = ("octopuses use pulsing mantles, expressive eyes, eight independently "
                  "curled arms, and pale sucker rows in one draw")
        profile = ("near octopuses use a rear-swept mantle, smooth analytic skin light, "
                   "and thicker tapered arms")
        geometry = "octopus near/far geometry 3186/24 vertices in one bounded batch"
        eyes = ("near octopuses use raised ocular bulbs, horizontal pupils, and fitted "
                "eye highlights")
        population = "octopuses 4 agents den, crawl, forage, and swim in reef and rocky shallows"
        camera = "octopus QA camera frames one exposed reef forager and all eight arms"
        def report():
            return {"passed": True, "views": [{"mode": "octopuses", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "octopuses.png", "intervals": [16.7] * 60,
                "result": {"output": marker + "\n" + profile + "\n" + geometry + "\n" + eyes + "\n"
                    + population + "\n" + camera
                    + "\noctopuses 1/4 visible (near 1, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_octopus_report(report()), "octopuses.png")
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output=population),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + profile
                + "\n" + geometry + "\n" + population + "\n" + camera
                + "\noctopuses 0/4 visible (near 0, far 0), 0 draw"),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_octopus_report(invalid)

    def test_coconut_crab_scope_requires_elevated_profile_and_live_context(self):
        self.assertIn("coconut-crabs", gate.SCOPES)
        population = ("six coastal palm-zone coconut crabs rest, forage, and retreat in the "
                      "shared crab batch")
        anatomy = ("coconut crabs use broad armored bodies, segmented terrestrial abdomens, "
                   "robust walking legs, and massive unequal claws")
        profile = ("coconut crabs use elevated compact bodies, planted high-jointed legs, "
                   "and arched abdominal plates")
        claws = ("near coconut crabs use rounded unequal claw palms over separated solid "
                 "pincer fingers")
        colour = ("coconut crabs use cool blue-grey, violet-brown, slate, and warm umber "
                  "limb armour with a front-raised terrestrial stance")
        camera = ("coconut-crab QA camera frames one large terrestrial animal in coastal "
                  "vegetation")
        def report():
            return {"passed": True, "views": [{"mode": "coconut-crabs", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "coconut-before.png",
                "afterSimulationScreenshot": "coconut-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": population + "\n" + anatomy + "\n" + profile + "\n"
                    + claws + "\n" + colour + "\n" + camera
                    + "\ncoconut crabs 1/6 visible in shared draw"}}]}
        self.assertEqual(gate.validate_coconut_crab_report(report()),
                         ("coconut-before.png", "coconut-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output=population),
            lambda value: value["views"][0].update(
                afterSimulationScreenshot="coconut-before.png"),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_coconut_crab_report(invalid)

    def test_crab_scope_requires_current_anatomy_two_frames_and_live_context(self):
        self.assertIn("crabs", gate.SCOPES)
        self.assertIn('["crabs", ["--viewpoint", "crabs", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        marker = "near crabs use tapered toes, socketed eyes, and solid pincer fingers"
        silhouette = "near crabs use flattened lateral carapaces, sturdy legs, and enlarged claw palms"
        surface = "near crabs use eighteen-sided smooth-lit mottled carapaces and solid tapered legs"
        anatomy = "near crabs use low broad shells with retained lateral carapace corners"
        underside = ("near crab carapaces close around a shallow dark underside instead of "
                     "ending at an open rim")
        claws = "near beach crabs use rounded claw palms and solid tapered forearms"
        habitat = "six debris-linked crab colonies share visible sand burrows"
        spacing = "live beach crabs keep 0.72m centre separation while foraging"
        camera = "crab QA camera frames a debris-linked burrow and its foraging colony"
        def report():
            return {"passed": True, "views": [{"mode": "crabs", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "crabs.png", "afterSimulationScreenshot": "crabs-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": marker + "\n" + silhouette + "\n" + surface + "\n" + anatomy
                    + "\n" + underside + "\n" + claws + "\n" + habitat + "\n" + spacing + "\n" + camera
                    + "\ncrab QA subject 28 is 1.28294m from the burrow"
                    + "\ncrabs 5/30 visible (near 5, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_crab_report(report()), ("crabs.png", "crabs-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output="crabs 5/30 visible (near 5, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker
                + "\ncrabs 5/30 visible (near 5, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette
                + "\n" + surface + "\n" + anatomy + "\n" + habitat + "\n" + spacing
                + "\n" + camera + "\ncrab QA subject 28 is 1.28294m from the burrow"
                + "\ncrabs 5/30 visible (near 5, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette
                + "\n" + surface + "\n" + anatomy + "\n" + underside + "\n" + habitat
                + "\n" + spacing + "\n" + camera
                + "\ncrab QA subject 28 is 1.28294m from the burrow"
                + "\ncrabs 5/30 visible (near 5, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette
                + "\ncrabs 0/30 visible (near 0, far 0), 0 draw"),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_crab_report(invalid)

    def test_crocodile_scope_requires_current_anatomy_two_frames_and_live_context(self):
        self.assertIn("crocodiles", gate.SCOPES)
        self.assertIn('["crocodiles", ["--viewpoint", "crocodiles", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        silhouette = ("crocodiles use long low armored bodies, broad skulls, muscular tails, "
                      "and planted splayed limbs")
        surface = ("near crocodiles use raised eyes, nostrils, separate jaws, teeth, dorsal "
                   "osteoderms, and wet mottled facet shading, with multi-section rounded snouts "
                   "and lateral eyes")
        armor = ("crocodile armor uses low paired trunk scutes that merge into a diminishing "
                 "articulated tail keel")
        pigment = ("near crocodile torso, tail, skull, jaw, and limb roots use bounded "
                   "reticulated facet pigment")
        camera = "crocodile QA camera frames an estuary animal broadside"
        def report():
            return {"passed": True, "views": [{"mode": "crocodiles", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "crocodiles.png",
                "afterSimulationScreenshot": "crocodiles-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": silhouette + "\n" + surface + "\n" + armor + "\n" + pigment + "\n" + camera
                    + "\ncrocodiles 1/3 visible (near 1, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_crocodile_report(report()),
                         ("crocodiles.png", "crocodiles-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output=surface + "\n" + armor + "\n" + pigment + "\n" + camera
                + "\ncrocodiles 1/3 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=silhouette + "\n" + armor + "\n" + pigment + "\n" + camera
                + "\ncrocodiles 1/3 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=silhouette + "\n" + surface
                + "\n" + pigment + "\n" + camera + "\ncrocodiles 1/3 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=silhouette + "\n" + surface
                + "\n" + armor + "\n" + camera
                + "\ncrocodiles 1/3 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=silhouette + "\n" + surface
                + "\n" + armor + "\n" + pigment
                + "\n" + camera + "\ncrocodiles 0/3 visible (near 0, far 0), 0 draw"),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_crocodile_report(invalid)

    def test_fish_scope_requires_current_anatomy_two_frames_and_live_context(self):
        self.assertIn("fish", gate.SCOPES)
        self.assertIn('["fish", ["--viewpoint", "fish", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        marker = "near fish use eleven-station sixteen-sided bodies, closed swept fins, and fitted highlighted eyes"
        profile = "near fish use capped smooth-lit bodies, caudal peduncles and progressive rear-body bends"
        pattern = "near fish use softly interpolated family flank lines, bars, and caudal marks"
        tail = "fish use narrow caudal peduncles and shallow swept forked tails"
        detail = "fish profile pass uses six-facet oval eyes, tapered fork tips, and a lower rear-swept dorsal"
        head = "near fish use blunt convex foreheads and rounded snouts instead of needle-point wedges"
        surface = "near fish use restrained upper-flank iridescence and palette-matched gill creases"
        def report():
            return {"passed": True, "views": [{"mode": "fish", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "fish.png", "afterSimulationScreenshot": "fish-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": marker + "\n" + profile + "\n" + pattern + "\n" + tail + "\n" + detail + "\n" + head + "\n" + surface
                    + "\nfish 6/48 visible (near 6, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_fish_report(report()), ("fish.png", "fish-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output="fish 6/48 visible (near 6, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + profile
                + "\nfish 6/48 visible (near 6, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + profile
                + "\n" + pattern + "\nfish 0/48 visible (near 0, far 0), 0 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + profile
                + "\n" + pattern + "\nfish 6/48 visible (near 6, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + profile
                + "\n" + pattern + "\n" + tail + "\n" + detail + "\n" + head
                + "\nfish 6/48 visible (near 6, far 0), 1 draw"),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_fish_report(invalid)

    def test_shark_scope_requires_current_anatomy_two_frames_and_live_context(self):
        self.assertIn("shark", gate.SCOPES)
        self.assertIn('["shark", ["--viewpoint", "shark", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        marker = "near shark uses a fourteen-station eighteen-sided capped body, closed fins, five paired gill slits, and fitted eyes"
        profile = "near shark uses a smooth-lit blunt head and body, caudal peduncle and heterocercal tail"
        proportions = "shark uses a long low spindle, reinforced caudal peduncle, and swept restrained fins"
        contrast = "shark profile pass uses charcoal flank contrast, a lower rear-swept dorsal, and shorter pectorals"
        skin = "shark skin pass preserves a dark dorsal and fin-edge silhouette with a raised pale lower flank after water tint"
        geometry = "shark near/far geometry 1896/15 vertices, closed fins and CPU faceted daylight"
        def report():
            return {"passed": True, "views": [{"mode": "shark", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "shark.png", "afterSimulationScreenshot": "shark-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": marker + "\n" + profile + "\n" + proportions + "\n" + contrast + "\n" + skin + "\n" + geometry + "\nshark visible activity 1, 1 draw"}}]}
        self.assertEqual(gate.validate_shark_report(report()), ("shark.png", "shark-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output="shark visible activity 1, 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\nshark culled activity 1, 0 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + profile
                + "\n" + geometry + "\nshark visible activity 1, 1 draw"),
            lambda value: value["views"][0]["result"].update(output=report()["views"][0]["result"]["output"].replace(skin + "\n", "")),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_shark_report(invalid)

    def test_dolphin_scope_requires_current_anatomy_two_frames_and_live_context(self):
        self.assertIn("dolphins", gate.SCOPES)
        self.assertIn('["dolphins", ["--viewpoint", "dolphins", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        marker = "near dolphins use closed-volume fins and flukes, short beaks, faceted eyes, mouth creases, and a blowhole"
        silhouette = "near dolphins use a nineteen-station closed melon profile, twenty-four-sided body, and vertex-blended countershading"
        stroke = "dolphin tail stock bends vertically through horizontal fluke strokes"
        profile = "near dolphins use an extended fitted rostrum, lower swept fins, and readable grey countershading"
        face = "dolphin facial pass uses a stout tapered rostrum and small six-facet oval eyes instead of flat diamond marks"
        skin = "dolphin skin pass uses continuous axial pigment, a pale swept lower flank, restrained tonal variation, a longer rounded rostrum, and softer swept fins"
        proportions = "near dolphins use a slimmer fusiform midbody, narrower bottlenose rostrum, stronger countershading, a taller falcate dorsal, and root-thickened tapered fins and flukes"
        pectorals = "near dolphins use narrow curved-back pectorals and a restrained fitted lower-jaw mouth seam"
        silhouette_polish = "dolphin silhouette pass uses a ten-point falcate dorsal, nine-point curved pectorals, eight-point forked flukes, and brighter swept ventral countershading"
        geometry = "dolphin near/far geometry 3300/15 vertices, closed fins and CPU faceted daylight"
        def report():
            return {"passed": True, "views": [{"mode": "dolphins", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "screenshot": "dolphins.png", "afterSimulationScreenshot": "dolphins-after.png",
                "intervals": [16.7] * 60,
                "result": {"output": marker + "\n" + silhouette + "\n" + stroke + "\n" + profile + "\n" + face + "\n" + skin + "\n" + proportions + "\n" + pectorals + "\n" + silhouette_polish + "\n" + geometry
                    + "\ndolphins 1/4 visible (near 1, far 0), 1 draw"}}]}
        self.assertEqual(gate.validate_dolphin_report(report()),
                         ("dolphins.png", "dolphins-after.png"))
        for mutate in (
            lambda value: value.update(passed=False),
            lambda value: value["views"][0].update(glError=1282),
            lambda value: value["views"][0].update(contextLost=True),
            lambda value: value["views"][0].update(intervals=[16.7] * 59),
            lambda value: value["views"][0]["result"].update(output=geometry + "\ndolphins 1/4 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette + "\n" + geometry + "\ndolphins 1/4 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette + "\n" + stroke + "\n" + geometry + "\ndolphins 1/4 visible (near 1, far 0), 1 draw"),
            lambda value: value["views"][0]["result"].update(output=marker + "\n" + silhouette + "\n" + stroke + "\n" + profile + "\n" + geometry + "\ndolphins 0/4 visible (near 0, far 0), 0 draw"),
            lambda value: value["views"][0]["result"].update(output=report()["views"][0]["result"]["output"].replace(proportions + "\n", "")),
            lambda value: value["views"][0]["result"].update(output=report()["views"][0]["result"]["output"].replace(pectorals + "\n", "")),
            lambda value: value["views"][0]["result"].update(output=report()["views"][0]["result"]["output"].replace(silhouette_polish + "\n", "")),
        ):
            invalid = report(); mutate(invalid)
            with self.assertRaises(RuntimeError): gate.validate_dolphin_report(invalid)

    def test_snake_scope_requires_both_views_and_visible_population(self):
        self.assertIn("snakes", gate.SCOPES)
        environment = gate.browser_environment("chrome", "snakes", Path("/evidence"))
        self.assertEqual(environment["ISLAND_BROWSER_SUPERVISED"], "chrome")
        self.assertEqual(environment["ISLAND_WEB_SMOKE_SCOPE"], "snakes")
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedSnakeCheck ? ["snakes", "snake-head", "tree-snakes"]', source)
        self.assertIn('snakeMotionFrames.length === 4', source)
        self.assertIn('snakes (?:[1-9]|1[0-6])\\/56', source)
        for mode in ("snakes", "snake-head", "tree-snakes"):
            self.assertIn(f'["{mode}", ["--viewpoint", "{mode}"]]',
                          (ROOT / "tools/web_smoke.html.in").read_text())

    def test_snake_report_rejects_frozen_missing_and_invalid_motion(self):
        log=('snakes use a readable dorsal saddle and active forked-tongue flicks\n'
             'near snakes use a broad low temple, tapered blunt muzzle, raised amber eyes, and fitted vertical pupils\n'
             'snake-head QA isolates facial geometry while the companion body view retains habitat grass\n'
             'tree-snake QA frames a live climber on the sampled trunk surface\n'
             'six arboreal snakes cling and climb on sampled broadleaf trunk triangles\n')+'\n'.join(
            f"snake runtime sample frame={i*8} head={i*0.2},2,0 speed=0.5 visible=1" for i in range(1,5))
        def report(text):
            return {"passed":True,"views":[{"mode":mode,"result":{"output":text}}
                                          for mode in ("snakes","snake-head","tree-snakes")]}
        self.assertEqual(len(gate.validate_snake_report(report(log))),3)
        for invalid in ("",log.replace("frame=16","frame=8"),log.replace("speed=0.5","speed=nan"),
                        log.replace("visible=1","visible=0"),log.replace("head=0.2,2,0","head=nan,2,0"),
                        '\n'.join(f"snake runtime sample frame={i*8} head=1,2,0 speed=0 visible=1" for i in range(1,5))):
            with self.assertRaises(RuntimeError): gate.validate_snake_report(report(invalid))
        missing=report(log);missing["views"].pop()
        with self.assertRaises(RuntimeError): gate.validate_snake_report(missing)
        with self.assertRaises(RuntimeError):
            gate.validate_snake_report(report(log.replace(
                "snakes use a readable dorsal saddle and active forked-tongue flicks\n", "")))
        with self.assertRaises(RuntimeError):
            gate.validate_snake_report(report(log.replace(
                "near snakes use a broad low temple, tapered blunt muzzle, raised amber eyes, and fitted vertical pupils\n", "")))
        with self.assertRaises(RuntimeError):
            gate.validate_snake_report(report(log.replace("snake-head QA isolates facial geometry while the companion body view retains habitat grass\n", "")))
        with self.assertRaises(RuntimeError):
            gate.validate_snake_report(report(log.replace("six arboreal snakes cling and climb on sampled broadleaf trunk triangles\n", "")))

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

    def test_wildlife_foliage_boar_gate_matches_split_material_draws(self):
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn("boars", gate.SCOPES)
        self.assertIn('focusedBoarCheck ? ["boars"]', source)
        self.assertIn('boar RGB bristle coat and solid anatomy, two opaque stock BasicEffect draws',
                      source)
        self.assertIn('near boars use broad tapered upper legs with compact split hooves', source)
        self.assertIn('near boar limbs blend into overlapping furred shoulder and haunch masses', source)
        self.assertIn('near boar lower legs use a continuous knee-to-cannon taper without spherical joint caps', source)
        self.assertIn('boars use a lifted shoulder mantle and broad sealed nasal disc', source)
        self.assertIn('near boars retain warm shoulder, cheek, nose, and eye contrast in canopy shade', source)
        self.assertIn('boar face retains a warm nasal disc and visible amber eye catchlight in wet understory', source)
        self.assertIn('near adult boars use sharp triangular pinnae and paired two-segment curved ivory tusks', source)
        self.assertIn('/boars [1-6]\\/6 visible, 2 draw/.test(log)', source)
        self.assertNotIn('/boars [1-6]\\/6 visible, 1 draw/.test(log)', source)
        log = ("boar QA camera frames and follows non-hostile inland wildlife\n"
               "near boars use broad tapered upper legs with compact split hooves\n"
               "near boar limbs blend into overlapping furred shoulder and haunch masses\n"
               "near boar lower legs use a continuous knee-to-cannon taper without spherical joint caps\n"
               "boars use a lifted shoulder mantle and broad sealed nasal disc\n"
               "near boars retain warm shoulder, cheek, nose, and eye contrast in canopy shade\n"
               "boar face retains a warm nasal disc and visible amber eye catchlight in wet understory\n"
               "near adult boars use sharp triangular pinnae and paired two-segment curved ivory tusks\n"
               "boars 1/6 visible, 2 draw")
        report = {"passed":True,"views":[{"mode":"boars","passed":True,"glError":0,
            "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
            "screenshot":"before.png","afterSimulationScreenshot":"after.png",
            "result":{"output":log,"error":"","exited":False}}]}
        self.assertEqual(gate.validate_boar_report(report), ("before.png", "after.png"))
        for mutate in (lambda r:r["views"][0].update({"contextLost":True}),
                       lambda r:r["views"][0].update({"intervals":[1.0]*59}),
                       lambda r:r["views"][0]["result"].update({"output":"boars 1/6 visible, 2 draw"}),
                       lambda r:r["views"][0]["result"].update({"output":log.replace(
                           "near boar limbs blend into overlapping furred shoulder and haunch masses\n", "")}),
                       lambda r:r["views"][0]["result"].update({"output":log.replace(
                           "near adult boars use sharp triangular pinnae and paired two-segment curved ivory tusks\n", "")}),
                       lambda r:r["views"][0].update({"afterSimulationScreenshot":"before.png"})):
            bad = {"passed":True,"views":[{"mode":"boars","passed":True,"glError":0,
                "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
                "screenshot":"before.png","afterSimulationScreenshot":"after.png",
                "result":{"output":log,"error":"","exited":False}}]}
            mutate(bad)
            with self.assertRaises(RuntimeError): gate.validate_boar_report(bad)

    def test_resident_scope_requires_current_silhouette_two_frames_and_live_context(self):
        self.assertIn("resident", gate.SCOPES)
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedResidentCheck ? ["resident"]', source)
        self.assertIn('["resident", ["--viewpoint", "resident", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        log = ("resident QA camera frames and follows one peaceful adult\n"
               "resident uses a shoulder-sloped woven tunic with open cap sleeves and anatomically tapered calves\n"
               "resident uses buried deltoid shoulders, split hands, and layered facial anatomy\n"
               "resident anatomy uses a tapered trapezius neck, rounded sleeve-deltoid transitions, shaped forearms, cheeks, jaw, eyelids, and nostril wings\n"
               "resident face uses a restrained nasal tip and separate upper and lower lip contours\n"
               "resident woven clothing adds a bound V neckline and split thigh-following wrap\n"
               "resident gait uses foot-synchronised thirty-degree arm counter-swing with relaxed elbows\n"
               "resident 1/1 visible, 2 draws, 5154 body triangles")
        report = {"passed":True,"views":[{"mode":"resident","passed":True,"glError":0,
            "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
            "screenshot":"before.png","afterSimulationScreenshot":"after.png",
            "result":{"output":log,"error":"","exited":False}}]}
        self.assertEqual(gate.validate_resident_report(report), ("before.png", "after.png"))
        for mutate in (lambda r:r["views"][0].update({"contextLost":True}),
                       lambda r:r["views"][0].update({"intervals":[1.0]*59}),
                       lambda r:r["views"][0]["result"].update({"output":"resident 1/1 visible"}),
                       lambda r:r["views"][0].update({"afterSimulationScreenshot":"before.png"})):
            bad = {"passed":True,"views":[{"mode":"resident","passed":True,"glError":0,
                "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
                "screenshot":"before.png","afterSimulationScreenshot":"after.png",
                "result":{"output":log,"error":"","exited":False}}]}
            mutate(bad)
            with self.assertRaises(RuntimeError): gate.validate_resident_report(bad)

    def test_bear_scope_requires_continuous_limbs_two_frames_and_live_context(self):
        self.assertIn("bear", gate.SCOPES)
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedBearCheck ? ["bear"]', source)
        self.assertIn('bear legs use continuous shoulder-to-wrist lofts and five-clawed broad paws',
                      source)
        self.assertIn('near bear paws use short two-segment horn claws with buried roots and downward tips',
                      source)
        self.assertIn('bear torso uses a continuous rump-to-neck loft without overlapping body shells',
                      source)
        self.assertIn('near bear uses a broader rounded skull, fitted brow volumes, larger rounded ears, and a short wide muzzle',
                      source)
        log = ("bear QA camera frames and follows the single peaceful adult\n"
               "bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes\n"
               "near bear uses twenty-two body-envelope fur wedges without another draw\n"
               "near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes\n"
               "bear legs use continuous shoulder-to-wrist lofts and five-clawed broad paws\n"
               "near bear paws use short two-segment horn claws with buried roots and downward tips\n"
               "bear torso uses a continuous rump-to-neck loft without overlapping body shells\n"
               "near bear uses a broader rounded skull, fitted brow volumes, larger rounded ears, and a short wide muzzle\n"
               "bear 1/1 visible, 2 draws, 5914 body triangles")
        report = {"passed":True,"views":[{"mode":"bear","passed":True,"glError":0,
            "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
            "screenshot":"before.png","afterSimulationScreenshot":"after.png",
            "result":{"output":log,"error":"","exited":False}}]}
        self.assertEqual(gate.validate_bear_report(report), ("before.png", "after.png"))
        for mutate in (lambda r:r["views"][0].update({"contextLost":True}),
                       lambda r:r["views"][0].update({"intervals":[1.0]*59}),
                       lambda r:r["views"][0]["result"].update({"output":"bear 1/1 visible"}),
                       lambda r:r["views"][0].update({"afterSimulationScreenshot":"before.png"})):
            bad = {"passed":True,"views":[{"mode":"bear","passed":True,"glError":0,
                "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
                "screenshot":"before.png","afterSimulationScreenshot":"after.png",
                "result":{"output":log,"error":"","exited":False}}]}
            mutate(bad)
            with self.assertRaises(RuntimeError): gate.validate_bear_report(bad)

    def test_campfire_scope_requires_current_material_two_frames_and_live_context(self):
        self.assertIn("campfire", gate.SCOPES)
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedCampfireCheck ? ["campfire"]', source)
        self.assertIn('["campfire", ["--viewpoint", "campfire", "--debug-overlay"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        log = ("campfire QA camera near\n"
               "campfire 1992 solid triangles, 42 flame/smoke/spark triangles in two culled draws\n"
               "campfire flame material uses broken translucent tongues, pale cores, a restrained blue base, and layered smoke\n"
               "campfire bed uses a pale mottled ash centre, scorched rim, and mixed glowing charcoal\n"
               "campfire and stored firewood stand on qualified broad dry-sand beach\n"
               "campfire first visible animated draw")
        report = {"passed":True,"views":[{"mode":"campfire","passed":True,"glError":0,
            "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
            "screenshot":"before.png","afterSimulationScreenshot":"after.png",
            "result":{"output":log,"error":"","exited":False}}]}
        self.assertEqual(gate.validate_campfire_report(report), ("before.png", "after.png"))
        for mutate in (lambda r:r["views"][0].update({"contextLost":True}),
                       lambda r:r["views"][0].update({"intervals":[1.0]*59}),
                       lambda r:r["views"][0]["result"].update({"output":"campfire first visible animated draw"}),
                       lambda r:r["views"][0].update({"afterSimulationScreenshot":"before.png"})):
            bad = {"passed":True,"views":[{"mode":"campfire","passed":True,"glError":0,
                "contextLost":False,"renderer":"ANGLE SwiftShader","intervals":[1.0]*60,
                "screenshot":"before.png","afterSimulationScreenshot":"after.png",
                "result":{"output":log,"error":"","exited":False}}]}
            mutate(bad)
            with self.assertRaises(RuntimeError): gate.validate_campfire_report(bad)

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
                    "goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes\n"
                    "goats use separated haunch, waist and raised shoulder masses, six-station necks, and six deterministic natural coat tints\n"
                    "near goats use paired surface nostrils and fitted lateral mouth creases"}}]}
        self.assertEqual(len(gate.validate_goat_report(report())), 4)
        for key, value in (("mode", "monkeys"), ("passed", False), ("glError", 1282),
                           ("contextLost", True), ("goatMotionFrames", []),
                           ("goatMotionFrames", ["same.png"] * 4)):
            bad = report(); bad["views"][0][key] = value
            with self.subTest(key=key), self.assertRaises(RuntimeError):
                gate.validate_goat_report(bad)
        for old, new in (("3/6", "0/6"), ("2 draw", "1 draw"),
                         ("goat fur loaded", "missing"), ("9 mips", "1 mips"),
                         ("near goats use paired surface nostrils", "missing muzzle detail"),
                         ("six-station necks", "four-station necks")):
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
        self.assertIn('["spider-web", ["--viewpoint", "spider-web"]]',
                      (ROOT / "tools/web_smoke.html.in").read_text())
        source = (ROOT / "tools/run_web_smoke.mjs").read_text()
        self.assertIn('focusedSpiderCheck ? ["spiders", "spider-web"]', source)
        self.assertIn('spiderMotionFrames.push(frame)', source)
        def report():
            return {"passed": True, "views": [{"mode": "spiders", "passed": True,
                "glError": 0, "contextLost": False, "renderer": "ANGLE SwiftShader",
                "intervals": [16.7]*60, "spiderMotionFrames": [f"pose-{i}.png" for i in range(4)],
                "result": {"output": "spider render proof: spiders 1/200 visible, 2 draw; contact-shadows=1\n"
                    "spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes\n"
                    "near spiders use tiered fitted eyes with upper-forward catchlights\n"
                    "spider lighting ground-transmission=0 body-transmission=0 body-tint=0.46,0.53,0.62\n"}},
                {"mode":"spider-web","passed":True,"glError":0,"contextLost":False,
                 "renderer":"ANGLE SwiftShader","screenshot":"web.png",
                 "result":{"output":"spider-web render proof: webs 1/6 visible, strands 434, 1 draw\n"}}]}
        self.assertEqual(len(gate.validate_spider_report(report())), 5)
        for key, value in (("mode", "goats"), ("passed", False), ("glError", 1282),
                           ("contextLost", True), ("renderer", "AMD"), ("intervals", []),
                           ("intervals", [float('nan')]*60), ("intervals", [0]*60),
                           ("spiderMotionFrames", []), ("spiderMotionFrames", ["same.png"]*4),
                           ("spiderMotionFrames", ["../escape.png", "a.png", "b.png", "c.png"])):
            bad=report(); bad["views"][0][key]=value
            with self.subTest(key=key), self.assertRaises(RuntimeError): gate.validate_spider_report(bad)
        for old, new in (("1/200", "0/200"), ("2 draw", "1 draw"), ("contact-shadows=1", "contact-shadows=0"),
                         ("setae loaded", "missing"), ("tiered fitted eyes", "uniform eyes"),
                         ("body-tint=0.46", "body-tint=0.25"),
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

    def test_firefox_walk_holds_real_input_until_game_acknowledges_it(self):
        source = (ROOT / "tools/run_firefox_web_smoke.mjs").read_text()
        self.assertIn('globalThis.islandFirefoxKeyEvents = []', source)
        self.assertIn('key: event.key, code: event.code, trusted: event.isTrusted', source)
        self.assertIn('movementBridgeEvents: globalThis.islandMovementKeyBridgeEvents ?? []', source)
        self.assertIn('const holdMovementUntilOutput = async (key, marker, attempts = 240)', source)
        self.assertIn('return await waitForOutput(context, marker, attempts)', source)
        self.assertIn('output = await holdMovementUntilOutput(', source)
        self.assertIn('"W", "island-simulator: first distance-scheduled land footstep played"', source)
        self.assertNotIn('{ type: "pause", duration: 700 }', source)

        game = (ROOT / "game/src/IslandGame.cpp").read_text()
        self.assertIn("InstallWebMovementKeyBridge", game)
        self.assertIn("globalThis.islandMovementKeyBridgeEvents = []", game)
        self.assertIn("volatile int WebMovementKeyMaskValue = 0", game)
        self.assertIn("EMSCRIPTEN_KEEPALIVE void IslandSetWebMovementKeyState", game)
        self.assertIn("_IslandSetWebMovementKeyState(bit, value)", game)
        self.assertIn("WebMovementKeyMaskValue;", game)
        self.assertIn('case "KeyW": case "ArrowUp"', game)
        self.assertIn('addEventListener("blur", clear', game)
        self.assertIn('document.visibilityState !== "visible"', game)
        self.assertIn('|| (webMovementKeyMask & 1) != 0', game)

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
