#!/usr/bin/env python3
"""CPU-only preflight checks; never launches a compositor or graphics process."""
from pathlib import Path
from contextlib import ExitStack
import json
import re
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
    validate_monkey_tree_stages, validate_monkey_climb_stages, validate_snake_motion_stages,
    validate_goat_motion_stages,
    validate_boar_motion_stages,
    validate_spider_motion_stages,
    validate_spider_render_log,
    validate_spider_web_render_log,
    validate_spider_lighting,
    validate_spider_habitat_selection,
    validate_campfire_stages,
    validate_stream_stages,
    validate_resident_stages,
)
from gpu_incident_qualification import MonitorError, contained_command
from run_fullscreen_software import (geometry, worker as fullscreen_worker,
                                     validate_boar_audio_trace, validate_goat_audio_trace,
                                     validate_snake_audio_trace)
from run_fullscreen_software import validate_control_transitions, owned_runtime_log
from run_fullscreen_software import validate_diving_trace, validate_frame_windows
from run_fullscreen_software import validate_cave_trace
from run_fullscreen_software import validate_fire_audio_trace
from run_fullscreen_software import validate_stream_crossing_trace
from run_software_capture import validate_build_configuration


class SoftwareCaptureTests(unittest.TestCase):
    def test_stream_crossing_requires_both_banks_channel_and_shared_support(self):
        good=[(-110,5.0,-34+i*.55,-4+i*.55,abs(-4+i*.55),0,0) for i in range(14)]
        result=validate_stream_crossing_trace(good)
        self.assertGreater(result["nearest"],0)
        for index,replacement in ((0,(-110,5,-1,0,0,0,0)),
                                  (7,(-110,5,-30,.1,.1,.2,0)),
                                  (7,(-110,5,-30,.1,.1,0,1))):
            bad=list(good);bad[index]=replacement
            with self.assertRaises(RuntimeError):validate_stream_crossing_trace(bad)
        with self.assertRaises(RuntimeError):validate_stream_crossing_trace(good[:4])
        reversed_route=list(good);reversed_route[8]=(-110,5,reversed_route[7][2]-.2,
            reversed_route[8][3],reversed_route[8][4],0,0)
        with self.assertRaises(RuntimeError):validate_stream_crossing_trace(reversed_route)

    def test_fire_audio_trace_requires_volume_collision_mute_resume_and_range(self):
        good=[(1,0,1,.26,2.4,.04,4)]*5+[(1,1,1,0,2.4,.04,4)]*25
        good+=[(1,0,1,.26,2.4,.04,4)]*5+[(1,0,0,.001,27,25,4)]
        validate_fire_audio_trace(good)
        for index,point in ((0,(0,0,1,.26,2.4,.04,4)),(7,(1,1,1,.2,2.4,.04,4)),
                            (9,(1,1,0,0,2.4,.04,4)),(35,(1,0,1,.2,27,25,4)),
                            (35,(1,0,0,.001,2.4,.04,4)),(0,(1,0,1,.26,2.4,-.1,4)),
                            (0,(1,0,1,.26,2.4,.04,7)),(2,(1,0,1,float("nan"),2.4,.04,4))):
            bad=list(good);bad[index]=point
            with self.assertRaises(RuntimeError):validate_fire_audio_trace(bad)
        for bad in ([],good[:5],[(1,0,1,0,2.4,.04,4)]*5+good[5:],good[:30]+[good[-1]]):
            with self.assertRaises(RuntimeError):validate_fire_audio_trace(bad)
    def test_campfire_animation_stages(self):
        good="\n".join(f"campfire motion stage={i} updates={(i+1)*8} time={(i+1)*.8:.1f} draws=2 effect-triangles=42" for i in range(4))
        self.assertEqual(len(validate_campfire_stages(good)),4)
        for bad in ("",good+"\n"+good,good.replace("stage=1","stage=0"),good.replace("time=0.8","time=nan"),
                    good.replace("updates=8 ","updates=9 "),good.replace("draws=2","draws=0"),
                    good.replace("triangles=42","triangles=420")):
            with self.assertRaises(RuntimeError):validate_campfire_stages(bad)
    def test_stream_animation_stages(self):
        good="\n".join(f"stream motion stage={i} frames={(i+1)*8} time={(i+1)*.8:.1f} draws=1 triangles=6080" for i in range(4))
        self.assertEqual(len(validate_stream_stages(good)),4)
        for bad in ("",good+"\n"+good,good.replace("stage=1","stage=0"),
                    good.replace("time=0.8","time=nan"),good.replace("frames=8 ","frames=9 "),
                    good.replace("draws=1","draws=0"),good.replace("triangles=6080","triangles=6081")):
            with self.assertRaises(RuntimeError):validate_stream_stages(bad)
    def test_resident_gait_stages_and_corruptions(self):
        rows=[]
        for stage in range(8):
            distance=max(0,stage-2)*.35
            speed=0 if stage<3 or stage==7 else .78
            swing=-.24 if stage%2==0 else .24
            rows.append(f"resident motion stage={stage} updates={(stage+1)*8} visible=1 "
                        f"position={42-distance},23.8,{-12+distance} speed={speed} mode={1 if stage<3 else 0} "
                        f"feet={41.9-distance},23.8,{-12+distance},1;{42.1-distance},23.8,{-11.9+distance},0 "
                        f"wrists={41.8-distance+.12*(-1)**stage},24.3,{-12+distance};"
                        f"{42.2-distance-.12*(-1)**stage},24.3,{-12+distance} "
                        f"arm-swing={swing},{-swing} "
                        "triangles=3638/24")
        good="\n".join(rows)
        result=validate_resident_stages(good)
        self.assertGreater(result["displacement"],1)
        self.assertGreater(min(result["arm_swing_ranges"]),.15)
        for bad in ("",good+"\n"+good,good.replace("stage=1","stage=0"),
                    good.replace("updates=8 ","updates=9 "),good.replace("visible=1","visible=0"),
                    good.replace("triangles=3638/24","triangles=10/24"),
                    good.replace("triangles=3638/24","triangles=3638/0"),
                    good.replace("wrists=", "wrists=nan,", 1),
                    good.replace("arm-swing=-0.24,0.24", "arm-swing=nan,0.24", 1),
                    good.replace("position=42.0,23.8,-12.0","position=nan,23.8,-12.0")):
            with self.assertRaises(RuntimeError):validate_resident_stages(bad)
    def test_cave_traversal_and_corruptions(self):
        points=[(0,0,-8,0,0),(0,.49,19.65,0,0),(0,1.7,19.65,1,0),
                (0,.50,19.65,0,0),(0,0,-6,0,0)]
        samples=[p for p in points for _ in range(5)]
        stages=dict(zip(("start","back","ceiling","landed","exit"),(4,9,14,19,24)))
        validate_cave_trace(samples,stages)
        for index,point in ((0,(0,0,-8,0,1)),(4,(0,0,0,0,0)),(9,(0,.5,21,0,0)),
                            (14,(0,7,19.65,1,0)),(14,(0,1.7,19.65,0,0)),
                            (19,(0,1.7,19.65,0,0)),(24,(0,0,4,0,0)),
                            (2,(float("nan"),0,0,0,0)),(3,(2,0,0,0,0))):
            bad=list(samples);bad[index]=point
            with self.subTest(index=index,point=point),self.assertRaises(RuntimeError):
                validate_cave_trace(bad,stages)
        for bad_stages in ({},dict(stages,back=3),dict(stages,exit=25),dict(stages,start=-1)):
            with self.assertRaises(RuntimeError):validate_cave_trace(samples,bad_stages)
        with self.assertRaises(RuntimeError):validate_cave_trace(samples[:5],stages)

    def test_runtime_build_configuration_matches_tree(self):
        for mode in ("Debug", "Release", "RelWithDebInfo", ""):
            cache = f"CMAKE_BUILD_TYPE:STRING={mode}\n"
            log = f"island-simulator: build configuration {mode}\n"
            self.assertEqual(validate_build_configuration(cache, log), mode)
            for bad in ("", log + log, log.replace(mode or "configuration", "unexpected")):
                with self.subTest(mode=mode, bad=bad), self.assertRaises(RuntimeError):
                    validate_build_configuration(cache, bad)
        with self.assertRaises(RuntimeError):
            validate_build_configuration("", "island-simulator: build configuration Release\n")

    def test_live_frame_accounting_and_corruptions(self):
        good = ("island-simulator: frame-window seconds=5.000 frames=150 fps=30.000 "
                "updates-per-frame=2.000 update-ms-per-frame=8.000 draw-submit-ms-per-frame=4.000 "
                "wildlife-ms-per-update(spider,goat,snake,monkey)=1.000,0.500,1.000,0.500 "
                "flight=1 camera=1,60,3")
        self.assertEqual(validate_frame_windows(good)[0]["fps"], 30)
        for bad in ("", good.replace("fps=30.000", "fps=60.000"),
                    good.replace("seconds=5.000", "seconds=nan"),
                    good.replace("frames=150", "frames=0"),
                    good.replace("updates-per-frame=2.000", "updates-per-frame=0"),
                    good.replace("1.000,0.500,1.000,0.500", "4,4,4,4"),
                    good.replace("draw-submit-ms-per-frame=4.000", "draw-submit-ms-per-frame=-1"),
                    good.replace("flight=1", "flight=2"), good.replace("camera=1,60,3", "camera=1,inf,3")):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_frame_windows(bad)

    def test_diving_trace_and_rejections(self):
        good=[(0,.58,-270,0),(0,-.5,-270,.5),(0,-1,-270,1),(0,.58,-270,0)]
        validate_diving_trace(good)
        for bad in ([],good[:3],good[1:],[(0,.58,-270,0)]*4,
                    [good[0],(0,-1,-270,0),*good[2:]],
                    [good[0],(0,float('nan'),-270,0),*good[2:]],
                    [good[0],(0,5,-270,0),*good[2:]],
                    [good[0],(0,-1,-270),*good[2:]]):
            with self.subTest(bad=bad),self.assertRaises(RuntimeError):
                validate_diving_trace(bad)

    def test_boar_capture_clock_and_corruptions(self):
        good = ''.join(f"island-simulator: boar motion stage={i} updates={(i+1)*8} time={(i+1)*.8:.1f} "
                       "visible=1 position=1,2,3 speed=0 phase=0 rooting=0.5\n" for i in range(8))
        self.assertEqual(len(validate_boar_motion_stages(good)["samples"]), 8)
        for bad in ("", good+good, good.replace("stage=7", "stage=6"),
                    good.replace("updates=8 ", "updates=9 "), good.replace("time=0.8", "time=nan"),
                    good.replace("visible=1", "visible=0"), good.replace("1,2,3", "1,inf,3"),
                    good.replace("speed=0", "speed=-1"), good.replace("phase=0", "phase=7"),
                    good.replace("rooting=0.5", "rooting=2")):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_boar_motion_stages(bad)

    def test_spider_bright_camera_selects_existing_habitat(self):
        rows = ''.join(f"spider habitat candidate={i} transmission={0.9 if i in (4, 7) else 0.1}\n"
                       for i in range(200))
        good = rows + "spider habitat selected=4\n"
        self.assertEqual(validate_spider_habitat_selection(good)["subject"], 4)
        for bad in ("", good.replace("selected=4", "selected=7"),
                    good.replace("selected=4", "selected=200"), good+"spider habitat selected=4\n",
                    good.replace("candidate=199", "candidate=198"),
                    good.replace("transmission=0.1", "transmission=nan", 1),
                    good.replace("transmission=0.1", "transmission=1.1", 1),
                    good.replace("transmission=0.1", "transmission=bad", 1)):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_spider_habitat_selection(bad)

    def test_spider_lighting_transfer_and_rejections(self):
        for transmission in (0, 0.5, 1):
            tint = ','.join(str(a + (1-a)*transmission) for a in (0.46, 0.53, 0.62))
            good = (f"spider lighting ground-transmission={transmission} "
                    f"body-transmission={transmission} body-tint={tint}\n")
            self.assertEqual(validate_spider_lighting(good)["bodyTransmission"], transmission)
        good = "spider lighting ground-transmission=0 body-transmission=0 body-tint=0.46,0.53,0.62\n"
        for bad in ("", good+good, good.replace("ground-transmission=0", "ground-transmission=nan"),
                    good.replace("body-transmission=0", "body-transmission=1.1"),
                    good.replace("0.46,0.53,0.62", "0.46,0.53"),
                    good.replace("0.46,0.53,0.62", "0.46,inf,0.62"),
                    good.replace("0.46,0.53,0.62", "0.46,0.53,0.32"),
                    good.replace("ground-transmission=0", "ground-transmission=bad")):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_spider_lighting(bad)

    def test_spider_render_requires_grounding(self):
        good = ("island-simulator: spider render proof: spiders 1/200 visible, 2 draw; contact-shadows=1\n"
                "spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes\n"
                "near spiders use tiered fitted eyes with upper-forward catchlights\n"
                "near spider abdomens use a tapered muted dorsal lance in the existing setae draw\n")
        validate_spider_render_log(good)
        for bad in ("", good.replace("1/200", "0/200"), good.replace("1/200", "201/200"),
                    good.replace("2 draw", "0 draw"), good.replace("contact-shadows=1", "contact-shadows=0"),
                    good.replace("contact-shadows=1", "contact-shadows=201"),
                    good.replace(" contact-shadows=1", ""), good.replace("spider setae loaded", "missing"),
                    good.replace("tiered fitted eyes", "uniform eyes"),
                    good.replace("tapered muted dorsal lance", "plain abdomen")):
            with self.assertRaises(RuntimeError):
                validate_spider_render_log(bad)

    def test_spider_web_render_requires_bounded_live_batch(self):
        good="island-simulator: spider-web render proof: webs 1/6 visible, strands 434, 1 draw\n"
        self.assertEqual(validate_spider_web_render_log(good),
                         {"visible":1,"total":6,"strands":434,"draws":1})
        self.assertEqual(validate_spider_web_render_log(good+good)["strands"],434)
        for bad in ("",good+good.replace("434","868"),good.replace("1/6","0/6"),good.replace("1/6","7/6"),
                    good.replace("434","433"),good.replace("1 draw","2 draw")):
            with self.subTest(bad=bad),self.assertRaises(RuntimeError):
                validate_spider_web_render_log(bad)

    def test_spider_live_motion_and_corruptions(self):
        def fixture(frozen=False):
            rows = []
            for i in range(8):
                x = 0 if frozen else i*.025
                feet = ';'.join(f"{x+(.08 if leg<4 else -.08)},2,{(leg%4-1.5)*.05},1" for leg in range(8))+';'
                rows.append(f"island-simulator: spider motion stage={i} updates={(i+1)*8} time={(i+1)*.8:.1f} "
                            f"visible=1 position={x},2,0 heading={i*.2:.1f} speed=0.035 feet={feet}")
            return '\n'.join(rows)
        good = fixture()
        self.assertEqual(len(validate_spider_motion_stages(good)["feet"]), 8)
        for bad in (fixture(True), good.split('\n', 1)[1], good+'\n'+good.split('\n')[0],
                    good.replace("stage=1", "stage=0"), good.replace("updates=8 ", "updates=9 "),
                    good.replace("time=0.8", "time=nan"), good.replace("visible=1", "visible=0"),
                    good.replace("speed=0.035", "speed=0.1"), good.replace("heading=0.2", "heading=2.0"),
                    good.replace("heading=0.2", "heading=nan"),
                    re.sub(r"position=\S+", "position=nan,2,0", good),
                    good.replace(",1;", ",0;"), good.replace(",1;", ",2;"),
                    re.sub(r"feet=.*", "feet=0,2,0,1;", good)):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_spider_motion_stages(bad)

    def test_goat_live_motion_stages_and_corruptions(self):
        rows = [f"island-simulator: goat motion stage={i} updates={(i+1)*8} time={(i+1)*.8:.1f} "
                f"visible=3 position={i*.2},10,20 speed=0.65 phase={i*.3} grazing={.8 if i==0 else 0}"
                for i in range(8)]
        good = "\n".join(rows)
        self.assertEqual(len(validate_goat_motion_stages(good)["positions"]), 8)
        for bad in ("\n".join(rows[:-1]), good+"\n"+rows[0], good.replace("stage=1", "stage=0"),
                    good.replace("updates=8 ", "updates=9 "), good.replace("visible=3", "visible=0"),
                    good.replace("speed=0.65", "speed=nan"), good.replace("phase=0.3", "phase=inf"),
                    good.replace("phase=0.3", "phase=9"), good.replace("position=0.2,10,20", "position=50,10,20"),
                    good.replace("grazing=0.8", "grazing=0"),
                    re.sub(r"position=\S+", "position=0,10,20", good),
                    re.sub(r"phase=\S+", "phase=0", good)):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                validate_goat_motion_stages(bad)
    def test_runtime_log_uses_namespace_identity_not_filename_pid(self):
        evidence = Mock()
        matching, unrelated = Mock(), Mock()
        matching.read_text.return_value = "timestamp runtime-start pid=504958 main-tid=504958 namespace-pid=44 namespace-main-tid=44"
        unrelated.read_text.return_value = "timestamp runtime-start pid=504959 main-tid=504959 namespace-pid=441 namespace-main-tid=441"
        evidence.glob.return_value = [unrelated, matching]
        self.assertIs(owned_runtime_log(evidence, 44)[0], matching)
        for logs in ([], [unrelated], [matching, matching]):
            evidence.glob.return_value = logs
            with self.assertRaises(RuntimeError):
                owned_runtime_log(evidence, 44)

    def test_persistent_control_transitions_require_complete_finite_ordered_states(self):
        lines = [f"timestamp island-simulator: control-transition event={event} submitted-frames={i+1} "
                 f"camera=1,2,3 yaw=0 pitch=0 flight-active={int(event == 0)} "
                 f"flight-position=1,2,3 fullscreen-requested={int(event == 3)} finite=1"
                 for i, event in enumerate([3, 4, 3, 4, 0, 1])]
        valid = "\n".join(lines)
        self.assertEqual(len(validate_control_transitions(valid, True)), 6)
        self.assertEqual(len(validate_control_transitions("\n".join(lines[:4]))), 4)
        self.assertEqual(len(validate_control_transitions("\n".join(lines[:2]), False, 1)), 2)
        self.assertEqual(validate_control_transitions("", False, 0), [])
        with self.assertRaises(RuntimeError):
            validate_control_transitions("", False, 3)
        for invalid in ("", "\n".join(lines[:-1]), valid + "\n" + lines[-1],
                        "\n".join(reversed(lines)), valid.replace("camera=1,2,3", "camera=nan,2,3"),
                        valid.replace("camera=1,2,3", "camera=1,2"), valid.replace("finite=1", "finite=0"),
                        valid.replace("flight-active=1", "flight-active=0"),
                        valid.replace("submitted-frames=6", "submitted-frames=1"),
                        valid.replace("flight-position=1,2,3", "flight-position=2,2,3")):
            with self.assertRaises(RuntimeError):
                validate_control_transitions(invalid, True)

    def test_snake_motion_requires_connected_live_sequence(self):
        def fixture(moving=True):
            return '\n'.join(
                f"island-simulator: snake motion stage={stage} updates={(stage+1)*8} time={(stage+1)*.8} visible=1 speed=0.38 phase=1 body="
                + ''.join(f"{(stage*.1 if moving else 0)-j*.055:.6f},2,0;" for j in range(41))
                for stage in range(4))
        good = fixture()
        self.assertEqual(len(validate_snake_motion_stages(good)), 4)
        for invalid in (fixture(False), good.replace('stage=2', 'stage=1'),
                        good.replace('updates=16', 'updates=17'), good.replace('time=0.8', 'time=nan'),
                        good.replace('time=0.8', 'time=0.9'),
                        good+'\nisland-simulator: snake motion stage=broken',
                        good.replace('visible=1', 'visible=0'),
                        good.replace('speed=0.38', 'speed=nan'),
                        good.replace('0.000000,2,0;', 'nan,2,0;'),
                        good.replace('0.000000,2,0;', '10,2,0;'),
                        good.split('\n', 1)[1]):
            with self.assertRaises(RuntimeError):
                validate_snake_motion_stages(invalid)

    def test_boar_audio_trace_rejects_missing_playback_mute_and_budget_failures(self):
        valid = [(1, 0, 2, 5, 3)] * 5 + [(1, 1, 2, 4, 0)] * 24 + [(1, 0, 3, 5, 3)] * 5
        validate_boar_audio_trace(valid)
        for index, state in ((0, (0, 0, 2, 5, 3)), (5, (1, 1, 3, 4, 0)),
                             (5, (1, 1, 2, 5, 3)), (0, (1, 0, 2, 7, 3)),
                             (-1, (1, 0, 2, 4, 0))):
            invalid = list(valid)
            invalid[index] = state
            with self.assertRaises(RuntimeError):
                validate_boar_audio_trace(invalid)
        for invalid in ([], valid[:20], [(1, 0, 3, 4, 0)] * 40):
            with self.assertRaises(RuntimeError):
                validate_boar_audio_trace(invalid)

    def test_snake_audio_trace_rejects_missing_playback_mute_and_budget_failures(self):
        valid=[(1,0,0,5,4,1)]*4+[(1,1,0,4,0,1)]*24+[(1,0,0,5,4,2)]*4
        validate_snake_audio_trace(valid)
        for index,state in ((0,(0,0,0,5,4,1)),(4,(1,1,0,4,0,2)),
                            (4,(1,1,0,5,4,1)),(0,(1,0,0,7,4,1)),
                            (-1,(1,0,0,4,0,1))):
            invalid=list(valid);invalid[index]=state
            with self.assertRaises(RuntimeError):validate_snake_audio_trace(invalid)
        for invalid in ([],valid[:20],[(1,0,0,4,0,2)]*40):
            with self.assertRaises(RuntimeError):validate_snake_audio_trace(invalid)

    def test_goat_audio_trace_rejects_missing_playback_mute_and_budget_failures(self):
        valid=[(1,0,0,5,6,0,2)]*4+[(1,1,0,4,0,0,2)]*24+[(1,0,0,5,6,0,3)]*4
        validate_goat_audio_trace(valid)
        for index,state in ((0,(0,0,0,5,6,0,2)),(4,(1,1,0,4,0,0,3)),
                            (4,(1,1,0,5,6,0,2)),(0,(1,0,0,7,6,0,2)),
                            (-1,(1,0,0,4,0,0,2))):
            invalid=list(valid);invalid[index]=state
            with self.assertRaises(RuntimeError):validate_goat_audio_trace(invalid)
        for invalid in ([],valid[:20],[(1,0,0,4,0,0,3)]*40):
            with self.assertRaises(RuntimeError):validate_goat_audio_trace(invalid)

    def test_fullscreen_geometry_rejects_missing_or_invalid_dimensions(self):
        self.assertEqual(geometry("X=-20\nY=10\nWIDTH=960\nHEIGHT=540\n")["WIDTH"], 960)
        for text in ("", "X=0\nY=0\nWIDTH=1\n", "X=0\nY=0\nWIDTH=0\nHEIGHT=540\n"):
            with self.assertRaises(RuntimeError):
                geometry(text)

    def test_fullscreen_worker_refuses_unsupervised_launch_before_process_creation(self):
        with patch.dict("os.environ", {}, clear=True), patch("subprocess.Popen") as launch:
            with self.assertRaisesRegex(RuntimeError, "supervised"):
                fullscreen_worker(Path("/build"), Path("/evidence"), Path("/wm"))
            launch.assert_not_called()

    cache = "CNA_GRAPHICS_RENDERER:STRING=OPENGL33\nCNA_ROOT_DIR:PATH=/dependency\n"

    def test_climb_requires_whole_cycle_same_tree_ascent_descent_and_release(self):
        times = [0, 5, 10.1, 18.5, 24, 29.1, 38.9, 0]
        heights = [5, 5.2, 5.3, 7.3, 6.4, 5.3, 5.01, 5]
        rows = [f"monkey climb stage={i} subject=2 tree={12 if i<7 else -1} "
                f"time={t} active={int(i<7)} centre-y={heights[i]}" for i,t in enumerate(times)]
        valid = "\n".join(rows)
        self.assertEqual(len(validate_monkey_climb_stages(valid)), 8)
        for invalid in ["\n".join(rows[:-1]), valid.replace("stage=2 subject=2", "stage=2 subject=3"),
                        valid.replace("time=18.5", "time=11.0"), valid.replace("centre-y=7.3", "centre-y=5.3"),
                        valid.replace("centre-y=5.01", "centre-y=6.01"),
                        valid.replace("stage=7 subject=2 tree=-1", "stage=7 subject=2 tree=12")]:
            with self.assertRaises(RuntimeError):
                validate_monkey_climb_stages(invalid)

    def test_tree_sequence_requires_same_subject_tree_clock_and_applied_contacts(self):
        def ease(t):
            t = max(0, min(1, t/.75))
            return t*t*(3-2*t)
        rows = []
        for index, t in enumerate([.1,1.4,2.2,3.4,4.9,5.5,5.9]):
            rows.append(f"monkey tree stage={index} subject=4 tree=20 elapsed={t} clock={100+t} "
                        f"weights={ease(t-1)*ease(6-t-.75)},{ease(t-1.75)*ease(6-t)}")
        rows.append("monkey tree stage=7 subject=4 tree=-1 elapsed=0 clock=106.1 weights=0,0")
        valid = "\n".join(rows)
        self.assertEqual(len(validate_monkey_tree_stages(valid)), 8)
        for invalid in ("\n".join(rows[:-1]), valid+"\n"+rows[0],
                        "\n".join([rows[1], rows[0], *rows[2:]]),
                        valid.replace("stage=1 subject=4", "stage=1 subject=2"),
                        valid.replace("stage=1 subject=4 tree=20", "stage=1 subject=4 tree=21"),
                        valid.replace("clock=103.4", "clock=101.0"),
                        valid.replace("weights=1,1", "weights=0,0"),
                        valid.replace("clock=106.1", "clock=105.9"),
                        valid.replace("elapsed=1.4", "elapsed=1.8"),
                        valid.replace("clock=101.4", "clock=1e999"),
                        valid.replace("weights=1,1", "weights=1.1,1"),
                        valid.replace("stage=7 subject=4 tree=-1", "stage=7 subject=4 tree=20")):
            with self.subTest(invalid=invalid), self.assertRaises(RuntimeError):
                validate_monkey_tree_stages(invalid)

    def test_monkey_close_subject_must_still_be_submitted_at_capture(self):
        valid = ("island-simulator: monkey capture visible 2; near 1; draws 4; subject 55,19,17\n"
                 "monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes\n"
                 "monkey fur uses one hidden underside/rear UV seam instead of mirrored crown and dorsal folds\n"
                 "near monkey tails keep a compact muscular hook and thirty-six raised fur clumps\n"
                 "near monkey faces use paired fur cheek ruffs around the skin mask\n"
                 "near monkey faces use fitted sculpted supraorbital ridges and warmer modelled skin\n"
                 "near monkey faces use a fitted dark nose pad and raised readable corneas\n"
                 "monkey face skin uses muted brown palettes and a recessed perimeter shade\n"
                 "near monkeys use an arched shoulder-rump trunk, broad blunt face, robust limbs, and compact tail\n"
                 "near monkeys use a lower tapered trunk, separated crown, and restrained furry tail root\n"
                 "near monkeys use a shallow waisted torso and shortened facial projection\n"
                 "monkey shoulder and hip mantles blend the trunk into all four animated limbs")
        validate_monkey_capture_log(valid)
        validate_monkey_capture_log(valid+"\nmonkey subject paired tree grips 1", True)
        for marker in ("", "monkey subject paired tree grips 0", "monkey subject paired tree grips 10"):
            with self.subTest(marker=marker), self.assertRaises(RuntimeError):
                validate_monkey_capture_log(valid+"\n"+marker, True)
        for invalid in ("", "monkeys 2/7 visible, 4 draws", valid.replace("near 1", "near 0"),
                        valid.replace("visible 2", "visible 0"), valid.replace("draws 4", "draws 0"),
                        valid.split("\n")[0], valid.replace("10 mips", "1 mips")):
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
