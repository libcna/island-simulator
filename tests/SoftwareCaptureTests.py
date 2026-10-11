#!/usr/bin/env python3
"""CPU-only preflight checks; never launches a compositor or graphics process."""
from pathlib import Path
from contextlib import ExitStack
import json
import signal
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from run_software_capture import (
    check_configuration, software_environment, validate_graphics_log, run_monitored_capture,
    inspection_arguments, validate_antialiasing_log, validate_monkey_capture_log,
)
from gpu_incident_qualification import MonitorError, contained_command


class SoftwareCaptureTests(unittest.TestCase):
    cache = "CNA_GRAPHICS_RENDERER:STRING=OPENGL33\nCNA_ROOT_DIR:PATH=/dependency\n"

    def test_monkey_close_subject_must_still_be_submitted_at_capture(self):
        valid = "island-simulator: monkey capture visible 2; near 1; draws 4; subject 55,19,17"
        validate_monkey_capture_log(valid)
        for invalid in ("", "monkeys 2/7 visible, 4 draws", valid.replace("near 1", "near 0"),
                        valid.replace("visible 2", "visible 0"), valid.replace("draws 4", "draws 0")):
            with self.subTest(invalid=invalid), self.assertRaises(RuntimeError):
                validate_monkey_capture_log(invalid)

    def test_missing_leaf_acceptance_is_narrow_and_opt_in(self):
        expected = ("island-simulator: leaf material fallback: Content/vegetation/leaf-lamina.png: "
                    "File not found; retaining procedural veins")
        validate_graphics_log(expected, missing_leaf=True)
        validate_graphics_log(expected + "\n" + expected, missing_leaf=True)
        with self.assertRaises(RuntimeError): validate_graphics_log(expected)
        for bad in ("", expected.replace("leaf-lamina.png", "other.png"),
                    expected.replace("retaining procedural veins", "ignored"),
                    expected + "\nleaf material loaded Content/vegetation/leaf-lamina.png",
                    expected + "\nforest-floor material fallback: missing",
                    expected + "\ngrass stock-effect fallback: shader failed",
                    expected.replace("File not found", "gl-error GL_INVALID_OPERATION")):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_graphics_log(bad, missing_leaf=True)

    def test_directory_fixture_hides_only_child_view_and_preserves_host(self):
        with tempfile.TemporaryDirectory(prefix="island-leaf-mask-") as directory:
            root = Path(directory)
            empty, target = root / "empty", root / "material"
            empty.mkdir()
            target.mkdir()
            asset = target / "leaf.png"
            asset.write_bytes(b"original host asset")
            command = contained_command([sys.executable, "-c",
                "import pathlib,sys; p=pathlib.Path(sys.argv[1]); "
                "assert not p.exists(); assert not pathlib.Path('/dev/dri').exists()", str(asset)],
                private_devices=True, read_only_directories=[(empty,target)])
            result = subprocess.run(command, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(asset.read_bytes(), b"original host asset")
            self.assertEqual(list(empty.iterdir()), [])

    def test_fixture_cannot_expose_devices_or_be_used_with_host_gpu_access(self):
        with tempfile.TemporaryDirectory(prefix="island-leaf-mask-") as directory:
            root = Path(directory)
            with self.assertRaises(MonitorError):
                contained_command(["fixture"], read_only_directories=[(root,root)])
            for source,target in ((Path("relative"),root), (root,Path("/")),
                                  (Path("/dev"),root), (root,Path("/proc")),
                                  (root / "absent",root)):
                with self.subTest(source=source,target=target), self.assertRaises(MonitorError):
                    contained_command(["fixture"], private_devices=True,
                                      read_only_directories=[(source,target)])

    def test_antialiasing_qualification_requires_applied_samples_not_just_request(self):
        for expected in (2, 4):
            marker = f"island-simulator: edge antialiasing requested {expected}x renderer-applied {expected}x"
            validate_antialiasing_log(marker, expected)
            validate_antialiasing_log(marker + "\n" + marker, expected)
            for bad in ("", f"edge antialiasing requested {expected}x", marker.replace(
                    f"renderer-applied {expected}x", "renderer-applied 0x"),
                    marker + "\nedge antialiasing requested 8x renderer-applied 8x"):
                with self.subTest(expected=expected, bad=bad), self.assertRaises(RuntimeError):
                    validate_antialiasing_log(bad, expected)

    def test_native_gl_only(self):
        self.assertEqual(check_configuration(self.cache), Path("/dependency"))

    def test_other_default_refused(self):
        with self.assertRaises(ValueError):
            check_configuration(self.cache.replace("OPENGL33", "VULKAN"))

    def test_missing_dependency_refused_with_actionable_error(self):
        for cache in ("CNA_GRAPHICS_RENDERER:STRING=OPENGL33\n",
                      "CNA_GRAPHICS_RENDERER:STRING=OPENGL33\nCNA_ROOT_DIR:PATH=\n"):
            with self.subTest(cache=cache), self.assertRaisesRegex(ValueError, "CNA_ROOT_DIR"):
                check_configuration(cache)

    def test_multi_renderer_refused(self):
        with self.assertRaises(ValueError):
            check_configuration(self.cache + "CNA_GRAPHICS_RENDERERS:STRING=OPENGL33;VULKAN\n")

    def test_boolean_extra_renderer_refused(self):
        with self.assertRaises(ValueError):
            check_configuration(self.cache + "CNA_RENDERER_VULKAN:BOOL=ON\n")

    def test_forced_software_and_worker_cap(self):
        environment = software_environment(Path("/evidence"))
        self.assertEqual(environment["LIBGL_ALWAYS_SOFTWARE"], "1")
        self.assertEqual(environment["GALLIUM_DRIVER"], "llvmpipe")
        self.assertEqual(environment["LP_NUM_THREADS"], "8")
        self.assertEqual(environment["SDL_VIDEO_DRIVER"], "x11")

    def test_material_and_stock_fallbacks_refused(self):
        for message in ("terrain material fallback: compile failed",
                        "shipwreck material fallback: invalid shader",
                        "grass stock-effect fallback: unavailable",
                        "gl-error GL_INVALID_OPERATION"):
            with self.subTest(message=message), self.assertRaises(RuntimeError):
                validate_graphics_log(message)

    def test_successful_material_diagnostic_accepted(self):
        validate_graphics_log("terrain close soil uses footprint-filtered clods and analytic relief")

    def test_diagnostic_only_rendering_refused(self):
        with self.assertRaisesRegex(RuntimeError, "cannot qualify"):
            validate_graphics_log("TEMPORARY SOFTWARE DIAGNOSTIC flat ocean normals")

    def test_inspection_arguments(self):
        self.assertEqual(inspection_arguments(None), [])
        self.assertEqual(inspection_arguments(["-87120", "-15369", "near"]),
                         ["--canopy-lod-inspection", "-87120", "-15369", "near"])
        for values in (["0", "0", "other"], ["1.5", "0", "near"], ["500001", "0", "near"],
                       ["0", "near"], ["x", "0", "far"]):
            with self.subTest(values=values), self.assertRaises(ValueError):
                inspection_arguments(values)

    def test_inspection_requires_exact_marker_and_rejects_other_diagnostics(self):
        values = ["-87120", "-15369", "near"]
        marker = ("TEMPORARY SOFTWARE DIAGNOSTIC canopy-lod target=-87120:-15369 "
                  "tier=near frozen-simulation=0 shader-time=1")
        validate_graphics_log(marker, values)
        for diagnostic in ("", marker.replace("tier=near", "tier=far"),
                           marker + "\nTEMPORARY SOFTWARE DIAGNOSTIC flat ocean normals",
                           marker + "\ngl-error GL_INVALID_OPERATION"):
            with self.subTest(diagnostic=diagnostic), self.assertRaises(RuntimeError):
                validate_graphics_log(diagnostic, values)


class CaptureMonitorTests(unittest.TestCase):
    """All child processes and journal reads are fake; no graphics or kernel fault injection."""

    baseline = {"__CURSOR": "start", "_BOOT_ID": "boot", "MESSAGE": "baseline"}

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="island-software-monitor-")
        self.addCleanup(self.directory.cleanup)
        self.evidence = Path(self.directory.name)
        self.journal = Mock()
        self.journal.latest.return_value = self.baseline
        self.journal.read.return_value = []
        self.process = Mock()
        self.process.poll.return_value = 0
        patches = ExitStack()  # TestCase.enterContext requires Python 3.11; retain 3.10 support.
        self.addCleanup(patches.close)
        self.spawn = patches.enter_context(patch("run_software_capture.subprocess.Popen",
                                                return_value=self.process))
        self.stop = patches.enter_context(patch("run_software_capture.stop_process_group"))
        self.sleep = patches.enter_context(patch("run_software_capture.time.sleep"))
        self.handlers = {number: signal.getsignal(number)
                         for number in (signal.SIGINT, signal.SIGTERM)}

    def tearDown(self):
        for number, handler in self.handlers.items():
            self.assertEqual(signal.getsignal(number), handler)

    def run_capture(self, **kwargs):
        return run_monitored_capture(["fake-game"], self.evidence, {}, self.evidence,
                                     self.journal, self.baseline, **kwargs)

    def record(self, message="ordinary kernel message", **fields):
        return {**self.baseline, "__CURSOR": "next", "MESSAGE": message, **fields}

    def test_success_checks_before_during_and_after_owned_cleanup(self):
        self.journal.read.side_effect = [[], [self.record()], []]
        self.assertEqual(self.run_capture(), 0)
        self.assertEqual(self.journal.latest.call_count, 3)
        self.assertEqual(self.journal.read.call_args_list[-1].args, ("--after-cursor", "next"))
        launch = self.spawn.call_args.args[0]
        self.assertIn("--unshare-pid", launch)
        self.assertIn("--die-with-parent", launch)
        self.assertIn("--dev", launch)
        self.assertNotIn("--dev-bind", launch)
        self.assertEqual(launch[-2:], ["--", "fake-game"])
        attribution = json.loads((self.evidence / "process-containment.json").read_text())
        self.assertEqual(attribution["command"], launch)
        self.assertTrue(attribution["private_devices"])
        self.stop.assert_called_once_with(self.process)
        self.assertTrue(self.spawn.call_args.kwargs["start_new_session"])
        live = (self.evidence / "kernel-live.jsonl").read_text().splitlines()
        self.assertEqual([json.loads(line) for line in live], [self.record()])

    def test_fault_before_launch_refuses_process(self):
        self.journal.read.return_value = [self.record("amdgpu: [gfxhub] page fault")]
        with self.assertRaisesRegex(MonitorError, "GPU fault"):
            self.run_capture()
        self.spawn.assert_not_called()
        self.stop.assert_not_called()

    def test_explicit_outer_device_bind_for_nested_android_sandbox(self):
        self.assertEqual(self.run_capture(private_devices=False), 0)
        launch = self.spawn.call_args.args[0]
        self.assertIn("--dev-bind", launch)
        self.assertNotIn("--dev", launch)
        attribution = json.loads((self.evidence / "process-containment.json").read_text())
        self.assertFalse(attribution["private_devices"])

    def test_first_live_fault_stops_before_waiting_for_process_exit(self):
        self.journal.read.side_effect = [[], [self.record("amdgpu: [gfxhub] page fault")], []]
        with self.assertRaisesRegex(MonitorError, "GPU fault"):
            self.run_capture()
        self.process.poll.assert_not_called()
        self.sleep.assert_not_called()
        self.stop.assert_called_once_with(self.process)
        self.assertIn("page fault", (self.evidence / "kernel-live.jsonl").read_text())

    def test_fault_during_cleanup_is_not_success(self):
        self.journal.read.side_effect = [[], [], [self.record("amdgpu: ring gfx_0.0.0 timeout")]]
        with self.assertRaisesRegex(MonitorError, "GPU fault"):
            self.run_capture()
        self.stop.assert_called_once_with(self.process)

    def test_journal_failure_after_launch_stops_child(self):
        self.journal.latest.side_effect = [self.baseline, MonitorError("journal access lost")]
        with self.assertRaisesRegex(MonitorError, "journal access lost"):
            self.run_capture()
        self.stop.assert_called_once_with(self.process)

    def test_boot_discontinuity_stops_child_even_with_empty_delta(self):
        self.journal.latest.side_effect = [self.baseline, self.record(_BOOT_ID="new-boot")]
        with self.assertRaisesRegex(MonitorError, "boot discontinuity"):
            self.run_capture()
        self.stop.assert_called_once_with(self.process)

    def test_boot_discontinuity_in_delta_is_preserved(self):
        self.journal.read.side_effect = [[], [self.record(_BOOT_ID="new-boot")]]
        with self.assertRaisesRegex(MonitorError, "boot discontinuity"):
            self.run_capture()
        self.assertIn("new-boot", (self.evidence / "kernel-live.jsonl").read_text())

    def test_timeout_stops_process(self):
        self.process.poll.return_value = None
        with self.assertRaisesRegex(MonitorError, "time limit"):
            self.run_capture(timeout=0.0)
        self.stop.assert_called_once_with(self.process)

    def test_signal_stops_process_and_restores_handlers(self):
        self.process.poll.return_value = None
        self.sleep.side_effect = lambda _: signal.getsignal(signal.SIGTERM)(signal.SIGTERM, None)
        with self.assertRaisesRegex(MonitorError, "interrupted by signal"):
            self.run_capture()
        self.stop.assert_called_once_with(self.process)

    def test_cleanup_failure_does_not_replace_primary_fault(self):
        self.journal.read.side_effect = [[], [self.record("GPU reset begin!")]]
        self.stop.side_effect = MonitorError("owned child did not terminate")
        with patch("run_software_capture.sys.stderr"), self.assertRaisesRegex(MonitorError, "GPU fault"):
            self.run_capture()

    def test_launch_failure_restores_handlers(self):
        self.spawn.side_effect = OSError("cannot launch")
        with self.assertRaisesRegex(OSError, "cannot launch"):
            self.run_capture()
        self.stop.assert_not_called()

    def test_nonzero_exit_is_returned_after_kernel_check(self):
        self.process.poll.return_value = 7
        self.assertEqual(self.run_capture(), 7)
        self.assertEqual(self.journal.read.call_count, 3)
        self.stop.assert_called_once_with(self.process)


if __name__ == "__main__":
    unittest.main()
