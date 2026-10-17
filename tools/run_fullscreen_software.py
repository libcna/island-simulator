#!/usr/bin/env python3
"""Native F11/optional Tab flight QA with private display and mandatory kernel supervision."""
from __future__ import annotations

import argparse
import datetime
import fcntl
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from gpu_incident_qualification import Journal, FAULT, artifact_identity
from run_software_capture import (check_configuration, run_monitored_capture,
                                  software_environment, validate_graphics_log)


def geometry(output):
    values = dict(re.findall(r"^(X|Y|WIDTH|HEIGHT)=(-?\d+)$", output, re.MULTILINE))
    if set(values) != {"X", "Y", "WIDTH", "HEIGHT"}:
        raise RuntimeError("incomplete native window geometry")
    result = {key: int(value) for key, value in values.items()}
    if result["WIDTH"] <= 0 or result["HEIGHT"] <= 0:
        raise RuntimeError("invalid native window size")
    return result


def validate_boar_audio_trace(samples):
    """Validate captured runtime states, not subjective sound identity or device quality."""
    if len(samples) < 30 or any(len(s) != 5 for s in samples):
        raise RuntimeError("missing boar audio trace")
    if any(s[0] != 1 or s[1] not in (0, 1) or not 4 <= s[3] <= 6
           or s[4] not in (0, 1, 2, 3) for s in samples):
        raise RuntimeError("unavailable audio or invalid voice budget/state")
    if any(b[2] < a[2] for a, b in zip(samples, samples[1:])):
        raise RuntimeError("boar playback counter regressed")
    muted = [s for s in samples if s[1]]
    if len(muted) < 24 or any(s[2] != muted[0][2] or s[3] != 4 or s[4] != 0 for s in muted):
        raise RuntimeError("mute failed to suppress transient playback")
    if muted[0][2] < 2 or samples[-1][1] or samples[-1][2] <= muted[-1][2]:
        raise RuntimeError("missing boar playback before/after mute")
    if not any(s[4] == 3 for s in samples):
        raise RuntimeError("no active boar source observed")


def validate_snake_audio_trace(samples):
    """Validate backend hiss playback/mute state, not perceptual snake identity."""
    if len(samples) < 30 or any(len(s) != 6 for s in samples):
        raise RuntimeError("missing snake audio trace")
    if any(s[0] != 1 or s[1] not in (0, 1) or not 4 <= s[3] <= 6
           or s[4] not in (0, 1, 2, 3, 4) for s in samples):
        raise RuntimeError("unavailable audio or invalid snake voice budget/state")
    if any(b[5] < a[5] for a, b in zip(samples, samples[1:])):
        raise RuntimeError("snake hiss playback counter regressed")
    muted = [s for s in samples if s[1]]
    if len(muted) < 24 or any(s[5] != muted[0][5] or s[3] != 4 or s[4] != 0 for s in muted):
        raise RuntimeError("mute failed to suppress snake hiss playback")
    if muted[0][5] < 1 or samples[-1][1] or samples[-1][5] <= muted[-1][5]:
        raise RuntimeError("missing snake hiss playback before/after mute")
    if not any(s[4] == 4 for s in samples):
        raise RuntimeError("no active snake hiss source observed")


def validate_goat_audio_trace(samples):
    """Validate backend goat playback/mute state, not perceptual animal identity."""
    if len(samples) < 30 or any(len(s) != 7 for s in samples):
        raise RuntimeError("missing goat audio trace")
    if any(s[0] != 1 or s[1] not in (0, 1) or not 4 <= s[3] <= 6
           or s[4] not in (0, 1, 2, 3, 4, 5, 6) for s in samples):
        raise RuntimeError("unavailable audio or invalid goat voice budget/state")
    if any(b[6] < a[6] for a, b in zip(samples, samples[1:])):
        raise RuntimeError("goat playback counter regressed")
    muted = [s for s in samples if s[1]]
    if len(muted) < 24 or any(s[6] != muted[0][6] or s[3] != 4 or s[4] != 0 for s in muted):
        raise RuntimeError("mute failed to suppress goat playback")
    if muted[0][6] < 2 or samples[-1][1] or samples[-1][6] <= muted[-1][6]:
        raise RuntimeError("missing goat playback before/after mute")
    if not any(s[4] == 6 for s in samples):
        raise RuntimeError("no active goat source observed")


def owned_runtime_log(evidence, namespace_pid):
    logs = [(path, path.read_text()) for path in evidence.glob("island-runtime-*.log")]
    logs = [(path, text) for path, text in logs if re.search(
        rf"runtime-start pid=\d+ main-tid=\d+ namespace-pid={namespace_pid}(?:\s|$)", text)]
    if len(logs) != 1:
        raise RuntimeError("missing or ambiguous owned persistent runtime log")
    return logs[0]


def validate_control_transitions(text, flight=False, fullscreen_cycles=2):
    """Require the exact scripted transitions from the persistent file, not stdout."""
    if fullscreen_cycles not in (0, 1, 2):
        raise RuntimeError("invalid fullscreen transition cycle count")
    records = []
    for line in text.splitlines():
        if "island-simulator: control-transition " not in line:
            continue
        fields = dict(re.findall(r"([\w-]+)=([^\s]+)", line.split("control-transition ", 1)[1]))
        try:
            event, frame = int(fields["event"]), int(fields["submitted-frames"])
            camera = [float(v) for v in fields["camera"].split(",")]
            position = [float(v) for v in fields["flight-position"].split(",")]
            values = camera + position + [float(fields["yaw"]), float(fields["pitch"])]
            if (len(camera) != 3 or len(position) != 3 or not all(map(math.isfinite, values))
                    or fields["finite"] != "1" or frame <= 0
                    or fields["flight-active"] != ("1" if event == 0 else "0")
                    or fields["fullscreen-requested"] != ("1" if event == 3 else "0")):
                raise ValueError("invalid transition state")
            if event == 0 and camera != position:
                raise ValueError("flight activation camera differs from flight position")
        except (KeyError, ValueError) as error:
            raise RuntimeError("malformed persistent control transition") from error
        records.append({"event": event, "frame": frame})
    expected = [3, 4] * fullscreen_cycles + ([0, 1] if flight else [])
    if [r["event"] for r in records] != expected:
        raise RuntimeError("missing, duplicate or reordered persistent control transitions")
    if any(b["frame"] <= a["frame"] for a, b in zip(records, records[1:])):
        raise RuntimeError("persistent transition frame counter did not advance")
    return records


def validate_diving_trace(samples):
    if len(samples)<4 or any(len(p)!=4 for p in samples):
        raise RuntimeError("missing swimming depth samples")
    if any(not all(map(math.isfinite,p)) or p[1]>.581 or abs(p[3]-max(0,-p[1]))>.001 for p in samples):
        raise RuntimeError("invalid swimming depth samples")
    if samples[0][1]<.57 or samples[-1][1]<.57 or max(p[3] for p in samples)<.8:
        raise RuntimeError("missing surface/dive/resurface sequence")


def validate_frame_windows(text):
    """Check live CPU/whole-frame accounting without imposing a software FPS budget."""
    records = []
    for line in text.splitlines():
        if "island-simulator: frame-window " not in line:
            continue
        fields = dict(re.findall(r"(\S+)=([^\s]+)", line.split("frame-window ", 1)[1]))
        try:
            seconds, fps = float(fields["seconds"]), float(fields["fps"])
            frames, flight = int(fields["frames"]), int(fields["flight"])
            updates = float(fields["updates-per-frame"])
            update_ms = float(fields["update-ms-per-frame"])
            draw_ms = float(fields["draw-submit-ms-per-frame"])
            wildlife = [float(v) for v in fields[
                "wildlife-ms-per-update(spider,goat,snake,monkey)"].split(",")]
            camera = [float(v) for v in fields["camera"].split(",")]
            values = [seconds, fps, updates, update_ms, draw_ms, *wildlife, *camera]
            if (not all(map(math.isfinite, values)) or seconds < 4.999
                    or frames <= 0 or updates <= 0 or flight not in (0, 1)
                    or len(wildlife) != 4 or len(camera) != 3
                    or any(v < 0 for v in [update_ms, draw_ms, *wildlife])
                    or not math.isclose(fps, frames / seconds, rel_tol=.001, abs_tol=.002)
                    or sum(wildlife) > update_ms / updates + .02):
                raise ValueError("inconsistent frame accounting")
        except (KeyError, ValueError) as error:
            raise RuntimeError("invalid persistent frame timing window") from error
        records.append(dict(seconds=seconds, frames=frames, fps=fps, flight=flight))
    if not records:
        raise RuntimeError("missing persistent frame timing windows")
    return records


def validate_cave_trace(samples, stages):
    if len(samples)<20 or set(stages)!={"start","back","ceiling","landed","exit"}:
        raise RuntimeError("incomplete live cave traversal")
    if any(len(p)!=5 or not all(map(math.isfinite,p)) or p[3] not in (0,1) or p[4]!=0 for p in samples):
        raise RuntimeError("invalid or rock-penetrating cave capsule")
    indices=[stages[key] for key in ("start","back","ceiling","landed","exit")]
    if any(type(i) is not int or not 0<=i<len(samples) for i in indices):
        raise RuntimeError("invalid cave stage index")
    start,back,ceiling,landed,exit_point=(samples[i] for i in indices)
    if not all(a<b for a,b in zip(indices,indices[1:])):
        raise RuntimeError("cave stages are out of order")
    if not (-9<start[2]<-6 and 18<back[2]<20 and exit_point[2]<-4):
        raise RuntimeError("cave entry/back wall/exit not reached")
    if any(abs(p[0])>.6 for p in samples):
        raise RuntimeError("cave traversal left its intended corridor")
    if any(p[3]!=0 for p in (start,back,landed,exit_point)) or ceiling[3]!=1:
        raise RuntimeError("cave walking and flight stages disagree")
    if not (.7<ceiling[1]-.025*ceiling[2]<2.0 and abs(landed[1]-.025*landed[2])<.08):
        raise RuntimeError("cave flight did not reach ceiling or land on floor")


def validate_fire_audio_trace(samples):
    if len(samples)<30:raise RuntimeError("missing live campfire audio/position samples")
    for p in samples:
        if (len(p)!=7 or not all(map(math.isfinite,p)) or p[0]!=1 or p[1] not in (0,1)
                or p[2] not in (0,1) or not 0<=p[3]<=1 or p[4]<0 or p[5]<-.002 or not 4<=p[6]<=6):
            raise RuntimeError("invalid campfire audio state or player entered firepit")
    muted=[i for i,p in enumerate(samples) if p[1]]
    if len(muted)<24 or any(samples[i][3]!=0 or samples[i][2]!=1 for i in muted):
        raise RuntimeError("campfire mute did not preserve silent ambient playback")
    if not any(p[2] and p[3]>.15 and p[4]<6 and 0<=p[5]<.2 for p in samples[:muted[0]]):
        raise RuntimeError("player did not reach the protected firepit with audible loop gain")
    if not any(p[2] and p[3]>.15 and p[4]<6 for p in samples[muted[-1]+1:]):
        raise RuntimeError("campfire did not resume after unmute")
    if not (samples[-1][4]>24 and samples[-1][2]==0 and samples[-1][3]<.005):
        raise RuntimeError("campfire did not stop beyond its sound range")


def validate_stream_crossing_trace(samples):
    if len(samples)<8:
        raise RuntimeError("missing live stream-crossing samples")
    if any(len(p)!=7 or not all(map(math.isfinite,p)) or p[4]<0 or abs(p[5])>.025 or p[6]!=0
           for p in samples):
        raise RuntimeError("invalid stream walking support or locomotion state")
    if samples[0][3]>-2.5 or samples[-1][3]<2.0:
        raise RuntimeError("player did not walk from one stream bank to the other")
    nearest=min(range(len(samples)),key=lambda i:samples[i][4])
    if samples[nearest][4]>.65 or nearest==0 or nearest==len(samples)-1:
        raise RuntimeError("stream crossing never reached the flowing channel")
    if any(samples[i][2]>samples[i+1][2]+.02 for i in range(len(samples)-1)):
        raise RuntimeError("stream crossing reversed instead of walking continuously forward")
    return {"samples":samples,"nearest":nearest}


def worker(build, evidence, window_manager, flight=False, boar_audio=False,
           diving=False, snake_audio=False, goat_audio=False, cave=False, campfire=False,
           stream_crossing=False):
    if (os.environ.get("ISLAND_FULLSCREEN_SUPERVISED") != "1"
            or Path("/dev/dri").exists() or not os.environ.get("DISPLAY")
            or os.environ.get("LIBGL_ALWAYS_SOFTWARE") != "1"):
        raise RuntimeError("worker requires supervised private-device software display")
    def command(*args):
        return subprocess.run(args, text=True, capture_output=True, check=True, timeout=10).stdout
    log_path = evidence / "game.log"
    stages = []
    with log_path.open("w") as stream, (evidence / "window-manager.log").open("w") as wm_log:
        manager = subprocess.Popen([str(window_manager), "--sm-disable", "--config-file",
                                    str(Path(__file__).with_name("fullscreen_openbox.xml"))],
                                   stdout=wm_log, stderr=subprocess.STDOUT)
        game_command=[str(build / "island-simulator"), "--no-user-settings",
                      "--debug-overlay", "--input-diagnostics"]
        if snake_audio:
            game_command.extend(["--viewpoint","snake-head"])
        elif goat_audio:
            game_command.extend(["--viewpoint","goats"])
        game = subprocess.Popen(game_command, stdout=stream,
                                stderr=subprocess.STDOUT)
        def log():
            return log_path.read_text(errors="replace")
        def wait(predicate, description):
            deadline = time.monotonic() + 35
            while time.monotonic() < deadline:
                if game.poll() is not None:
                    raise RuntimeError(f"game exited during {description}")
                value = predicate()
                if value:
                    return value
                time.sleep(.1)
            raise RuntimeError(f"timeout waiting for {description}")
        def keys(key, marker):
            command("xdotool", "windowfocus", "--sync", window)
            previous = log().count(marker)
            command("xdotool", "keydown", key)
            try:
                wait(lambda: log().count(marker) > previous, marker)
            finally:
                command("xdotool", "keyup", key)
        try:
            wait(lambda: "WINDOW" in command("xprop", "-root", "_NET_SUPPORTING_WM_CHECK"),
                 "private window manager")
            wait(lambda: "visible terrain cells" in log(), "first presented scene")
            if 'gl-renderer="llvmpipe ' not in log():
                raise RuntimeError("game did not confirm llvmpipe")
            window = command("xdotool", "search", "--onlyvisible", "--pid", str(game.pid),
                             "--name", "^island-simulator$").splitlines()[0]
            command("xdotool", "windowfocus", "--sync", window)
            def snapshot(label):
                result = {"label": label, **geometry(command("xdotool", "getwindowgeometry", "--shell", window)),
                          "display": [int(v) for v in command("xdotool", "getdisplaygeometry").split()],
                          "state": command("xprop", "-id", window, "_NET_WM_STATE")}
                stages.append(result)
                (evidence / "stages.json").write_text(json.dumps(stages, indent=2)+"\n")
                return result
            # The game deliberately creates a non-resizable window; do not bypass its
            # WM size hints to invent a state users cannot enter. Compare the real original.
            before = snapshot("windowed-before")
            keys("F1", "debug overlay disabled")
            # Snake playback has its own focused native gate.  The normal invocation
            # remains the authoritative two-cycle F11/focus regression test.
            fullscreen_cycles = 0 if snake_audio or goat_audio or cave or campfire or diving or stream_crossing else 2
            for cycle in range(fullscreen_cycles):
                # Some X11 window managers return focus to their root/desktop while
                # restoring a non-resizable window from fullscreen.  Reassert focus
                # before each independent key edge so the second cycle exercises the
                # game instead of intermittently delivering F11 to the desktop.
                command("xdotool", "windowfocus", "--sync", window)
                command("xdotool", "keydown", "F11")
                try:
                    wait(lambda: log().count("fullscreen on requested") == cycle+1, "fullscreen request")
                    wait(lambda: "_NET_WM_STATE_FULLSCREEN" in command("xprop", "-id", window, "_NET_WM_STATE"),
                         "actual X11 fullscreen state")
                    frames = log().count("input sample captured")
                    wait(lambda: log().count("input sample captured") >= frames+3, "held-key frames")
                    if log().count("fullscreen on requested") != cycle+1 or log().count("fullscreen off requested") != cycle:
                        raise RuntimeError("held F11 repeated the fullscreen toggle")
                    full = snapshot(f"fullscreen-{cycle}")
                    if [full["WIDTH"],full["HEIGHT"]] != full["display"]:
                        raise RuntimeError("fullscreen window does not cover the private display")
                    if cycle == 0:
                        command("import", "-window", window, str(evidence / "fullscreen.png"))
                finally:
                    command("xdotool", "keyup", "F11")
                # Observe a released-key frame before creating the next rising edge.
                frames = log().count("input sample captured")
                wait(lambda: log().count("input sample captured") > frames, "released F11 frame")
                keys("F11", "fullscreen off requested")
                wait(lambda: "_NET_WM_STATE_FULLSCREEN" not in command("xprop", "-id", window, "_NET_WM_STATE"),
                     "windowed state")
                # The WM may move focus during the fullscreen-off transition before
                # keys() sends its release.  Focus the client and resend the harmless
                # release so SDL observes an up frame and the next cycle has a real
                # rising edge.
                command("xdotool", "windowfocus", "--sync", window)
                command("xdotool", "keyup", "F11")
                # Removing the EWMH fullscreen atom precedes some window managers'
                # configure events. Keep the exact restoration requirement, but wait
                # for that asynchronous operation rather than judging an intermediate frame.
                expected_geometry = tuple(before[k] for k in ("X", "Y", "WIDTH", "HEIGHT"))
                def restored_geometry():
                    actual = geometry(command("xdotool", "getwindowgeometry", "--shell", window))
                    return tuple(actual[k] for k in ("X", "Y", "WIDTH", "HEIGHT")) == expected_geometry
                wait(restored_geometry, "exact original window position and size restoration")
                after = snapshot(f"windowed-{cycle}")
                if (after["WIDTH"], after["HEIGHT"]) != (before["WIDTH"], before["HEIGHT"]):
                    raise RuntimeError("fullscreen did not restore the original windowed size")
                if (after["X"],after["Y"]) != (before["X"],before["Y"]):
                    raise RuntimeError("fullscreen did not restore the original window position")
                frames = log().count("input sample captured")
                wait(lambda: log().count("input sample captured") > frames, "windowed released-key frame")
            command("import", "-window", window, str(evidence / "windowed.png"))
            if not snake_audio and not goat_audio and not cave and not campfire and not stream_crossing:
                # Only this owned dialog receives the background F11; never another
                # desktop/client. Restoring focus must not synthesize a game toggle.
                focus_target = subprocess.Popen(["xmessage", "-title", "island-fullscreen-focus",
                                                 "-buttons", "OK:0", "Owned fullscreen QA focus target"],
                                                stdout=stream, stderr=subprocess.STDOUT)
                try:
                    def find_focus_target():
                        result = subprocess.run(["xdotool", "search", "--onlyvisible", "--pid", str(focus_target.pid),
                                                 "--name", "^island-fullscreen-focus$"],
                                                text=True, capture_output=True, timeout=5)
                        return result.stdout.splitlines()[0] if result.returncode == 0 else None
                    target = wait(find_focus_target, "owned focus target")
                    command("xdotool", "windowactivate", "--sync", target)
                    command("xdotool", "keydown", "F11")
                    command("xdotool", "keyup", "F11")
                    command("xdotool", "windowactivate", "--sync", window)
                    frames = log().count("input sample captured")
                    wait(lambda: log().count("input sample captured") >= frames+3, "restored-focus frames")
                    if (log().count("fullscreen on requested") != fullscreen_cycles
                            or log().count("fullscreen off requested") != fullscreen_cycles):
                        raise RuntimeError("background key or focus restoration toggled fullscreen")
                    snapshot("focus-restored")
                finally:
                    focus_target.terminate()
                    try:
                        focus_target.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        focus_target.kill()
                        focus_target.wait(timeout=5)
            # Fixed inspection viewpoints intentionally reject player/mouse capture.
            # The normal fullscreen gate still exercises W capture and Escape release;
            # the snake-audio mode keeps its camera locked to the retreating subject.
            if not snake_audio and not goat_audio:
                keys("w", "input sample captured 1 relative 1")
                keys("Escape", "input sample captured 0 relative 0")
            if stream_crossing:
                number=r"([-+\d.eE]+)"
                def stream_samples():
                    section=log().split("debug teleport stream crossing")[-1]
                    return [tuple(map(float,m)) for m in re.findall(
                        r"stream crossing position "+number+r" "+number+r" "+number
                        +r" signed-across "+number+r" route-distance "+number
                        +r" support-error "+number+r" mode (\d+)",section)]
                keys("F1","debug overlay enabled")
                keys("p","debug teleport stream crossing")
                keys("F1","debug overlay disabled")
                wait(lambda: len(stream_samples())>=2,"stream south-bank support samples")
                command("import","-window",window,str(evidence/"stream-before.png"))
                command("xdotool","keydown","w")
                try:
                    wait(lambda: stream_samples() and stream_samples()[-1][3]>2.0,
                         "walk across stream to north bank")
                finally:
                    command("xdotool","keyup","w")
                count=len(stream_samples())
                wait(lambda: len(stream_samples())>=count+2,"released stream-bank samples")
                command("import","-window",window,str(evidence/"stream-after.png"))
                result=validate_stream_crossing_trace(stream_samples())
                (evidence/"stream-crossing-result.json").write_text(json.dumps(
                    {"passed":True,**result,"scope":"actual keyboard walk over rendered stream bed"},indent=2)+"\n")
            if campfire:
                def fire_samples():
                    number=r"([-+\d.eE]+)"
                    return [tuple(map(float,m)) for m in re.findall(
                        r"fire audio available ([01]) muted ([01]) playing ([01]) gain "+number
                        +r" distance "+number+r" clearance "+number+r" sources (\d+)",log())]
                keys("F1","debug overlay enabled")
                keys("o","debug teleport campfire")
                keys("F1","debug overlay disabled")
                wait(lambda: fire_samples() and fire_samples()[-1][2]==1,"actual campfire playback")
                command("xdotool","keydown","w")
                try:
                    wait(lambda: fire_samples()[-1][5]<.2,"approach protected firepit")
                    count=len(fire_samples())
                    wait(lambda: len(fire_samples())>=count+4,"held forward at firepit boundary")
                finally:command("xdotool","keyup","w")
                command("import","-window",window,str(evidence/"campfire-close.png"))
                keys("m","soundscape muted")
                count=len(fire_samples())
                wait(lambda: len(fire_samples())>=count+4,"drain muted campfire pipeline")
                recording=evidence/"game-audio.raw"
                start=recording.stat().st_size
                count=len(fire_samples())
                wait(lambda: len(fire_samples())>=count+24,"six simulated seconds of fire silence")
                end=recording.stat().st_size
                with recording.open("rb") as pcm:
                    pcm.seek(start);silent=pcm.read(end-start)
                if not silent or any(silent):raise RuntimeError("muted fire mix is not exact PCM silence")
                keys("m","soundscape unmuted")
                wait(lambda: fire_samples()[-1][3]>.15,"actual campfire volume after unmute")
                count=len(fire_samples())
                wait(lambda: len(fire_samples())>=count+4,"resumed fire audio packets")
                with recording.open("rb") as pcm:
                    pcm.seek(end);resumed=pcm.read()
                if not resumed or not any(resumed):raise RuntimeError("resumed campfire mix stayed silent")
                command("xdotool","keydown","s")
                try:
                    wait(lambda: fire_samples()[-1][4]>26,"walk beyond campfire sound range")
                finally:command("xdotool","keyup","s")
                wait(lambda: fire_samples()[-1][2]==0 and fire_samples()[-1][3]<.005,"fire voice release/fade")
                samples=fire_samples();validate_fire_audio_trace(samples)
                command("import","-window",window,str(evidence/"campfire-away.png"))
                (evidence/"campfire-result.json").write_text(json.dumps({"passed":True,"samples":samples,
                    "mutedBytes":len(silent),"resumedBytes":len(resumed),
                    "scope":"actual firepit approach, CNA loop/gain, disk PCM mute/resume and range exit; not speaker listening"},indent=2)+"\n")
            if cave:
                def cave_samples():
                    return [tuple(map(float,m)) for m in re.findall(
                        r"cave sample local ([-+\d.eE]+) ([-+\d.eE]+) ([-+\d.eE]+) flight ([01]) solid ([01])",log())]
                cave_stages={}
                def save_cave_stage(name):
                    count=len(cave_samples())
                    wait(lambda: len(cave_samples())>=count+2,f"cave {name} presentation")
                    cave_stages[name]=len(cave_samples())-1
                    command("import","-window",window,str(evidence/f"cave-{name}.png"))
                keys("F1","debug overlay enabled")
                keys("l","debug teleport cave entrance")
                keys("F1","debug overlay disabled")
                wait(lambda: cave_samples(),"cave approach diagnostics")
                save_cave_stage("start")
                command("xdotool","keydown","w")
                try:
                    wait(lambda: cave_samples()[-1][2]>19.5,"walking into cave back wall")
                    count=len(cave_samples())
                    wait(lambda: len(cave_samples())>=count+4,"held forward against cave wall")
                finally:
                    command("xdotool","keyup","w")
                save_cave_stage("back")
                keys("Tab","flight on;")
                command("xdotool","keydown","space")
                try:
                    wait(lambda: cave_samples()[-1][1]>.025*cave_samples()[-1][2]+.75,"inner ceiling ascent")
                    count=len(cave_samples())
                    wait(lambda: len(cave_samples())>=count+4,"held ascent against roof")
                finally:
                    command("xdotool","keyup","space")
                save_cave_stage("ceiling")
                command("xdotool","keydown","Shift_L")
                try:
                    wait(lambda: cave_samples()[-1][1]<.025*cave_samples()[-1][2]+.05,"cave floor descent")
                finally:
                    command("xdotool","keyup","Shift_L")
                keys("Tab","flight off; safe dry-land return")
                save_cave_stage("landed")
                command("xdotool","keydown","s")
                try:
                    wait(lambda: cave_samples()[-1][2]<-5,"walking out of cave")
                finally:
                    command("xdotool","keyup","s")
                save_cave_stage("exit")
                samples=cave_samples()
                validate_cave_trace(samples,cave_stages)
                (evidence/"cave-result.json").write_text(json.dumps({"passed":True,
                    "samples":samples,"stages":cave_stages,
                    "scope":"actual keyboard entry/back wall/inner roof/floor/exit; not portable qualification"},indent=2)+"\n")
            if boar_audio:
                def samples():
                    return [tuple(map(int, m)) for m in re.findall(
                        r"audio sample available (\d+) muted (\d+) boars (\d+) sources (\d+) wildlife (\d+)", log())]
                wait(lambda: samples() and samples()[-1][0] == 1, "available disk audio device")
                keys("F1", "debug overlay enabled")
                keys("k", "debug teleport boar herd")
                wait(lambda: samples()[-1][2] >= 2, "two actual boar playbacks")
                command("import", "-window", window, str(evidence / "boar-audio.png"))
                keys("m", "soundscape muted")
                wait(lambda: samples()[-1][1] == 1, "muted audio diagnostics")
                first = len(samples())
                wait(lambda: len(samples()) >= first+4, "mute pipeline drain")
                recording = evidence / "game-audio.raw"
                start = recording.stat().st_size
                muted_start = len(samples())
                wait(lambda: len(samples()) >= muted_start+24, "six simulated muted seconds")
                muted = samples()[muted_start:]
                if any(s[1] != 1 or s[2] != muted[0][2] or s[3] != 4 or s[4] != 0 for s in muted):
                    raise RuntimeError("mute retained a transient or started a boar call")
                end = recording.stat().st_size
                with recording.open("rb") as pcm:
                    pcm.seek(start)
                    silent = pcm.read(end-start)
                if not silent or any(silent):
                    raise RuntimeError("muted disk audio is missing or not exact silence")
                keys("m", "soundscape unmuted")
                previous = muted[-1][2]
                wait(lambda: samples()[-1][2] > previous, "boar playback after unmute")
                resumed_sample_start=len(samples())
                wait(lambda: len(samples())>=resumed_sample_start+4,
                     "boar resumed audio pipeline drain")
                wait(lambda: recording.stat().st_size>=end+4096,
                     "boar resumed disk-audio flush")
                if any(s[3] > 6 for s in samples()):
                    raise RuntimeError("audio source budget exceeded")
                if not re.search(r"Writing to file .*format=(?:F32|S16|S32).*channels=2 freq=", log()):
                    raise RuntimeError("missing signed PCM/stereo disk output identity")
                with recording.open("rb") as pcm:
                    pcm.seek(end)
                    resumed = pcm.read()
                if not resumed or not any(resumed):
                    raise RuntimeError("unmuted disk audio stayed silent")
                validate_boar_audio_trace(samples())
                (evidence / "boar-audio-result.json").write_text(json.dumps({
                    "passed":True,"samples":samples(),"mutedBytes":len(silent),
                    "resumedBytes":len(resumed),"scope":"native disk-device playback/mute; not subjective listening"},indent=2)+"\n")
            if snake_audio:
                def samples():
                    return [tuple(map(int, m)) for m in re.findall(
                        r"audio sample available (\d+) muted (\d+) boars (\d+) sources (\d+) wildlife (\d+) snake-hisses (\d+)", log())]
                wait(lambda: samples() and samples()[-1][0] == 1, "available disk audio device")
                keys("F1", "debug overlay enabled")
                wait(lambda: samples()[-1][5] >= 1, "actual snake hiss playback")
                command("import", "-window", window, str(evidence / "snake-audio.png"))
                keys("m", "soundscape muted")
                wait(lambda: samples()[-1][1] == 1, "muted audio diagnostics")
                recording=evidence/"game-audio.raw"
                start=recording.stat().st_size
                muted_start=len(samples())
                wait(lambda: len(samples())>=muted_start+24,"six simulated muted seconds")
                muted=samples()[muted_start:]
                if any(s[1]!=1 or s[5]!=muted[0][5] or s[3]!=4 or s[4]!=0 for s in muted):
                    raise RuntimeError("mute retained a transient or started a snake hiss")
                end=recording.stat().st_size
                with recording.open("rb") as pcm:
                    pcm.seek(start);silent=pcm.read(end-start)
                if not silent or any(silent):
                    raise RuntimeError("muted disk audio is missing or not exact silence")
                keys("m","soundscape unmuted")
                previous=muted[-1][5]
                wait(lambda:samples()[-1][5]>previous,"snake hiss playback after unmute")
                resumed_sample_start=len(samples())
                wait(lambda:len(samples())>=resumed_sample_start+4,
                     "snake resumed audio pipeline drain")
                wait(lambda:recording.stat().st_size>=end+4096,
                     "snake resumed disk-audio flush")
                if any(s[3]>6 for s in samples()):
                    raise RuntimeError("audio source budget exceeded")
                if not re.search(r"Writing to file .*format=(?:F32|S16|S32).*channels=2 freq=",log()):
                    raise RuntimeError("missing signed PCM/stereo disk output identity")
                with recording.open("rb") as pcm:
                    pcm.seek(end);resumed=pcm.read()
                if not resumed or not any(resumed):
                    raise RuntimeError("unmuted disk audio stayed silent")
                validate_snake_audio_trace(samples())
                (evidence/"snake-audio-result.json").write_text(json.dumps({
                    "passed":True,"samples":samples(),"mutedBytes":len(silent),
                    "resumedBytes":len(resumed),"scope":"native disk-device hiss playback/mute; not subjective listening"},indent=2)+"\n")
            if goat_audio:
                def samples():
                    return [tuple(map(int, m)) for m in re.findall(
                        r"audio sample available (\d+) muted (\d+) boars (\d+) sources (\d+) wildlife (\d+) snake-hisses (\d+) goats (\d+)", log())]
                wait(lambda: samples() and samples()[-1][0] == 1, "available disk audio device")
                keys("F1", "debug overlay enabled")
                wait(lambda: samples()[-1][6] >= 2, "two actual goat playbacks")
                command("import", "-window", window, str(evidence / "goat-audio.png"))
                keys("m", "soundscape muted")
                wait(lambda: samples()[-1][1] == 1, "muted audio diagnostics")
                recording=evidence/"game-audio.raw"
                start=recording.stat().st_size
                muted_start=len(samples())
                wait(lambda: len(samples())>=muted_start+24,"six simulated muted seconds")
                muted=samples()[muted_start:]
                if any(s[1]!=1 or s[6]!=muted[0][6] or s[3]!=4 or s[4]!=0 for s in muted):
                    raise RuntimeError("mute retained a transient or started a goat call")
                end=recording.stat().st_size
                with recording.open("rb") as pcm:
                    pcm.seek(start);silent=pcm.read(end-start)
                if not silent or any(silent):
                    raise RuntimeError("muted disk audio is missing or not exact silence")
                keys("m","soundscape unmuted")
                previous=muted[-1][6]
                wait(lambda:samples()[-1][6]>previous,"goat playback after unmute")
                resumed_sample_start=len(samples())
                wait(lambda:len(samples())>=resumed_sample_start+4,
                     "goat resumed audio pipeline drain")
                wait(lambda:recording.stat().st_size>=end+4096,
                     "goat resumed disk-audio flush")
                if any(s[3]>6 for s in samples()):
                    raise RuntimeError("audio source budget exceeded")
                if not re.search(r"Writing to file .*format=(?:F32|S16|S32).*channels=2 freq=",log()):
                    raise RuntimeError("missing signed PCM/stereo disk output identity")
                with recording.open("rb") as pcm:
                    pcm.seek(end);resumed=pcm.read()
                if not resumed or not any(resumed):
                    raise RuntimeError("unmuted disk audio stayed silent")
                validate_goat_audio_trace(samples())
                (evidence/"goat-audio-result.json").write_text(json.dumps({
                    "passed":True,"samples":samples(),"mutedBytes":len(silent),
                    "resumedBytes":len(resumed),"scope":"native disk-device goat playback/mute; not subjective listening"},indent=2)+"\n")
            if diving:
                def swim_samples():
                    return [tuple(map(float,m)) for m in re.findall(
                        r"swim position ([-+\d.eE]+) ([-+\d.eE]+) ([-+\d.eE]+) depth ([-+\d.eE]+)",log())]
                def underwater_audio_samples():
                    number=r"([-+\d.eE]+)"
                    return [tuple(map(float,m)) for m in re.findall(
                        r"audio sample available ([01]) muted [01].* underwater "+number
                        +r" air-gain "+number+r" water-gain "+number
                        +r" water-pitch-offset "+number,log())]
                keys("F1","debug overlay enabled")
                keys("F12","debug teleport offshore current")
                keys("F1","debug overlay disabled")
                wait(lambda: len(swim_samples())>=2,"surface swimming samples")
                pace=log().count("run mode enabled")+log().count("run mode disabled")
                command("xdotool","keydown","Shift_L")
                try:
                    wait(lambda: swim_samples()[-1][3]>.8,"actual submersion")
                    count=len(swim_samples())
                    wait(lambda: len(swim_samples())>=count+3,"underwater presentation")
                    command("import","-window",window,str(evidence/"diving-underwater.png"))
                finally:
                    command("xdotool","keyup","Shift_L")
                count=len(swim_samples())
                wait(lambda: len(swim_samples())>=count+2,"released descent")
                command("xdotool","keydown","space")
                try:
                    wait(lambda: swim_samples()[-1][1]>.57,"resurface")
                finally:
                    command("xdotool","keyup","space")
                command("import","-window",window,str(evidence/"diving-surface.png"))
                samples=swim_samples()
                validate_diving_trace(samples)
                if log().count("run mode enabled")+log().count("run mode disabled")!=pace:
                    raise RuntimeError("diving Shift changed ground running mode")
                if "flight on;" in log():
                    raise RuntimeError("swimming Space incorrectly activated flight")
                if "submerged soundscape muffles airborne ambience and deepens the existing water loop" not in log():
                    raise RuntimeError("diving did not reach the depth-responsive soundscape")
                audio=underwater_audio_samples()
                if not any(a[0]==1 and a[1]>.95 and a[2]<.20 and a[3]>1.20 and a[4]<-.11
                           for a in audio):
                    raise RuntimeError("live diving did not apply the bounded deep-water audio mix")
                recording=evidence/"game-audio.raw"
                if not recording.is_file() or recording.stat().st_size<4096:
                    raise RuntimeError("diving disk-audio output is missing or empty")
                with recording.open("rb") as pcm:
                    if not any(pcm.read(min(recording.stat().st_size,1048576))):
                        raise RuntimeError("diving disk-audio output contains only silence")
                (evidence/"diving-result.json").write_text(json.dumps({"passed":True,
                    "samples":samples,"underwaterAudio":audio,
                    "audioBytes":recording.stat().st_size,
                    "scope":"actual descent/ascent, bounded depth mix and disk PCM; not speaker listening"},indent=2)+"\n")
            if flight:
                def flight_samples():
                    return [tuple(map(float,m)) for m in re.findall(
                        r"flight position ([-+\d.eE]+) ([-+\d.eE]+) ([-+\d.eE]+) above-ground ([-+\d.eE]+)", log())]
                command("xdotool", "keydown", "Tab")
                try:
                    wait(lambda: len(flight_samples())>=3,"flight activation presentation")
                    command("import", "-window", window, str(evidence / "flight-notice.png"))
                    wait(lambda: len(flight_samples())>=4,"held Tab flight frames")
                    if log().count("flight on;")!=1 or "flight off;" in log():
                        raise RuntimeError("held Tab toggled flight repeatedly")
                finally:
                    command("xdotool", "keyup", "Tab")
                command("xdotool", "keydown", "space")
                try:
                    wait(lambda: flight_samples()[-1][3]>99.9,"100m flight ceiling")
                    samples=len(flight_samples())
                    wait(lambda: len(flight_samples())>=samples+4,"held ascent at ceiling")
                finally:
                    command("xdotool", "keyup", "space")
                # Look downward using actual relative mouse input for an aerial audit.
                command("xdotool", "mousemove", "--window", window, "640", "360")
                command("xdotool", "mousedown", "1")
                try:
                    wait(lambda: "input sample captured 1 relative 1" in log().split("flight on;")[-1],
                         "flight mouse capture")
                finally:
                    command("xdotool", "mouseup", "1")
                command("xdotool", "mousemove_relative", "--", "0", "220")
                samples=len(flight_samples())
                wait(lambda: len(flight_samples())>=samples+3,"aerial look frames")
                command("import", "-window", window, str(evidence / "flight-ceiling.png"))
                command("xdotool", "keydown", "Shift_L")
                try:
                    wait(lambda: flight_samples()[-1][3]<60,"flight descent")
                finally:
                    command("xdotool", "keyup", "Shift_L")
                count=len(flight_samples())
                wait(lambda: len(flight_samples())>=count+2,"released descent frames")
                start=flight_samples()[-1][1]
                count=len(flight_samples())
                command("xdotool", "keydown", "space", "Shift_R")
                try:
                    wait(lambda: len(flight_samples())>=count+3,"opposing vertical keys")
                    if abs(flight_samples()[-1][1]-start)>.1:
                        raise RuntimeError("Space plus Shift did not cancel vertical movement")
                finally:
                    command("xdotool", "keyup", "space", "Shift_R")
                count=len(flight_samples())
                command("xdotool", "keydown", "Shift_R")
                try:
                    wait(lambda: len(flight_samples())>=count+3,"right Shift descent")
                    if flight_samples()[-1][1]>=start-.1:
                        raise RuntimeError("right Shift did not descend")
                finally:
                    command("xdotool", "keyup", "Shift_R")
                samples=flight_samples()
                if any(not 1.71<=p[3]<=100.001 or (p[0]**2+p[2]**2)**.5>330.001 for p in samples):
                    raise RuntimeError("live flight exceeded terrain/height/radius bounds")
                keys("Tab", "flight off; safe dry-land return")
                frames=log().count("input sample captured")
                wait(lambda: log().count("input sample captured")>=frames+3,"landed frames")
                command("import", "-window", window, str(evidence / "flight-landed.png"))
                (evidence / "flight-result.json").write_text(json.dumps({"passed":True,"samples":samples,
                    "scope":"native Tab repeat, Space ascent/ceiling, Shift descent and safe return; horizontal boundary is CPU tested"},indent=2)+"\n")
            if "debug teleport monkey troop" in log():
                raise RuntimeError("F11 still teleported to monkeys")
            validate_graphics_log(log(), None)
            # Runtime filenames deliberately use outer procfs IDs for kernel correlation;
            # Popen.pid belongs to the private PID namespace. Match its startup identity.
            runtime_path, runtime_text = owned_runtime_log(evidence, game.pid)
            transitions = validate_control_transitions(runtime_text, flight or cave, fullscreen_cycles)
            if flight:
                windows = validate_frame_windows(runtime_text)
                if not any(row["flight"] for row in windows):
                    raise RuntimeError("flight produced no live performance window")
                (evidence / "frame-windows-result.json").write_text(json.dumps({
                    "passed": True, "source": runtime_path.name, "windows": windows,
                    "scope": "CPU timing instrumentation only; software FPS is not a hardware budget"},
                    indent=2)+"\n")
            (evidence / "control-transitions-result.json").write_text(json.dumps({
                "passed": True, "source": runtime_path.name, "records": transitions,
                "scope": "persistent native scripted transitions; not GPU crash qualification"}, indent=2)+"\n")
            (evidence / "input-result.json").write_text(json.dumps({
                "passed": True, "stages": stages,
                "heldRepeatSuppressed": not snake_audio and not goat_audio and not cave and not campfire and not stream_crossing,
                "backgroundKeyIgnoredAfterFocusReturn": not snake_audio and not goat_audio and not cave and not campfire and not stream_crossing,
                "flightChecked": flight,
                "caveChecked": cave,
                "campfireChecked": campfire,
                "streamCrossingChecked": stream_crossing,
                "boarAudioChecked": boar_audio,
                "snakeAudioChecked": snake_audio,
                "goatAudioChecked": goat_audio,
                "mouseCapturedAndReleasedAfterToggle": not snake_audio and not goat_audio}, indent=2)+"\n")
        finally:
            game.terminate()
            try:
                game.wait(timeout=8)
            except subprocess.TimeoutExpired:
                game.kill()
                game.wait(timeout=5)
            manager.terminate()
            try:
                manager.wait(timeout=5)
            except subprocess.TimeoutExpired:
                manager.kill()
                manager.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--flight", action="store_true", help="also exercise Tab flight, ceiling and return")
    parser.add_argument("--cave", action="store_true", help="focused live cave entry, wall, roof, floor and exit check")
    parser.add_argument("--campfire", action="store_true", help="focused live firepit approach, crackling mute/resume and range check")
    parser.add_argument("--stream-crossing", action="store_true", help="focused live keyboard walk across the rendered stream bed")
    parser.add_argument("--diving", action="store_true", help="exercise underwater descent and resurfacing")
    parser.add_argument("--boar-audio", action="store_true", help="exercise boar playback and mute through SDL disk output")
    parser.add_argument("--snake-audio", action="store_true", help="exercise close snake-hiss playback and mute through SDL disk output")
    parser.add_argument("--goat-audio", action="store_true", help="exercise goat playback and mute through SDL disk output")
    parser.add_argument("--window-manager", type=Path,
                        default=shutil.which("openbox") or Path.home()/"deps/xtools/bin/openbox")
    parser.add_argument("--worker", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if (args.cave or args.campfire or args.stream_crossing) and sum((args.flight,args.diving,args.boar_audio,args.snake_audio,args.goat_audio,args.cave,args.campfire,args.stream_crossing))!=1:
        parser.error("--cave/--campfire/--stream-crossing use separate focused scopes")
    build = args.build.resolve(strict=True)
    window_manager = args.window_manager.resolve(strict=True)
    if args.worker:
        worker(build, args.worker.resolve(strict=True), window_manager, args.flight,
               args.boar_audio, args.diving, args.snake_audio, args.goat_audio, args.cave,
               args.campfire,args.stream_crossing)
        return
    if sum((args.boar_audio,args.snake_audio,args.goat_audio))>1:
        raise RuntimeError("choose only one focused wildlife audio scope")
    for executable in ("xdotool", "xprop", "import", "xmessage"):
        if not shutil.which(executable):
            raise RuntimeError(f"missing {executable}")
    root = Path(__file__).resolve().parent.parent
    platform = check_configuration((build / "CMakeCache.txt").read_text()) / "tools/platform"
    os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:8])
    with (root / "build/gpu-qualification.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        journal = Journal()
        baseline = journal.latest()
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
        evidence = build / ("software-stream-crossing" if args.stream_crossing else "software-campfire" if args.campfire else "software-cave" if args.cave else "software-boar-audio" if args.boar_audio else
                            "software-snake-audio" if args.snake_audio else
                            "software-goat-audio" if args.goat_audio else
                            "software-flight" if args.flight else "software-fullscreen") / stamp
        evidence.mkdir(parents=True)
        command = [str(platform / "wayland_test_server.sh"), "--compositor", "weston", "--renderer", "pixman",
                   str(platform / "run_gpu_tests_private.sh"), "--inside-private-compositor", "exec",
                   sys.executable, str(Path(__file__).resolve()), str(build), "--worker", str(evidence),
                   "--window-manager", str(window_manager)]
        if args.flight:
            command.append("--flight")
        if args.cave:
            command.append("--cave")
        if args.campfire:
            command.append("--campfire")
        if args.stream_crossing:
            command.append("--stream-crossing")
        if args.diving:
            command.append("--diving")
        if args.boar_audio:
            command.append("--boar-audio")
        if args.snake_audio:
            command.append("--snake-audio")
        if args.goat_audio:
            command.append("--goat-audio")
        environment = software_environment(evidence)
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY"):
            environment.pop(key, None)
        environment["ISLAND_FULLSCREEN_SUPERVISED"] = "1"
        environment["XDG_CONFIG_HOME"] = str(evidence / "config")
        if args.boar_audio or args.snake_audio or args.goat_audio or args.campfire or args.diving:
            environment["SDL_AUDIO_DRIVER"] = "disk"
            environment["SDL_AUDIO_DISK_OUTPUT_FILE"] = str(evidence / "game-audio.raw")
            environment["SDL_AUDIO_DISK_TIMESCALE"] = "1.0"
        metadata = {"result":"running", "command":command, "kernel_before":baseline,
                    "artifact":artifact_identity(build / "island-simulator"),
                    "window_manager":artifact_identity(window_manager),
                    "harness":artifact_identity(Path(__file__)),
                    "window_manager_config":artifact_identity(Path(__file__).with_name("fullscreen_openbox.xml")),
                    "scope":"private llvmpipe fullscreen/optional flight or disk-audio input; not hardware qualification"}
        report = evidence / "run.json"
        report.write_text(json.dumps(metadata, indent=2)+"\n")
        print(f"Native input evidence: {evidence}", flush=True)
        failure = None
        try:
            result = run_monitored_capture(command, evidence, environment, evidence, journal, baseline, timeout=240)
            if result:
                raise RuntimeError(f"fullscreen worker exited {result}")
            if not json.loads((evidence / "input-result.json").read_text()).get("passed"):
                raise RuntimeError("missing fullscreen input pass")
            metadata["result"] = "passed"
        except (OSError, RuntimeError, subprocess.SubprocessError) as error:
            failure = error
        finally:
            try:
                records = journal.read("--after-cursor", baseline["__CURSOR"])
                (evidence / "kernel-final.json").write_text(json.dumps(records, indent=2)+"\n")
                if any(r["_BOOT_ID"] != baseline["_BOOT_ID"] or FAULT.search(r["MESSAGE"]) for r in records):
                    raise RuntimeError("kernel fault/boot discontinuity after fullscreen cleanup")
            except (OSError, RuntimeError) as error:
                failure = failure or error
            if failure:
                metadata.update(result="failed", error=str(failure))
            report.write_text(json.dumps(metadata, indent=2)+"\n")
        if failure:
            raise failure
        print("Native software input passed; no new kernel GPU faults")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        sys.exit(f"Native fullscreen gate: {error}")
