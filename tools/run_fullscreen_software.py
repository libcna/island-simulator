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


def owned_runtime_log(evidence, namespace_pid):
    logs = [(path, path.read_text()) for path in evidence.glob("island-runtime-*.log")]
    logs = [(path, text) for path, text in logs if re.search(
        rf"runtime-start pid=\d+ main-tid=\d+ namespace-pid={namespace_pid}(?:\s|$)", text)]
    if len(logs) != 1:
        raise RuntimeError("missing or ambiguous owned persistent runtime log")
    return logs[0]


def validate_control_transitions(text, flight=False):
    """Require the exact scripted transitions from the persistent file, not stdout."""
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
    if [r["event"] for r in records] != [3, 4, 3, 4] + ([0, 1] if flight else []):
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


def worker(build, evidence, window_manager, flight=False, boar_audio=False, diving=False):
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
        game = subprocess.Popen([str(build / "island-simulator"), "--no-user-settings",
                                 "--debug-overlay", "--input-diagnostics"], stdout=stream,
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
            for cycle in range(2):
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
                after = snapshot(f"windowed-{cycle}")
                if (after["WIDTH"], after["HEIGHT"]) != (before["WIDTH"], before["HEIGHT"]):
                    raise RuntimeError("fullscreen did not restore the original windowed size")
                if (after["X"],after["Y"]) != (before["X"],before["Y"]):
                    raise RuntimeError("fullscreen did not restore the original window position")
                frames = log().count("input sample captured")
                wait(lambda: log().count("input sample captured") > frames, "windowed released-key frame")
            command("import", "-window", window, str(evidence / "windowed.png"))
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
                if log().count("fullscreen on requested") != 2 or log().count("fullscreen off requested") != 2:
                    raise RuntimeError("background key or focus restoration toggled fullscreen")
                snapshot("focus-restored")
            finally:
                focus_target.terminate()
                try:
                    focus_target.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    focus_target.kill()
                    focus_target.wait(timeout=5)
            keys("w", "input sample captured 1 relative 1")
            keys("Escape", "input sample captured 0 relative 0")
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
            if diving:
                def swim_samples():
                    return [tuple(map(float,m)) for m in re.findall(
                        r"swim position ([-+\d.eE]+) ([-+\d.eE]+) ([-+\d.eE]+) depth ([-+\d.eE]+)",log())]
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
                (evidence/"diving-result.json").write_text(json.dumps({"passed":True,
                    "samples":samples,"scope":"actual descent and ascent; not visual/audio acceptance"},indent=2)+"\n")
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
            transitions = validate_control_transitions(runtime_text, flight)
            (evidence / "control-transitions-result.json").write_text(json.dumps({
                "passed": True, "source": runtime_path.name, "records": transitions,
                "scope": "persistent native scripted transitions; not GPU crash qualification"}, indent=2)+"\n")
            (evidence / "input-result.json").write_text(json.dumps({
                "passed": True, "stages": stages, "heldRepeatSuppressed": True,
                "backgroundKeyIgnoredAfterFocusReturn": True,
                "flightChecked": flight,
                "boarAudioChecked": boar_audio,
                "mouseCapturedAndReleasedAfterToggle": True}, indent=2)+"\n")
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
    parser.add_argument("--diving", action="store_true", help="exercise underwater descent and resurfacing")
    parser.add_argument("--boar-audio", action="store_true", help="exercise boar playback and mute through SDL disk output")
    parser.add_argument("--window-manager", type=Path,
                        default=shutil.which("openbox") or Path.home()/"deps/xtools/bin/openbox")
    parser.add_argument("--worker", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    build = args.build.resolve(strict=True)
    window_manager = args.window_manager.resolve(strict=True)
    if args.worker:
        worker(build, args.worker.resolve(strict=True), window_manager, args.flight, args.boar_audio, args.diving)
        return
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
        evidence = build / ("software-boar-audio" if args.boar_audio else "software-flight" if args.flight else "software-fullscreen") / stamp
        evidence.mkdir(parents=True)
        command = [str(platform / "wayland_test_server.sh"), "--compositor", "weston", "--renderer", "pixman",
                   str(platform / "run_gpu_tests_private.sh"), "--inside-private-compositor", "exec",
                   sys.executable, str(Path(__file__).resolve()), str(build), "--worker", str(evidence),
                   "--window-manager", str(window_manager)]
        if args.flight:
            command.append("--flight")
        if args.diving:
            command.append("--diving")
        if args.boar_audio:
            command.append("--boar-audio")
        environment = software_environment(evidence)
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY"):
            environment.pop(key, None)
        environment["ISLAND_FULLSCREEN_SUPERVISED"] = "1"
        environment["XDG_CONFIG_HOME"] = str(evidence / "config")
        if args.boar_audio:
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
        print(f"Native fullscreen evidence: {evidence}", flush=True)
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
        print("Native software fullscreen input passed; no new kernel GPU faults")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        sys.exit(f"Native fullscreen gate: {error}")
