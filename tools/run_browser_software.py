#!/usr/bin/env python3
"""Supervise one software browser suite, with no host GPU devices and no uncontained fallback."""
from __future__ import annotations

import argparse
import datetime
import fcntl
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from gpu_incident_qualification import FAULT, Journal, MonitorError, artifact_identity
from run_software_capture import run_monitored_capture, validate_monkey_climb_stages, validate_snake_runtime_samples

SCOPES = ("full", "terrain", "wildlife-foliage", "coast", "grounding", "vegetation", "grass-motion", "litter", "materials", "monkeys", "monkey-climb", "fullscreen", "flight", "snakes")


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
