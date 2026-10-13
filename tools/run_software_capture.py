#!/usr/bin/env python3
"""Capture one native OPENGL33 viewpoint using forced llvmpipe and private pixman.

This is a software visual check, NOT hardware incident qualification. The hardware failure latch
is never read as permission, changed, or removed. Requires kernel-journal access (use sg if needed).
"""
import argparse
import datetime
import fcntl
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time
from validate_capture import profile_for_viewpoint

from gpu_incident_qualification import (
    CAPTURE_VIEWPOINTS, FAULT, GL_ERROR, Journal, MonitorError, stop_process_group,
    contained_command, artifact_identity,
)


def validate_campfire_stages(log):
    rows=re.findall(r"campfire motion stage=(\d+) updates=(\d+) time=(\S+) draws=(\d+) effect-triangles=(\d+)",log)
    if len(rows)!=4 or [int(row[0]) for row in rows]!=list(range(4)):
        raise RuntimeError("campfire requires four ordered animation stages")
    for stage,updates,seconds,draws,triangles in rows:
        if (int(updates)!=(int(stage)+1)*8 or not math.isclose(float(seconds),int(updates)*.1,abs_tol=1e-5)
                or int(draws)!=2 or int(triangles)!=42):
            raise RuntimeError("invalid campfire animation clock or rendering budget")
    return rows


def validate_stream_stages(log):
    rows=re.findall(r"stream motion stage=(\d+) frames=(\d+) time=(\S+) draws=(\d+) triangles=(\d+)",log)
    if len(rows)!=4 or [int(row[0]) for row in rows]!=list(range(4)):
        raise RuntimeError("stream requires four ordered flow-animation stages")
    for stage,frames,seconds,draws,triangles in rows:
        expected=(int(stage)+1)*8
        if (int(frames)!=expected or not math.isclose(float(seconds),expected*.1,abs_tol=1e-5)
                or int(draws)!=1 or int(triangles)!=6080):
            raise RuntimeError("invalid stream animation clock or rendering budget")
    return rows


def validate_resident_stages(text):
    rows=re.findall(r"resident motion stage=(\d+) updates=(\d+) visible=(\d+) position=(\S+) "
                    r"speed=(\S+) mode=(\d+) feet=(\S+) triangles=(\d+)/(\d+)",text)
    if len(rows)!=8 or [int(row[0]) for row in rows]!=list(range(8)):
        raise RuntimeError("resident requires eight ordered gait stages")
    positions=[];feet=[]
    for stage,updates,visible,position,speed,mode,encoded_feet,body_triangles,shadow_triangles in rows:
        try:
            p=tuple(map(float,position.split(',')))
            pair=[tuple(map(float,foot.split(',')[:3])) for foot in encoded_feet.split(';')]
            valid=(int(updates)==(int(stage)+1)*8 and int(visible)==1 and len(p)==3
                   and all(map(math.isfinite,p)) and math.isfinite(float(speed))
                   and 0<=float(speed)<=1.01 and 0<=int(mode)<=3 and len(pair)==2
                   and all(len(foot)==3 and all(map(math.isfinite,foot)) for foot in pair)
                   and int(body_triangles)>=1000 and int(shadow_triangles)==24)
        except ValueError:
            valid=False
        if not valid:raise RuntimeError("invalid resident pose or bounded render budget")
        positions.append(p);feet.append(pair)
    displacement=max(math.dist(positions[0],p) for p in positions[1:])
    foot_motion=max(math.dist(feet[0][leg],pair[leg]) for pair in feet[1:] for leg in range(2))
    if displacement<1.0 or foot_motion<0.15:
        raise RuntimeError("resident did not complete visible body and planted-foot motion")
    return {"positions":positions,"displacement":displacement,"foot_motion":foot_motion}


def validate_bear_stages(text):
    rows=re.findall(r"bear motion stage=(\d+) updates=(\d+) visible=(\d+) position=(\S+) "
                    r"speed=(\S+) behavior=(\d+) forage=(\S+) feet=(\S+) triangles=(\d+)/(\d+)",text)
    if len(rows)!=8 or [int(row[0]) for row in rows]!=list(range(8)):
        raise RuntimeError("bear requires eight ordered locomotion stages")
    positions=[];feet=[]
    for stage,updates,visible,position,speed,behavior,forage,encoded_feet,body_triangles,shadow_triangles in rows:
        try:
            p=tuple(map(float,position.split(',')))
            paws=[tuple(map(float,foot.split(',')[:3])) for foot in encoded_feet.rstrip(';').split(';')]
            valid=(int(updates)==(int(stage)+1)*8 and int(visible)==1 and len(p)==3
                   and all(map(math.isfinite,p)) and 0<=float(speed)<=1.651
                   and 0<=int(behavior)<=5 and 0<=float(forage)<=1 and len(paws)==4
                   and all(len(paw)==3 and all(map(math.isfinite,paw)) for paw in paws)
                   and 1000<=int(body_triangles)<=12000 and int(shadow_triangles)==18)
        except ValueError:
            valid=False
        if not valid:raise RuntimeError("invalid bear pose or bounded render budget")
        positions.append(p);feet.append(paws)
    displacement=max(math.dist(positions[0],p) for p in positions[1:])
    paw_motion=max(math.dist(feet[0][leg],paws[leg]) for paws in feet[1:] for leg in range(4))
    if displacement<.35 or paw_motion<.10:
        raise RuntimeError("bear did not complete visible quadrupedal body and paw motion")
    return {"positions":positions,"displacement":displacement,"paw_motion":paw_motion}


def validate_snake_runtime_samples(log):
    """Validate sparse live-view motion, not continuous gait or visual quality."""
    rows = re.findall(r"snake runtime sample frame=(\d+) head=(\S+) speed=(\S+) visible=(\d+)", log)
    if len(rows) != 4 or log.count("snake runtime sample frame=") != 4 or [int(r[0]) for r in rows] != [8, 16, 24, 32]:
        raise RuntimeError("snake runtime motion samples incomplete or unordered")
    heads = []
    for _, point, speed, visible in rows:
        try:
            head = tuple(map(float, point.split(',')))
            valid = (len(head) == 3 and all(map(math.isfinite, head)) and math.isfinite(float(speed))
                     and 0 <= float(speed) <= 1.151 and 1 <= int(visible) <= 4)
        except ValueError:
            valid = False
        if not valid:
            raise RuntimeError("invalid snake runtime motion state")
        heads.append(head)
    displacement = max(math.dist(heads[0], h) for h in heads[1:])
    if displacement < 0.1:
        raise RuntimeError("snake stayed still during runtime motion review")
    return {"heads": heads, "displacement": displacement}


def validate_snake_motion_stages(text):
    """Require live, finite, connected body samples; not a visual gait-quality oracle."""
    rows = re.findall(r"^island-simulator: snake motion stage=(\d+) updates=(\d+) time=(\S+) visible=(\d+) "
                      r"speed=(\S+) phase=(\S+) body=(.*)$", text, re.M)
    if len(rows) != 4 or text.count("snake motion stage=") != 4 or [int(row[0]) for row in rows] != list(range(4)):
        raise RuntimeError("snake motion requires four ordered stages")
    bodies = []
    for stage, updates, elapsed, visible, speed, phase, encoded in rows:
        try:
            expected_updates=(int(stage)+1)*8
            if int(updates)!=expected_updates or not math.isclose(float(elapsed),expected_updates*0.1,abs_tol=0.00001):
                raise RuntimeError("snake capture simulation clock is not frame-deterministic")
            body = [tuple(map(float, point.split(','))) for point in encoded.rstrip(';').split(';')]
            valid = (1 <= int(visible) <= 4 and math.isfinite(float(speed))
                     and 0 <= float(speed) <= 1.151 and math.isfinite(float(phase))
                     and len(body) == 41 and all(len(p) == 3 for p in body)
                     and all(math.isfinite(v) for p in body for v in p))
        except ValueError:
            valid = False
        if not valid:
            raise RuntimeError("invalid snake motion sample")
        if any(abs(math.hypot(a[0]-b[0], a[2]-b[2])-.055) > .003
               for a, b in zip(body, body[1:])):
            raise RuntimeError("disconnected snake body in motion sample")
        bodies.append(body)
    if max(math.dist(bodies[0][0], body[0]) for body in bodies[1:]) < .01:
        raise RuntimeError("snake motion sequence did not move")
    return bodies


def validate_boar_motion_stages(text):
    """Validate the capture clock and finite poses, not artistic quality or gait."""
    rows = re.findall(r"^island-simulator: boar motion stage=(\d+) updates=(\d+) time=(\S+) visible=(\d+) "
                      r"position=(\S+) speed=(\S+) phase=(\S+) rooting=(\S+)$", text, re.M)
    if len(rows) != 8 or text.count("boar motion stage=") != 8 or [int(r[0]) for r in rows] != list(range(8)):
        raise RuntimeError("boar capture requires eight ordered stages")
    samples = []
    for stage, updates, elapsed, visible, point, speed, phase, rooting in rows:
        try:
            p = tuple(map(float, point.split(',')))
            s, ph, root, t = map(float, (speed, phase, rooting, elapsed))
            expected = (int(stage)+1)*8
            valid = (int(updates) == expected and math.isclose(t, expected*.1, abs_tol=1e-5)
                     and 1 <= int(visible) <= 6 and len(p) == 3 and all(map(math.isfinite, p))
                     and all(map(math.isfinite, (s, ph, root))) and 0 <= s <= 3
                     and 0 <= ph <= math.tau+1e-5 and 0 <= root <= 1)
        except ValueError:
            valid = False
        if not valid:
            raise RuntimeError("invalid boar capture state or clock")
        samples.append(dict(position=p, speed=s, phase=ph, rooting=root, time=t))
    return {"samples": samples}


def validate_goat_motion_stages(text):
    """Eight live samples: presentation/motion evidence, not an anatomy/contact oracle."""
    rows = re.findall(r"^island-simulator: goat motion stage=(\d+) updates=(\d+) time=(\S+) visible=(\d+) "
                      r"position=(\S+) speed=(\S+) phase=(\S+) grazing=(\S+)$", text, re.M)
    if len(rows) != 8 or text.count("goat motion stage=") != 8 or [int(r[0]) for r in rows] != list(range(8)):
        raise RuntimeError("goat motion requires eight ordered stages")
    positions, phases, grazing = [], [], []
    for stage, updates, elapsed, visible, point, speed, phase, graze in rows:
        try:
            p = tuple(map(float, point.split(',')))
            s, ph, g, t = map(float, (speed, phase, graze, elapsed))
            expected = (int(stage)+1)*8
            valid = (int(updates) == expected and math.isclose(t, expected*.1, abs_tol=1e-5)
                     and 1 <= int(visible) <= 6 and len(p) == 3 and all(map(math.isfinite, p))
                     and all(map(math.isfinite, (s, ph, g))) and 0 <= s <= 1.20001
                     and 0 <= ph <= math.tau+1e-5 and 0 <= g <= 1)
        except ValueError:
            valid = False
        if not valid:
            raise RuntimeError("invalid goat motion state or clock")
        positions.append(p); phases.append(ph); grazing.append(g)
    if any(math.dist(a, b) > .961 for a, b in zip(positions, positions[1:])):
        raise RuntimeError("goat motion exceeds maximum travel between samples")
    if max(math.dist(positions[0], p) for p in positions[1:]) < .15 or max(phases)-min(phases) < .1:
        raise RuntimeError("goat motion sequence did not walk")
    if max(grazing)-min(grazing) < .3:
        raise RuntimeError("goat motion sequence did not transition from grazing")
    return {"positions": positions, "phases": phases, "grazing": grazing}


def validate_spider_render_log(text):
    count = r"(?:[1-9]|[1-9][0-9]|1[0-9]{2}|200)"
    if not re.search(rf"spider render proof: spiders {count}/200 visible, 2 draw; contact-shadows={count}(?:\s|$)", text):
        raise RuntimeError("spider capture lacks a live population or grounding shadows")
    if "spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in text:
        raise RuntimeError("spider capture lacks the setae albedo")


def validate_spider_web_render_log(text):
    rows=re.findall(r"spider-web render proof: webs (\d+)/(\d+) visible, strands (\d+), (\d+) draw",text)
    if not rows or len(set(rows))!=1:
        raise RuntimeError("spider-web capture proof is missing or inconsistent")
    visible,total,strands,draws=map(int,rows[0])
    if not (1<=visible<=total==6 and strands==visible*434 and draws==1):
        raise RuntimeError("spider-web capture lacks the bounded live strand batch")
    return {"visible":visible,"total":total,"strands":strands,"draws":draws}


def validate_spider_lighting(text):
    rows = re.findall(r"spider lighting ground-transmission=(\S+) body-transmission=(\S+) body-tint=(\S+)", text)
    if len(rows) != 1:
        raise RuntimeError("spider lighting diagnostic missing or duplicated")
    try:
        ground, body = map(float, rows[0][:2])
        tint = tuple(map(float, rows[0][2].split(',')))
    except ValueError as error:
        raise RuntimeError("malformed spider lighting diagnostic") from error
    if len(tint) != 3 or not all(math.isfinite(v) and 0 <= v <= 1 for v in (ground, body, *tint)):
        raise RuntimeError("invalid spider lighting range")
    # Independent check of the shared canopy transfer, not a visual acceptance rule.
    if any(abs(actual - (ambient + (1 - ambient) * body)) > 0.00001
           for actual, ambient in zip(tint, (0.46, 0.53, 0.62))):
        raise RuntimeError("spider lighting differs from the shared canopy transfer")
    return {"groundTransmission": ground, "bodyTransmission": body, "bodyTint": tint}


def validate_spider_habitat_selection(text):
    rows = re.findall(r"spider habitat candidate=(\d+) transmission=(\S+)", text)
    selected = re.findall(r"spider habitat selected=(\d+)", text)
    if [int(row[0]) for row in rows] != list(range(12)) or len(selected) != 1:
        raise RuntimeError("incomplete spider habitat comparison")
    try:
        values = [float(row[1]) for row in rows]
    except ValueError as error:
        raise RuntimeError("malformed spider habitat transmission") from error
    if not all(math.isfinite(v) and 0 <= v <= 1 for v in values):
        raise RuntimeError("invalid spider habitat transmission")
    subject = int(selected[0])
    if subject != max(range(12), key=values.__getitem__):
        raise RuntimeError("spider camera did not select first brightest existing habitat")
    return {"bodyTransmissions": values, "subject": subject}


def validate_spider_motion_stages(text):
    """Sparse actual crawling presentation, not continuous foot-lock/contact proof."""
    rows = re.findall(r"^island-simulator: spider motion stage=(\d+) updates=(\d+) time=(\S+) visible=(\d+) "
                      r"position=(\S+) heading=(\S+) speed=(\S+) feet=(.*)$", text, re.M)
    if len(rows) != 8 or text.count("spider motion stage=") != 8 or [int(r[0]) for r in rows] != list(range(8)):
        raise RuntimeError("spider motion requires eight ordered stages")
    positions, headings, feet_samples = [], [], []
    for stage, updates, elapsed, visible, position, heading, speed, encoded in rows:
        try:
            expected = (int(stage)+1)*8
            p = tuple(map(float, position.split(',')))
            h, s, t = map(float, (heading, speed, elapsed))
            feet = [tuple(map(float, point.split(','))) for point in encoded.rstrip(';').split(';')]
            valid = (int(updates) == expected and math.isclose(t, expected*.1, abs_tol=1e-5)
                     and 1 <= int(visible) <= 12 and len(p) == 3 and all(map(math.isfinite, p))
                     and all(map(math.isfinite, (h, s, t))) and abs(h) <= math.pi+1e-5 and 0 <= s <= .069
                     and len(feet) == 8 and all(len(f) == 4 and all(map(math.isfinite, f)) and f[3] in (0, 1) for f in feet)
                     and sum(f[3] for f in feet) >= 6
                     and all(math.hypot(f[0]-p[0], f[2]-p[2]) <= .1401 for f in feet)
                     and len({f[:3] for f in feet}) == 8)
        except ValueError:
            valid = False
        if not valid:
            raise RuntimeError("invalid spider motion state or clock")
        positions.append(p); headings.append(h); feet_samples.append(feet)
    if any(math.dist(a, b) > .055 for a, b in zip(positions, positions[1:])):
        raise RuntimeError("spider exceeded maximum travel between samples")
    if any(abs(math.remainder(b-a, math.tau)) > .481 for a, b in zip(headings, headings[1:])):
        raise RuntimeError("spider exceeded maximum turn between samples")
    if max(math.dist(positions[0], p) for p in positions[1:]) < .02:
        raise RuntimeError("spider motion sequence did not crawl")
    if max(math.dist(feet_samples[0][i][:3], feet[i][:3]) for feet in feet_samples[1:] for i in range(8)) < .01:
        raise RuntimeError("spider feet stayed frozen during crawling review")
    return {"positions": positions, "headings": headings, "feet": feet_samples}


def check_configuration(text):
    cache = dict(re.findall(r"^([^/#\n:]+):[^=\n]+=(.*)$", text, re.M))
    if cache.get("CNA_GRAPHICS_RENDERER") != "OPENGL33" \
            or cache.get("CNA_GRAPHICS_RENDERERS", "") not in ("", "OPENGL33"):
        raise ValueError("software captures require a native OPENGL33-only build")
    for key, value in cache.items():
        if key.startswith("CNA_RENDERER_") and value == "ON" and key != "CNA_RENDERER_OPENGL33":
            raise ValueError(f"unexpected enabled renderer: {key}")
    if not cache.get("CNA_ROOT_DIR"):
        raise ValueError("software capture requires a configured CNA_ROOT_DIR")
    return Path(cache["CNA_ROOT_DIR"])


def validate_build_configuration(cache_text, log):
    expected = re.search(r"^CMAKE_BUILD_TYPE:STRING=(.*)$", cache_text, re.M)
    actual = re.findall(r"^island-simulator: build configuration (.*)$", log, re.M)
    if not expected or actual != [expected.group(1)]:
        raise RuntimeError("runtime build configuration does not match the configured capture tree")
    return actual[0]


def software_environment(evidence):
    environment = os.environ.copy()
    environment.update(LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe", LP_NUM_THREADS="8",
                       SDL_VIDEO_DRIVER="x11", SDL_VIDEODRIVER="x11", ISLAND_GPU_DIAGNOSTICS="1",
                       ISLAND_RUNTIME_LOG_DIRECTORY=str(evidence),
                       CNA_WAYLAND_TEST_KEEP_LOG=str(evidence / "compositor.log"))
    return environment


def inspection_arguments(values):
    if values is None:
        return []
    if len(values) != 3 or values[2] not in ("near", "far"):
        raise ValueError("canopy inspection requires X_MM Z_MM near|far")
    for value in values[:2]:
        if not re.fullmatch(r"-?\d+", value) or not -500000 <= int(value) <= 500000:
            raise ValueError("canopy inspection coordinates must be integers within +/-500000")
    return ["--canopy-lod-inspection", str(int(values[0])), str(int(values[1])), values[2]]


def validate_monkey_capture_log(diagnostics, require_tree_grips=False):
    if "monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes" not in diagnostics:
        raise RuntimeError("monkey capture did not load the required fur material")
    # Submission is necessary, not proof of visible pixels: still inspect the image.
    if not re.search(r"monkey capture visible [1-9][0-9]*; near [1-9][0-9]*; draws [1-9][0-9]*;", diagnostics):
        raise RuntimeError("monkey capture did not retain a close subject at readback")
    if "near monkey tails keep a compact muscular hook and thirty-six raised fur clumps" not in diagnostics:
        raise RuntimeError("monkey capture did not retain the current furry tail anatomy")
    if "near monkey faces use paired fur cheek ruffs around the skin mask" not in diagnostics:
        raise RuntimeError("monkey capture did not retain the fur-framed face")
    if "near monkeys use an arched shoulder-rump trunk, broad blunt face, robust limbs, and compact tail" not in diagnostics:
        raise RuntimeError("monkey capture did not retain the current primate proportions")
    if require_tree_grips and not re.search(r"monkey subject paired tree grips 1\b", diagnostics):
        raise RuntimeError("monkey-tree capture subject has no reachable paired bark grips")


def validate_monkey_climb_stages(diagnostics):
    number = r"(-?\d+(?:\.\d+)?)"
    rows = re.findall(r"monkey climb stage=(\d+) subject=(\d+) tree=(-?\d+) time=" + number
                      + r" active=(\d+) centre-y=" + number, diagnostics)
    if len(rows) != 8:
        raise RuntimeError("monkey-climb requires eight complete-cycle stages")
    result = []
    for index, row in enumerate(rows):
        stage, subject, tree, seconds, active, height = row
        if int(stage) != index or subject != rows[0][1] or not math.isfinite(float(height)):
            raise RuntimeError("invalid monkey climb stage identity/height")
        if index < 7:
            if abs(float(seconds)-[0, 5, 10.1, 18.5, 24, 29.1, 38.9][index]) > .1:
                raise RuntimeError("monkey climb omitted a required cycle phase")
            if active != "1" or tree != rows[0][2] or int(tree) < 0:
                raise RuntimeError("monkey climb lost its supporting tree")
            if index and float(seconds) <= float(rows[index-1][3]):
                raise RuntimeError("monkey climb clock did not advance")
        elif active != "0" or tree != "-1":
            raise RuntimeError("monkey climb did not safely finish ground release")
        result.append({"stage": index, "seconds": float(seconds), "centre_y": float(height)})
    if result[3]["centre_y"]-result[0]["centre_y"] < 1.5:
        raise RuntimeError("monkey climb did not achieve a visible ascent")
    if abs(result[6]["centre_y"]-result[0]["centre_y"]) > .1:
        raise RuntimeError("monkey climb did not descend to its original attachment height")
    return result


def validate_antialiasing_log(diagnostics, expected_samples):
    samples = re.findall(r"edge antialiasing requested (\d+)x renderer-applied (\d+)x", diagnostics)
    expected = (str(expected_samples), str(expected_samples))
    if not samples or any(sample != expected for sample in samples):
        raise RuntimeError(f"software qualification requires requested/applied {expected_samples}x MSAA")


def validate_monkey_tree_stages(diagnostics):
    number = r"([-+0-9.eE]+)"
    rows = re.findall(r"monkey tree stage=(\d+) subject=(\d+) tree=(-?\d+) elapsed=" + number
                      + r" clock=" + number + r" weights=" + number + "," + number, diagnostics)
    if len(rows) != 8 or diagnostics.count("monkey tree stage=") != 8:
        raise RuntimeError("monkey-tree requires exactly eight recorded transfer stages")
    stages = []
    for row in rows:
        stage, subject, tree = map(int, row[:3])
        elapsed, clock, left, right = map(float, row[3:])
        if not all(math.isfinite(value) for value in (elapsed, clock, left, right)):
            raise RuntimeError("non-finite monkey-tree stage")
        stages.append(dict(stage=stage, subject=subject, tree=tree, elapsed=elapsed,
                           clock=clock, weights=[left, right]))
    times = [.05, 1.35, 2.1, 3.3, 4.8, 5.4, 5.8]
    start = stages[0]["clock"] - stages[0]["elapsed"]
    def ease(t):
        t = max(0.0, min(1.0, t / .75))
        return t*t*(3-2*t)
    for index, stage in enumerate(stages):
        if stage["stage"] != index or stage["subject"] != stages[0]["subject"]:
            raise RuntimeError("monkey-tree stages must retain one subject in order")
        if index and stage["clock"] <= stages[index-1]["clock"]:
            raise RuntimeError("monkey-tree animation clock must advance between stages")
        if index < 7:
            t = stage["elapsed"]
            if (stage["tree"] < 0 or stage["tree"] != stages[0]["tree"]
                    or not times[index] <= t <= times[index]+.501
                    or abs(stage["clock"]-t-start) > .002):
                raise RuntimeError("monkey-tree stages must belong to one uninterrupted inspection")
            expected = [ease(t-1)*ease(6-t-.75), ease(t-1.75)*ease(6-t)]
        else:
            if stage["tree"] != -1 or stage["elapsed"] != 0 or not 6 <= stage["clock"]-start <= 6.6:
                raise RuntimeError("monkey-tree final stage must release the tree after inspection")
            expected = [0, 0]
        if any(not 0 <= value <= 1 or abs(value-wanted) > .002
               for value, wanted in zip(stage["weights"], expected)):
            raise RuntimeError("monkey-tree rendered contacts do not match sequential hand transfer")
    return stages


def validate_graphics_log(diagnostics, inspection=None, *, missing_leaf=False):
    markers = re.findall(r"TEMPORARY SOFTWARE DIAGNOSTIC[^\n]*", diagnostics)
    if inspection is None:
        if markers:
            raise RuntimeError("diagnostic-only rendering cannot qualify the full scene")
    else:
        values = inspection_arguments(inspection)[1:]
        expected = (f"TEMPORARY SOFTWARE DIAGNOSTIC canopy-lod target={values[0]}:{values[1]} "
                    f"tier={values[2]} frozen-simulation=0 shader-time=1")
        if not markers or any(marker != expected for marker in markers):
            raise RuntimeError("canopy inspection marker missing, mismatched or contains another diagnostic")
    if GL_ERROR.search(diagnostics):
        raise RuntimeError("GL error in software capture")
    if missing_leaf:
        expected = re.compile(r"island-simulator: leaf material fallback: "
            r"Content/vegetation/leaf-lamina\.png: [^\r\n]+; retaining procedural veins")
        if not expected.search(diagnostics) or "leaf material loaded" in diagnostics:
            raise RuntimeError("missing-leaf fixture requires its exact fallback and no loaded marker")
        diagnostics = expected.sub("expected missing-leaf diagnostic", diagnostics)
    if re.search(r"(?:stock-effect|material) fallback:", diagnostics):
        raise RuntimeError("GL error or degraded shader path in software capture")


def run_monitored_capture(command, root, environment, evidence, journal, baseline,
                          timeout=65.0, poll_interval=0.2, *, private_devices=True,
                          read_only_directories=()):
    """Own one PID namespace and fail closed while preserving the first kernel failure.

    Software rendering is not permission to ignore an unrelated GPU fault on the shared host.
    Journal calls have their own bounded timeout; the poll interval is not a response-time SLA.
    Killing a client cannot retract already submitted hardware work or guarantee desktop recovery.
    """
    cursor = baseline["__CURSOR"]
    process = None
    previous_handlers = {}

    def interrupt(signum, _frame):
        raise MonitorError(f"software capture interrupted by signal {signum}")

    def collect(stream):
        nonlocal cursor
        latest = journal.latest()  # An empty delta alone does not prove journal access.
        records = journal.read("--after-cursor", cursor)
        for record in records:
            stream.write(json.dumps(record) + "\n")
        stream.flush()  # Preserve the offending message before raising or terminating children.
        if latest["_BOOT_ID"] != baseline["_BOOT_ID"] or any(
                record["_BOOT_ID"] != baseline["_BOOT_ID"] for record in records):
            raise MonitorError("kernel boot discontinuity during software capture")
        if any(FAULT.search(record["MESSAGE"]) for record in records):
            raise MonitorError("GPU fault during software capture; stopping owned workload")
        if records:
            cursor = records[-1]["__CURSOR"]

    with (evidence / "kernel-live.jsonl").open("w") as kernel:
        try:
            for number in (signal.SIGINT, signal.SIGTERM):
                previous_handlers[number] = signal.signal(number, interrupt)
            collect(kernel)  # Refuse a fault between baseline acquisition and launch.
            launch = contained_command(command, private_devices=private_devices,
                                       read_only_directories=read_only_directories)
            (evidence / "process-containment.json").write_text(json.dumps({
                "command": launch, "workload_command": command,
                "launcher": artifact_identity(Path(launch[0])),
                "policy": ("pid-lifetime plus private devices" if private_devices else "pid-lifetime-only")
                    + "; caller procfs retained for kernel attribution",
                "private_devices": private_devices,
            }, indent=2) + "\n")
            with (evidence / "workload.log").open("w") as output:
                process = subprocess.Popen(launch, cwd=root, env=environment,
                                           stdout=output, stderr=subprocess.STDOUT,
                                           start_new_session=True)
                deadline = time.monotonic() + timeout
                while True:
                    collect(kernel)
                    result = process.poll()
                    if result is not None:
                        break
                    if time.monotonic() >= deadline:
                        raise MonitorError("software capture exceeded its time limit")
                    time.sleep(poll_interval)
                stop_process_group(process)  # The wrapper may exit before its children.
                process = None
                collect(kernel)
                return result
        finally:
            # Do not let a second interruption bypass owned-child cleanup. Restore the caller's
            # handlers even if cleanup fails, and do not replace an existing fault with that error.
            active_error = sys.exc_info()[1]
            try:
                for number in previous_handlers:
                    signal.signal(number, signal.SIG_IGN)
                if process is not None:
                    try:
                        stop_process_group(process)
                    except (OSError, RuntimeError) as error:
                        if active_error is None:
                            raise
                        print(f"Software capture cleanup also failed: {error}", file=sys.stderr)
            finally:
                for number, handler in previous_handlers.items():
                    signal.signal(number, handler)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("viewpoint", choices=sorted(CAPTURE_VIEWPOINTS | {"campfire", "campfire-wide", "cave", "cave-inside", "stream", "turtle-nest", "hut", "hut-material", "helicopter", "helicopter-inside", "resident", "bear", "boars", "goats", "spiders", "spiders-bright", "spider-web", "snakes", "snake-head", "canopy", "monkey-tree", "monkey-climb"}))
    parser.add_argument("--canopy-lod-inspection", nargs=3, metavar=("X_MM", "Z_MM", "TIER"))
    parser.add_argument("--missing-leaf", action="store_true",
                        help="test only the leaf-image fallback using a read-only namespace overlay")
    args = parser.parse_args()
    inspection = inspection_arguments(args.canopy_lod_inspection)
    if args.viewpoint == "canopy" and not inspection:
        parser.error("canopy viewpoint requires --canopy-lod-inspection")
    if args.missing_leaf and (args.viewpoint != "ground-broadleaf" or inspection):
        parser.error("missing-leaf qualification requires the ordinary ground-broadleaf view")
    build = args.build.resolve()
    root = Path(__file__).resolve().parent.parent
    cache_text = (build / "CMakeCache.txt").read_text()
    dependency = check_configuration(cache_text)
    journal = Journal()
    baseline = journal.latest()  # Refuse an empty/inaccessible kernel journal before launch.
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    evidence = build / "software-captures" / f"{stamp}-{args.viewpoint}"
    evidence.mkdir(parents=True)
    capture = evidence / "capture.ppm"
    platform = dependency / "tools/platform"
    command = [str(platform / "wayland_test_server.sh"), "--compositor", "weston", "--renderer", "pixman",
               str(platform / "run_gpu_tests_private.sh"), "--inside-private-compositor", "exec",
               str(build / "island-simulator"), "--capture", str(capture), "--viewpoint", args.viewpoint,
               "--no-user-settings", *inspection]
    scope = "single-tree frozen LOD inspection only" if inspection else "software visual validation only"
    # Exercise title-relative content loading from an unrelated, build-owned working directory.
    metadata = {"scope": scope, "command": command, "cwd": str(evidence), "kernel_before": baseline}
    leaf_asset = build / "Content/vegetation/leaf-lamina.png"
    bindings = []
    if args.missing_leaf:
        metadata["scope"] = "missing leaf material fallback only; not normal visual qualification"
        metadata["leaf_before"] = artifact_identity(leaf_asset)
        empty = evidence / "empty-leaf-directory"
        empty.mkdir()
        bindings = [(empty, leaf_asset.parent)]
    failure = ""
    try:
        with (build / "gpu-qualification.lock").open("a") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            # Persist the exact command and baseline before launching any rendering process.
            metadata["monitor_poll_seconds"] = 0.2
            (evidence / "run.json").write_text(json.dumps(metadata, indent=2) + "\n")
            result = run_monitored_capture(command, evidence, software_environment(evidence),
                                           evidence, journal, baseline, read_only_directories=bindings)
            if result:
                raise RuntimeError(f"software capture exited {result}")
            log = (evidence / "workload.log").read_text(errors="replace")
            metadata["build_configuration"] = validate_build_configuration(cache_text, log)
            if 'gl-renderer="llvmpipe ' not in log or 'renderer="OPENGL33"' not in log:
                raise RuntimeError("runtime did not confirm the expected software renderer")
            diagnostics = log + "".join(p.read_text(errors="replace") for p in evidence.glob("island-runtime-*.log"))
            if args.viewpoint in {"campfire", "campfire-wide"}:
                validate_campfire_stages(log)
                for stage in range(4):
                    if not Path(str(capture)+f".fire-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing live campfire animation-stage image")
                # Capture mode intentionally disables audio; real backend switching is
                # exercised by IslandSoundscapeRuntimeTests, not a visual smoke run.
                for marker in ("campfire first visible animated draw",
                               "1992 solid triangles, 42 flame/smoke/spark triangles",
                               "campfire flame material uses broken translucent tongues, pale cores, a restrained blue base, and layered smoke"):
                    if marker not in diagnostics:raise RuntimeError(f"campfire integration diagnostic missing: {marker}")
            if args.viewpoint == "stream":
                metadata["stream_stages"]=validate_stream_stages(log)
                for stage in range(4):
                    if not Path(str(capture)+f".stream-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing live stream animation-stage image")
                if "route-following downstream ripple and translucent current streaks" not in diagnostics:
                    raise RuntimeError("current stream flow material is missing")
                if "stream water uses bank-edge sheen and broken shallow riffles" not in diagnostics:
                    raise RuntimeError("current stream edge/riffle material is missing")
                if "stream channel uses shared low-frequency pools and pinches" not in diagnostics:
                    raise RuntimeError("current irregular stream-width profile is missing")
                if not re.search(r"stream-bank stones (?:1[0-8]), non-solid existing rock variants beyond the carved bank", diagnostics):
                    raise RuntimeError("current stream-bank stone composition is missing")
                if "stream terrain uses a compact incised profile and slope-limited exposed bank strata" not in diagnostics:
                    raise RuntimeError("current physical stream-bank terrain is missing")
            if args.viewpoint in ("hut", "hut-material"):
                for marker in ("hut raised veranda uses full-height outer posts, split rails, fascia, and three steps",
                               "hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports",
                               "hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes"):
                    if marker not in diagnostics:
                        raise RuntimeError(f"hut integration diagnostic missing: {marker}")
            if args.viewpoint in ("helicopter", "helicopter-inside"):
                camera_marker=("helicopter QA camera frames rear cabin toward accessible cockpit"
                               if args.viewpoint=="helicopter-inside" else
                               "helicopter QA camera frames grounded exterior and open doors")
                for marker in (camera_marker,
                               "grounded helicopter original procedural shell, open pilot/rear doors, interior and stationary rotors",
                               "helicopter uses rounded twenty-sided cockpit glazing, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane",
                               "helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber",
                               "helicopter carries four outward-open framed door panels with glazing and handles"):
                    if marker not in diagnostics:
                        raise RuntimeError(f"helicopter integration diagnostic missing: {marker}")
                if not re.search(r"helicopter 2 draw", diagnostics):
                    raise RuntimeError("helicopter viewpoint did not retain its bounded two-draw model")
            if args.viewpoint in ("snakes", "snake-head"):
                if "snakes use a readable dorsal saddle and active forked-tongue flicks" not in diagnostics:
                    raise RuntimeError("current snake anatomy material is missing")
                validate_snake_motion_stages(log)
                for stage in range(4):
                    if not Path(str(capture)+f".snake-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing snake motion stage image")
            if args.viewpoint == "boars":
                motion = validate_boar_motion_stages(log)
                (evidence / "boar-motion.json").write_text(json.dumps(motion, indent=2) + "\n")
                for stage in range(8):
                    if not Path(str(capture)+f".boar-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing boar motion stage image")
            if args.viewpoint == "goats":
                motion = validate_goat_motion_stages(log)
                (evidence / "goat-motion.json").write_text(json.dumps(motion, indent=2) + "\n")
                for stage in range(8):
                    if not Path(str(capture)+f".goat-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing goat motion stage image")
            if args.viewpoint in ("spiders", "spiders-bright"):
                if args.viewpoint == "spiders-bright":
                    habitats = validate_spider_habitat_selection(log)
                    (evidence / "spider-habitats.json").write_text(json.dumps(habitats, indent=2) + "\n")
                lighting = validate_spider_lighting(log)
                (evidence / "spider-lighting.json").write_text(json.dumps(lighting, indent=2) + "\n")
                motion = validate_spider_motion_stages(log)
                (evidence / "spider-motion.json").write_text(json.dumps(motion, indent=2) + "\n")
                for stage in range(8):
                    if not Path(str(capture)+f".spider-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing spider motion stage image")
            if args.viewpoint=="resident":
                motion=validate_resident_stages(log)
                (evidence/"resident-motion.json").write_text(json.dumps(motion,indent=2)+"\n")
                for stage in range(8):
                    if not Path(str(capture)+f".resident-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing resident gait-stage image")
                for marker in ("one adult resident uses original articulated human geometry",
                               "resident uses a shoulder-sloped woven tunic with open cap sleeves and anatomically tapered calves",
                               "resident uses buried deltoid shoulders, split hands, and layered facial anatomy",
                               "resident QA camera frames and follows one peaceful adult",
                               "resident capture visible 1; draws 2"):
                    if marker not in diagnostics:
                        raise RuntimeError(f"resident integration diagnostic missing: {marker}")
            if args.viewpoint=="bear":
                motion=validate_bear_stages(log)
                (evidence/"bear-motion.json").write_text(json.dumps(motion,indent=2)+"\n")
                for stage in range(8):
                    if not Path(str(capture)+f".bear-stage-{stage}.ppm").is_file():
                        raise RuntimeError("missing bear locomotion-stage image")
                for marker in ("one large peaceful bear uses original articulated quadruped anatomy",
                               "bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes",
                               "near bear uses twenty-two body-envelope fur wedges without another draw",
                               "near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes",
                               "bear capture visible 1; draws 2"):
                    if marker not in diagnostics:
                        raise RuntimeError(f"bear integration diagnostic missing: {marker}")
            validate_graphics_log(diagnostics, args.canopy_lod_inspection, missing_leaf=args.missing_leaf)
            validate_antialiasing_log(diagnostics, 0)
            if not args.missing_leaf and "leaf material loaded Content/vegetation/leaf-lamina.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in diagnostics:
                raise RuntimeError("packaged live-leaf material is missing; check assets and executable")
            if "forest-floor material loaded Content/terrain/forest-floor.png; 512x512, 10 mips, 1398100 RGBA8 bytes; interior windows" not in diagnostics:
                raise RuntimeError("packaged forest-floor material is missing; check assets and executable")
            if args.viewpoint == "shipwreck" and "shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas" not in diagnostics:
                raise RuntimeError("packaged shipwreck wood material is missing; check assets and executable")
            if args.viewpoint == "shipwreck" and "shipwreck weathered wood follows per-piece board and beam UVs" not in diagnostics:
                raise RuntimeError("shipwreck board-local UV material is missing; check for a stale executable")
            if args.viewpoint == "shipwreck" and "shipwreck restrained rusted iron straps, rivets, hinges and rudder braces" not in diagnostics:
                raise RuntimeError("shipwreck iron fittings are missing; check for a stale executable")
            if args.viewpoint == "shipwreck" and "shipwreck hull uses asymmetric rising sheer, curved keel, and irregular splintered break sections" not in diagnostics:
                raise RuntimeError("current shipwreck hull silhouette is missing; check for a stale executable")
            if args.viewpoint == "shipwreck" and "shipwreck beach broadside has true missing plank bays with exposed structural ribs" not in diagnostics:
                raise RuntimeError("current shipwreck broadside damage is missing; check for a stale executable")
            if args.viewpoint == "shipwreck" and "shipwreck bow and stern use raked lower profiles with thick end timbers" not in diagnostics:
                raise RuntimeError("current shipwreck end profiles are missing; check for a stale executable")
            if not re.search(r"forest-floor filter (?:anisotropic, requested maximum 4x, device-clamped"
                             r"|trilinear, anisotropy unavailable)", diagnostics):
                raise RuntimeError("forest-floor sampler capability decision is missing")
            if "boar soles fit rendered terrain independently with fixed-length limbs" not in diagnostics:
                raise RuntimeError("current boar terrain contact is missing; check for a stale executable")
            if "boar terrain contact limited by limb reach" in diagnostics:
                raise RuntimeError("boar contact exceeded anatomical reach in the captured scene")
            if args.viewpoint == "canopy" and not re.search(
                    r"elevated canopy inspection camera position=[^\n]+ full-scene=1", diagnostics):
                raise RuntimeError("targeted canopy camera was not established")
            if args.viewpoint == "mangroves":
                if not re.search(r"localized mangrove colony (1[5-9]|2[0-9]|30) trees, low crowns, multiple stems, stilt roots, canopy variant 8", diagnostics):
                    raise RuntimeError("mangrove colony population/form diagnostic missing")
                if "mangrove QA target exposes stilt roots and low crown" not in diagnostics:
                    raise RuntimeError("mangrove detail camera did not select its target")
            if args.viewpoint == "turtle-nest":
                if "two turtle nesting traces use paired flipper tracks and disturbed dry-sand nests" not in diagnostics:
                    raise RuntimeError("turtle nesting trace geometry is missing")
                if "turtle-nest QA camera follows paired tracks from disturbed dry sand toward the sea" not in diagnostics:
                    raise RuntimeError("turtle nesting trace camera did not select its target")
            if args.viewpoint in {"cave", "cave-inside"}:
                side = "inside" if args.viewpoint == "cave-inside" else "outside"
                for marker in (f"cave QA camera {side}", "cave ground replacement",
                               "cave rock first visible draw", "20m walkable interior",
                               "cave compact asymmetric mouth uses normal-lit fractured stone and restrained moss",
                               "cave narrow eroded lip grows into an irregular closed rock mantle",
                               "cave connected shell strips share smooth lighting normals while portal breaks stay sharp",
                               "cave terrain shoulders use an irregular rock apron around the open corridor",
                               "cave rock loaded Content/structures/cave-rock.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored triplanar sampling"):
                    if marker not in diagnostics:
                        raise RuntimeError(f"cave integration diagnostic missing: {marker}")
            if args.viewpoint == "reef-habitat":
                if "spatial reef uses branching, rounded, plate coral and underwater-rock LOD forms" not in diagnostics:
                    raise RuntimeError("spatial reef form diagnostic missing")
                if "branching reef form uses dense upright staghorn stems, offset side forks, and small growing tips" not in diagnostics:
                    raise RuntimeError("current staghorn coral silhouette diagnostic missing")
                if not re.search(r"reef habitat QA camera underwater with [1-9][0-9]* neighbours", diagnostics):
                    raise RuntimeError("reef-habitat camera did not select a populated patch")
                if not re.search(r"[1-9][0-9]*/[1-9][0-9]* reef habitat", diagnostics):
                    raise RuntimeError("reef-habitat capture did not submit enough instances")
            if args.viewpoint == "reef":
                for marker in ("reef QA camera uses an elevated wide substrate view",
                               "reef substrate separates pale lagoon, shared habitat patches, crest, sand channels, and deep drop-off",
                               "broken reef-crest foam follows the shared 62-86m habitat field without new draws"):
                    if marker not in diagnostics:
                        raise RuntimeError("reef camera or macro substrate is stale")
                if not re.search(r"reef QA camera targets peak broken crest weight (?:0\.[0-9]*[1-9]|1(?:\.0*)?) at", diagnostics):
                    raise RuntimeError("reef camera did not target a nonzero breaker segment")
                if not re.search(r"[1-9][0-9]*/[1-9][0-9]* reef outcrops", diagnostics):
                    raise RuntimeError("reef camera did not retain spatial outcrop context")
            if "terrain substrate persists at distance with footprint-filtered noise" not in diagnostics:
                raise RuntimeError("current terrain distance policy is missing; check for a stale executable")
            if "terrain palette overrides preserve smooth slope daylight; opaque 28-byte vertices" not in diagnostics:
                raise RuntimeError("current terrain slope lighting is missing; check for a stale executable")
            if "canopy per-tree frustum boxes contain both LODs and wind" not in diagnostics:
                raise RuntimeError("current canopy visibility path is missing; check for a stale executable")
            if "clearing-edge colonies respect full route footprints; bushes=" not in diagnostics:
                raise RuntimeError("current clearing-edge composition is missing; check for a stale executable")
            if "understory leaf identities share asymmetric posture and curled profiles across LOD" not in diagnostics:
                raise RuntimeError("current understory leaf profiles are missing; check for a stale executable")
            if "pinnate understory shares rachis attachment knots and retained leaflet wind across LOD" not in diagnostics:
                raise RuntimeError("current connected fern geometry is missing; check for a stale executable")
            if "broadleaf paddle and lance laminae share cupped profiles and asymmetric margins across LOD" not in diagnostics:
                raise RuntimeError("current broadleaf lamina geometry is missing; check for a stale executable")
            if "leaf daylight contrast and stable tip pigment; footprint-filtered leaf and bark detail" not in diagnostics:
                raise RuntimeError("current jungle material response is missing; check for a stale executable")
            if "canopy wood separates albedo from baked shade with validated outward normals and one-sided daylight" not in diagnostics:
                raise RuntimeError("current canopy wood lighting is missing; check for a stale executable")
            if "canopy bark follows local wood axes with seam-safe circular footprint filtering" not in diagnostics:
                raise RuntimeError("current canopy bark coordinates are missing; check for a stale executable")
            if "canopy bark has instance-stable warped grain and interrupted filtered fissures" not in diagnostics:
                raise RuntimeError("current canopy bark variation is missing; check for a stale executable")
            if "canopy trunks use root flare, rounded buttress ridges and grey-brown bark with moss, lichen, contact occlusion and canopy shade" not in diagnostics:
                raise RuntimeError("current canopy trunk surface is missing; check for a stale executable")
            if "canopy trunk pigment follows normalized height across LOD tiers" not in diagnostics:
                raise RuntimeError("current trunk pigment continuity is missing; check for a stale executable")
            if "canopy blades use spatial pitch and roll with identical retained LOD poses" not in diagnostics:
                raise RuntimeError("current canopy leaf posture is missing; check for a stale executable")
            if "curved canopy blades retain source identities within original triangle budgets" not in diagnostics:
                raise RuntimeError("current canopy leaf outline is missing; check for a stale executable")
            if "canopy mesh variant=0 near-vertices=4818 near-triangles=4640 far-vertices=1071 far-triangles=1120" not in diagnostics:
                raise RuntimeError("current far canopy support branches are missing; check for a stale executable")
            if "eight structural crown forms share eight LOD lobe anchors and supported trunk bends" not in diagnostics:
                raise RuntimeError("current canopy structure is missing; check for a stale executable")
            if "forest soil separates humus and moss with footprint-filtered fallen leaves" not in diagnostics:
                raise RuntimeError("current forest substrate is missing; check for a stale executable")
            if "grass culling bounds cover measured meshes, slope and full shared motion limits" not in diagnostics:
                raise RuntimeError("current grass motion bounds are missing; check for a stale executable")
            if "grass near blades form seven unequal fanned tufts with coherent wind" not in diagnostics:
                raise RuntimeError("current grass tuft layout is missing; check for a stale executable")
            if "grass progressive near taper and seed roots share the sampled blade profile" not in diagnostics:
                raise RuntimeError("current grass blade profile is missing; check for a stale executable")
            if "grass shared rest-shape normals follow curvature and instance scale" not in diagnostics:
                raise RuntimeError("current grass surface normals are missing; check for a stale executable")
            if "grass retains foliage colour at distance with normalized-height basal tint" not in diagnostics:
                raise RuntimeError("current grass material continuity is missing; check for a stale executable")
            if "grass pigment uses separate height occlusion and filtered longitudinal veins" not in diagnostics:
                raise RuntimeError("current grass surface material is missing; check for a stale executable")
            if "grass LOD proxies share seven tuft roots, profiles and wind phases" not in diagnostics:
                raise RuntimeError("current grass shared LOD is missing; check for a stale executable")
            if "grass anchor-distance contraction uses nearest-bound cell culling" not in diagnostics:
                raise RuntimeError("current grass distance continuity is missing; check for a stale executable")
            if "juvenile ground broadleaf uses curved tagged laminae and retained near/far leaf identities" not in diagnostics:
                raise RuntimeError("current ground broadleaf geometry is missing; check for a stale executable")
            if "split ground leaves use connected lobed surfaces with shared LOD knots and root-anchored wind" not in diagnostics:
                raise RuntimeError("current split ground leaf geometry is missing; check for a stale executable")
            if "forest-floor colonies retain route footprints; groups=" not in diagnostics:
                raise RuntimeError("Forest-floor colony diagnostic missing")
            if "jungle root origins and culling bounds follow rendered terrain triangles" not in diagnostics:
                raise RuntimeError("Rendered jungle root-contact diagnostic missing")
            if "fallen leaves conform to rendered triangles using the shared grass height texture" not in diagnostics:
                raise RuntimeError("Shared-height-map litter contact diagnostic missing")
            if "shrubs use supported shoots and curved individual leaves with shared LOD identities and complete leaf outlines" not in diagnostics:
                raise RuntimeError("current shrub geometry is missing; check for a stale executable")
            if "palms lean away from the fixed east trade wind by coastal windward exposure" not in diagnostics:
                raise RuntimeError("current prevailing wind is missing; check for a stale executable")
            if "sky adds shared-haze horizon, warm sun glow and eleven fixed distant cumulus masses" not in diagnostics:
                raise RuntimeError("current sky atmosphere is missing; check for a stale executable")
            if "fair-weather cumulus and wisps drift slowly west with prevailing wind" not in diagnostics:
                raise RuntimeError("current wind-driven cloud drift is missing; check for a stale executable")
            if "sea turtles use domed scute carapaces, beaked heads and bending paddle strokes" not in diagnostics:
                raise RuntimeError("current sea-turtle form is missing; check for a stale executable")
            if args.viewpoint == "turtles":
                if "near turtles use subdued scute seams, fuller heads, nostrils and mouth creases" not in diagnostics:
                    raise RuntimeError("current close sea-turtle detail is missing; check for a stale executable")
                if "near turtles use staggered scutes and closed tapered-volume paddles" not in diagnostics:
                    raise RuntimeError("current sea-turtle scute and paddle detail is missing; check for a stale executable")
                if "near turtles use compact integrated heads, narrower hydrodynamic paddles, and restrained shell seams" not in diagnostics:
                    raise RuntimeError("current sea-turtle silhouette refinement is missing; check for a stale executable")
                if not re.search(r"turtles [1-5]/5 visible \(near [1-5], far [0-5]\), 1 draw", diagnostics):
                    raise RuntimeError("sea-turtle capture did not submit a near detailed animal")
            if args.viewpoint == "fish":
                if "near fish use seven-station volumetric bodies, caudal peduncles and progressive rear-body bends" not in diagnostics:
                    raise RuntimeError("current close fish form is missing; check for a stale executable")
                if "near fish use rounded ten-facet bodies, closed-volume fins, and two-facet eyes" not in diagnostics:
                    raise RuntimeError("current close fish anatomy is missing; check for a stale executable")
                if not re.search(r"fish [1-9][0-9]*/48 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", diagnostics):
                    raise RuntimeError("fish capture did not submit a near detailed school")
            if args.viewpoint == "shark":
                if "near shark uses an eight-station body, blunt snout, caudal peduncle and heterocercal tail" not in diagnostics:
                    raise RuntimeError("current close shark form is missing; check for a stale executable")
                if "near shark uses a rounded ten-facet body, closed-volume fins, five paired gill slits, and faceted eyes" not in diagnostics:
                    raise RuntimeError("current close shark anatomy is missing; check for a stale executable")
                if not re.search(r"shark visible activity \S+, 1 draw", diagnostics):
                    raise RuntimeError("shark capture did not submit the near detailed animal")
            if args.viewpoint == "dolphins":
                if "dolphins use fusiform bodies, rostrums, curved dorsal fins and horizontal animated flukes" not in diagnostics:
                    raise RuntimeError("current dolphin form is missing; check for a stale executable")
                if "near dolphins use closed-volume fins and flukes, short beaks, faceted eyes, mouth creases, and a blowhole" not in diagnostics:
                    raise RuntimeError("current close dolphin anatomy is missing; check for a stale executable")
                if "near dolphins use a thirteen-station melon-to-peduncle profile, natural countershading, a compact falcate dorsal, and swept pectoral fins" not in diagnostics:
                    raise RuntimeError("current dolphin silhouette proportions are missing; check for a stale executable")
                if "dolphin tail stock bends vertically through horizontal fluke strokes" not in diagnostics:
                    raise RuntimeError("current species-appropriate dolphin tail stroke is missing")
                if "dolphin near/far geometry 1476/15 vertices, closed fins and CPU faceted daylight" not in diagnostics:
                    raise RuntimeError("current dolphin geometry inventory is missing; check for a stale executable")
                if not re.search(r"dolphins [1-4]/4 visible \(near [1-4], far [0-4]\), 1 draw", diagnostics):
                    raise RuntimeError("dolphin capture did not submit a near detailed animal")
            if args.viewpoint == "crabs":
                if "near crabs use tapered toes, socketed eyes, and solid pincer fingers" not in diagnostics:
                    raise RuntimeError("current close crab detail is missing; check for a stale executable")
                if "near crabs use flattened lateral carapaces, sturdy legs, and enlarged claw palms" not in diagnostics:
                    raise RuntimeError("current crab silhouette proportions are missing; check for a stale executable")
                if not re.search(r"crabs [1-9][0-9]*/30 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw", diagnostics):
                    raise RuntimeError("crab capture did not submit a near detailed colony")
            if args.viewpoint == "bush" and "bush QA target variant 0 at player-eye detail distance" not in diagnostics:
                raise RuntimeError("bush camera did not select the intended shrub")
            if args.viewpoint == "ground-broadleaf" and "ground-broadleaf QA target variant 1 at player-eye detail distance" not in diagnostics:
                raise RuntimeError("ground broadleaf camera did not select the intended plant species")
            if "grass mesh vertices/triangles near=862/640 middle=60/40 far=42/28" not in diagnostics:
                raise RuntimeError("unexpected grass mesh inventory")
            if "ocean short waves use phased wind packets with analytic envelope gradients" not in diagnostics:
                raise RuntimeError("current wave packets are missing; check for a stale executable")
            if "windward ocean waves and foam strengthen east and calm in the west bay" not in diagnostics:
                raise RuntimeError("current wind-aware ocean response is missing; check for a stale executable")
            if "grass near mesh adds 48 low curved turf blades with height-scaled motion" not in diagnostics:
                raise RuntimeError("current low turf layer is missing; check for a stale executable")
            if "grass roots sample RGB24 rendered terrain triangles before wind" not in diagnostics:
                raise RuntimeError("current grass grounding is missing; check for a stale executable")
            if "grass instances use area-preserving asymmetric tuft footprints" not in diagnostics:
                raise RuntimeError("current grass footprint variation is missing; check for a stale executable")
            if "ocean bounded grazing contrast and depth-limited filtered breaker foam" not in diagnostics:
                raise RuntimeError("current ocean surface response is missing; check for a stale executable")
            if "wood uses outward shared normals and scale-correct fixed-sun lighting" not in diagnostics:
                raise RuntimeError("current wood lighting is missing; check for a stale executable")
            if "land prop shadows match model yaw and rendered terrain triangles" not in diagnostics:
                raise RuntimeError("current land shadow geometry is missing; check for a stale executable")
            if "rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering" not in diagnostics:
                raise RuntimeError("current stone surface is missing; check for a stale executable")
            if "coastal contact patches follow rendered seabed triangles in the existing batch" not in diagnostics:
                raise RuntimeError("current coastal shadows are missing; check for a stale executable")
            if args.viewpoint == "shipwreck" and not re.search(
                    r"coastal contact shadows [1-9][0-9]*", diagnostics):
                raise RuntimeError("coastal viewpoint did not draw any grounding patches")
            if "ocean shallow attenuation uses interpolated bed depth and contact fade" not in diagnostics \
                    or "coastal sand stays continuous below water with filtered seabed ripples" not in diagnostics:
                raise RuntimeError("current coastal materials are missing; check for a stale executable")
            if "ocean shared nine-wave spectrum uses filtered short-wave normals" not in diagnostics:
                raise RuntimeError("current ocean spectrum is missing; check for a stale executable")
            if "shoreline advancing wash uses footprint-filtered porous foam" not in diagnostics:
                raise RuntimeError("current shoreline foam is missing; check for a stale executable")
            if "shoreline foam follows bounded east-to-west coastal wind exposure" not in diagnostics:
                raise RuntimeError("current wind-aware shoreline foam is missing; check for a stale executable")
            if "shared fixed-sun canopy transmission lights terrain and grass" not in diagnostics:
                raise RuntimeError("current canopy-light feature is missing; check for a stale executable")
            if "jungle ground layers receive shared canopy shade; crowns excluded" not in diagnostics:
                raise RuntimeError("current jungle receiver shader is missing; check for a stale executable")
            if "jungle leaves use smooth normals, filtered veins and restrained wax shading" not in diagnostics:
                raise RuntimeError("current leaf material is missing; check for a stale executable")
            if "boars receive bilinear per-vertex canopy light in their existing batch" not in diagnostics:
                raise RuntimeError("current boar lighting is missing; check for a stale executable")
            if args.viewpoint in ("snakes","snake-head"):
                if not re.search(r"snakes (?:[1-9]|[1-4][0-9]|50)/50 visible, 1 draw",diagnostics):
                    raise RuntimeError("snake capture did not submit the live population")
            if args.viewpoint == "goats" and not re.search(r"goats [1-6]/6 visible, 2 draw", diagnostics):
                raise RuntimeError("goat capture did not submit the live population")
            if args.viewpoint in ("spiders", "spiders-bright"):
                validate_spider_render_log(diagnostics)
            if args.viewpoint=="spider-web":
                validate_spider_web_render_log(diagnostics)
            if args.viewpoint == "goats" and "goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in diagnostics:
                raise RuntimeError("goat fur material was not loaded")
            if "boar bristle loaded Content/wildlife/boar-bristle.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in diagnostics:
                raise RuntimeError("current boar coat is missing; check for a stale executable")
            if args.viewpoint=="boars" and not re.search(r"boars [1-6]/6 visible, 2 draw", diagnostics):
                raise RuntimeError("boar coat/solid anatomy draw split missing")
            if "boars use connected limb poses, stance contact and body-rooted bristles" not in diagnostics:
                raise RuntimeError("current boar pose is missing; check for a stale executable")
            if args.viewpoint=="boars" and "near boars use broad tapered upper legs with compact split hooves" not in diagnostics:
                raise RuntimeError("current boar limb anatomy is missing; check for a stale executable")
            if args.viewpoint=="boars" and "boars use a lifted shoulder mantle and broad sealed nasal disc" not in diagnostics:
                raise RuntimeError("current boar shoulder/muzzle silhouette is missing; check for a stale executable")
            if "monkeys receive footprint-averaged canopy light through 14-bone skinning" not in diagnostics:
                raise RuntimeError("current monkey lighting is missing; check for a stale executable")
            if args.viewpoint in {"monkeys", "monkey-tree", "monkey-climb"}:
                validate_monkey_capture_log(diagnostics, args.viewpoint in {"monkey-tree", "monkey-climb"})
            if args.viewpoint in {"monkey-tree", "monkey-climb"}:
                metadata["monkey_tree_stages"] = (validate_monkey_climb_stages(diagnostics)
                    if args.viewpoint == "monkey-climb" else validate_monkey_tree_stages(diagnostics))
                for stage in metadata["monkey_tree_stages"]:
                    stage_path = Path(str(capture) + f".tree-stage-{stage['stage']}.ppm")
                    validation = subprocess.run([sys.executable, str(root / "tools/validate_capture.py"),
                        str(stage_path), "--profile", "scene", "--output",
                        str(evidence / f"tree-stage-{stage['stage']}-validation.json")],
                        check=False, timeout=15)
                    if validation.returncode:
                        raise RuntimeError(f"monkey-tree stage {stage['stage']} image validation failed")
            validation = subprocess.run([sys.executable, str(root / "tools/validate_capture.py"),
                                         str(capture), "--profile", profile_for_viewpoint(args.viewpoint),
                                         "--output", str(evidence / "validation.json")],
                                        check=False, timeout=15)
            if validation.returncode:
                raise RuntimeError("capture image validation failed")
    except (OSError, RuntimeError, subprocess.SubprocessError, KeyboardInterrupt) as error:
        failure = str(error) or "interrupted"
    finally:
        if args.missing_leaf:
            try:
                metadata["leaf_after"] = artifact_identity(leaf_asset)
                if metadata["leaf_before"] != metadata["leaf_after"]:
                    failure = failure or "host leaf asset changed during namespace fixture"
            except OSError as error:
                failure = failure or f"host leaf asset verification failed: {error}"
        try:
            records = journal.read("--after-cursor", baseline["__CURSOR"])
            latest = journal.latest()
            (evidence / "kernel-after.json").write_text(json.dumps(records, indent=2) + "\n")
            if latest["_BOOT_ID"] != baseline["_BOOT_ID"] or any(FAULT.search(r["MESSAGE"]) for r in records):
                failure = failure or "kernel fault or boot discontinuity during software check"
        except RuntimeError as error:
            failure = failure or str(error)
        metadata["failure"] = failure
        (evidence / "run.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(f"Software capture {'FAILED: ' + failure if failure else 'passed'}; evidence: {evidence}")
    return 1 if failure else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError) as error:
        print(f"Software capture refused: {error}", file=sys.stderr)
        raise SystemExit(2)
