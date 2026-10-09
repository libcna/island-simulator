# island-simulator handoff

Prepared 2026-10-09 at the operator's request. Finish the current bounded grass
walking review, commit and push the accumulated project work, then stop. The
overall development campaign is **not complete**. Resume only on new operator
direction; do not interpret this handoff as permission for unattended GPU tests.

## Read first: unresolved desktop-threatening GPU failure

**Do not run this game on the host AMD GPU. Do not clear the hardware stop latch.**
The October 8 GPUVM faults caused a MODE2 reset, VRAM loss and GNOME logout. A
later single-client jungle benchmark also faulted: concurrency is not necessary.
The actual invalid operation/root cause has not been identified or fixed.
Clean software-renderer tests are not evidence that the hardware workload is safe.

Read these before incident work:

- `docs/incidents/2026-10-08-amdgpu-gpuvm-reset.md`: reconstruction, exact retained
  commands, attribution limits, source investigation and recovery observations.
- `docs/incidents/capture-evidence-inventory.md`: retained versus missing evidence
  and the acceptance gate for any future hardware capture.
- `docs/incidents/mesa-recorder-readiness.md` and `debian-mesa-patch-audit.md`:
  why merely enabling GALLIUM_DDEBUG or installing a driver is not a qualified plan.
- `docs/incidents/process-containment.md`: detached-process lifetime fix and
  software device isolation. This supervisor fix is not the GPU fault fix.
- `docs/incidents/evidence/2026-10-08/README.md`: archived evidence hashes.
- `docs/incidents/maintainer-sharing-summary.md` and `upstream-triage-draft.md`:
  drafts only; no upstream issue or attachments have been submitted. The request
  to push this repository is not permission to file issues or upload raw evidence
  to another service. Obtain separate approval for that escalation.

Original fault PID 3497873 / TID 3497891 most likely ran the jungle benchmark;
direct command-line mapping is **not proven**. `island-sim:cs0` is Mesa's AMD
submission worker, not a game-created rendering thread. Later SwapBuffers and
MakeCurrent crashes followed the reset and do not establish its cause.
Serial fault PID 3593099 / TID 3593107 is directly mapped to the jungle benchmark.
The partial devcoredump has the graphics ring but lacks referenced userspace IBs
and uploaded descriptors. `/sys/class/devcoredump/devcd3/data` expired; do not
ask the operator to read it again or induce a new fault to replace it.

Local latch: `build/gpu-incident-evidence/QUALIFICATION_STOP.json`, SHA-256
`c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1`.
It is ignored local state: a fresh clone lacking this file is **not** hardware
authorization. A new hardware attempt requires a qualified bounded recorder,
a discriminating hypothesis and explicit approval of the residual desktop risk.

## Product and actual state

Peaceful C++23 first-person exploration of an approximately 500 m tropical
island, fixed sunny daytime. No quests, combat, survival, crafting, inventory,
progression, changing weather or day/night cycle. All repository text is English.
`PLAN.md` is the detailed campaign ledger, not superseded by this shorter handoff.
The original budget was roughly 300 hours with a requested realism extension;
it is not a requirement to spend time unnecessarily or a reliable remaining-time
estimate. The visuals are still stylized, not a completed realism deliverable.

Already implemented:

- Mouse look, WASD and arrows, ground/slope collision, swimming and soft offshore
  current boundary. Shift **toggles** walk/run (4.3/8 m/s); it is not hold-to-run.
  Android has touch controls. See controls and platform limitations in README.
- Island terrain, beaches, paths/clearings, animated ocean/shoreline, distant
  islets, palms, layered jungle, rocks, logs, driftwood, litter and mesh grass.
- Original 20 m split shipwreck with exposed structure and 12 separate debris
  pieces, plus six conservative collision proxies. This is a large ship, not a
  small boat, and adds no objectives.
- Birds, fish schools, nonhostile shark, skinned monkeys, crabs, butterflies,
  turtles and six wild boars. Boar feet sample terrain; stance locking/stride
  synchronization and broader animation polish remain unfinished.
- Layered procedural ambience and spatial wildlife/player audio; not a finished
  soundscape quality claim. Deterministic simulation/placement and extensive CPU
  regression coverage, runtime identity logging and debug views.
- Native OpenGL 3.3, WebGL 2 and Android GLES 3 builds. Current actual runtime
  evidence uses llvmpipe or SwiftShader only. ARM64 compiles/packages but its
  runtime and physical-phone performance remain unverified.

Recent work in the accumulated commit includes canopy structure/lighting/culling,
supported shrubs and curved/split/fern leaves, forest-floor colonies, rendered-
triangle root/grass/litter contact, boar ground contact, original image materials,
portable ocean/shore material work and safer validation harnesses. Detailed
decisions and measured limitations are in the corresponding `docs/*.md` files.

Latest material pass separates grass pigment from fixed baked side/height
lighting. Actual normals supply directional light; normalized-height occlusion
is separate. Filtered longitudinal veins/midrib use one added float per vertex;
seed heads skip the detail. Custom stride is 40 bytes / nine attributes, +3,856
bytes total across shared meshes, with no new draw, texture or instance. Unlit
stock fallback preserves its old colour policy. See `docs/grass-surface-material.md`.

## Dependency and machine boundaries

- CNA `next` and Sharp Runtime `next` are strictly **read-only**, including their
  files, Git metadata and branch state. Do not patch even confirmed framework
  bugs. Use narrow game-side adapters only when justified; record actual defects
  in `BUGS.md`, not every unsupported feature or application mistake.
- `dependencies.lock` is authoritative. Current pins: CNA
  `97783d1e090c6d84f4b6a6d7746516e65184569e`, Sharp Runtime
  `21c06910f7cf887cceebb45200e07b43dd45fef4`, EasyGL develop
  `1edf4c1a4708149b6a699365f43b58aa467c345f`, MetaGL develop
  `40829ca76a4e99511a7734297ee1dc0e1b926320`.
- Existing CNA export: `build/dependencies/cna-next`. See
  `tools/materialize_cna_next.sh` and `cmake/VerifyDependencies.cmake`; do not
  rematerialize or switch a shared tree just to bypass a pin mismatch.
- Read `../AGENTS.md`. Every build must use `CCACHE_DIR=/rv/cnaccache` and
  `CCACHE_BASEDIR=/rv`. Never set `CCACHE_DISABLE` or `CCACHE_MAXSIZE`, create
  another cache, or silently bypass it. Escalate permissions when sandbox writes
  to the shared cache are blocked.
- Use two build workers here; never exceed the user's eight-core cap. Do not
  overlap heavy builds/render workloads. Web uses EMCC_CORES=2/BINARYEN_CORES=2.
- Reuse `build/` (native), `build-probe/` (Web and small probes), existing Android
  intermediates. Preserve `build-consumer/` historical incident binaries. Never
  build in `/tmp` or create ticket/date-suffixed build trees.
- Keep original artwork and incident evidence. Do not commit ignored builds,
  browser profiles, SDKs, APKs, credentials or temporary captures indiscriminately.
  Last disk check: 359 GiB free. Only reproducible rolling packages were replaced.

## Build and test commands

Run from the repository root. Existing presets are configured; reconfigure only
if genuinely needed. More setup details: `docs/build-linux.md`, `build-web.md`,
`build-android.md`. Host texture preparation requires Python 3.10+ and Pillow
(here 3.13.5 / 11.1.0); no Python/Pillow is shipped in the game.

```sh
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build --preset linux-debug -j2
ctest --test-dir build -LE display --output-on-failure -j2

# Safe, monitored native software capture; not an interactive AMD launch.
sg systemd-journal -c 'python3 tools/run_software_capture.py build grass'
# Isolated missing-leaf fixture: never move/delete the actual asset.
sg systemd-journal -c 'python3 tools/run_software_capture.py build ground-broadleaf --missing-leaf'

source /home/robertvokac/emsdk/emsdk_env.sh
export EMCC_CORES=2 BINARYEN_CORES=2
cmake --build --preset web-webgl2 -j2
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope litter --timeout 300'
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope grass-motion --timeout 420'
tools/package_web_release.sh build-probe

export ANDROID_SDK_ROOT=/home/robertvokac/Android/Sdk
tools/run_android_gradle.sh :app:assembleDebug :app:lintDebug -PislandAbi=x86_64
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint grass'
# Preserve the rolling tested emulator APK, then restore the default phone ABI.
cp android/app/build/outputs/apk/debug/app-debug.apk build/android-software/island-x86_64.apk
tools/run_android_gradle.sh :app:assembleDebug :app:lintDebug
```

Do **not** run unrestricted display CTest or the historical parallel GPU command.
The software runners share `build/gpu-qualification.lock`, monitor kernel logs
before/during/after, fail closed, and contain detached descendants with bubblewrap.
Native/browser private devices hide DRM; Android has an audited inner sandbox
exposing KVM but not the host GPU/display, private ADB port 5038, emulator port
5558, two vCPUs and 2 GiB. KVM is available; no new root command is needed.
Journal access through `sg systemd-journal` works. Never bypass inaccessible logs.
Stop on any new GPU fault, including unrelated host activity. Do not restart a
still-running tool job just because an output poll timed out.

The browser harness uses acknowledged mouse/F1 gestures: short fixed pulses were
unreliable. Preserve evidence and fix input ownership, not assertions, if it fails.
The Android wrapper removes only the previous generated debug APK before assembly
to avoid AGP ZIP bloat; it retains expensive intermediates and shared cache.

## Evidence, screenshots and audits

All paths below are relative to this repository. Most runtime evidence is ignored
local data and is **not included in Git**. A fresh clone must rerun safe checks;
do not silently present absent local evidence as reproduced. Archived incident
sources and small reports under `docs/incidents/evidence/` are versioned separately.

- Latest native grass: `build/software-captures/20261009T130135.460537Z-grass/`.
  Baseline: `20261009T124913.245139Z-grass/` in the same parent. Each has PPM,
  per-PID/runtime/workload logs, run metadata and kernel delta.
  Convenience PNGs: `build-probe/island-grass-material-before.png` and
  `build-probe/island-grass-material-after.png`.
- Missing-leaf test: `build/software-captures/20261009T124801.964528Z-ground-broadleaf/`.
  Passed and inspected; original hash/size/inode/device/mtime unchanged. Normal
  qualification still rejects all fallback paths; the fixture accepts only its
  specific diagnosed procedural-leaf fallback.
- Web still views: `build/browser-software/20261009T130317.675153Z-chrome-litter/`.
  Both views pass 60 intervals, GL error zero, live context, empty kernel deltas.
  `island-browser-litter-grass.png` is the inspected grass image.
- Final real-input walk: `build/browser-software/20261009T131053.908704Z-chrome-grass-motion/`.
  Passed; results and inspection limits are recorded below.
- Android: `build/android-software/20261009T130636.434000Z-grass/`.
  `game/island-android.png` and `island-android-logcat.txt`: inspected 1280x720 image,
  PID 1770/TID 1868, correct material marker, no kernel fault, emulator exit zero.
- `docs/grass-surface-material.md` contains current APK sizes/hashes. Current
  Web package is `build-probe/web-release`: 10,263,469 raw / 3,498,026 gzip bytes.
  Last completed non-display suite: 33/33 passed in 22.97 s.
- Original materials: `assets/art-studies/`, prompts/provenance in
  `ASSET_LICENSES.md` and `assets/manifest.json`. Retain both selected and rejected
  studies needed for the documented review. Runtime prepared floor/leaf PNGs
  total 766,261 bytes, CPU-tested pixel-equivalent to the original runtime filter;
  see `docs/prepared-materials.md`. Do not mistake image generation for runtime
  screenshot evidence or claim these artistic images are measured PBR textures.
- Large incident material remains in `build/gpu-incident-evidence/`, including
  `software-jungle-trace/jungle.trace`, and historical `build-consumer/`.
  Use the incident report/inventory for exact authoritative paths and hashes;
  do not clean these as disposable build output. Never replay traces on AMD.

## Next tasks after a new operator instruction

1. Read the final walking result and PLAN.md. Keep the hardware stop boundary.
2. Improve nonuniform ground cover and tree-trunk/lighting presentation based on
   actual first-person images. The current ground remains sparse, tufts repeat,
   and trunks/overall lighting look stylized. Do not add unrelated game systems.
3. Review discrete vegetation/grass LOD transitions and normal response under wind;
   filtered grain alone does not prove temporal stability. Maintain portable
   GLSL 330/300 ES and measured draw/memory budgets.
4. Continue wildlife animation/contact polish (boar stance/stride and monkey
   transitions), coastline/water integration and soundscape quality. Repeatedly
   run the changed system, not just compile it.
5. Requalify full Web input/lifecycle and Android touch/lifecycle after relevant
   changes. ARM64 physical-device runtime/performance and Linux AMD hardware
   acceptance are still open. Never promote old pre-incident results to current
   qualification.
6. Incident work is a separate approval-gated track: bounded durable failing-IB /
   uploaded-descriptor evidence is missing. No confirmed framework repair or
   root-cause claim is justified by the current software audits.

## Final bounded task result

The existing Web build was already up to date (no compiler work). Contained
Chrome/SwiftShader run `20261009T131053.908704Z-chrome-grass-motion` passes with
real W/S and pointer-capture input: 19.0084 m outward, 0.7167 m return error,
94 telemetry samples, 47 grass LOD population states and 20 canopy transitions.
All 98 recorded compositor frames are distinct, 2,565,775 bytes total, with no
truncation or recording error. Thirty additional frame intervals pass; GL error
is zero, context remains live and live/final kernel deltas are empty.

Inspected full-size `island-browser-grass-motion-step-{0,3,6}.png` and recorded
`island-browser-grass-motion-16-frame-{025,026,027,075}.jpg` in that run directory.
No gross geometry/material break was visible in those samples. Sparse coverage
and repeated tufts remain conspicuous. This sampled 640x360 compressed sequence
cannot establish absence of fine shimmer or seamless LOD changes; do not claim
that this review proves photorealism or high-frame-rate temporal stability.

Owned host browser PID 4112991 is gone, with no remaining game/emulator/compositor
process. Hardware latch unchanged; 359 GiB free. No incident evidence was removed.
Final non-display regression suite passes 33/33 groups in 22.23 s. No source
implementation was changed during this final bounded review; only documentation
was finalized after the already-validated material pass.
The operator's final action is an ordinary push of branch `develop` to `origin`,
including previously unpushed project history, followed by stopping this campaign.
