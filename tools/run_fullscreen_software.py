#!/usr/bin/env python3
"""Native F11 input QA, owned private display and mandatory software/kernel supervision."""
from __future__ import annotations

import argparse
import datetime
import fcntl
import json
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


def worker(build, evidence, window_manager):
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
            if "debug teleport monkey troop" in log():
                raise RuntimeError("F11 still teleported to monkeys")
            validate_graphics_log(log(), None)
            (evidence / "input-result.json").write_text(json.dumps({
                "passed": True, "stages": stages, "heldRepeatSuppressed": True,
                "backgroundKeyIgnoredAfterFocusReturn": True,
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
    parser.add_argument("--window-manager", type=Path,
                        default=shutil.which("openbox") or Path.home()/"deps/xtools/bin/openbox")
    parser.add_argument("--worker", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    build = args.build.resolve(strict=True)
    window_manager = args.window_manager.resolve(strict=True)
    if args.worker:
        worker(build, args.worker.resolve(strict=True), window_manager)
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
        evidence = build / "software-fullscreen" / stamp
        evidence.mkdir(parents=True)
        command = [str(platform / "wayland_test_server.sh"), "--compositor", "weston", "--renderer", "pixman",
                   str(platform / "run_gpu_tests_private.sh"), "--inside-private-compositor", "exec",
                   sys.executable, str(Path(__file__).resolve()), str(build), "--worker", str(evidence),
                   "--window-manager", str(window_manager)]
        environment = software_environment(evidence)
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY"):
            environment.pop(key, None)
        environment["ISLAND_FULLSCREEN_SUPERVISED"] = "1"
        environment["XDG_CONFIG_HOME"] = str(evidence / "config")
        metadata = {"result":"running", "command":command, "kernel_before":baseline,
                    "artifact":artifact_identity(build / "island-simulator"),
                    "window_manager":artifact_identity(window_manager),
                    "harness":artifact_identity(Path(__file__)),
                    "window_manager_config":artifact_identity(Path(__file__).with_name("fullscreen_openbox.xml")),
                    "scope":"private llvmpipe native fullscreen input; not hardware qualification"}
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
