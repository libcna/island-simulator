#!/usr/bin/env python3
"""CPU-only tests. Never starts adb, an emulator, a namespace, or graphics."""
import os
import io
import subprocess
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile
import warnings
from types import SimpleNamespace
from unittest.mock import mock_open, patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import run_android_software as gate
import android_capture
from PIL import Image
from android_touch_geometry import geometry, from_log, TouchGeometry


class AndroidSoftwareTests(unittest.TestCase):
    def test_failed_presentation_retains_fresh_app_log_before_teardown(self):
        for pid in ("1864", "1864 1870", ""):
            with self.subTest(pid=pid), tempfile.TemporaryDirectory(prefix="island-failure-test-") as temporary:
                args = SimpleNamespace(apk=Path("test.apk"), apk_sha256="test",
                    sdk=Path("/sdk"), evidence=Path(temporary), lifecycle=False)
                failure = subprocess.CalledProcessError(1, "runtime-gate")
                responses = [failure, SimpleNamespace(stdout=pid)]
                if pid == "1864": responses.append(SimpleNamespace(returncode=0))
                responses.append(SimpleNamespace(returncode=0))
                with patch.object(gate, "validate_apk", return_value={"sha256":"test"}), \
                     patch.object(gate, "game_environment", return_value={}), \
                     patch.object(gate.subprocess, "run", side_effect=responses) as run:
                    with self.assertRaises(subprocess.CalledProcessError): gate.run_game(args)
                    commands = [call.args[0] for call in run.call_args_list]
                    self.assertEqual(any("--pid=1864" in command for command in commands), pid == "1864")
                    self.assertIn("screencap", commands[-1])
                    self.assertEqual((Path(temporary)/"game-failure-logcat.txt").exists(), pid == "1864")

    def test_graphics_gate_rejects_stock_link_failures_and_guest_gl_errors(self):
        source = (Path(__file__).resolve().parent.parent /
                  "tools/run_android_runtime_smoke.sh").read_text()
        declaration = next(line for line in source.splitlines()
                           if line.startswith("GRAPHICS_ERROR_PATTERN="))
        pattern = declaration.split("=", 1)[1].strip("'")
        self.assertEqual(source.count('grep -Eiq "$GRAPHICS_ERROR_PATTERN"'), 4)
        fixtures = (
            ("[CNA EasyGL 3D] skinned (vertex-lit) link failed:", True),
            ("[CNA EasyGL 3D] colored VS failed:", True),
            ("[CNA EasyGL 3D] lit FS failed:", True),
            ("GL2Encoder.cpp:s_glGetUniformLocation:2221 GL error 0x502", True),
            ("GL error 1282", True), ("GL error 0x0000", False),
            ("shader compilation failed", True), ("FATAL EXCEPTION", True),
            ("island-simulator: fatal error: fixture", True),
            ("Failed to initialize 101010-2 format, error = EGL_SUCCESS", False),
            ("renderer OPENGLES3; startup first draw 100 ms", False))
        for message, rejected in fixtures:
            with self.subTest(message=message):
                result = subprocess.run(["grep", "-Eiq", pattern], input=message,
                                        text=True, check=False)
                self.assertEqual(result.returncode, 0 if rejected else 1)

    def test_android_capture_rejects_black_and_incomplete_png(self):
        data = io.BytesIO()
        Image.new("RGB", (640, 360)).save(data, format="PNG")
        self.assertFalse(android_capture.validate_png(data.getvalue())["valid"])
        with self.assertRaises(OSError):
            android_capture.validate_png(b"incomplete")

    def test_android_capture_accepts_non_degenerate_scene_fixture(self):
        image = Image.new("RGB", (640, 360))
        image.putdata([(x % 256, y % 256, (x + y) % 256)
                       for y in range(360) for x in range(640)])
        data = io.BytesIO()
        image.save(data, format="PNG")
        self.assertTrue(android_capture.validate_png(data.getvalue())["valid"])

    def test_android_capture_wait_is_bounded_and_requires_image_evidence(self):
        for passes in (True, False):
            with self.subTest(passes=passes), tempfile.TemporaryDirectory() as temporary:
                output = Path(temporary) / "capture.png"
                reports = [{"valid": False}, {"valid": passes}]
                with patch.object(android_capture, "validate_png", side_effect=reports), \
                     patch.object(android_capture.subprocess, "run",
                                  return_value=SimpleNamespace(stdout=b"fixture")) as run, \
                     patch.object(android_capture.time, "monotonic", side_effect=
                                  [0, 0, 0, 0, 1, 1, 2, 3]), \
                     patch.object(android_capture.time, "sleep"):
                    if passes:
                        android_capture.capture("fake-adb", output, timeout=3)
                    else:
                        with self.assertRaisesRegex(RuntimeError, "no usable"):
                            android_capture.capture("fake-adb", output, timeout=3)
                    self.assertEqual(run.call_count, 2)
                    self.assertTrue(output.with_suffix(".validation.json").exists())

    def test_android_capture_passes_explicit_profile_through_png_decode(self):
        data = io.BytesIO()
        Image.new("RGB", (640, 360)).save(data, format="PNG")
        with patch.object(android_capture, "validate_capture", return_value={"valid": True}) as validate:
            android_capture.validate_png(data.getvalue(), profile="shaded-detail")
            self.assertEqual(validate.call_args.kwargs, {"profile": "shaded-detail"})
        with tempfile.TemporaryDirectory() as temporary, \
             patch.object(android_capture, "validate_png", return_value={"valid": True}) as decode, \
             patch.object(android_capture.subprocess, "run",
                          return_value=SimpleNamespace(stdout=data.getvalue())):
            android_capture.capture("fake-adb", Path(temporary) / "capture.png",
                                    profile="shaded-detail")
            decode.assert_called_once_with(data.getvalue(), profile="shaded-detail")

    @staticmethod
    def material_fixture(directory):
        root = directory / "prepared"
        for relative in gate.MATERIAL_PATHS:
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"prepared fixture " + relative.encode())
        return root

    def test_touch_test_requires_a_real_interactive_camera(self):
        args = SimpleNamespace(apk=Path("game.apk"), boot=True, touch_selector=True,
                               lifecycle=True, traversal=False, viewpoint="interactive")
        gate.validate_game_options(args)
        args.viewpoint = "clearing-edge"
        with self.assertRaisesRegex(ValueError, "fixed QA cameras"):
            gate.validate_game_options(args)
        args.touch_selector = False
        gate.validate_game_options(args)
        args.traversal = True
        with self.assertRaisesRegex(ValueError, "fixed QA cameras"):
            gate.validate_game_options(args)
        args.traversal = False
        args.apk = None
        with self.assertRaisesRegex(ValueError, "require --apk"):
            gate.validate_game_options(args)

    def test_apk_abi_is_checked_before_installation(self):
        with tempfile.TemporaryDirectory(prefix="island-android-test-") as temporary:
            apk = Path(temporary) / "fixture.apk"
            materials = self.material_fixture(Path(temporary))
            for libraries, accepted in ((["lib/x86_64/libmain.so"], True),
                                        (["lib/arm64-v8a/libmain.so"], False),
                                        (["lib/x86_64/libmain.so", "lib/arm64-v8a/libmain.so"], False),
                                        (["lib/x86_64/libother.so"], False)):
                with zipfile.ZipFile(apk, "w") as package:
                    for relative in gate.MATERIAL_PATHS:
                        package.writestr("assets/" + relative, (materials / relative).read_bytes())
                    for library in libraries:
                        package.writestr(library, b"test fixture")
                if accepted:
                    result = gate.validate_apk(apk, materials)
                    self.assertEqual(len(result["sha256"]), 64)
                    self.assertEqual(set(result["prepared_materials"]), set(gate.MATERIAL_PATHS))
                else:
                    with self.assertRaisesRegex(RuntimeError, "x86_64-only"):
                        gate.validate_apk(apk, materials)

    def test_both_apk_materials_must_be_unique_and_match_prepared_bytes(self):
        with tempfile.TemporaryDirectory(prefix="island-android-test-") as temporary:
            apk = Path(temporary) / "fixture.apk"
            materials = self.material_fixture(Path(temporary))
            for damaged in gate.MATERIAL_PATHS:
                for fault in ("missing", "size", "hash", "duplicate"):
                    with self.subTest(material=damaged, fault=fault), warnings.catch_warnings():
                        warnings.simplefilter("ignore", UserWarning)  # Intentional duplicate ZIP fixture.
                        with zipfile.ZipFile(apk, "w") as package:
                            package.writestr("lib/x86_64/libmain.so", b"fixture")
                            for relative in gate.MATERIAL_PATHS:
                                payload = (materials / relative).read_bytes()
                                if relative == damaged:
                                    if fault == "missing": continue
                                    if fault == "size": payload = payload[:-1]
                                    if fault == "hash": payload = bytes([payload[0] ^ 1]) + payload[1:]
                                    if fault == "duplicate": package.writestr("assets/" + relative, payload)
                                package.writestr("assets/" + relative, payload)
                        with self.assertRaisesRegex(RuntimeError, "material PNG"):
                            gate.validate_apk(apk, materials)

    def test_game_checks_are_explicit_and_bounded(self):
        args = SimpleNamespace(touch_selector=True, lifecycle=False, traversal=False, viewpoint="clearing-edge")
        environment = gate.game_environment(args)
        self.assertEqual(environment["ISLAND_ANDROID_SMOKE_TIMEOUT_SECONDS"], "120")
        self.assertEqual(environment["ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY"], "1")
        self.assertEqual(environment["ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE"], "0")
        self.assertEqual(environment["ISLAND_ANDROID_SMOKE_ARGUMENTS"], "--viewpoint,clearing-edge")
        args.viewpoint = "interactive"
        self.assertEqual(gate.game_environment(args)["ISLAND_ANDROID_SMOKE_ARGUMENTS"], "")
        args.traversal = True
        for key in ("FOOTSTEPS", "SWIMMING", "AUDIO"):
            self.assertEqual(gate.game_environment(args)["ISLAND_ANDROID_SMOKE_REQUIRE_" + key], "1")

    def test_gestures_follow_fitted_content_not_physical_screen_edges(self):
        self.assertEqual(geometry(1280, 720, 1280, 720), TouchGeometry(1219, 60, 320, 547, 120, 600))
        self.assertEqual(geometry(2400, 1080, 1280, 720), TouchGeometry(2069, 90, 720, 820, 180, 900))
        self.assertEqual(geometry(720, 1280, 1280, 720), TouchGeometry(685, 471, 180, 744, 504, 774))
        for width, height in ((1280, 720), (2400, 1080), (720, 1280), (1920, 1200), (1365, 767)):
            with self.subTest(width=width, height=height):
                point = geometry(width, height, 1280, 720)
                self.assertLess(point.movement_x, width // 2)
                self.assertGreater(point.selector_x, width // 2)
                self.assertLess(point.movement_high_y, point.movement_low_y)
                self.assertLess(point.movement_low_y, point.movement_bottom_y)
                self.assertLess(point.movement_bottom_y, height)

    def test_swimming_gestures_repeat_but_require_both_evidence_gates(self):
        # Execute the actual shell gate with fake touch/log/time providers: no device or sleep.
        source = (Path(__file__).resolve().parent.parent / "tools" /
                  "run_android_runtime_smoke.sh").read_text()
        block = source.split('if [ "$REQUIRE_SWIMMING" -eq 1 ]; then', 1)[1]
        block = 'if [ "$REQUIRE_SWIMMING" -eq 1 ]; then' + block.split(
            '\nif grep -Eiq "$GRAPHICS_ERROR_PATTERN"', 1)[0]
        fixture = r'''
set -eu
clock=0; gestures=0
date() { printf '%s\n' "$clock"; }
sleep() { clock=$((clock + 1)); }
fake_adb() {
    if [ "$1" = shell ]; then
        gestures=$((gestures + 1)); clock=$((clock + 9))
    elif [ "$gestures" -ge 3 ]; then
        if [ "$emit_swim" = 1 ]; then
            echo 'island-simulator: first traversal entered swimming state'
        fi
        if [ "$emit_audio" = 1 ]; then
            echo 'island-simulator: first water-transition accent played'
        fi
    fi
    return 0
}
ADB=fake_adb; GAME_PID=123; REQUIRE_SWIMMING=1; REQUIRE_AUDIO=1
MOVE_X=320; MOVE_HIGH_Y=120; MOVE_BOTTOM_Y=600
LOG_FILE="$1"; emit_swim="$2"; emit_audio="$3"
'''
        with tempfile.TemporaryDirectory(prefix="island-touch-gate-") as directory:
            for swim, audio in ((1, 1), (0, 1), (1, 0)):
                with self.subTest(swim=swim, audio=audio):
                    result = subprocess.run(
                        ["bash", "-c", fixture + block, "fixture",
                         str(Path(directory) / "log"), str(swim), str(audio)],
                        capture_output=True, text=True, timeout=5)
                    self.assertEqual(result.returncode, 0 if swim and audio else 1,
                                     result.stderr)
                    expected = "3, swimming qualified 1" if swim and audio else "10, swimming qualified 0"
                    self.assertIn("shoreline gestures " + expected, result.stdout)

    def test_touch_geometry_rejects_invalid_or_missing_extent_evidence(self):
        for values in ((0, 720, 1280, 720), (1280, float("nan"), 1280, 720),
                       (True, 720, 1280, 720), (65537, 720, 1280, 720), (16, 16, 65536, 16),
                       (16, 65536, 16, 65536)):
            with self.subTest(values=values), self.assertRaises(ValueError):
                geometry(*values)
        for log in ("", "Window size: 1280x720", "viewport 1280x720 logical backbuffer"):
            with self.subTest(log=log), self.assertRaises(ValueError):
                from_log(log)
        log = "Window size: 2400x1080\nWindow size: 1280x720\nviewport 1280x720 logical backbuffer\n"
        self.assertEqual(from_log(log), geometry(1280, 720, 1280, 720))

    def test_game_must_report_software_gles_without_graphics_errors(self):
        renderer = 'renderer="OPENGLES3" gl-renderer="Google SwiftShader"\n'
        good = renderer + 'monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes\n'
        with tempfile.TemporaryDirectory(prefix="island-android-test-") as temporary:
            directory = Path(temporary)
            log = directory / "island-android-logcat.txt"
            log.write_text(good)
            gate.validate_game_logs(directory, False)
            spiders = good + ("spider render proof: spiders 1/200 visible, 2 draw; contact-shadows=1\n"
                "spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes\n"
                "near spiders use tiered fitted eyes with upper-forward catchlights\n"
                "spider lighting ground-transmission=0 body-transmission=0 body-tint=0.46,0.53,0.62\n")
            log.write_text(spiders)
            gate.validate_game_logs(directory, False, "spiders")
            for old, new in (("1/200", "0/200"), ("2 draw", "1 draw"),
                             ("contact-shadows=1", "contact-shadows=0"), ("setae loaded", "missing"),
                             ("tiered fitted eyes", "uniform eyes"),
                             ("body-tint=0.46", "body-tint=0.1"), ("spider lighting", "missing")):
                log.write_text(spiders.replace(old, new))
                with self.assertRaises(RuntimeError): gate.validate_game_logs(directory, False, "spiders")
            coat = "goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes\n"
            goats = good + coat + "goat render proof: goats 3/6 visible, 2 draw;\n"
            log.write_text(goats)
            gate.validate_game_logs(directory, False, "goats")
            for old, new in ((coat, ""), ("3/6", "0/6"), ("2 draw", "1 draw"),
                             ("9 mips, 349524", "1 mips, 262144"), ("goat render proof: ", "")):
                log.write_text(goats.replace(old, new))
                with self.assertRaises(RuntimeError): gate.validate_game_logs(directory, False, "goats")
            boars = good + ("near boars use broad tapered upper legs with compact split hooves\n"
                "boars 2/6 visible, 2 draw\n")
            log.write_text(boars)
            gate.validate_game_logs(directory, False, "boars")
            for old, new in (("broad tapered", "missing"), ("2/6", "0/6"), ("2 draw", "1 draw")):
                log.write_text(boars.replace(old, new))
                with self.assertRaises(RuntimeError): gate.validate_game_logs(directory, False, "boars")
            turtle_surface = ("near turtles use a twenty-by-sixteen carapace surface, "
                "eighteen-sided seven-ring heads, and irregular mottled shell and skin tones\n")
            log.write_text(good + turtle_surface)
            gate.validate_game_logs(directory, False, "turtles")
            log.write_text(good)
            with self.assertRaisesRegex(RuntimeError, "sea-turtle surface"):
                gate.validate_game_logs(directory, False, "turtles")
            crocodile_surface = ("near crocodiles use raised eyes, nostrils, separate jaws, "
                                 "teeth, dorsal osteoderms, and wet mottled facet shading, with "
                                 "multi-section rounded snouts and lateral eyes\n")
            crocodile_visible = "crocodiles 1/3 visible (near 1, far 0), 1 draw\n"
            log.write_text(good + crocodile_surface + crocodile_visible)
            gate.validate_game_logs(directory, False, "crocodiles")
            log.write_text(good + crocodile_visible)
            with self.assertRaisesRegex(RuntimeError, "crocodile surface"):
                gate.validate_game_logs(directory, False, "crocodiles")
            log.write_text(good + crocodile_surface
                           + "crocodiles 0/3 visible (near 0, far 0), 0 draw\n")
            with self.assertRaisesRegex(RuntimeError, "visible single-batch crocodile"):
                gate.validate_game_logs(directory, False, "crocodiles")
            log.write_text(good)
            for view in ("hut", "hut-material"):
                with self.assertRaisesRegex(RuntimeError, "hut material"):
                    gate.validate_game_logs(directory, False, view)
                log.write_text(good+"hut beam-aligned weathered grain and filtered thatch fibres\n")
                with self.assertRaisesRegex(RuntimeError, "hut timber relief"):
                    gate.validate_game_logs(directory, False, view)
                log.write_text(log.read_text()+"hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation\n")
                with self.assertRaisesRegex(RuntimeError, "hut facade details"):
                    gate.validate_game_logs(directory, False, view)
                log.write_text(log.read_text()+"hut four-pane muted glazing and braced plank door\n")
                with self.assertRaisesRegex(RuntimeError, "raised hut veranda"):
                    gate.validate_game_logs(directory, False, view)
                log.write_text(log.read_text()+"hut raised veranda uses full-height outer posts, split rails, fascia, and three steps\n")
                with self.assertRaisesRegex(RuntimeError, "enclosed hut gable"):
                    gate.validate_game_logs(directory, False, view)
                log.write_text(log.read_text()+"hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports\n")
                with self.assertRaisesRegex(RuntimeError, "hut material texture"):
                    gate.validate_game_logs(directory, False, view)
                log.write_text(log.read_text()+"hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes\n")
                gate.validate_game_logs(directory, False, view)
                log.write_text(good)
            times = [0, 5, 10.1, 18.5, 24, 29.1, 38.9, 0]
            heights = [5, 5.2, 5.3, 7.3, 6.4, 5.3, 5.01, 5]
            stages = "\n".join(f"monkey climb stage={i} subject=3 tree={187 if i<7 else -1} "
                f"time={t} active={int(i<7)} centre-y={heights[i]}" for i,t in enumerate(times))
            held = "\nmonkey climb held summit presented after eight cycle stages\n"
            log.write_text(good+stages+held)
            gate.validate_game_logs(directory, False, "monkey-climb")
            for incomplete in (good, good+stages, good+stages.replace("time=18.5", "time=11.0")+held):
                log.write_text(incomplete)
                with self.assertRaises(RuntimeError):
                    gate.validate_game_logs(directory, False, "monkey-climb")
            for missing in (renderer, good.replace("10 mips", "1 mips")):
                log.write_text(missing)
                with self.assertRaisesRegex(RuntimeError, "required monkey fur"):
                    gate.validate_game_logs(directory, False)
            for bad in ("GL_INVALID_OPERATION", "gl-error invalid", "Fatal signal 6", "FATAL EXCEPTION",
                        "terrain material fallback: bad", "grass stock-effect fallback: bad",
                        "shader compilation failed", "gl-debug source severity=high"):
                log.write_text(good + bad)
                with self.subTest(bad=bad), self.assertRaisesRegex(RuntimeError, "diagnostic"):
                    gate.validate_game_logs(directory, False)
            for bad in (good.replace("SwiftShader", "Radeon"), good.replace("OPENGLES3", "VULKAN")):
                log.write_text(bad)
                with self.assertRaisesRegex(RuntimeError, "confirm"):
                    gate.validate_game_logs(directory, False)
            log.write_text(good)
            (directory / "island-android-hot-resume-logcat.txt").write_text("onResume()")
            cold = directory / "island-android-cold-relaunch-logcat.txt"
            cold.write_text(good)
            gate.validate_game_logs(directory, True)
            cold.write_text("onResume()")
            with self.assertRaisesRegex(RuntimeError, "confirm"):
                gate.validate_game_logs(directory, True)

    def test_snake_views_require_live_finite_ordered_motion(self):
        good = ('renderer="OPENGLES3" gl-renderer="Google SwiftShader"\n'
                'monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes\n')
        rows = [f"snake runtime sample frame={frame} head={i * .2},2,3 speed=1.15 visible=1\n"
                for i, frame in enumerate((8, 16, 24, 32))]
        with tempfile.TemporaryDirectory(prefix="island-android-test-") as temporary:
            directory = Path(temporary)
            log = directory / "island-android-logcat.txt"
            for view in ("snakes", "snake-head"):
                log.write_text(good + ''.join(rows))
                gate.validate_game_logs(directory, False, view)
                invalid = [[], rows[:3], rows[::-1], rows + rows[:1],
                           [row.replace(f"head={i * .2}", "head=0") for i, row in enumerate(rows)],
                           [row.replace("head=0.0", "head=nan") for row in rows],
                           [row.replace("speed=1.15", "speed=9") for row in rows],
                           [row.replace("visible=1", "visible=0") for row in rows]]
                for samples in invalid:
                    with self.subTest(view=view, samples=samples):
                        log.write_text(good + ''.join(samples))
                        with self.assertRaises(RuntimeError):
                            gate.validate_game_logs(directory, False, view)

    def test_emulator_is_explicitly_software_and_bounded(self):
        command = gate.emulator_command(Path("/sdk"))
        for option, value in (("-gpu", "swiftshader"), ("-cores", "2"), ("-accel", "on"),
                              ("-memory", "2048"), ("-port", "5558")):
            self.assertEqual(command[command.index(option) + 1], value)
        for option in ("-no-window", "-no-audio", "-no-snapstorage", "-no-snapshot"):
            self.assertIn(option, command)
        for option in ("-wipe-data", "host", "auto"):
            self.assertNotIn(option, command)

    def test_private_devices_network_and_only_owned_writable_binds(self):
        with patch.object(gate.os, "readlink", return_value="net:[host]"):
            command = gate.isolated_command(Path("/project"), Path("/state"), Path("/proof"),
                                            Path("/sdk"), Path("/user"), False)
        self.assertEqual(command[:4], ["bwrap", "--ro-bind", "/", "/"])
        self.assertEqual(command[command.index("--dev-bind") + 1:command.index("--dev-bind") + 3],
                         ["/dev/kvm", "/dev/kvm"])
        binds = [command[i + 1:i + 3] for i, value in enumerate(command) if value == "--bind"]
        self.assertEqual(binds, [["/state", "/state"], ["/proof", "/proof"],
                                 ["/state/android", "/user/.android"]])
        for option in ("--unshare-net", "--unshare-pid", "--unshare-ipc", "--die-with-parent"):
            self.assertIn(option, command)
        self.assertNotIn("--boot", command)
        self.assertNotIn("/dev/dri", command)

    def test_environment_does_not_inherit_driver_or_host_adb_overrides(self):
        with patch.dict(os.environ, {"HOME": "/user", "DISPLAY": ":0", "LD_PRELOAD": "bad.so",
                                    "ADB_SERVER_SOCKET": "tcp:localhost:5037",
                                    "VK_ICD_FILENAMES": "host-driver"}, clear=True):
            environment = gate.isolated_environment(Path("/sdk"), Path("/state"))
        self.assertEqual(environment["HOME"], "/user")
        self.assertEqual(environment["ADB_SERVER_SOCKET"], "tcp:localhost:5038")
        self.assertEqual(environment["ANDROID_SERIAL"], "emulator-5558")
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "LD_PRELOAD", "VK_ICD_FILENAMES"):
            self.assertNotIn(key, environment)

    def test_guest_proof_requires_actual_gles_line_not_unrelated_swiftshader_text(self):
        valid = "GLES: Google Inc., Google SwiftShader, OpenGL ES 3.0\n"
        self.assertEqual(gate.validate_guest_renderer(valid), valid.strip())
        for text in ("", "SwiftShader is available\nGLES: AMD, Radeon, GLES 3.2",
                     "GLES: Google SwiftShader Radeon", valid + valid):
            with self.subTest(text=text), self.assertRaises(RuntimeError):
                gate.validate_guest_renderer(text)

    def test_existing_user_configuration_is_not_overwritten(self):
        with tempfile.TemporaryDirectory(prefix="island-android-test-") as temporary:
            state = Path(temporary)
            (state / "OWNER").write_text(gate.OWNER)
            gate.prepare_avd(state)
            gate.prepare_avd(state)  # Reuse without wiping runtime data.
            config = state / "android/avd" / (gate.AVD + ".avd/config.ini")
            self.assertIn("hw.gpu.mode=swiftshader", config.read_text())
            reformatted = config.read_text().replace("=", " = ")
            config.write_text(reformatted)
            gate.prepare_avd(state)
            self.assertEqual(config.read_text(), reformatted)
            config.write_text("unexpected configuration")
            with self.assertRaisesRegex(RuntimeError, "configuration"):
                gate.prepare_avd(state)
            self.assertEqual(config.read_text(), "unexpected configuration")

    def test_unowned_state_is_refused(self):
        with tempfile.TemporaryDirectory(prefix="island-android-test-") as temporary:
            state = Path(temporary)
            (state / "OWNER").write_text("another project")
            with self.assertRaisesRegex(RuntimeError, "ownership"):
                gate.prepare_avd(state)
            self.assertFalse((state / "android").exists())

    def test_duplicate_properties_are_not_silently_accepted(self):
        for text in ("hw.gpu.mode=host\nhw.gpu.mode=swiftshader", "missing separator", "=value"):
            with self.subTest(text=text), self.assertRaises(RuntimeError):
                gate.parse_properties(text)

    def test_runtime_isolation_is_checked_before_any_emulator_launch(self):
        with patch.object(gate.os.path, "exists", return_value=False), \
                patch.object(gate.os, "readlink", return_value="net:[private]"), \
                patch.dict(os.environ, {}, clear=True), \
                patch("builtins.open", mock_open()) as device, \
                patch.object(gate.os, "sched_getaffinity", return_value={0, 1}):
            gate.require_private_devices("net:[host]")
            device.assert_called_once_with("/dev/kvm", "rb+", buffering=0)
            with patch.object(gate.os.path, "exists", return_value=True), \
                    self.assertRaisesRegex(RuntimeError, "GPU device"):
                gate.require_private_devices("net:[host]")
            with self.assertRaisesRegex(RuntimeError, "network namespace"):
                gate.require_private_devices("net:[private]")
            with patch.dict(os.environ, {"DISPLAY": ":0"}), \
                    self.assertRaisesRegex(RuntimeError, "display environment"):
                gate.require_private_devices("net:[host]")
            with patch.object(gate.os, "sched_getaffinity", return_value=set(range(9))), \
                    self.assertRaisesRegex(RuntimeError, "eight-core"):
                gate.require_private_devices("net:[host]")


if __name__ == "__main__":
    unittest.main()
