# Android OpenGL ES 3 Build

## Current status

The project has an Android application for API 24 and newer. The native game target builds as
`libmain.so`, uses CNA's SDL3 activity bridge, and selects the `OPENGLES3` renderer explicitly.
The supported package ABIs are `arm64-v8a` for devices and `x86_64` for emulator testing. Both
debug variants build; the x86_64 APK has also installed and rendered the current complete island
scene on an API 35 emulator.

The Android UI is fixed landscape/fullscreen and includes dual-zone touch movement/look controls
with generated on-screen affordances. It uses the same restrained walking/swimming camera-motion
model as desktop, although an on-screen opt-out is not implemented yet. The host loads `kvm` and
`kvm_amd`, exposes `/dev/kvm` as device `10:232`, and grants the developer access through the
`kvm` group. Android Emulator 36.5.11 now runs the API 35 x86_64 AVD with KVM and host-GPU
gfxstream. The complete scene, independent movement/look gestures, same-process hot resume, and
AAudio pause/resume are validated there. Physical-device ergonomics, driver coverage, thermals,
and extended performance remain open.

If another host reports KVM at misc-device `10:232` but lacks the device node after its modules are
loaded, recreate it through udev (preferred) or explicitly as root, then start a fresh shell so the
configured `kvm` group is active:

```bash
sudo udevadm trigger --subsystem-match=misc --sysname-match=kvm --action=add
sudo udevadm settle
ls -l /dev/kvm
```

Only if udev still does not create the node:

```bash
sudo mknod -m 660 /dev/kvm c 10 232
sudo chown root:kvm /dev/kvm
```

Do not run the emulator or game as root. Verify access with
`$ANDROID_SDK_ROOT/emulator/emulator -accel-check`; a shell opened before group membership changed
can instead launch the emulator or validation command through `sg kvm`.

The current arm64 package includes the portable procedural sand/soil terrain material with
near-field interior microvariation, sparse curved leaf litter, and close dry-sand pebble/shell
flecks, the broadleaf jungle with
branch-supported near crowns, rounded far crowns, per-tree LOD, pointed-leaf near bushes with a
compact far tier, and forest-floor plants including upright-broadleaf and folded split-leaf near
variants with per-instance LOD/range selection,
speed-responsive player bending plus curved, two-sided daylight shading, per-instance blade
stature, and a sparse juvenile underlayer in the instanced mesh grass,
segmented near-palm leaflets, procedural near-trunk scars, restrained coconut clusters, and
staggered per-instance near/far LOD,
157 land rocks and 14 emergent reef outcrops with near-only strata/lichen plus a low fixed-sea-level
wet band, 12 fallen logs, 31 sparse beach-driftwood pieces with a three-limb forked near variant,
an authored three-piece cove arc, and per-instance large-prop range/LOD, 63 fallen palm
fronds, 265 near-field shell/pebble clusters, including a five-rock/one-log authored clearing-
landmark composition, 24 simulated
procedural birds with slow radial route drift, banked turns, and intermittent glides, eight fish schools,
with depth-following pitch, banked turns, forked tails, 51/12-vertex CPU-lit near/far geometry,
single-surface fins, and near head detail, one occasional
non-hostile offshore shark with matching pose and silhouette treatment, seven GPU-skinned
procedural monkeys with bounded calm-state head attention, fixed sunny sky
with layered horizontal high wisps,
five fogged distant islets with 11 simplified palm silhouettes,
one bounded terrain-conforming contact-shadow batch for near palms, canopy trees, land rocks,
wood, and monkeys,
animated shore-following foam, and procedural surf/breeze/jungle, two sparse wave accents, player
water-movement audio, two water entry/exit accents, movement-driven shader-integrated water
ripples, short hysteretic entry/exit surface bursts, a procedural near-surface swimming veil with
offshore-current modulation, a deeper current-responsive undertow mix in the existing water loop,
and a pooled
positional voice with two bird and two monkey phrases. That package
builds and passes application lint. The current x86_64 runtime gate constructed this complete scene,
reported its biome-aware grass path, and completed a GLES 3 first draw under KVM. The inspected
overview establishes integrated rendering, including the corrected translucent touch HUD, but is
not detailed enough to qualify every layer's close appearance.
The latest single-ABI arm64 debug qualification build produced an 18,747,868-byte APK with a
15,937,584-byte stripped `libmain.so` and passed `:app:lintDebug`. It includes the narrowed,
noise-mottled portable footpath, close sand granulation, sparse soft-edged pebble/shell flecks,
and shoreline-raked ripples, irregular
faceted near-palm root wedges, reused crossing-wave ocean colour, reduced CPU-lit fish/shark
geometry, the shared run-capable input resolver used by hardware keyboards, and shared-buffer
terrain cell batching. It also validates every indexed procedural mesh plus the monkey fourteen-
bone hierarchy/palette, clips, tracks, and key transforms before GPU upload. A fresh x86_64 build of the
same sources and current CNA pin passed the KVM/host-GPU GLES 3 first-frame gate at 2003.189 ms
total startup: 861.991 ms world generation, 4.482 ms framework initialization, 1075.736 ms content
creation, 20.038 ms before the first draw, and 40.942 ms for the first draw body. The inspected
2400 by 1080 capture retains the complete scene and clean translucent touch controls. The default
arm64 package was then restored. Public terrain queries additionally fail closed
before non-finite or out-of-domain values can reach procedural noise-cell conversion. The APK verifies
with signature
scheme v2, contains only the selected ABI, and remains the same size after an immediate no-change
incremental assembly.

A later x86_64 pass exercised the denser 34-blade near-grass geometry and its new GLSL ES 300
per-blade variation on the same KVM/host-GPU configuration. The complete scene reached a clean
`OPENGLES3` first frame with no shader error in 3946.689 ms total, including a 103.050 ms first draw
body. Its inspected 2400 by 1080 screenshot contains the expected varied grass silhouettes and
clean touch-overlay blending. This is functional shader evidence, not a physical-mobile frame-time
or close-grass quality claim. The arm64-only package above was restored afterward.
The subsequent renderer-independent touch hardening rejects poisoned display/contact values before
they reach movement, camera, or HUD state. Core regressions cover cancellation and recovery; the
same sources compile into the current WebGL 2 and arm64 GLES 3 packages. The Android runtime was
not repeated for input values that a normal platform touch event cannot produce.
A later x86_64 KVM run exercised the new touch-run path. A sustained full-travel movement gesture
advanced the player through the complete scene and visibly changed the premultiplied movement ring
and thumb from green to warm gold without restoring the former rectangular blend artifact. Core
regressions prove the 92-percent engage and 78-percent release hysteresis; physical-device feel
and threshold tuning remain open. The headless emulator was stopped and the arm64 package above
was restored afterward.

The game-owned activity also accepts the native QA command line from the
`com.libcna.islandsimulator.DEBUG_ARGUMENTS` string-array intent extra, but only when Android marks
the installed application debuggable. Release packages ignore the extra. This keeps fixed-view
renderer qualification inside island-simulator instead of requiring an SDL/CNA modification.

## Requirements

- JDK 17 or newer; JDK 21 is used by the current workspace.
- Android SDK platform 35 and build tools 36.1.0.
- Android NDK `29.0.14206865`.
- SDK CMake `4.1.2`.
- `ccache`, using the shared `/rv/cnaccache` cache.
- The dependency revisions in `dependencies.lock`, materialized with
  `tools/materialize_cna_next.sh`.

Set the SDK path before invoking the wrapper:

```bash
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
export ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/29.0.14206865"
```

## Native arm64 qualification build

The CMake preset uses the workspace-approved reusable `build-consumer/` tree and no more than two
compile jobs:

```bash
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
cmake --preset android-arm64-gles3
cmake --build --preset android-arm64-gles3
```

The output is `build-consumer/libmain.so`. It is useful for fast compiler/linker qualification;
use Gradle to produce an installable package.

## Debug APK

The project reuses the Gradle 8.12 wrapper stored in the pinned CNA `next` source export rather
than checking in a second binary wrapper JAR. The helper enforces the shared compiler cache and a
two-worker limit. CNA's pinned SDL Java sources remain read-only and are compiled through the
separate `:sdlbridge` library module; application-owned Java/resources retain their own lint gate.

Build for a physical arm64 device:

```bash
tools/run_android_gradle.sh :app:assembleDebug
```

Build for an x86_64 emulator:

```bash
tools/run_android_gradle.sh -PislandAbi=x86_64 :app:assembleDebug
```

The APK is written under `android/app/build/outputs/apk/debug/`. Install and launch with:

```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.libcna.islandsimulator/.IslandSimulatorActivity
```

Always invoke debug assembly through `tools/run_android_gradle.sh`. Android Gradle Plugin can keep
the superseded uncompressed native library as unreachable ZIP data when it updates an existing APK
after a C++ relink. The project wrapper removes only that generated APK before an explicit
`assembleDebug` or `packageDebug`; native/object intermediates remain incremental, and the package
task normally takes under a second when everything else is current. A clean APK should be close to
the sum shown by `unzip -lv`, rather than larger by approximately one stripped `libmain.so`.

Each build writes the same `app-debug.apk` output path, so rebuild without `-PislandAbi=x86_64`
before distributing the default arm64-v8a package. Confirm the selected ABI and package metadata:

```bash
unzip -l android/app/build/outputs/apk/debug/app-debug.apk
"$ANDROID_SDK_ROOT/build-tools/36.1.0/aapt" dump badging \
  android/app/build/outputs/apk/debug/app-debug.apk
"$ANDROID_SDK_ROOT/build-tools/36.1.0/apksigner" verify --verbose --print-certs \
  android/app/build/outputs/apk/debug/app-debug.apk
```

Run application-owned Android lint with:

```bash
tools/run_android_gradle.sh :app:lintDebug
```

The imported SDL bridge is intentionally a separate library module. Its pinned upstream
compatibility sources contain optional Bluetooth/HID and legacy API paths that produce Android
lint diagnostics even when those features are not used by island-simulator; they do not weaken the
strict app-module result and must not be patched in CNA. Re-evaluate the bridge boundary whenever
the CNA baseline changes.

## Current runtime evidence

The current qualification pass uses the API 35 `Medium_Phone` Google Play x86_64 AVD with Android
Emulator 36.5.11, `-accel on`, and `-gpu host`. `emulator -accel-check` reports KVM version 12 as
usable. Gfxstream translates GLES through the host AMD Radeon 780M. The guest remains at its
physical 1080 by 2400 / 420 dpi configuration and presents the landscape application surface at
2400 by 1080. CNA preserves its requested 16:9 game viewport as a centred 1920 by 1080 region.

The current APK loaded `libSDL3.so` and `libmain.so`, created an RGBA8888 GLES 3 surface, opened a
44.1 kHz stereo AAudio stream, reported renderer `OPENGLES3`, and completed its first draw. The
shared-buffer terrain build measured 487.895 ms for world generation, 3.421 ms for framework
initialization, 1107.472 ms for content creation, 20.058 ms before the first draw, 47.937 ms for
the first draw body, and 1666.782 ms total. The previous unaccelerated QEMU TCG pass took
421449.067 ms total; that historical number remains useful only to explain why it was not a valid
runtime target.

The first accelerated screenshot exposed opaque white touch rectangles caused by game-owned
straight-alpha pixels entering CNA `SpriteBatch`'s premultiplied-alpha blend state. The game now
premultiplies both generated circle texels and tint colours. A rebuilt GLES 3 capture shows clean
translucent rings without rectangular backgrounds. Separately injected left-zone movement and
right-zone relative-look gestures changed the player position and view while the process remained
alive. Two separated external-keyboard Shift events on the updated x86_64 build also produced the
expected 8.0 m/s run-enabled and 4.3 m/s walk-restored platform diagnostics. Deterministic core
tests continue to cover simultaneous contact ownership and cancellation; real-finger simultaneous
ergonomics still require a physical device.

Pressing Home destroyed the surface and paused AAudio; bringing the activity back recreated the
surface, restarted the same audio stream, and retained the same process. The hot activity launch
reported 77 ms total / 80 ms wait. No fatal, shader-compilation, `GL_INVALID_OPERATION`, or prior
software-AVD uniform-query error appeared in the accelerated logs. A later memory snapshot reported
87.3 MiB total PSS, 167.3 MiB RSS, and 0.5 MiB swap PSS. These are emulator observations, not
physical-device budgets.

A later debug-argument launch selected `--viewpoint reef --debug-overlay` on the same KVM/host-GPU
configuration. The actual 2400 by 1080 frame reports `OPENGLES3`, contains two visible reef
outcrops from the 14-object population, and visibly retains a dry upper rock with the darker/cooler
sea-level contact band. It reached first draw in 3879.191 ms total (1324.831 ms world, 4.658 ms
framework, 2473.582 ms content, 22.667 ms pre-draw, and 53.453 ms first draw). No fatal,
shader-compilation, or `GL_INVALID_OPERATION` diagnostic appeared. This qualifies the material
path on gfxstream, not a physical mobile GPU or a sustained performance budget.

The same launch bridge then captured beach, close grass, jungle, water, and monkey viewpoints.
All five reported `OPENGLES3`, completed a first draw in 1606.499--2217.599 ms, and emitted no
fatal, shader-compilation, or `GL_INVALID_OPERATION` diagnostic. Side-by-side inspection against
the corresponding WebGL 2 frames found matching scene geometry, broad materials, grass and jungle
density, water colour, sky, and the near skinned-monkey silhouette. Together with the reef frame,
this is a six-view GLES 3 appearance matrix on gfxstream. It is not a pixel-identity test, an
automated image comparison, or a physical-device performance qualification.

After the shoreline-transition surface burst was added, a fresh fixed water launch exercised the
revised GLSL ES 300 fragment path. The inspected 2400 by 1080 frame reports `OPENGLES3`, a
0.6-strength burst at phase 0.6, and visibly retains the same water/shore composition with clean
touch controls. It completed first draw in 4247.442 ms total and emitted no fatal,
shader-compilation, or `GL_INVALID_OPERATION` diagnostic. This qualifies the functional effect on
gfxstream, not its sustained cost on a physical mobile GPU.

A subsequent no-window qualification after moving the CNA pin to `fe20e3e9c` again used KVM
version 12 and host-GPU gfxstream, completed a cold GLES 3 first frame in 2003.189 ms, and produced
an inspected clean overview. This proves that the current 31-driftwood complete scene starts with
the qualified dependency baseline; the overview is not a close-view qualification of the cove
landmark itself, and the one-run timing is not a performance comparison.

SurfaceFlinger presentation samples at the same initial scene improved after terrain visible-cell
run batching from 40.903 ms mean (24.45 frames/s, 71 of 126 intervals over 40 ms) to 34.163 and
32.590 ms mean (29.27 and 30.68 frames/s, respectively, with 6 and 1 intervals over 40 ms). This is
useful same-AVD before/after evidence, but it includes compositor/emulator effects and is not a
mobile GPU frame-time claim. Physical-device input latency, touch ergonomics, audio-focus
interruption, sustained memory, thermals, and clean process-death restoration remain open.

Useful diagnostics for a connected test target are:

```bash
adb shell pidof com.libcna.islandsimulator
adb logcat -c
adb shell am start -W -n com.libcna.islandsimulator/.IslandSimulatorActivity
adb logcat -d
```

Once the device reports `device` and `adb shell getprop sys.boot_completed` reports `1`, the
repeatable first-frame gate installs the selected APK,
clears logcat, launches the activity, waits up to ten minutes for the `OPENGLES3` renderer,
first-draw timing, and biome-grass diagnostics, rejects fatal/shader errors, and saves both logcat
and a device screenshot:

```bash
tools/run_android_runtime_smoke.sh \
  "$ANDROID_SDK_ROOT/platform-tools/adb" \
  android/app/build/outputs/apk/debug/app-debug.apk
```

The default evidence directory is ignored `build/android-runtime-smoke/`. The gate proves a live
GLES 3 frame and process; it does not by itself qualify touch ergonomics, audio, lifecycle recovery,
performance, memory, or thermals. Override the deliberately generous software-emulator timeout with
`ISLAND_ANDROID_SMOKE_TIMEOUT_SECONDS` when testing faster hardware.

For a fixed debug QA camera, pass a comma-separated native argument array. The application ignores
this extra in non-debuggable packages:

```bash
ISLAND_ANDROID_SMOKE_ARGUMENTS=--viewpoint,reef,--debug-overlay \
  tools/run_android_runtime_smoke.sh \
  "$ANDROID_SDK_ROOT/platform-tools/adb" \
  android/app/build/outputs/apk/debug/app-debug.apk \
  build/android-runtime-smoke-reef
```

## Dependency overrides

The Gradle project defaults to the current workspace layout. Alternate read-only checkouts can be
selected without editing the game or dependencies:

```bash
tools/run_android_gradle.sh :app:assembleDebug \
  -PcnaRootDir=/path/to/cna-next \
  -PsharpRuntimeRoot=/path/to/sharp-runtime-next \
  -PeasyGlRoot=/path/to/easy-gl \
  -PmetaGlRoot=/path/to/meta-gl
```

Do not patch CNA or Sharp Runtime to make an Android build pass. Record confirmed dependency bugs
in `BUGS.md` and keep any required workaround inside island-simulator.
