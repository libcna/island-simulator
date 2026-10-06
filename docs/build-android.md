# Android OpenGL ES 3 Build

## Current status

The project has an Android application for API 24 and newer. The native game target builds as
`libmain.so`, uses CNA's SDL3 activity bridge, and selects the `OPENGLES3` renderer explicitly.
The supported package ABIs are `arm64-v8a` for devices and `x86_64` for emulator testing. Both
debug variants build; the x86_64 APK has also installed and rendered the current island on an API
35 emulator.

The Android UI is fixed landscape/fullscreen and includes dual-zone touch movement/look controls
with generated on-screen affordances. It uses the same restrained walking/swimming camera-motion
model as desktop, although an on-screen opt-out is not implemented yet. The available emulator has
no KVM acceleration, so its first
GLES3 render is valid renderer evidence but not touch, lifecycle, or performance qualification.
Those claims still require a stable accelerated emulator or physical device.

The current arm64 package includes the portable procedural sand/soil terrain material with
near-field interior microvariation and sparse curved leaf litter, the broadleaf jungle with
branch-supported near crowns, rounded far crowns, per-tree LOD, low bushes, and forest-floor plants,
speed-responsive player bending plus curved, two-sided daylight shading in the instanced mesh grass,
segmented near-palm leaflets, procedural near-trunk scars, restrained coconut clusters, and
staggered per-instance near/far LOD,
152 land rocks, 14 emergent reef outcrops, fallen logs, sparse beach driftwood, 63 fallen palm
fronds, 265 near-field shell/pebble clusters, 24 simulated
procedural birds, eight fish schools,
one occasional non-hostile offshore shark, seven GPU-skinned procedural monkeys, fixed sunny sky
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
builds and passes application lint, but
the existing emulator capture predates these layers; do not treat compilation as GLES3 runtime
qualification for the new jungle, prop, distant-scenery, bird, fish, shark, monkey, sky,
contact-shadow, revised-palm, grass-interaction, water-ripple, water-view, or audio systems.
The latest arm64 debug qualification build produced a 33,453,944-byte APK with a
15,721,568-byte stripped `libmain.so` and passed `:app:lintDebug`.

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

The first runtime pass used the API 35 `Medium_Phone` Google Play x86_64 AVD with Android Emulator
software acceleration disabled (`-accel off`) and SwiftShader GLES. The host exposes no `/dev/kvm`.
The APK installed successfully, SDL loaded `libSDL3.so` and `libmain.so`, created a 2400 by 1080
surface, and a captured frame visibly contains the terrain, palms, and mesh grass. The process
continued submitting EGL frames without an application crash or shader exception.

This AVD took about eight minutes to cold boot and typically needed 0.6--1.7 seconds per rendered
frame. Its System and System UI processes repeatedly entered ANR/restart states under QEMU TCG,
including while the game remained alive. Therefore this pass does not qualify frame rate, input
latency, touch ergonomics, pause/resume, audio focus, sustained memory, thermals, or clean shutdown.
Guest root access would not supply the missing host virtualization. Use KVM or a physical device
for the remaining runtime matrix.

Useful diagnostics for a connected test target are:

```bash
adb shell pidof com.libcna.islandsimulator
adb logcat -c
adb shell am start -W -n com.libcna.islandsimulator/.IslandSimulatorActivity
adb logcat -d
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
