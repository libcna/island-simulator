#!/usr/bin/env python3
"""Preflight or boot a project-owned, GPU-inaccessible Android emulator on Linux.

This is NOT AMD qualification. Never changes the hardware stop latch, shared adb server,
existing AVDs, or SDK images. Application installation requires an explicit --apk.
"""
import argparse
import datetime
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import zipfile

from gpu_incident_qualification import FAULT, Journal, MonitorError, artifact_identity, CAPTURE_VIEWPOINTS
from run_software_capture import (run_monitored_capture, validate_monkey_climb_stages,
    validate_snake_runtime_samples, validate_spider_render_log, validate_spider_lighting)

AVD = "IslandSoftwareApi35"
IMAGE = "system-images/android-35/google_apis_playstore/x86_64"
OWNER = "island-simulator isolated Android software runtime v1\n"


def emulator_command(sdk):
    return [str(sdk / "emulator/emulator"), "-avd", AVD, "-gpu", "swiftshader",
            "-no-window", "-no-audio", "-no-snapstorage", "-no-snapshot",
            "-accel", "on", "-cores", "2", "-memory", "2048", "-port", "5558",
            "-camera-back", "none", "-camera-front", "none",
            "-crash-report-mode", "disabled", "-no-metrics"]


def isolated_command(root, state, evidence, sdk, home, boot, game_arguments=()):
    command = ["bwrap", "--ro-bind", "/", "/", "--dev", "/dev",
               "--dev-bind", "/dev/kvm", "/dev/kvm", "--proc", "/proc",
               "--unshare-pid", "--unshare-ipc", "--unshare-net", "--cap-drop", "ALL",
               "--tmpfs", "/tmp", "--tmpfs", "/run", "--die-with-parent",
               "--bind", str(state), str(state), "--bind", str(evidence), str(evidence),
               "--bind", str(state / "android"), str(home / ".android"),
               sys.executable, str(root / "tools/run_android_software.py"),
               "--worker", "--state", str(state), "--evidence", str(evidence),
               "--sdk", str(sdk), "--host-net", os.readlink("/proc/self/ns/net")]
    if boot:
        command.append("--boot")
    command.extend(game_arguments)
    return command


def isolated_environment(sdk, state):
    # Allowlist: do not inherit driver overrides, preload libraries, adb endpoints or display
    # settings. HOME retains its real value; its .android directory is privately overmounted.
    result = {key: os.environ[key] for key in ("HOME", "USER", "LOGNAME") if key in os.environ}
    result.update(PATH="/usr/bin:/bin", LANG="C.UTF-8", PYTHONDONTWRITEBYTECODE="1",
                  ANDROID_SDK_ROOT=str(sdk), ANDROID_EMULATOR_HOME=str(state / "android"),
                  ANDROID_AVD_HOME=str(state / "android/avd"),
                  ANDROID_ADB_SERVER_PORT="5038", ANDROID_SERIAL="emulator-5558",
                  ADB_SERVER_SOCKET="tcp:localhost:5038")
    return result


def require_private_devices(host_net):
    if os.path.exists("/dev/dri") or os.path.exists("/dev/nvidia0"):
        raise RuntimeError("host GPU device access is forbidden")
    if os.readlink("/proc/self/ns/net") == host_net:
        raise RuntimeError("private network namespace was not established")
    if any(os.environ.get(key) for key in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY")):
        raise RuntimeError("display environment leaked into the emulator namespace")
    with open("/dev/kvm", "rb+", buffering=0):
        pass
    if len(os.sched_getaffinity(0)) > 8:
        raise RuntimeError("CPU affinity exceeds the eight-core budget")


def emulator_processes_alive():
    """Any live emulator or QEMU on the host; the owned instance always matches too."""
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            command = (entry / "cmdline").read_bytes()
        except OSError:
            continue
        if b"qemu-system" in command or b"/emulator/emulator" in command:
            return True
    return False


def clear_stale_avd_locks(avd):
    """A crashed owned emulator (2026-10-10, exit -11) leaves lock files that make the next
    launch fail before boot with "Running multiple emulators with the same AVD". Remove them
    only when no emulator process exists anywhere; never touch a live instance's locks."""
    locks = [path for path in avd.iterdir() if path.name.endswith(".lock")]
    if not locks:
        return
    if emulator_processes_alive():
        raise RuntimeError("AVD lock files exist and an emulator process is alive; refusing to launch")
    for path in locks:
        shutil.rmtree(path) if path.is_dir() else path.unlink()
    print(f"Removed {len(locks)} stale AVD lock file(s) left by a crashed emulator", flush=True)


def prepare_avd(state):
    """Create only fresh project-owned data; never seed from a user's AVD."""
    if (state / "OWNER").read_text() != OWNER:
        raise RuntimeError("runtime state ownership marker is missing or mismatched")
    avds = state / "android/avd"
    avd = avds / (AVD + ".avd")
    avd.mkdir(parents=True, exist_ok=True)
    clear_stale_avd_locks(avd)
    config = (f"AvdId={AVD}\nabi.type=x86_64\nhw.cpu.arch=x86_64\nhw.cpu.ncore=2\n"
              "hw.gpu.enabled=yes\nhw.gpu.mode=swiftshader\nhw.ramSize=2048\n"
              "hw.lcd.width=1280\nhw.lcd.height=720\nhw.lcd.density=240\n"
              "hw.keyboard=yes\nhw.mainKeys=no\nhw.sdCard=no\nhw.audioInput=no\n"
              "hw.camera.back=none\nhw.camera.front=none\n"
              # This installed Play image expands smaller requests to its 6 GiB minimum.
              # The data image is sparse; retain one reusable instance, not repeated copies.
              "disk.dataPartition.size=6442450944\nfastboot.forceColdBoot=yes\n"
              "fastboot.forceFastBoot=no\nshowDeviceFrame=no\n"
              f"image.sysdir.1={IMAGE}/\ntag.id=google_apis_playstore\n"
              "target=android-35\nPlayStore.enabled=true\nvm.heapSize=256\n")
    path = avd / "config.ini"
    if path.exists() and parse_properties(path.read_text()) != parse_properties(config):
        raise RuntimeError("owned AVD configuration changed; review it instead of overwriting")
    if not path.exists():
        path.write_text(config)
    pointer = avds / (AVD + ".ini")
    expected = f"avd.ini.encoding=UTF-8\npath={avd}\ntarget=android-35\n"
    if pointer.exists() and pointer.read_text() != expected:
        raise RuntimeError("owned AVD pointer changed; refusing to follow it")
    if not pointer.exists():
        pointer.write_text(expected)


def parse_properties(text):
    """The emulator reformats INI spacing on exit; compare exact keys/values, not bytes."""
    result = {}
    for line in text.splitlines():
        if not line.strip():
            continue
        key, separator, value = line.partition("=")
        key, value = key.strip(), value.strip()
        if not separator or not key or key in result:
            raise RuntimeError("invalid or duplicate AVD configuration property")
        result[key] = value
    return result


def validate_guest_renderer(text):
    lines = [line for line in text.splitlines() if re.match(r"\s*GLES:", line)]
    if len(lines) != 1 or "SwiftShader" not in lines[0]:
        raise RuntimeError("guest SurfaceFlinger did not confirm SwiftShader GLES")
    if re.search(r"Radeon|radeonsi|NVIDIA|Intel.*Graphics", lines[0], re.I):
        raise RuntimeError("guest reports a hardware renderer")
    return lines[0].strip()


MATERIAL_PATHS = ("Content/terrain/forest-floor.png", "Content/vegetation/leaf-lamina.png",
                  "Content/wildlife/monkey-fur.png", "Content/structures/hut-palm-fibre.png",
                  "Content/wildlife/goat-fur.png", "Content/wildlife/spider-setae.png",
                  "Content/wildlife/boar-bristle.png", "Content/wildlife/bear-fur.png",
                  "Content/structures/shipwreck-weathered-wood.png",
                  "Content/structures/cave-rock.png")


def validate_apk(apk, prepared_root=None):
    if prepared_root is None:
        prepared_root = Path(__file__).resolve().parent.parent / "android/app/build/generated/islandPreparedMaterials"
    materials = {}
    with zipfile.ZipFile(apk) as package:
        libraries = [name for name in package.namelist() if name.startswith("lib/") and name.endswith(".so")]
        if "lib/x86_64/libmain.so" not in libraries or any(not name.startswith("lib/x86_64/") for name in libraries):
            raise RuntimeError("software emulator requires an x86_64-only APK containing libmain.so")
        for relative in MATERIAL_PATHS:
            source = prepared_root / relative
            name = "assets/" + relative
            if package.namelist().count(name) != 1:
                raise RuntimeError(f"APK must contain exactly one prepared material PNG: {relative}")
            expected = artifact_identity(source)
            if package.getinfo(name).file_size != source.stat().st_size:
                raise RuntimeError(f"APK material PNG size differs from its prepared source: {relative}")
            digest = hashlib.sha256()
            with package.open(name) as stream:
                for block in iter(lambda: stream.read(65536), b""):
                    digest.update(block)
            if digest.hexdigest() != expected["sha256"]:
                raise RuntimeError(f"APK material PNG hash differs from its prepared source: {relative}")
            materials[relative] = expected
    return {**artifact_identity(apk), "prepared_materials": materials}


def game_environment(args):
    environment = os.environ.copy()  # Already allowlisted and isolated by the parent.
    environment.update(ISLAND_ANDROID_SMOKE_TIMEOUT_SECONDS="120",
                       ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY=str(int(args.touch_selector)),
                       ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE=str(int(args.lifecycle)),
                       ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS=str(int(args.traversal)),
                       ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING=str(int(args.traversal)),
                       ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=str(int(args.traversal)),
                       ISLAND_ANDROID_SMOKE_ARGUMENTS=("" if args.viewpoint == "interactive" else
                                                       "--viewpoint," + args.viewpoint))
    return environment


def validate_game_options(args):
    if args.apk and not args.boot:
        raise ValueError("--apk requires --boot")
    if (args.touch_selector or args.lifecycle or args.traversal) and not args.apk:
        raise ValueError("input/lifecycle checks require --apk")
    if (args.touch_selector or args.traversal) and args.viewpoint != "interactive":
        raise ValueError("touch checks require --viewpoint interactive; fixed QA cameras reject input")


def validate_game_logs(directory, lifecycle, viewpoint=None):
    names = ["island-android-logcat.txt"]
    if lifecycle:
        names += ["island-android-hot-resume-logcat.txt", "island-android-cold-relaunch-logcat.txt"]
    logs = [(directory / name).read_text(errors="replace") for name in names]
    for log in logs:
        if re.search(r"FATAL EXCEPTION|Fatal signal|island-simulator: fatal error|GL_INVALID_|"
                     r"gl-error |gl-debug .*severity=high|shader.*(?:failed|error)|"
                     r"(?:stock-effect|material) fallback:", log, re.I):
            raise RuntimeError("game graphics/fatal/fallback diagnostic; inspect retained logcat")
    # A resumed process does not repeat graphics startup attribution, but a cold launch must.
    for log in [logs[0], *([logs[-1]] if lifecycle else [])]:
        if 'renderer="OPENGLES3"' not in log or not re.search(r'gl-renderer="[^"\n]*SwiftShader', log):
            raise RuntimeError("application did not confirm OPENGLES3 on SwiftShader")
        if "monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes" not in log:
            raise RuntimeError("application did not confirm the required monkey fur material")
        if viewpoint in ("snakes", "snake-head"):
            if "near snakes use a broad low temple, tapered blunt muzzle, raised amber eyes, and fitted vertical pupils" not in log:
                raise RuntimeError("application did not confirm the current snake head anatomy")
            validate_snake_runtime_samples(log)
            if viewpoint == "snake-head" and "snake-head QA isolates facial geometry while the companion body view retains habitat grass" not in log:
                raise RuntimeError("application did not confirm the isolated snake head view")
        if viewpoint == "spiders":
            validate_spider_render_log(log)
            validate_spider_lighting(log)
        if viewpoint == "goats":
            if "goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in log:
                raise RuntimeError("application did not confirm the goat coat texture")
            if not re.search(r"goat render proof: goats [1-6]/6 visible, 2 draw;", log):
                raise RuntimeError("application did not confirm visible goats and both material draws")
        if viewpoint == "boars":
            if "near boars use broad tapered upper legs with compact split hooves" not in log:
                raise RuntimeError("application did not confirm current boar limb anatomy")
            if not re.search(r"boars [1-6]/6 visible, 2 draw", log):
                raise RuntimeError("application did not confirm visible boars and both material draws")
        if viewpoint == "bear" and "bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes" not in log:
            raise RuntimeError("application did not confirm the bear fur texture")
        if viewpoint == "turtles" and "near turtles use a twenty-by-sixteen carapace surface, eighteen-sided seven-ring heads, and irregular mottled shell and skin tones" not in log:
            raise RuntimeError("application did not confirm the current sea-turtle surface")
        if viewpoint == "crocodiles" and "near crocodiles use raised eyes, nostrils, separate jaws, teeth, dorsal osteoderms, and wet mottled facet shading, with multi-section rounded snouts and lateral eyes" not in log:
            raise RuntimeError("application did not confirm the current crocodile surface")
        if viewpoint == "crocodiles" and not re.search(r"crocodiles [1-3]/3 visible \(near [1-3], far [0-3]\), 1 draw", log):
            raise RuntimeError("application did not confirm a visible single-batch crocodile")
        if viewpoint in ("hut", "hut-material") and "hut beam-aligned weathered grain and filtered thatch fibres" not in log:
            raise RuntimeError("application did not confirm the hut material")
        if viewpoint in ("hut", "hut-material") and "hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation" not in log:
            raise RuntimeError("application did not confirm the hut timber relief")
        if viewpoint in ("hut", "hut-material") and "hut four-pane muted glazing and braced plank door" not in log:
            raise RuntimeError("application did not confirm the hut facade details")
        if viewpoint in ("hut", "hut-material") and "hut raised veranda uses full-height outer posts, split rails, fascia, and three steps" not in log:
            raise RuntimeError("application did not confirm the raised hut veranda")
        if viewpoint in ("hut", "hut-material") and "hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports" not in log:
            raise RuntimeError("application did not confirm the enclosed hut gable")
        if viewpoint in ("hut", "hut-material") and "hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes" not in log:
            raise RuntimeError("application did not confirm the hut material texture")
        if viewpoint == "shipwreck" and "shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas" not in log:
            raise RuntimeError("application did not confirm the shipwreck wood texture")
        if viewpoint == "shipwreck" and "shipwreck weathered wood follows per-piece board and beam UVs" not in log:
            raise RuntimeError("application did not confirm shipwreck board-local UVs")
        if viewpoint == "shipwreck" and "shipwreck restrained rusted iron straps, rivets, hinges and rudder braces" not in log:
            raise RuntimeError("application did not confirm shipwreck iron fittings")
        if viewpoint == "grass" and "grass instances use area-preserving asymmetric tuft footprints" not in log:
            raise RuntimeError("application did not confirm grass footprint variation")
        if viewpoint == "monkey-climb":
            validate_monkey_climb_stages(log)
            if "monkey climb held summit presented after eight cycle stages" not in log:
                raise RuntimeError("climbing screenshot was not qualified after the complete cycle")


def run_game(args):
    if validate_apk(args.apk)["sha256"] != args.apk_sha256:
        raise RuntimeError("APK changed between parent attribution and isolated installation")
    root = Path(__file__).resolve().parent.parent
    output = args.evidence / "game"
    command = [str(root / "tools/run_android_runtime_smoke.sh"),
               str(args.sdk / "platform-tools/adb"), str(args.apk), str(output)]
    print("Game command: " + json.dumps(command), flush=True)
    try:
        with (args.evidence / "game-workload.log").open("w") as log:
            subprocess.run(command, env=game_environment(args), stdout=log, stderr=subprocess.STDOUT,
                           timeout=360 if args.lifecycle else 210, check=True)
    except (OSError, subprocess.SubprocessError):
        # The shell's last log snapshot precedes its presentation wait. Preserve a
        # fresh app-only log after failure, before teardown, to distinguish a late
        # GL/lifecycle error from a slow or stalled first presentation.
        failure_adb = [str(args.sdk / "platform-tools/adb"), "-P", "5038", "-s", "emulator-5558"]
        try:
            pid = subprocess.run([*failure_adb, "shell", "pidof", "com.libcna.islandsimulator"],
                                 capture_output=True, text=True, timeout=10, check=True).stdout.strip()
            if re.fullmatch(r"[0-9]+", pid):
                with (args.evidence / "game-failure-logcat.txt").open("w") as log:
                    subprocess.run([*failure_adb, "logcat", "--pid=" + pid, "-d", "-v", "threadtime"],
                                   stdout=log, stderr=subprocess.STDOUT, timeout=10, check=True)
        except (OSError, subprocess.SubprocessError) as error:
            print(f"Failure app log unavailable: {error}", file=sys.stderr)
        # Capture the actual failed presentation before the owned emulator is shut down.
        try:
            with (args.evidence / "failure-screen.png").open("wb") as image:
                subprocess.run([str(args.sdk / "platform-tools/adb"), "-P", "5038", "-s",
                                "emulator-5558", "exec-out", "screencap", "-p"],
                               stdout=image, stderr=subprocess.PIPE, timeout=10, check=True)
        except (OSError, subprocess.SubprocessError) as error:
            print(f"Failure screenshot unavailable: {error}", file=sys.stderr)
        raise
    validate_game_logs(output, args.lifecycle, args.viewpoint)
    (args.evidence / "game-proof.json").write_text(json.dumps({
        "apk_sha256": args.apk_sha256, "viewpoint": args.viewpoint,
        "touch_selector": args.touch_selector, "lifecycle": args.lifecycle, "traversal": args.traversal,
        "scope": "software GLES functional check; inspect images separately; not AMD/mobile performance",
    }, indent=2) + "\n")


def worker(args):
    require_private_devices(args.host_net)
    print("Isolation verified: private network, no DRM/display, accessible KVM, <=8 CPUs", flush=True)
    if not args.boot:
        return
    prepare_avd(args.state)
    adb = [str(args.sdk / "platform-tools/adb"), "-P", "5038", "-s", "emulator-5558"]

    def adb_run(*arguments, check=True):
        return subprocess.run([*adb, *arguments], capture_output=True, text=True,
                              timeout=10, check=check)

    process = None
    try:
        adb_run("start-server")
        with (args.evidence / "emulator.log").open("w") as log:
            command = emulator_command(args.sdk)
            print("Emulator command: " + json.dumps(command), flush=True)
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
            print(f"Owned emulator PID inside namespace: {process.pid}", flush=True)
            deadline = time.monotonic() + 150
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise RuntimeError(f"emulator exited before boot: {process.returncode}")
                result = adb_run("shell", "getprop", "sys.boot_completed", check=False)
                if result.returncode == 0 and result.stdout.strip() == "1":
                    break
                time.sleep(1)
            else:
                raise RuntimeError("software emulator did not boot within 150 seconds")
            surface = adb_run("shell", "dumpsys", "SurfaceFlinger").stdout
            (args.evidence / "surfaceflinger.txt").write_text(surface)
            renderer = validate_guest_renderer(surface)
            (args.evidence / "guest-properties.txt").write_text(adb_run("shell", "getprop").stdout)
            (args.evidence / "guest-proof.json").write_text(json.dumps({
                "renderer": renderer, "serial": "emulator-5558", "application_installed": False,
                "scope": "software emulator boot only; not Android game or AMD qualification",
            }, indent=2) + "\n")
            print("Boot passed: " + renderer, flush=True)
            if args.apk:
                # Match Android CTS setup for the first-run fullscreen education overlay.
                # This is only our owned AVD, never a connected personal device. See build-android.md.
                adb_run("shell", "settings", "put", "secure", "immersive_mode_confirmations", "confirmed")
                confirmation = adb_run("shell", "settings", "get", "secure",
                                       "immersive_mode_confirmations").stdout.strip()
                if confirmation != "confirmed":
                    raise RuntimeError("owned guest fullscreen introduction was not acknowledged")
                print("Owned guest fullscreen introduction: confirmed", flush=True)
                run_game(args)
    finally:
        active_error = sys.exc_info()[1]
        cleanup_errors = []
        if process is not None:
            try:
                adb_run("emu", "kill", check=False)
            except (OSError, subprocess.SubprocessError) as error:
                cleanup_errors.append(str(error))
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                cleanup_errors.append("emulator required forced termination")
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=3)
            print(f"Owned emulator exit code: {process.returncode}", flush=True)
            if process.returncode != 0:
                cleanup_errors.append(f"emulator exit code {process.returncode}")
        # This server is in our private network/PID namespace, never the shared host server.
        try:
            adb_run("kill-server")
        except (OSError, subprocess.SubprocessError) as error:
            cleanup_errors.append(str(error))
        if cleanup_errors:
            message = "owned emulator cleanup: " + "; ".join(cleanup_errors)
            if active_error is None:
                raise RuntimeError(message)
            print(message, file=sys.stderr, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--boot", action="store_true", help="also boot the isolated emulator")
    parser.add_argument("--apk", type=Path, help="install/test this x86_64 APK after software renderer proof")
    parser.add_argument("--viewpoint", choices=sorted(CAPTURE_VIEWPOINTS | {"spiders", "goats", "hut", "hut-material", "boars", "snakes", "snake-head", "interactive", "monkey-climb"}),
                        default="clearing-edge")
    parser.add_argument("--touch-selector", action="store_true")
    parser.add_argument("--lifecycle", action="store_true")
    parser.add_argument("--traversal", action="store_true", help="require land footsteps and swimming via touch")
    parser.add_argument("--apk-sha256", help=argparse.SUPPRESS)
    parser.add_argument("--worker", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--state", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--evidence", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--host-net", help=argparse.SUPPRESS)
    args = parser.parse_args()
    try:
        validate_game_options(args)
    except ValueError as error:
        parser.error(str(error))
    if args.worker:
        worker(args)
        return
    root = Path(__file__).resolve().parent.parent
    sdk = args.sdk.resolve(strict=True)
    for item in ("emulator/emulator", "platform-tools/adb", f"{IMAGE}/system.img",
                 f"{IMAGE}/vendor.img", f"{IMAGE}/kernel-ranchu", f"{IMAGE}/ramdisk.img"):
        if not (sdk / item).is_file():
            raise RuntimeError(f"required installed SDK file is missing: {sdk / item}")
    if shutil.which("bwrap") is None:
        raise RuntimeError("bubblewrap is required; no unisolated fallback is permitted")
    game_arguments = []
    apk_identity = None
    if args.apk:
        args.apk = args.apk.resolve(strict=True)
        apk_identity = validate_apk(args.apk)
        game_arguments = ["--apk", str(args.apk), "--apk-sha256", apk_identity["sha256"],
                          "--viewpoint", args.viewpoint]
        if args.touch_selector:
            game_arguments.append("--touch-selector")
        if args.lifecycle:
            game_arguments.append("--lifecycle")
        if args.traversal:
            game_arguments.append("--traversal")
    os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:8])
    with (root / "build/gpu-qualification.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        journal = Journal()
        baseline = journal.latest()
        base = root / "build/android-software"
        state = base / "state"
        if state.exists():
            if (state / "OWNER").read_text() != OWNER:
                raise RuntimeError("existing runtime directory is not owned by this tool")
        else:
            state.mkdir(parents=True)
            (state / "OWNER").write_text(OWNER)
        (state / "android").mkdir(exist_ok=True)
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
        suffix = "-" + args.viewpoint if args.apk else "-boot" if args.boot else "-preflight"
        evidence = base / (stamp + suffix)
        evidence.mkdir()
        command = isolated_command(root, state, evidence, sdk, Path.home(), args.boot, game_arguments)
        environment = isolated_environment(sdk, state)
        metadata = {"command": command, "environment": environment, "kernel_before": baseline,
                    "boot_requested": args.boot, "result": "running", "apk": apk_identity,
                    "supervisor_pid": os.getpid(), "gate": artifact_identity(Path(__file__)),
                    "smoke_helper": artifact_identity(root / "tools/run_android_runtime_smoke.sh"),
                    "touch_geometry": artifact_identity(root / "tools/android_touch_geometry.py")}
        report = evidence / "run.json"
        report.write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"Evidence: {evidence}", flush=True)
        try:
            result = run_monitored_capture(command, root, environment, evidence, journal,
                                           baseline, timeout=600 if args.apk else 200 if args.boot else 15,
                                           private_devices=False)  # Inner sandbox exposes only its validated KVM device.
            final = journal.read("--after-cursor", baseline["__CURSOR"])
            (evidence / "kernel-final.json").write_text(json.dumps(final, indent=2) + "\n")
            if any(record["_BOOT_ID"] != baseline["_BOOT_ID"] or FAULT.search(record["MESSAGE"])
                   for record in final):
                raise MonitorError("kernel fault or boot discontinuity at final verification")
            if result:
                raise RuntimeError(f"isolated worker exited {result}; inspect workload/emulator logs")
            if args.boot and not (evidence / "guest-proof.json").is_file():
                raise RuntimeError("boot worker did not produce guest renderer proof")
            if args.apk and not (evidence / "game-proof.json").is_file():
                raise RuntimeError("game worker did not produce application proof")
            metadata["result"] = "passed"
        except (OSError, RuntimeError, subprocess.SubprocessError) as error:
            metadata["result"] = "failed"
            metadata["error"] = str(error)
            raise
        finally:
            report.write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        sys.exit(f"Android software gate: {error}")
