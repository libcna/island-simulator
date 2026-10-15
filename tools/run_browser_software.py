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

SCOPES = ("full", "terrain", "cliffs", "sky", "distant", "canopy-light", "snags", "saplings", "wildlife-foliage", "coast", "shipwreck", "grounding", "vegetation", "palms", "mangroves", "bushes", "ground-broadleaf", "split-ground", "grass-motion", "litter", "materials", "stream", "wetland", "birds", "bats", "monkeys", "monkey-climb", "fullscreen", "flight", "snakes", "hut", "helicopter", "campfire", "cave", "bunker", "goats", "deer", "boars", "spiders", "turtles", "crabs", "hermit-crabs", "coconut-crabs", "butterflies", "geckos", "skinks", "dragonflies", "frogs", "rodents", "flies", "ants", "cicadas", "beetles", "crocodiles", "fish", "stingrays", "octopuses", "shark", "dolphins", "bear", "resident")


def validate_shipwreck_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "shipwreck":
        raise RuntimeError("shipwreck report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frames = (view.get("screenshot"), view.get("afterSimulationScreenshot"))
    intervals = view.get("intervals", [])
    required = (
        "shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas",
        "shipwreck weathered wood follows per-piece board and beam UVs",
        "shipwreck restrained rusted iron straps, rivets, hinges and rudder braces",
        "shipwreck hull uses asymmetric rising sheer, curved keel, and irregular splintered break sections",
        "shipwreck beach broadside has true missing plank bays with exposed structural ribs",
        "shipwreck bow and stern use raked lower profiles with thick end timbers",
        "shipwreck missing plank bays expose eleven-centimetre inner edges",
        "shipwreck stern deckhouse uses separated broken roof planks and framed window bays",
        "shipwreck 20m broken hull, 12 debris pieces, one portable culled batch, 5532 triangles",
        "shipwreck visible, 12 debris, 1 draw",
    )
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
        or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                   and math.isfinite(value) and value > 0 for value in intervals)
        or not all(isinstance(frame, str) and frame.endswith(".png") for frame in frames)
        or len(set(frames)) != 2 or not all(marker in log for marker in required)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("shipwreck view lacks current hull, material, frames or clean graphics state")
    return frames


def validate_cave_report(report):
    views = report.get("views", [])
    if (report.get("passed") is not True
        or [view.get("mode") for view in views] != ["cave", "cave-inside"]):
        raise RuntimeError("cave report lacks successful exterior and interior views")
    frames = []
    for view in views:
        result = view.get("result", {})
        log = result.get("output", "")
        intervals = view.get("intervals", [])
        frame = view.get("screenshot")
        side = "inside" if view.get("mode") == "cave-inside" else "outside"
        if (view.get("passed") is not True or view.get("glError") != 0
            or view.get("contextLost") is not False or result.get("error") or result.get("exited")
            or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 60
            or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                       and math.isfinite(value) and value > 0 for value in intervals)
            or not isinstance(frame, str) or not frame.endswith(".png")
            or f"cave QA camera {side}" not in log
            or "cave rock first visible draw" not in log
            or "cave rock 2623 triangles in one culled draw; 20m walkable interior" not in log
            or "cave terrain shoulders use an irregular rock apron around the open corridor" not in log
            or "cave connected shell strips share smooth lighting normals while portal breaks stay sharp" not in log
            or "cave adds six high-clearance eroded ceiling pendants in the existing draw" not in log
            or "cave mouth is embedded in a rising inland mountain slope" not in log
            or "cave crown is concealed from above by blended earth and 371 irregular three-blade tufts in the existing draw" not in log
            or "cave rock loaded Content/structures/cave-rock.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored triplanar sampling" not in log
            or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
            raise RuntimeError("cave view lacks current material, terrain blend or clean graphics state")
        frames.append(frame)
    return frames


def validate_bunker_report(report):
    expected = ["bunker", "bunker-corridor", "bunker-computers", "bunker-reactor",
                "bunker-service", "bunker-end"]
    views = report.get("views", [])
    if report.get("passed") is not True or [view.get("mode") for view in views] != expected:
        raise RuntimeError("bunker report lacks exterior, corridor, room and sealed-end views")
    frames = []
    required = (
        "concealed inland bunker has an open rock portal, descending access corridor, computer room, service room, and fictional reactor-generator chamber",
        "bunker uses bounded baked light pools, luminous ceiling fixtures, active terminal screens, and a contained cyan generator core in two static draws",
        "bunker computer room has four desks and terminals, keyboard consoles, eight server cabinets, and a separate generator control bank",
        "bunker concrete and metal use portable world-space mottling, restrained age variation, and baked face shading",
        "bunker deep spine ends at a sealed maintenance bulkhead and the service branch has a complete lit shell",
    )
    for view in views:
        result = view.get("result", {})
        log = result.get("output", "")
        intervals = view.get("intervals", [])
        frame = view.get("screenshot")
        if (view.get("passed") is not True or view.get("glError") != 0
            or view.get("contextLost") is not False or result.get("error") or result.get("exited")
            or "SwiftShader" not in view.get("renderer", "") or len(intervals) != 12
            or not all(isinstance(value, (int, float)) and not isinstance(value, bool)
                       and math.isfinite(value) and value > 0 for value in intervals)
            or not isinstance(frame, str) or not frame.endswith(".png")
            or f"bunker QA camera {view.get('mode')}" not in log
            or not all(marker in log for marker in required)
            or re.search(r"bunker material fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",
                         log, re.I)):
            raise RuntimeError("bunker view lacks current shell, material, closure or clean graphics state")
        frames.append(frame)
    return frames


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
            or "helicopter uses a smoothly tapered thirty-two-sided cockpit canopy, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane" not in log
            or "helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges" not in log
            or "helicopter rotor head adds four blade grips, pitch links, a mast cap, and muted yellow tip bands" not in log
            or "helicopter raised tapered engine cowling feeds two rear exhaust outlets behind the rotor mast" not in log
            or "helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber" not in log
            or "helicopter carries four outward-open framed door panels with glazing and handles" not in log
            or "helicopter cockpit uses four round gauges, paired articulated cyclic and collective controls, and four pedals" not in log
            or "helicopter realism pass adds a rounded chin and tail-root fairing, twelve-sided six-station tail boom, maintenance seams, engine intakes, shaped cushions, harnesses, centre pedestal, and overhead console" not in log
            or "helicopter curved shell uses analytic per-vertex lighting across the canopy, cowling, tail boom, fairings, and cushions without added geometry" not in log
            or not re.search(r"helicopter 2 draw", log)
            or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
            raise RuntimeError("helicopter view lacks current glazing, bounded draws or clean graphics state")
        frames.append(frame)
    return frames


def validate_campfire_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "campfire":
        raise RuntimeError("campfire report lacks one successful focused view")
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
        or "campfire QA camera near" not in log
        or "campfire first visible animated draw" not in log
        or "campfire flame material uses broken translucent tongues, pale cores, a restrained blue base, and layered smoke" not in log
        or "campfire bed uses a pale mottled ash centre, scorched rim, and mixed glowing charcoal" not in log
        or "campfire and stored firewood stand on qualified broad dry-sand beach" not in log
        or "campfire 1992 solid triangles, 42 flame/smoke/spark triangles in two culled draws" not in log
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("campfire view lacks current moving material, frames or clean graphics state")
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
        or "bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes" not in log
        or "near bear uses twenty-two body-envelope fur wedges without another draw" not in log
        or "near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes" not in log
        or "bear legs use continuous shoulder-to-wrist lofts and five-clawed broad paws" not in log
        or "bear torso uses a continuous rump-to-neck loft without overlapping body shells" not in log
        or "near bear uses a broader rounded skull, fitted brow volumes, larger rounded ears, and a short wide muzzle" not in log
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
        or "resident uses a shoulder-sloped woven tunic with open cap sleeves and anatomically tapered calves" not in log
        or "resident uses buried deltoid shoulders, split hands, and layered facial anatomy" not in log
        or "resident anatomy uses a tapered trapezius neck, rounded sleeve-deltoid transitions, shaped forearms, cheeks, jaw, eyelids, and nostril wings" not in log
        or "resident woven clothing adds a bound V neckline and split thigh-following wrap" not in log
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
        or "near boar lower legs use a continuous knee-to-cannon taper without spherical joint caps" not in log
        or "boars use a lifted shoulder mantle and broad sealed nasal disc" not in log
        or "near boars retain warm shoulder, cheek, nose, and eye contrast in canopy shade" not in log
        or "boar face retains a warm nasal disc and visible amber eye catchlight in wet understory" not in log
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
        or "near dolphins use closed-volume fins and flukes, short beaks, faceted eyes, mouth creases, and a blowhole" not in log
        or "near dolphins use a nineteen-station closed melon profile, twenty-four-sided body, and vertex-blended countershading" not in log
        or "dolphin tail stock bends vertically through horizontal fluke strokes" not in log
        or "near dolphins use an extended fitted rostrum, lower swept fins, and readable grey countershading" not in log
        or "dolphin facial pass uses a stout tapered rostrum and small six-facet oval eyes instead of flat diamond marks" not in log
        or "dolphin skin pass uses continuous axial pigment, a pale swept lower flank, restrained tonal variation, a longer rounded rostrum, and softer swept fins" not in log
        or "dolphin near/far geometry 3168/15 vertices, closed fins and CPU faceted daylight" not in log
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
        or "shark near/far geometry 1896/15 vertices, closed fins and CPU faceted daylight" not in log
        or "near shark uses a smooth-lit blunt head and body, caudal peduncle and heterocercal tail" not in log
        or "near shark uses a fourteen-station eighteen-sided capped body, closed fins, five paired gill slits, and fitted eyes" not in log
        or "shark uses a long low spindle, reinforced caudal peduncle, and swept restrained fins" not in log
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
        or "near fish use eleven-station sixteen-sided bodies, closed swept fins, and fitted highlighted eyes" not in log
        or "near fish use softly interpolated family flank lines, bars, and caudal marks" not in log
        or "fish use narrow caudal peduncles and shallow swept forked tails" not in log
        or "near fish use capped smooth-lit bodies, caudal peduncles and progressive rear-body bends" not in log
        or "fish profile pass uses six-facet oval eyes, tapered fork tips, and a lower rear-swept dorsal" not in log
        or not re.search(r"fish [1-9][0-9]*/48 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("fish view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_stingray_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="stingrays":
        raise RuntimeError("stingray report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frame=view.get("screenshot");intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not isinstance(frame,str) or not frame.endswith(".png")
        or "stingrays use broad undulating discs, raised eyes, dorsal spots, pale undersides, and long tapered tails in one draw" not in log
        or "stingrays 5 agents glide, forage, and settle over shallow sandy seabed" not in log
        or "stingray QA camera follows one shallow-sand glide" not in log
        or not re.search(r"stingrays [1-5]/5 visible \(near [1-5], far [0-5]\), 1 draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("stingray view lacks current anatomy or clean graphics state")
    return frame


def validate_octopus_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="octopuses":
        raise RuntimeError("octopus report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frame=view.get("screenshot");intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not isinstance(frame,str) or not frame.endswith(".png")
        or "octopuses use pulsing mantles, expressive eyes, eight independently curled arms, and pale sucker rows in one draw" not in log
        or "octopuses 4 agents den, crawl, forage, and swim in reef and rocky shallows" not in log
        or "octopus QA camera frames one exposed reef forager and all eight arms" not in log
        or not re.search(r"octopuses [1-4]/4 visible \(near [1-4], far [0-4]\), 1 draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("octopus view lacks current anatomy or clean graphics state")
    return frame


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
        or "near crabs use tapered toes, socketed eyes, and solid pincer fingers" not in log
        or "near crabs use flattened lateral carapaces, sturdy legs, and enlarged claw palms" not in log
        or "near crabs use eighteen-sided smooth-lit mottled carapaces and solid tapered legs" not in log
        or "near crabs use low broad shells with retained lateral carapace corners" not in log
        or "six debris-linked crab colonies share visible sand burrows" not in log
        or "live beach crabs keep 0.72m centre separation while foraging" not in log
        or "crab QA camera frames a debris-linked burrow and its foraging colony" not in log
        or not re.search(r"crab QA subject [0-9]+ is [0-9.]+m from the burrow", log)
        or not re.search(r"crabs [1-9][0-9]*/30 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("crab view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_butterfly_report(report):
    views = report.get("views", [])
    if (report.get("passed") is not True or len(views) != 1
        or views[0].get("mode") != "butterflies"):
        raise RuntimeError("butterfly report lacks one successful focused view")
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
        or "butterflies feed at three visible deterministic nectar-flower patches" not in log
        or "butterfly QA camera frames one concrete nectar patch and its six animals" not in log
        or not re.search(r"butterflies [1-9][0-9]*/18 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("butterfly view lacks nectar habitat, frames or clean graphics state")
    return frames


def validate_gecko_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="geckos":
        raise RuntimeError("gecko report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "eight inland geckos rest, scurry from the player, and hide in the shared butterfly batch" not in log
        or "gecko QA camera frames a resting inland lizard at ground level" not in log
        or not re.search(r"geckos [1-8]/8 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("gecko view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_skink_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="skinks":
        raise RuntimeError("skink report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "eight open-ground skinks bask, forage, and flee in the shared butterfly batch" not in log
        or "skinks use smooth elongated trunks, short articulated limbs, tapered tails, longitudinal stripes, and highlighted eyes" not in log
        or "skink QA camera frames one striped basking lizard on open inland ground" not in log
        or not re.search(r"skinks [1-8]/8 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("skink view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_dragonfly_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="dragonflies":
        raise RuntimeError("dragonfly report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "nine stream and wetland dragonflies hover, patrol, and dart in the shared butterfly batch" not in log
        or "dragonfly QA camera frames one visible wet-habitat insect broadside" not in log
        or not re.search(r"dragonflies [1-9]/9 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("dragonfly view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_frog_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="frogs":
        raise RuntimeError("frog report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "ten stream and wetland frogs rest, hop, and retreat in the shared butterfly batch" not in log
        or "frog QA camera frames one resting stream-bank animal" not in log
        or not re.search(r"frogs [1-9][0-9]*/10 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("frog view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_rodent_report(report):
    views=report.get("views",[])
    if (report.get("passed") is not True or len(views)!=2
        or [view.get("mode") for view in views] != ["mice", "rats"]):
        raise RuntimeError("rodent report lacks successful mouse and rat views")
    all_frames=[]
    for view,species,total,camera_marker in (
        (views[0],"mice",12,"mouse QA camera frames one field mouse at ground level"),
        (views[1],"rats",8,"rat QA camera frames one larger inland rat at ground level")):
        result=view.get("result",{});log=result.get("output","")
        frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
        intervals=view.get("intervals",[])
        if (view.get("passed") is not True or view.get("glError")!=0
            or view.get("contextLost") is not False or result.get("error") or result.get("exited")
            or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
            or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                       and math.isfinite(value) and value>0 for value in intervals)
            or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
            or len(set(frames))!=2
            or "twelve field mice and eight rats forage, scurry, and hide in the shared small-fauna batch" not in log
            or "near rodents use species-scaled lofted bodies, pointed muzzles, rounded ears, whiskers, paws, eyes, and long segmented tails" not in log
            or camera_marker not in log
            or not re.search(rf"{species} [1-9][0-9]*/{total} visible in shared draw",log)
            or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
            raise RuntimeError(f"{species} view lacks current anatomy, frames or clean graphics state")
        all_frames.extend(frames)
    return all_frames


def validate_fly_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="flies":
        raise RuntimeError("fly report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "four close-range fly swarms use twenty-four hover, orbit, and dart agents in the shared small-fauna batch" not in log
        or "near flies use compact dark thoraxes, tapered abdomens, highlighted eyes, six legs, and two beating pale wings" not in log
        or "fly QA camera frames one close insect within its inland swarm" not in log
        or not re.search(r"flies [1-9][0-9]*/24 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("fly view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_ant_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="ants":
        raise RuntimeError("ant report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "four close-range ant trails use sixty-four soil and trunk-climbing agents in the shared small-fauna batch" not in log
        or "close ants use three-part bodies, six articulated legs, paired antennae, and visible carried leaf fragments" not in log
        or "ant QA camera frames a visible worker on one inland soil trail" not in log
        or not re.search(r"ants [1-9][0-9]*/64 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("ant view lacks current trails, anatomy or clean graphics state")
    return frames


def validate_cicada_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="cicadas":
        raise RuntimeError("cicada report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "twelve cicadas rest, call, and make short flights from real broadleaf trunks in the shared small-fauna batch" not in log
        or "close cicadas use broad heads, highlighted eyes, stout tapered bodies, six legs, and two veined roof-folded wings" not in log
        or "cicada QA camera frames one trunk-resting animal broadside" not in log
        or not re.search(r"cicadas [1-9][0-9]*/12 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("cicada view lacks current habitat, anatomy or clean graphics state")
    return frames


def validate_beetle_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="beetles":
        raise RuntimeError("beetle report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "fifteen tropical ground beetles rest, forage, and hide in three inland leaf-litter colonies" not in log
        or "close beetles use domed paired elytra, a centre seam, pronotum, head, antennae, eyes, and six articulated legs" not in log
        or "beetle QA camera frames one foraging animal in inland leaf litter" not in log
        or not re.search(r"beetles [1-9][0-9]*/15 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("beetle view lacks current habitat, anatomy or clean graphics state")
    return frames


def validate_hermit_crab_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="hermit-crabs":
        raise RuntimeError("hermit-crab report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "ten sheltered-beach hermit crabs pause, forage, and retreat in the shared crab batch" not in log
        or "hermit crabs use raised spiral shells, projecting eyes and antennae, six walking legs, and unequal claws" not in log
        or "hermit-crab QA camera frames one shell-bearing animal on sheltered sand" not in log
        or not re.search(r"hermit crabs [1-9][0-9]*/10 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("hermit-crab view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_coconut_crab_report(report):
    views=report.get("views",[])
    if report.get("passed") is not True or len(views)!=1 or views[0].get("mode")!="coconut-crabs":
        raise RuntimeError("coconut-crab report lacks one successful focused view")
    view=views[0];result=view.get("result",{});log=result.get("output","")
    frames=(view.get("screenshot"),view.get("afterSimulationScreenshot"))
    intervals=view.get("intervals",[])
    if (view.get("passed") is not True or view.get("glError")!=0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or "SwiftShader" not in view.get("renderer","") or len(intervals)!=60
        or not all(isinstance(value,(int,float)) and not isinstance(value,bool)
                   and math.isfinite(value) and value>0 for value in intervals)
        or not all(isinstance(frame,str) and frame.endswith(".png") for frame in frames)
        or len(set(frames))!=2
        or "six coastal palm-zone coconut crabs rest, forage, and retreat in the shared crab batch" not in log
        or "coconut crabs use broad armored bodies, segmented terrestrial abdomens, robust walking legs, and massive unequal claws" not in log
        or "coconut-crab QA camera frames one large terrestrial animal in coastal vegetation" not in log
        or not re.search(r"coconut crabs [1-6]/6 visible in shared draw",log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high",log,re.I)):
        raise RuntimeError("coconut-crab view lacks current anatomy, frames or clean graphics state")
    return frames


def validate_crocodile_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "crocodiles":
        raise RuntimeError("crocodile report lacks one successful focused view")
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
        or "crocodiles use long low armored bodies, broad skulls, muscular tails, and planted splayed limbs" not in log
        or "near crocodiles use raised eyes, nostrils, separate jaws, teeth, dorsal osteoderms, and wet mottled facet shading" not in log
        or "crocodile QA camera frames an estuary animal broadside" not in log
        or not re.search(r"crocodiles [1-3]/3 visible \(near [1-3], far [0-3]\), 1 draw", log)
        or re.search(r"fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high", log, re.I)):
        raise RuntimeError("crocodile view lacks current anatomy, frames or clean graphics state")
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
        or "near turtles use staggered scutes and closed tapered-volume paddles" not in log
        or "near turtles use compact integrated heads, narrower hydrodynamic paddles, and restrained shell seams" not in log
        or "near turtles use a twenty-by-sixteen carapace surface, eighteen-sided seven-ring heads, and irregular mottled shell and skin tones" not in log
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
        or "stream water uses bank-edge sheen and broken shallow riffles" not in log
        or "stream water uses depth-graded colour and braided moving current threads" not in log
        or "stream channel uses shared low-frequency pools and pinches" not in log
        or not re.search(r"stream-bank stones (?:1[0-8]), non-solid existing rock variants beyond the carved bank", log)
        or "stream terrain uses a compact incised profile and slope-limited exposed bank strata" not in log
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


def validate_deer_report(report):
    views = report.get("views", [])
    if report.get("passed") is not True or len(views) != 1 or views[0].get("mode") != "deer":
        raise RuntimeError("deer report lacks one successful focused view")
    view = views[0]
    result = view.get("result", {})
    log = result.get("output", "")
    frame = view.get("screenshot")
    if (view.get("passed") is not True or view.get("glError") != 0
        or view.get("contextLost") is not False or result.get("error") or result.get("exited")
        or not re.search(r"deer [1-4]/4 visible, 1 draw", log)
        or "four inland deer use long legs, pointed ears, spotted coats and one antlered silhouette" not in log
        or "deer QA camera frames one antlered clearing animal broadside" not in log
        or "fallback:" in log
        or not isinstance(frame, str) or not frame.endswith(".png")):
        raise RuntimeError("deer view lacks current anatomy, visible batch or live graphics state")
    return (frame,)


def validate_hut_report(report):
    views = report.get("views", [])
    if not report.get("passed") or [v.get("mode") for v in views] != ["hut", "hut-material"]:
        raise RuntimeError("hut report lacks both successful views")
    for view in views:
        log = view.get("result", {}).get("output", "")
        # Counts are after frustum culling: the close camera sees six of seven patches.
        if (not view.get("passed") or view.get("glError") != 0 or view.get("contextLost") is not False
            or "hut beam-aligned weathered grain and filtered thatch fibres" not in log
            or "hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation" not in log
            or "hut four-pane muted glazing and braced plank door" not in log
            or "hut raised veranda uses full-height outer posts, split rails, fascia, and three steps" not in log
            or "hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports" not in log
            or "hut wall boards meet edge-to-edge on all four sides and both gables without see-through gaps" not in log
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
        if "snakes use a readable dorsal saddle and active forked-tongue flicks" not in log:
            raise RuntimeError("snake browser report lacks the current anatomy material")
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
            if args.scope == "cave":
                cave_report=json.loads((evidence / "island-browser-cave.json").read_text())
                frames=validate_cave_report(cave_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("cave image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("cave exterior and interior frames are byte-identical")
                metadata["cave_frames"]=identities
            if args.scope == "bunker":
                bunker_report=json.loads((evidence / "island-browser-bunker.json").read_text())
                frames=validate_bunker_report(bunker_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("bunker image file missing")
                identities=[artifact_identity(path) for path in paths]
                if len({identity["sha256"] for identity in identities}) != len(identities):
                    raise RuntimeError("bunker exterior/interior evidence contains duplicate views")
                metadata["bunker_frames"]=identities
            if args.scope == "campfire":
                campfire_report=json.loads((evidence / "island-browser-campfire.json").read_text())
                frames=validate_campfire_report(campfire_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("campfire image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("campfire separated-time frames are byte-identical")
                metadata["campfire_frames"]=identities
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
            if args.scope == "butterflies":
                butterfly_report=json.loads((evidence / "island-browser-butterflies.json").read_text())
                frames=validate_butterfly_report(butterfly_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("butterfly image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("butterfly separated-time frames are byte-identical")
                metadata["butterfly_frames"]=identities
            if args.scope == "geckos":
                gecko_report=json.loads((evidence / "island-browser-geckos.json").read_text())
                frames=validate_gecko_report(gecko_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("gecko image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("gecko separated-time frames are byte-identical")
                metadata["gecko_frames"]=identities
            if args.scope == "skinks":
                skink_report=json.loads((evidence / "island-browser-skinks.json").read_text())
                frames=validate_skink_report(skink_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("skink image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("skink separated-time frames are byte-identical")
                metadata["skink_frames"]=identities
            if args.scope == "dragonflies":
                dragonfly_report=json.loads((evidence / "island-browser-dragonflies.json").read_text())
                frames=validate_dragonfly_report(dragonfly_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("dragonfly image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("dragonfly separated-time frames are byte-identical")
                metadata["dragonfly_frames"]=identities
            if args.scope == "frogs":
                frog_report=json.loads((evidence / "island-browser-frogs.json").read_text())
                frames=validate_frog_report(frog_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("frog image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("frog separated-time frames are byte-identical")
                metadata["frog_frames"]=identities
            if args.scope == "rodents":
                rodent_report=json.loads((evidence / "island-browser-rodents.json").read_text())
                frames=validate_rodent_report(rodent_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("rodent image file missing")
                identities=[artifact_identity(path) for path in paths]
                if (identities[0]["sha256"]==identities[1]["sha256"]
                    or identities[2]["sha256"]==identities[3]["sha256"]):
                    raise RuntimeError("rodent separated-time frames are byte-identical")
                metadata["rodent_frames"]=identities
            if args.scope == "flies":
                fly_report=json.loads((evidence / "island-browser-flies.json").read_text())
                frames=validate_fly_report(fly_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("fly image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("fly separated-time frames are byte-identical")
                metadata["fly_frames"]=identities
            if args.scope == "ants":
                ant_report=json.loads((evidence / "island-browser-ants.json").read_text())
                frames=validate_ant_report(ant_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("ant image file missing")
                metadata["ant_frames"]=[artifact_identity(path) for path in paths]
            if args.scope == "cicadas":
                cicada_report=json.loads((evidence / "island-browser-cicadas.json").read_text())
                frames=validate_cicada_report(cicada_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("cicada image file missing")
                metadata["cicada_frames"]=[artifact_identity(path) for path in paths]
            if args.scope == "beetles":
                beetle_report=json.loads((evidence / "island-browser-beetles.json").read_text())
                frames=validate_beetle_report(beetle_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("beetle image file missing")
                metadata["beetle_frames"]=[artifact_identity(path) for path in paths]
            if args.scope == "hermit-crabs":
                hermit_report=json.loads((evidence / "island-browser-hermit-crabs.json").read_text())
                frames=validate_hermit_crab_report(hermit_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("hermit-crab image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("hermit-crab separated-time frames are byte-identical")
                metadata["hermit_crab_frames"]=identities
            if args.scope == "coconut-crabs":
                coconut_report=json.loads((evidence / "island-browser-coconut-crabs.json").read_text())
                frames=validate_coconut_crab_report(coconut_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("coconut-crab image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"]==identities[1]["sha256"]:
                    raise RuntimeError("coconut-crab separated-time frames are byte-identical")
                metadata["coconut_crab_frames"]=identities
            if args.scope == "crocodiles":
                crocodile_report=json.loads((evidence / "island-browser-crocodiles.json").read_text())
                frames=validate_crocodile_report(crocodile_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("crocodile image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("crocodile separated-time frames are byte-identical")
                metadata["crocodile_frames"]=identities
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
            if args.scope == "stingrays":
                stingray_report=json.loads((evidence / "island-browser-stingrays.json").read_text())
                frame=validate_stingray_report(stingray_report)
                path=evidence/frame
                if not path.is_file():
                    raise RuntimeError("stingray image file missing")
                metadata["stingray_frame"]=artifact_identity(path)
            if args.scope == "octopuses":
                octopus_report=json.loads((evidence / "island-browser-octopuses.json").read_text())
                frame=validate_octopus_report(octopus_report)
                path=evidence/frame
                if not path.is_file():
                    raise RuntimeError("octopus image file missing")
                metadata["octopus_frame"]=artifact_identity(path)
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
            if args.scope == "shipwreck":
                shipwreck_report=json.loads((evidence / "island-browser-shipwreck.json").read_text())
                frames=validate_shipwreck_report(shipwreck_report)
                paths=[evidence / frame for frame in frames]
                if not all(path.is_file() for path in paths):
                    raise RuntimeError("shipwreck separated-time image file missing")
                identities=[artifact_identity(path) for path in paths]
                if identities[0]["sha256"] == identities[1]["sha256"]:
                    raise RuntimeError("shipwreck separated-time frames are byte-identical")
                metadata["shipwreck_frames"]=identities
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
            if args.scope == "deer":
                deer_report=json.loads((evidence / "island-browser-deer.json").read_text())
                frames=validate_deer_report(deer_report)
                if not all((evidence / frame).is_file() for frame in frames):
                    raise RuntimeError("deer image file missing")
                metadata["deer_frames"]=[artifact_identity(evidence / frame) for frame in frames]
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
