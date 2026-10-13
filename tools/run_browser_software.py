#!/usr/bin/env python3
"""Supervise one software browser suite, with no host GPU devices and no uncontained fallback."""
from __future__ import annotations

import argparse
import datetime
import fcntl
import json
import math
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys

from gpu_incident_qualification import FAULT, Journal, MonitorError, artifact_identity
from run_software_capture import (run_monitored_capture, validate_monkey_climb_stages,
    validate_snake_runtime_samples, validate_spider_render_log, validate_spider_lighting)

SCOPES = ("full", "terrain", "wildlife-foliage", "coast", "grounding", "vegetation", "grass-motion", "litter", "materials", "stream", "monkeys", "monkey-climb", "fullscreen", "flight", "snakes", "hut", "helicopter", "goats", "boars", "spiders", "turtles", "crabs", "fish", "shark", "dolphins", "bear", "resident")


def validate_helicopter_report(report):
    views = report.get("views", [])
    if (report.get("passed") is not True
        or [view.get("mode") for view in views] != ["helicopter", "helicopter-inside"]):
        raise RuntimeError("helicopter report lacks successful exterior and interior views")
    frames = []
    for view in views:
        result = view.get("result", {})
        log = result.get("output", "")
        intervals = view.get("intervals", [])
        frame = view.get("screenshot")
        if (view.get("passed") is not True or view.get("glError") != 0
            or view.get("contextLost") is not False or result.get("error") or result.get("exited")
            or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
            or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                       and math.isfinite(value) and value > 0 for value in intervals)
            or not isinstance(frame, str) or not frame.endswith(".png")
            or "helicopter uses rounded twenty-sided cockpit glazing, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane" not in log
            or not re.search(r"helicopter 2 draw", log)
            or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
            raise RuntimeError("helicopter view lacks current glazing, bounded draws or clean graphics state")
        frames.append(frame)
    return frames


def validate_bear_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "bear":
        raise RuntimeError("bear report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "bear QA camera frames and follows the single peaceful adult" not in log
        or "near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes" not in log
        or not re.search(r"bear 1/1 visible, 2 draws, [1-9][0-9]* body triangles", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("bear view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_resident_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "resident":
        raise RuntimeError("resident report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "resident QA camera frames and follows one peaceful adult" not in log
        or "resident uses a shoulder-sloped sleeveless woven tunic and anatomically tapered calves" not in log
        or not re.search(r"resident 1/1 visible, 2 draws, [1-9][0-9]* body triangles", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("resident view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_boar_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "boars":
        raise RuntimeError("boar report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "boar QA camera frames and follows non-hostile inland wildlife" not in log
        or "near boars use broad tapered upper legs with compact split hooves" not in log
        or not re.search(r"boars [1-6]/6 visible, 2 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("boar view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_dolphin_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "dolphins":
        raise RuntimeError("dolphin report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "near dolphins use short closed eight-facet beaks, shaped lit fins, faceted eyes, mouth creases, and a two-facet blowhole" not in log
        or "dolphin near/far geometry 558/15 vertices, single-surface fins and CPU faceted daylight" not in log
        or not re.search(r"dolphins [1-4]/4 visible \(near [1-4], far [0-4]\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("dolphin view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_shark_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "shark":
        raise RuntimeError("shark report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "near shark uses a rounded eight-station head, ten-facet body, lit shaped fins, five paired gill slits, and faceted eyes" not in log
        or not re.search(r"shark visible activity \S+, 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("shark view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_fish_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "fish":
        raise RuntimeError("fish report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "near fish use rounded seven-station heads, ten-facet bodies, lit fins, and two-facet eyes" not in log
        or not re.search(r"fish [1-9][0-9]*/48 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("fish view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_crab_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "crabs":
        raise RuntimeError("crab report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "near crabs use fixed-sun facets, tapered distal toes, and socketed eyes" not in log
        or not re.search(r"crabs [1-9][0-9]*/30 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("crab view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_turtle_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "turtles":
        raise RuntimeError("turtle report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2
        or "near turtles use staggered interlocking scutes and scaled paddle joints" not in log
        or not re.search(r"turtles [1-5]/5 visible \(near [1-5], far [0-5]\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("turtle view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_stream_report(report):
    views=report.get("views",[])
    if not report.get("passed") or len(views)!=1 or views[0].get("mode")!="stream":
        raise RuntimeError("stream report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    if (not view.get("passed") or view.get("glError")!=0 or view.get("contextLost") is not False
        or result.get("error") or result.get("exited") or "SwiftShader" not in view.get("renderer","")
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "route-following downstream ripple and translucent current streaks" not in log
        or "stream water first visible draw" not in log):
        raise RuntimeError("stream view lacks its live material, frames or clean graphics state")
    return frames


def validate_spider_report(report):
    views = report.get("views", [])
    if (report.get("passed") is not True or len(views) != 2
        or [view.get("mode") for view in views] != ["spiders", "spider-web"]):
        raise RuntimeError("spider report lacks successful ground and web views")
    view,web = views
    result = view.get("result", {})
    frames = view.get("spiderMotionFrames", [])
    intervals = view.get("intervals", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(v, (int, float)) and not isinstance(v, bool)
                   and math.isfinite(v) and v > 0 for v in intervals)
        or len(frames) != 4 or not all(isinstance(f, str) and f.endswith(".png")
                                     and Path(f).name == f for f in frames)
        or len(set(frames)) != 4):
        raise RuntimeError("spider view lacks pose images or live software graphics state")
    log = result.get("output", "")
    if re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I):
        raise RuntimeError("spider runtime reports material or graphics failure")
    validate_spider_render_log(log)
    validate_spider_lighting(log)
    web_result=web.get("result",{});web_log=web_result.get("output","")
    web_frame=web.get("screenshot")
    if (web.get("passed") is not True or web.get("glError") != 0
        or web.get("contextLost") is not False or web_result.get("error") or web_result.get("exited")
        or "SwiftShader" not in web.get("renderer","")
        or not isinstance(web_frame,str) or not web_frame.endswith(".png")
        or not re.search(r"spider-web render proof: webs [1-6]/6 visible, strands [1-9][0-9]*, 1 draw",web_log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",web_log,re.I)):
        raise RuntimeError("spider web view lacks visible strands or clean graphics state")
    return [*frames,web_frame]


def validate_goat_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "goats":
        raise RuntimeError("goat report lacks successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = view.get("goatMotionFrames", [])
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or not re.search(r"goats [1-6]/6 visible, 2 draw;", log)
        or "goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in log
        or "fallback:" in log or len(frames) != 4
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 4):
        raise RuntimeError("goat view lacks coat, visible population, pose images or live graphics state")
    return frames


def validate_hut_report(report):
    views = report.get("views", [])
    if not report.get("passed") or [v.get("mode") for v in views] != ["hut", "hut-material"]:
        raise RuntimeError("hut report lacks both successful views")
    for view in views:
        log = view.get("result", {}).get("output", "")
        # Counts are after frustum culling: the close camera sees six of seven patches.
        if (not view.get("passed") or view.get("glError") != 0 or view.get("contextLost") is not False
            or "hut beam-aligned weathered grain and filtered thatch fibres" not in log
            or "hut four-pane muted glazing and braced plank door" not in log
            or "hut raised veranda uses full-height outer posts, split rails, fascia, and three steps" not in log
            or "hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in log
            or f"hut 1 draw, {7 if view['mode'] == 'hut' else 6} contact patches" not in log
            or "material fallback:" in log):
            raise RuntimeError("hut view lacks qualified material, draw or graphics state")
    return [v["mode"] for v in views]


def validate_climb_report(report):
    views = report.get("views", [])
    if not report.get("passed") or len(views) != 1 or views[0].get("mode") != "monkey-climb":
        raise RuntimeError("browser climb report is incomplete or has the wrong view")
    log = views[0].get("result", {}).get("output", "")
    if "monkey climb held summit presented after eight cycle stages" not in log:
        raise RuntimeError("browser climb did not present the held summit")
    return validate_monkey_climb_stages(log)


def validate_snake_report(report):
    views=report.get("views", [])
    if not report.get("passed") or [v.get("mode") for v in views]!=["snakes","snake-head"]:
        raise RuntimeError("snake browser report lacks both successful views")
    result=[]
    for view in views:
        log=view.get("result", {}).get("output", "")
        result.append({"mode":view["mode"], **validate_snake_runtime_samples(log)})
    return result


def browser_environment(browser, scope, evidence):
    environment = os.environ.copy()
    for name in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "DRI_PRIME", "ISLAND_WEB_SMOKE_SCOPE"):
        environment.pop(name, None)
    environment.update(LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe", LP_NUM_THREADS="8",
                       ISLAND_BROWSER_SUPERVISED=browser, ISLAND_BROWSER_EVIDENCE=str(evidence))
    if scope != "full":
        environment["ISLAND_WEB_SMOKE_SCOPE"] = scope
    return environment


def browser_command(root, build, node, browser, scope):
    if browser not in ("chrome", "firefox") or scope not in SCOPES:
        raise ValueError("unsupported browser or scope")
    if browser == "firefox" and scope != "full":
        raise ValueError("Firefox supports only its complete existing suite, not focused scopes")
    script = "run_web_smoke.mjs" if browser == "chrome" else "run_firefox_web_smoke.mjs"
    return [str(node), str(root / "tools" / script), str(build)]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--browser", choices=("chrome", "firefox"), default="chrome")
    parser.add_argument("--scope", choices=SCOPES, default="terrain")
    parser.add_argument("--node", default=os.environ.get("EMSDK_NODE", "node"))
    parser.add_argument("--timeout", type=float, default=240)
    args = parser.parse_args(argv)
    if not 5 <= args.timeout <= 3600:
        parser.error("timeout must be between 5 and 3600 seconds")
    root = Path(__file__).resolve().parent.parent
    build = args.build.resolve(strict=True)
    node_path = shutil.which(args.node)
    if node_path is None:
        raise RuntimeError("Node executable missing; pass --node or set EMSDK_NODE")
    node = Path(node_path).resolve(strict=True)
    command = browser_command(root, build, node, args.browser, args.scope)
    if shutil.which("bwrap") is None:
        raise RuntimeError("bubblewrap is required; no uncontained browser fallback")
    # No browser/process profile is created until the package, runtime and journal are checked.
    artifacts = {name: artifact_identity(build / name) for name in (
        "island-simulator.html", "island-simulator.js", "island-simulator.wasm", "island-simulator.data")}
    runtime = subprocess.run([str(node), "-e", "if(typeof WebSocket==='undefined')process.exit(2)"],
                             capture_output=True, text=True, timeout=5)
    if runtime.returncode:
        raise RuntimeError("Node with global WebSocket is required (workspace Emscripten Node 24)")
    os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:8])
    # Share the existing native/Android lock; changing Web build paths cannot bypass it.
    with (root / "build/gpu-qualification.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        journal = Journal()
        baseline = journal.latest()
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
        evidence = root / "build/browser-software" / f"{stamp}-{args.browser}-{args.scope}"
        evidence.mkdir(parents=True)
        environment = browser_environment(args.browser, args.scope, evidence)
        metadata = {"command": command, "scope": "software-only; not AMD qualification",
                    "timeout_seconds": args.timeout, "kernel_before": baseline,
                    "artifacts": artifacts, "harness": artifact_identity(Path(command[1])),
                    "node": artifact_identity(node), "result": "running",
                    "forced_environment": {key: environment[key] for key in (
                        "LIBGL_ALWAYS_SOFTWARE", "GALLIUM_DRIVER", "LP_NUM_THREADS",
                        "ISLAND_BROWSER_SUPERVISED", "ISLAND_BROWSER_EVIDENCE")}}
        report = evidence / "run.json"
        report.write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"Browser software evidence: {evidence}", flush=True)
        failure = None
        try:
            result = run_monitored_capture(command, root, environment, evidence, journal, baseline,
                                           timeout=args.timeout, private_devices=True)
            if result:
                raise RuntimeError(f"browser suite exited {result}; inspect workload.log")
            if args.scope == "monkey-climb":
                climb = json.loads((evidence / "island-browser-monkey-climb.json").read_text())
                metadata["monkey_climb_stages"] = validate_climb_report(climb)
            if args.scope == "snakes":
                snake_report=json.loads((evidence / "island-browser-snakes.json").read_text())
                metadata["snake_motion_samples"]=validate_snake_report(snake_report)
            if args.scope == "hut":
                hut_report=json.loads((evidence / "island-browser-hut.json").read_text())
                metadata["hut_views"]=validate_hut_report(hut_report)
            if args.scope == "helicopter":
                helicopter_report=json.loads((evidence / "island-browser-helicopter.json").read_text())
                frames=validate_helicopter_report(helicopter_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("helicopter image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("helicopter exterior and interior frames are byte-identical")
                metadata["helicopter_frames"]=identities
            if args.scope == "turtles":
                turtle_report=json.loads((evidence / "island-browser-turtles.json").read_text())
                frames=validate_turtle_report(turtle_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("turtle image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("turtle separated-time frames are byte-identical")
                metadata["turtle_frames"]=identities
            if args.scope == "crabs":
                crab_report=json.loads((evidence / "island-browser-crabs.json").read_text())
                frames=validate_crab_report(crab_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("crab image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("crab separated-time frames are byte-identical")
                metadata["crab_frames"]=identities
            if args.scope == "fish":
                fish_report=json.loads((evidence / "island-browser-fish.json").read_text())
                frames=validate_fish_report(fish_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("fish image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("fish separated-time frames are byte-identical")
                metadata["fish_frames"]=identities
            if args.scope == "shark":
                shark_report=json.loads((evidence / "island-browser-shark.json").read_text())
                frames=validate_shark_report(shark_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("shark image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("shark separated-time frames are byte-identical")
                metadata["shark_frames"]=identities
            if args.scope == "dolphins":
                dolphin_report=json.loads((evidence / "island-browser-dolphins.json").read_text())
                frames=validate_dolphin_report(dolphin_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("dolphin image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("dolphin separated-time frames are byte-identical")
                metadata["dolphin_frames"]=identities
            if args.scope == "bear":
                bear_report=json.loads((evidence / "island-browser-bear.json").read_text())
                frames=validate_bear_report(bear_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("bear image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("bear separated-time frames are byte-identical")
                metadata["bear_frames"]=identities
            if args.scope == "resident":
                resident_report=json.loads((evidence / "island-browser-resident.json").read_text())
                frames=validate_resident_report(resident_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("resident image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("resident separated-time frames are byte-identical")
                metadata["resident_frames"]=identities
            if args.scope == "boars":
                boar_report=json.loads((evidence / "island-browser-boars.json").read_text())
                frames=validate_boar_report(boar_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("boar image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("boar separated-time frames are byte-identical")
                metadata["boar_frames"]=identities
            if args.scope == "goats":
                goat_report=json.loads((evidence / "island-browser-goats.json").read_text())
                metadata["goat_pose_images"]=validate_goat_report(goat_report)
            if args.scope == "spiders":
                spider_report=json.loads((evidence / "island-browser-spiders.json").read_text())
                frames=validate_spider_report(spider_report)
                if not all((evidence / frame).is_file() for frame in frames):
                    raise RuntimeError("spider pose image file missing")
                metadata["spider_pose_images"]=frames
            if args.scope == "stream":
                stream_report=json.loads((evidence/"island-browser-stream.json").read_text())
                frames=validate_stream_report(stream_report)
                paths=[evidence/frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("stream separated-time image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("stream separated-time frames are byte-identical")
                metadata["stream_frames"]=identities
            metadata["result"] = "passed"
        except (OSError, RuntimeError, subprocess.SubprocessError) as error:
            failure = error
        finally:
            try:
                final = journal.read("--after-cursor", baseline["__CURSOR"])
                (evidence / "kernel-final.json").write_text(json.dumps(final, indent=2) + "\n")
                if any(record["_BOOT_ID"] != baseline["_BOOT_ID"] or FAULT.search(record["MESSAGE"])
                       for record in final):
                    raise MonitorError("kernel fault or boot discontinuity after browser cleanup")
            except (OSError, RuntimeError) as error:
                failure = failure or error
            if failure:
                metadata.update(result="failed", error=str(failure))
            report.write_text(json.dumps(metadata, indent=2) + "\n")
        if failure:
            raise failure
        print("Software browser suite passed; no new kernel GPU faults detected.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        sys.exit(f"Browser software gate: {error}")
