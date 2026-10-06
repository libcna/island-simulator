# Android OpenGL ES 3 Build

## Current status

The project has an Android application for API 24 and newer. The native game target builds as
`libmain.so`, uses CNA's SDL3 activity bridge, and selects the `OPENGLES3` renderer explicitly.
The supported package ABIs are `arm64-v8a` for devices and `x86_64` for emulator testing. Both
debug variants build; the x86_64 APK has also installed and rendered the current complete island
scene on an API 35 emulator.

The Android UI is fixed landscape/fullscreen and includes dual-zone touch movement/look controls
with generated on-screen affordances. It uses the same restrained walking/swimming camera-motion
model as desktop, although an on-screen opt-out is not implemented yet. The host now loads `kvm`
and `kvm_amd`, and the developer is a member of the `kvm` group, but this environment still exposes
no `/dev/kvm` device node. Its existing software-emulated GLES3 render is valid renderer evidence
but not touch, lifecycle, or performance qualification.
Those claims still require a stable accelerated emulator or physical device.

The current host reports KVM at misc-device `10:232`. If its device node remains absent after the
modules are loaded, recreate it through udev (preferred) or explicitly as root, then start a fresh
shell so the configured `kvm` group is active:

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
can instead launch the validation command through `sg kvm`.

The current arm64 package includes the portable procedural sand/soil terrain material with
near-field interior microvariation and sparse curved leaf litter, the broadleaf jungle with
branch-supported near crowns, rounded far crowns, per-tree LOD, pointed-leaf near bushes with a
compact far tier, and forest-floor plants including upright-broadleaf and folded split-leaf near
variants with per-instance LOD/range selection,
speed-responsive player bending plus curved, two-sided daylight shading in the instanced mesh grass,
segmented near-palm leaflets, procedural near-trunk scars, restrained coconut clusters, and
staggered per-instance near/far LOD,
157 land rocks, 14 emergent reef outcrops, 12 fallen logs, sparse beach driftwood with a three-limb
forked near variant and per-instance large-prop range/LOD, 63 fallen palm
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
ripples, a procedural near-surface swimming veil with
offshore-current modulation, a deeper current-responsive undertow mix in the existing water loop,
and a pooled
positional voice with two bird and two monkey phrases. That package
builds and passes application lint. The current x86_64 runtime gate constructed this complete scene,
reported its biome-aware grass path, and completed a GLES 3 first draw. The software-emulator
screenshot establishes integrated rendering but is not detailed enough to qualify every layer's
appearance, touch feel, audio output, lifecycle, or performance.
The latest single-ABI arm64 debug qualification build produced an 18,705,724-byte APK with a
15,895,440-byte stripped `libmain.so` and passed `:app:lintDebug`. It includes the narrowed,
noise-mottled portable footpath, reused crossing-wave ocean colour, reduced CPU-lit fish/shark
geometry, and the shared run-capable input resolver used by hardware keyboards. The APK verifies
with signature
scheme v2, contains only the selected ABI, and remains the same size after an immediate no-change
incremental assembly.

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

The runtime passes used the API 35 `Medium_Phone` Google Play x86_64 AVD with Android Emulator
software acceleration disabled (`-accel off`) and SwiftShader GLES. The host exposes no `/dev/kvm`.
An initial 2400 by 1080 pass visibly rendered terrain, palms, mesh grass, and the touch HUD. For a
repeatable current-content pass, the guest display was temporarily reduced to 800 by 360 so its
software-rendered System UI could keep a surface alive. The setting was test-environment state, not
an application quality tier, and the AVD was returned to its original 1080 by 2400 / 420 dpi state
afterward.

The current APK loaded `libSDL3.so` and `libmain.so`, created an 800 by 360 RGBA8888 surface, and
reported biome-aware grass, renderer `OPENGLES3`, and a completed first draw. Application timings
were 157126.852 ms for world generation, 256204.401 ms for content creation, 7708.990 ms for the
first draw body, and 421449.067 ms total. These extreme QEMU TCG values are diagnostic evidence,
not a mobile baseline. The captured frame contains the complete current scene and touch HUD behind
Android's one-time fullscreen help overlay.

The AVD took 229 seconds for the successful cold boot. Its System and System UI processes repeatedly
entered ANR/restart states under QEMU TCG, including while the game remained alive. Gfxstream also
logged 27 `GL_INVALID_OPERATION` reports at uniform-location queries even though shader creation and
the first frame completed; reproduce that observation on a stable target before classifying its
source. This pass therefore does not qualify frame rate, input latency, touch ergonomics,
pause/resume, audio focus, sustained memory, thermals, or clean shutdown. Guest root access would
not supply the missing host virtualization. Use KVM or a physical device for the remaining runtime
matrix.

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
