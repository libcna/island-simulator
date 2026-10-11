# Android OpenGL ES 3 Build

Host Python 3.10+ with Pillow (`python3-pil`) is required for incremental texture
preparation. Override the interpreter with `-PislandPython=/path/to/python3` when
needed. No Python or Pillow runtime is packaged in the APK. See `prepared-materials.md`.

## Current grass-surface validation (2026-10-09)

Both ABI assembly/lint passes complete (2 s each). Isolated API 35 KVM/SwiftShader
`20261009T130636.434000Z-grass` passes, app PID 1770 / entry TID 1868. The image
is inspected, kernel deltas empty and owned emulator exits zero. Default ARM64
package is restored with its three native libraries and both prepared materials.
Current rolling APKs are 20,349,179 / 20,440,564 bytes; hashes and limits are in
`grass-surface-material.md`. ARM64 runtime, physical-device performance and renewed
touch/lifecycle qualification are not claimed.

## Previous compact-material validation (2026-10-09)

Both ABI build/lint passes complete (x86_64 4 s, restored ARM64 2 s). Isolated API
35 KVM/SwiftShader `20261009T123933.093621Z-ground-broadleaf` passes; image inspected,
empty live/final kernel deltas, owned emulator exit zero. Both PNGs match the
native/Web prepared hashes. Then-validated APKs were 20,345,947 bytes (x86_64) and
20,437,764 bytes (ARM64); hashes and loading observations are in `prepared-materials.md`.
ARM64 runtime and physical-device performance remain unverified. No renewed
touch/lifecycle/traversal qualification. Historical rolling paths below have since
been replaced; original logs and incident evidence remain intact.

## Previous fallen-leaf contact validation (2026-10-09)

Both ABI assembly/lint passes complete (5 s each). CNA's existing GLES3 primarily-
tested-on-Linux warning remains. Isolated API 35 KVM/SwiftShader
`20261009T113904.058650Z-litter` passes, app PID 1958 / entry TID 2035, first-draw
total 10,925.940 ms. Required diagnostic and inspected 1280x720 frame agree with
native/Web. Live/final kernel deltas are empty; owned emulator exit zero. No renewed
touch/lifecycle/traversal or ARM64 runtime qualification. See `fallen-leaf-contact.md`.

Then-tested x86_64 APK: `build/android-software/island-x86_64.apk`, 24,000,040 bytes,
SHA-256 `038ca6db797a8f30b6e8ff9e011d2ed2715b406a1822a14931f809da16c61cd5`.
Then-built ARM64-only APK: `android/app/build/outputs/apk/debug/app-debug.apk`, 24,092,049 bytes,
SHA-256 `dbec7a5c8b506ec7d88fbe06d08e8c6b1ded93f9f422fec42c914147fccf7f06`.
Only three arm64-v8a native libraries are packaged. Only rolling generated APKs
were replaced; original incident and historical runtime evidence was preserved.

## Previous jungle root-contact validation (2026-10-09)

Both ABI assembly/lint passes complete (x86_64 3 s; restored ARM64 4 s). Isolated
API 35 KVM/SwiftShader `20261009T112028.208485Z-ground-broadleaf` passes, app PID
1844 / entry TID 1901, first-draw total 12,042.214 ms. The root-contact diagnostic
is present and the 1280x720 image is inspected. Live/final kernel deltas are empty;
the owned emulator exits zero. This does not repeat touch/lifecycle/traversal tests,
qualify ARM64 runtime or measure physical-phone performance. See `jungle-root-contact.md`.

Then-tested x86_64 APK (rolling path since replaced): 23,996,584 bytes,
SHA-256 `770faa0f61f738e35ea27d6df740eda0a243caa93f78d90a36ad590b981fafdc`.
Then-built ARM64-only APK (rolling path since replaced): 24,087,145 bytes,
SHA-256 `91ba164b6a9fe6ec8c2fef269e49b6104785be2377ccb5a0962dc7244bb474d9`.
Only three arm64-v8a native libraries are packaged. Rolling generated packages are
replaced; original incident and historical runtime evidence is retained.

## Previous forest-floor colony validation (2026-10-09)

Both ABI assembly/lint passes complete (4 s each). Isolated API 35 KVM/SwiftShader
`20261009T105740.149768Z-jungle` passes, app PID 1755 / entry TID 1894, first-draw
total 9,830.804 ms. The 1280x720 image is inspected and native/Web/Android agree on
199 groups / 866 companions. Live/final kernel deltas are empty; emulator exit zero.
This fixed view does not repeat touch/lifecycle/traversal checks. ARM64 runtime and
physical-device performance remain unverified. See `forest-floor-colonies.md`.

Then-tested x86_64 APK (rolling path since replaced): 23,994,728 bytes,
SHA-256 `da92d4a77f316b5ee1ea761f42fb1be2daee4b74c59f6463bb06132528611456`.
Then-built ARM64-only APK (rolling path since replaced): 24,085,225 bytes,
SHA-256 `3864c1c70ffb149442b3a7e2b4b75618dea1654199a8d41c06cbe8d449ea7733`.
Only three arm64-v8a native libraries are packaged. Rolling generated APKs replace
the previous packages; all incident and historical runtime evidence is retained.

## Previous shrub-outline validation (2026-10-09)

Both ABI assembly/lint passes complete (x86_64 3 s; restored ARM64 4 s).
Isolated API 35 KVM/SwiftShader `20261009T103304.055335Z-bush` passes, app PID
1845 / entry TID 1906, first-draw total 10,688.124 ms. The 1280x720 image is
inspected; live/final kernel deltas are empty and the owned emulator exits zero.
This is a fixed-scene check, not renewed touch/lifecycle/traversal qualification.
ARM64 runtime and physical-device performance remain unverified. See
`shrub-lod-continuity.md` for the geometry tradeoff and real-input Web walk.

Then-tested x86_64 APK (rolling path since replaced): 23,980,920 bytes,
SHA-256 `fc2becbbcb6e45cd2fe40d164483cc7a5f8e60d43ef3578bfc678d58543907a8`.
Then-built ARM64-only APK (rolling path since replaced): 24,071,529 bytes,
SHA-256 `27195324d69e025f2edee12fb62d51b6e26ec94fec5310fa7c5930182d261805`.
Only three arm64-v8a native libraries are packaged. Rolling generated APKs replace
the previous packages; all incident and historical runtime evidence is retained.

## Previous supported-shoot shrub validation (2026-10-09)

Both ABI assembly/lint passes complete (x86_64 4 s; restored default ARM64 5 s).
Isolated API 35 KVM/SwiftShader `20261009T100800.243131Z-bush` passes, app PID
1865 / entry TID 1968, first-draw total 10,910.550 ms. The inspected 1280x720
frame shows the targeted shrub and mobile overlay. Live/final kernel deltas are
empty; the owned emulator exits zero. This does not repeat touch/lifecycle/traversal
gates or qualify ARM64 runtime/physical-device performance. See `shrub-structure.md`.

Then-tested x86_64 APK (rolling path since replaced): 23,980,712 bytes,
SHA-256 `0def474a0aa051d31140f0d2f9225a1aa57b5a07142e963f1bfd5b3f5498187d`.
Then-built ARM64-only APK (rolling path since replaced): 24,071,265 bytes,
SHA-256 `5a7351980df2c30680c4776ad35272fecf28c3b98f48e2a98074442b8f5b0add`.
Only three arm64-v8a native libraries are packaged. Rolling generated packages are
replaced; original incident artifacts and historical runtime evidence are retained.

## Previous connected split-leaf validation (2026-10-09)

Both ABI assembly/lint passes complete (x86_64 3 s; ARM64 4 s). Isolated API 35
KVM/SwiftShader run `20261009T093932.612353Z-ground` passes, app PID 1813 / entry
TID 1886, first-draw total 10,536.045 ms. The inspected 1280x720 image shows the
connected rounded lobes and touch overlay. Live/final kernel deltas are empty;
the owned emulator exits zero. This fixed-view gate does not repeat touch,
lifecycle or traversal qualification. ARM64 runtime/physical performance remains
unverified. See `split-ground-leaves.md` for cost and remaining art limitations.

Then-tested x86_64 APK (rolling path since replaced): 23,949,832 bytes,
SHA-256 `dd9ca4441695e65b2dc54b00f7d0d06f98328e5763926442ed1e4429f104d2f1`.
Then-built ARM64-only APK (rolling path since replaced): 24,041,225 bytes,
SHA-256 `d9f8e4d9e9d9ae59062ac866f1fae68bc8b8c45414c4b7f8e2058803b40a91b8`.
Only the three arm64-v8a native libraries are packaged in the default APK. Rolling
packages replace superseded generated APKs; historical runtime evidence is retained.

## Previous juvenile broadleaf validation (2026-10-09)

Both ABI assembly/lint runs pass (5 s each), using shared ccache/two workers. Isolated
API 35 KVM/SwiftShader view `20261009T090756.767342Z-ground-broadleaf` passes, with
app PID 1773 / entry TID 1880, total first-draw startup 14,121.283 ms. The targeted
1280x720 plant image is inspected; live/final kernel deltas are empty and the owned
emulator exits zero. This is a fixed-scene functional check, not renewed touch/lifecycle
coverage or physical-device performance. See `ground-broadleaf.md`.

Then-tested x86_64 APK (rolling path since replaced): 23,950,984 bytes,
SHA-256 `ab899d1174235fc3575cabed2a8f353e742e4718d58b2183e97e57c192fcbd50`.
Then-built ARM64-only APK (rolling path since replaced): 24,040,377 bytes,
SHA-256 `5ab195876e0634b3cfc2f9c922e7b55d050925c2baed93e97b27d8265a053f1c`.
ARM64 is assembled/linted, not runtime-tested. Rolling package paths replace the previous
grass-distance packages; their runtime evidence remains, without duplicate APK archives.

## Previous grass distance-continuity validation (2026-10-09)

Both ABI assembly/lint runs pass (x86_64 6 s, ARM64 4 s), using shared ccache and two
workers. Isolated API 35 KVM/SwiftShader grass run
`build/android-software/20261009T084206.226212Z-grass/` passes, including the new
anchor-distance shader diagnostic and 2x MSAA. App PID 1830 / entry TID 1925;
1280x720 image inspected, empty kernel deltas, owned emulator exits zero. This
is one fixed view, not a renewed touch/lifecycle matrix or physical-device test.

Then-tested x86_64 APK (rolling path since replaced): 23,946,360 bytes,
SHA-256 `a3cf391d3606976d5eedf27408afba39a867d314661cebf4289b06879d60286c`.
Then-built ARM64-only APK (rolling path since replaced): 24,036,449 bytes,
SHA-256 `df1f3866a331b4353b1ede17cd77051f635bbe44815c39facb752074fcf55898`.
ARM64 has assembly/lint evidence only. See `grass-distance-continuity.md` for scope.
The two rolling package paths replace the previous MSAA packages; prior runtime logs,
images and hash metadata remain, without another pair of duplicate APK archives.

## Previous MSAA validation (2026-10-09)

Both ABI assembly/lint runs pass through shared ccache with two workers. The isolated API 35
x86_64 KVM/SwiftShader grass gate at `build/android-software/20261009T081708.472756Z-grass/`
confirms requested/applied 2x MSAA, app PID 1825, entry TID 1941, first-draw total 10,171.731 ms.
The inspected 1280x720 PNG retains the world and touch HUD. Kernel deltas are empty; the owned
emulator exits zero. See `edge-antialiasing.md` for scope/cost; this is not hardware performance.

Then-tested x86_64 APK (rolling path since replaced): 23,944,616 bytes,
SHA-256 `74d3bb05f6e8c57ee865ecaae0b5b8bc76900a3f5101cde9d7272f43bef2aed9`.
Then-built ARM64-only APK (rolling path since replaced): 24,034,761 bytes,
SHA-256 `95c1ce00a6623837d24cad35c18147a8592c109befc808b044bb7b87b4e7c54f`.
ARM64 is assembled/linted, not runtime-tested. Prior APKs are preserved as `before-msaa` below.

## Previous broadleaf-lamina validation (2026-10-09)

Both ABI builds/lint pass; x86_64 isolated KVM/SwiftShader clearing-edge runtime
`build/android-software/20261009T073001.635653Z-clearing-edge/` passes the lamina marker with
empty kernel deltas and emulator exit zero. App PID 1770, total first-draw startup 12,489.783 ms;
close broadleaf PNG inspected. See `broadleaf-lamina.md` for exact scope and remaining limits.

Prior x86_64 APK: `build/android-software/island-x86_64.before-msaa.apk`, 23,920,040 bytes,
SHA-256 `ee0f0cbf01b1fbf4a8789085e3025f44c149777cfee9e1d0e1f7164d4e6d1577`.
Prior ARM64 APK: `build/android-software/island-arm64.before-msaa.apk`, 24,009,257 bytes,
SHA-256 `8ec534cc86f1c91de81cfe2a920c193548b68d830ad9b7b682e5d6aa09e97421`.
ARM64 runtime and physical-device cost are not qualified.

## Latest pinnate-fern validation (2026-10-09)

Both ABI builds/lint pass; isolated x86_64 KVM/SwiftShader jungle runtime
`build/android-software/20261009T070727.885381Z-jungle/` passes the fern marker, with empty
kernel deltas and emulator exit zero. App PID 1779, first-draw total 10,684.628 ms; screenshot
inspected. See `understory-ferns.md` for evidence/cost and remaining hardware/motion limits.

Prior x86_64 APK: `build/android-software/island-x86_64.before-broadleaf.apk`, 23,918,072 bytes,
SHA-256 `ae376130a71fcd94782d8228a02585cbd80d5b3f61a63a2cf8bb6ec2dd974dff`.
Prior ARM64 APK: `build/android-software/island-arm64.before-broadleaf.apk`, 24,007,649 bytes,
SHA-256 `5adad5d287bf527b3950241d07ad21b4f873bfbcc4fc68babb0324effe56ee5b`.
ARM64 has build/lint evidence, not physical-device runtime qualification.

## Latest eight-form canopy validation (2026-10-09)

Both ABIs assemble/lint successfully; isolated KVM/SwiftShader x86_64 jungle runtime
`build/android-software/20261009T064727.074751Z-jungle/` passes the eight-form gate with empty
kernel deltas. App PID 1846, first-draw total 10,171.476 ms; screenshot inspected and emulator
exits zero. This is software functional evidence, not ARM64/device performance.

Prior x86_64 APK: `build/android-software/island-x86_64.before-ferns.apk`, 23,910,056 bytes,
SHA-256 `c48c5ce92a5af42e150d000bc3224a8e4146caea49606cae222bcaeec88a7dc8`.
Prior ARM64 APK: `build/android-software/island-arm64.before-ferns.apk`, 24,001,121 bytes,
SHA-256 `ee01d9e5ba214a6b70a4bd5a4495f116b889f3f556195133a9c8987aff50bebe`.
See `canopy-form-expansion.md` for mesh cost, exact commands and remaining qualification.

## Capability-gated floor filtering (2026-10-09)

The current material requests 4x anisotropic filtering when CNA reports support, retaining the
trilinear path otherwise. Both x86_64 and default ARM64 assembly/lint pass. Isolated software
runtime `build/android-software/20261009T062509.021834Z-jungle/` passes with the expected
unsupported-capability/trilinear decision, app PID 1803, native entry TID 1882, total first-draw
startup 8893.653 ms and an inspected 1280x720 screenshot. Live/final kernel deltas are empty;
the owned emulator exits zero. This also validates the new nested process containment with
an actual emulator/game, not only preflight. No physical-device/aniso performance claim.

Prior floor-filter x86_64 APK: `build/android-software/island-x86_64.before-eight-canopy.apk`, 23,907,400 bytes,
SHA-256 `8c6541db1d1c9e3946a464ce9edb81d40e4934707b25d0c1b0a9a44a1f4e3f5c`.
Prior floor-filter ARM64 APK: `build/android-software/island-arm64.before-eight-canopy.apk`, 23,998,641 bytes,
SHA-256 `1ed1ff9ea343b9614d98e7b7a74342c7705851d69bd0ad85d053443830c16595`.
Previous packages are retained as `island-x86_64.before-floor-filter.apk` and
`island-arm64.before-floor-filter.apk` under `build/android-software/`.

## File-backed forest-floor integration (2026-10-09)

The APK now contains `assets/Content/terrain/forest-floor.png`, staged from the hashed v1
source through the annotated `StageIslandAssets` output and AGP generated-source registration.
Packaging/lint and regeneration with missing staged data are verified; ZIP contents match the
source hash. The isolated software runner refuses a missing or mismatched floor asset before
launch. CNA/Sharp Runtime remain read-only. Earlier no-file-backed-asset statements describe
older milestones, not this package.

The corrected API 35 KVM/SwiftShader jungle run is
`build/android-software/20261009T054739.380317Z-jungle/`: PASS, PID 1829, first draw 10,065.572 ms,
inspected 1280x720 screenshot, empty live/final kernel deltas and owned-emulator exit zero.
The preceding `20261009T054439.701531Z-jungle/` failed the stale pre-floor memory assertion;
its logs remain preserved. No GPU fault was recorded and its emulator also exited zero.
This historical run's texture payload/budget was 6,033,888/6,054,104 bytes, excluding CPU shadows,
decoder allocations, driver overhead and framebuffers. No physical-device performance claim.

The earlier floor-integration x86_64 APK is now `build/android-software/island-x86_64.before-floor-filter.apk`, 23,903,544 bytes,
SHA-256 `910a4ffc4db3f3b2b6b890381cb2bf86d5b7426e3d9330b22c20473c1dcad1df`;
the prior coat/contact build is `island-x86_64.before-floor-runtime.apk`.
Default arm64 assembly/lint is restored and passes (5 s). Its APK contains only arm64 native
libraries (`libmain.so` 16,756,072 bytes) and the same floor PNG: 23,994,577 bytes, SHA-256
`7af6e724362b2cae356ffd289842acaba5b7ab3c6650373c5b87f796e8afb32c`.
See `forest-floor-material-review.md` for loading, filtering, memory and provenance details.

## Isolated software boot gate after the GPU incident

`tools/run_android_software.py` defaults to an isolation preflight, not an emulator launch.
On Linux with bubblewrap, usable KVM and readable kernel journal:

```bash
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk'
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot'
```

Replace the SDK path for another machine. No sudo/root emulator is needed. This gate reuses the
installed API 35 Google Play x86_64 system image read-only and keeps one project-owned AVD under
`build/android-software/state`. It never copies personal userdata. A private mount namespace
exposes KVM but not DRM devices, hides host display sockets, and makes the host filesystem
read-only except the owned runtime/evidence directories. Private networking/PIDs isolate its
adb server (port 5038, explicit `emulator-5558`) from other projects. CPU affinity is capped at
eight, the guest has two vCPUs/2048 MiB, and rendering is explicitly headless SwiftShader.
Do not remove these safeguards or substitute `auto`/`host`. There is no unisolated fallback.

The parent records the exact command/environment and a kernel cursor before launch, monitors
kernel messages, and terminates the owned namespace on a fault, lost monitoring or timeout.
The shared monitor additionally wraps the worker in lifetime-only PID containment; its exact
outer command and launcher hash are in `process-containment.json`. The inner Android sandbox
still supplies all device/network/filesystem isolation. The 2026-10-09 post-change preflight
`build/android-software/20261009T060433.012207Z-preflight/` verifies the nested arrangement,
accessible KVM, absent DRM/display access and CPU limit, with an empty kernel delta; it does
not boot Android or requalify the APK.
The boot deadline is 150 seconds; the entire workload is bounded to 200 seconds. The worker
requires a SwiftShader GLES line from guest SurfaceFlinger. The hardware stop latch is untouched.
This command **does not install or run the game** and is not AMD/mobile performance qualification.

After building `-PislandAbi=x86_64`, explicit application checks use the same isolation:

```bash
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint clearing-edge'
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint interactive --touch-selector --lifecycle'
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk build/android-software/island-x86_64.apk --viewpoint interactive --touch-selector --traversal'
```

The gate requires an x86_64-only APK, records its SHA-256 and both helper identities, then
rechecks the package before installation. Game logcat must independently identify OPENGLES3
and SwiftShader, without fatal signals, GL errors, shader failures or material/stock fallbacks.
It invokes the existing strict scene smoke checks rather than replacing them with a boot-only
success. Application checks have a 600-second outer bound, a 120-second startup gate, and a
210/360-second helper bound without/with lifecycle checks. Inspect screenshots separately:
log success is not proof of an unobstructed frame. Failure screenshots are retained where possible.

Touch assertions require `--viewpoint interactive`: fixed inspection cameras intentionally
ignore player input. The owned guest acknowledges the fullscreen introduction using
`settings put secure immersive_mode_confirmations confirmed`, with readback validation, following
[Android CTS setup](https://android.googlesource.com/platform/cts/+/9c015a5784523f3f9805ab1e15b0fc0e3e1a103b/tests/tests/view/AndroidTest.xml).
This prevents the first-run system overlay from covering the image/stealing input. It never
changes settings on a personal device or existing user AVD. Do not infer touch movement,
swimming, audible output or physical-mobile speed from the selector/lifecycle gates alone.

The separate `--traversal` gate requires land-footstep, swimming-entry and water-accent events,
not merely successful adb gesture submission. Selector and traversal coordinates now share
`android_touch_geometry.py` (Python 3 required), derived from actual window/logical diagnostics
through the aspect-fit viewport. Invalid/missing extents fail closed. Unit tests cover pillarbox,
letterbox and odd-size layouts; fixed QA cameras are rejected before launch for input gates.
Runtime `20261009T031108.597736Z-interactive` passes on the 1280x720 software guest: PID 1708,
7,939.514 ms startup, land footstep and water accent, followed by authoritative swimming entry.
The inspected screenshot shows the player at the water surface facing the beach, with readable
transparent shallows and touch controls. Live/final kernel deltas are empty and the emulator
exits zero. This validates that route at that size, not other physical devices or audible output.

An interrupted namespace can leave emulator PID-file locks even with snapshots disabled. The
next namespace may reuse the recorded PID (observed: 12), causing "snapshot operation pending"
instead of recognizing a stale owner. Do not automatically delete locks by checking that PID
on the host. First establish that the owned namespace/emulator has terminated and acquire the
qualification lock; preserve only the confirmed stale owned `snapshot.lock.lock` and
`hardware-qemu.ini.lock` in the failed run's evidence before retrying. Do not wipe userdata,
touch a personal AVD or remove the `multiinstance.lock` flock file. The bounded monitor prioritizes
stopping owned work on a fault over allowing lengthy emulator snapshot cleanup.

The first isolated boot on October 9 completed in 33.141 seconds, reported SwiftShader GLES 3.0,
and left empty live/final kernel deltas. Evidence is
`build/android-software/20261009T025311.463227Z-boot`. Both emulator/private adb exited; the
existing host adb PID 3509859 remained. Runtime state occupies about 814 MiB and is reused.
Retained emulator warnings include offline update/modem services, shutdown-time adb operations,
`stop: Not implemented`, and a snapshot-save diagnostic despite snapshot-disabling flags
(an owned `snapshots/default_boot/ram.img` exists). Do not describe this as an error-free emulator
log or assume the flags prevent every snapshot artifact. No CNA/Sharp defect is inferred.
Next: validate bounded game installation/render/input inside this same isolation after building
the current x86_64 package with the shared ccache; restore the arm64 deliverable afterwards.

The reuse check initially refused before launch because the emulator reformats INI whitespace
and enforces a 6 GiB logical userdata minimum for this Play image. Property-based validation
now accepts formatting only, rejects duplicate/changed settings, and requests that minimum.
The second actual boot (`20261009T025531.379815Z-boot`) completed in 21.406 seconds with the
same confirmed software renderer, exit code zero, no forced termination and empty kernel deltas.
The shutdown diagnostic persists. The apparent 2 GiB snapshot RAM file is sparse with zero
allocated blocks; total owned state after reuse is about 830 MiB. Keep this single runtime state
for subsequent checks instead of creating or copying another AVD.

## Current status

AMDGPU qualification is suspended by the October 8 GPUVM incident. Do not start a host-GPU
emulator for the commands below while the hardware stop latch is active. Runtime observations
in this document predate the latest boar, canopy, shoreline and ocean passes unless explicitly
stated otherwise. The current arm64 ocean/coastal-material/contact-shadow/smooth-stone/wood-lighting
build and lint pass (the latest leaf-daylight/material-filter arm64 assemble/lint pass took 2 s).
The October 9 isolated x86_64 SwiftShader interactive run now validates current-source startup,
the touch-sensitivity selector, same-process Home/resume and a new-process cold relaunch.
Evidence `20261009T030406.948847Z-interactive` records PIDs 1828/3105, startup totals
8,712.503/4,269.163 ms, three inspected unobstructed 1280x720 images, no game graphics/fatal/
fallback diagnostics and empty live/final kernel deltas. The headless emulator exits zero.
This is limited software GLES functional evidence, not physical-device or AMD qualification;
close material views, movement/swimming and audio require their own checks (the separate
software traversal result above now covers one walking/swimming route and sound submission).
Shadows use the existing stock-effect batch; stone adds one
normal attribute (seven total including instances) to the portable custom prop shader.
Current fixed clearing-edge and boar views now also pass under this isolated GLES path:
`20261009T030520.313648Z-clearing-edge` (PID 1803, 8,909.279 ms startup) and
`20261009T030630.007754Z-boars` (PID 1919, 9,328.215 ms). Both actual 1280x720 images were
inspected without system overlays, with no application graphics/fallback/fatal diagnostics,
empty kernel deltas and emulator exit zero. The clearing view retains leaf profiles/materials,
forest substrate and grass; the boar view shows coat, connected limbs, bristles and ground shadow.
These static observations do not qualify all animation states, LOD transitions or moving-camera
aliasing. The pre-wood-lighting tested package is retained at
`build/android-software/island-x86_64.before-wood-daylight.apk`, SHA-256
`2d69f1f838d71cac0a5817c73b393205212000071d597018d62a083c09f1fcfd`.
The current canopy wood pass also runs successfully in isolated GLES:
`20261009T032443.475599Z-clearing-edge`, PID 1931, startup 8,910.839 ms. The inspected image
retains coherent one-sided trunk daylight; all generated wood passes outward-normal checks.
No application graphics/fallback/fatal errors, empty kernel deltas and emulator exit zero.
That preceding package is preserved at `build/android-software/island-x86_64.before-local-bark.apk`, SHA-256
`e96bf363e34cc7d88d0e2a5bac44667110578fae59a795aa8c57082e157ce403`.
The subsequent part-local bark mapping/filtering pass also succeeds in isolated GLES:
`20261009T033732.269341Z-clearing-edge`, PID 1795, startup 7,718.907 ms. The inspected image
retains bent-trunk grain and the complete mobile scene/HUD, no application graphics/fallback/
fatal errors, empty live/final kernel deltas, emulator exit zero. That preceding package is
preserved as `build/android-software/island-x86_64.before-irregular-bark.apk`, SHA-256
`537affa7bb7d9cc205f1273ca2e4654274506003fff30b2de904a322ca321977`.
The instance-stable irregular-bark pass runs in evidence
`20261009T034859.063876Z-clearing-edge`, PID 1716, startup 9,603.086 ms. Actual full-size image
inspected with intact bark/HUD/environment, current material marker, no application graphics/
fallback/fatal diagnostics, empty live/final kernel deltas, emulator exit zero. That package is
preserved as `build/android-software/island-x86_64.before-canopy-posture.apk`, SHA-256
`f251abff67b3b1674146c01dec6b4ca054d26d38ead293e0d817cf6519e4d90e`.
Spatial canopy leaf posture subsequently passes the actual interior-jungle view:
`20261009T040340.569803Z-jungle`, PID 1857, startup 9,240.148 ms. The inspected 1280x720
image retains spatial crowns, undergrowth and mobile HUD, with current posture diagnostic,
no application GL/shader/fallback/fatal errors, empty live/final kernel deltas, emulator exit zero.
That package is preserved as `build/android-software/island-x86_64.before-canopy-outline.apk`, SHA-256
`ab547504404d617a0d6e924e654aad058969d786d0516a534274d946c3a5755f`.
The subsequent seven-vertex canopy-outline pass succeeds in isolated GLES:
`20261009T042044.413856Z-jungle`, PID 1842, startup 6,116.737 ms. The actual 1280x720 image
was inspected with intact foliage, terrain and mobile HUD. The current outline marker is present;
there are no application GL/shader/fallback/fatal errors, live/final kernel deltas are empty,
and the emulator exits zero. That package is preserved as
`build/android-software/island-x86_64.before-far-branches.apk`, SHA-256
`c38d21938f6c400cebb50f61e59e538591a83832d53242fed3119282224abbd3`.
The far-support-branch pass first exposed a diagnostic omission: mesh counts were stdout-only,
not mirrored to logcat. `20261009T043330.125615Z-jungle` therefore fails the strict new count
assertion after 120 seconds despite rendering; retain its log and failure image. The emulator
exits zero and kernel deltas are empty. The game now mirrors actual generated counts through
the existing platform logger instead of relaxing the assertion.
Repeated evidence `20261009T043634.972775Z-jungle` passes with PID 1874, startup 9,281.072 ms,
far count 1029 vertices/1064 triangles, inspected intact 1280x720 image/mobile HUD, no application
GL/shader/fallback/fatal errors, empty live/final kernel deltas and emulator exit zero.
That package is preserved as `build/android-software/island-x86_64.before-trunk-pigment.apk`, SHA-256
`5a216e57063d5e5ec2477797294795ea66abbdc30d8d5ed3180a40367f0c2a95`.
Height-based trunk pigment then passes isolated GLES jungle
`20261009T044554.537658Z-jungle`, PID 1802, startup 9,423.360 ms, with the current pigment
marker, unchanged mesh counts and inspected intact 1280x720 scene/HUD. No application
GL/shader/fallback/fatal errors, empty live/final kernel deltas, emulator exit zero.
That package is preserved as `build/android-software/island-x86_64.before-boar-ground-contact.apk`, SHA-256
`577178804b1cd7e5ebd8d9e1b889746e3af4b12416af58e9857253df96e59399`.
The per-foot boar terrain-contact pass then succeeds in isolated GLES evidence
`20261009T050603.871427Z-boars`, PID 1868/main TID 1940, startup 8,865.189 ms. The new
fixed-length/independent-contact marker is present, with no limited-reach warning or application
GL/fallback/fatal errors. The 1280x720 scene and touch HUD are inspected; live/final kernel deltas
are empty and the owned emulator exits zero. This is not physical-phone or AMD qualification.
Current retained `build/android-software/island-x86_64.apk` has SHA-256
`6fb0bafbbab787461ba7c3db69f842fc33ae7b4269cdcc85d19db414225fdcb6`.
Use this current APK with the updated strict smoke marker after restoring the default arm64
package; the older APK is a comparison artifact, not a passing input to newer marker checks.
Rebuild when sources change and keep one reusable x86_64 intermediate tree.
The understory pass changes CPU-generated leaf profiles within the same mesh budgets and
vertex format; its diagnostic is verified inside the packaged arm64 `libmain.so`. Compilation
and that package check are not Android runtime/appearance qualification.
The grass custom path now uses shared rest-shape normals in 36-byte vertices and eight attributes
including instances; isolated Android views exercise it, but dedicated close-grass review and
physical-device qualification remain pending.
The near mesh now includes 48 short turf blades in the existing draw, with shared pointed tips
limiting their addition to 144 triangles/240 vertices per shared mesh. Mobile cost and appearance
still need device validation; the successful build is not that validation.
Grass no longer fades into a synthetic green ground colour at distance. Custom blades use the
existing vertex-alpha byte for normalized basal material height with opaque fragment output;
vertex size/attribute count and texture inventory stay unchanged. Android assembly/lint passes,
but this shader's execution and the colour transition remain unqualified on an Android device.
The custom grass shader also samples an immutable 241 by 241 RGBA8 rendered-terrain height map
to ground blade footprints. Its extra 232,324 bytes and three vertex texel fetches are included
in current budgets; Android shader execution, precision and GPU cost still require device testing.
The forest terrain material now separates warm humus from local moss and adds two footprint-
filtered fallen-leaf populations with colour/midrib variation; it adds no textures or geometry.
Desktop/Web software execution and the isolated Android clearing-edge view now exercise this
latest shader; a physical Android device has not qualified it.
The terrain substrate now persists at distance instead of fading into the old green vertex
palette; broad/medium noise uses pixel-footprint filtering. Its extra far-field fragment work
also remains unmeasured on physical mobile GPUs.
Slope daylight now travels in the third terrain-data component; vertices grow from 24 to 28 bytes
(+232,324 bytes), with unchanged baked RGB and opaque alpha. The arm64 build passes with this
format, but device execution is still pending. CNA's generic configure warning that GLES3 is
primarily tested on Linux remains visible; the successful package is not runtime qualification.
Four structural crown forms share eight near/far lobe anchors and a bounded trunk centreline
with correctly placed branch roots. The far tier retains original leaf subsets and now uses
997 vertices/952 triangles per variant; the texture inventory and near geometry are unchanged.
Per-tree frustum rejection avoids submitting offscreen canopy using conservative bounds and
shared CPU/GLSL wind limits. Mobile appearance, LOD transitions and performance remain unverified
by the successful cross-build.
See the incident report and `PLAN.md`.

The project has an Android application for API 24 and newer. The native game target builds as
`libmain.so`, uses CNA's SDL3 activity bridge, and selects the `OPENGLES3` renderer explicitly.
The supported package ABIs are `arm64-v8a` for devices and `x86_64` for emulator testing. Both
debug variants were built before the incident; the x86_64 APK also installed and rendered that
revision's island scene on an API 35 emulator.

The Android UI is fixed landscape/fullscreen and includes dual-zone touch movement/look controls
with generated on-screen affordances. It uses the same restrained walking/swimming camera-motion
model as desktop, although an on-screen opt-out is not implemented yet. The host loads `kvm` and
`kvm_amd`, exposes `/dev/kvm` as device `10:232`, and grants the developer access through the
`kvm` group. Android Emulator 36.5.11 was tested with the API 35 x86_64 AVD using KVM and host-GPU
gfxstream. The complete scene, independent movement/look gestures, same-process hot resume,
three selectable touch-look response levels, AAudio pause/resume, and a force-stop/new-process
cold relaunch were validated there before the incident. A
deterministic touch traversal also walks from the
south-beach spawn through the shallows, enters the authoritative swimming state, and plays the
water-transition accent. Physical-device ergonomics, driver coverage, thermals, and extended
performance remain open.

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
near-field interior microvariation, broad reused-field leaf-humus/cool-moss breakup, sparse curved
leaf litter, and close dry-sand pebble/shell flecks, the broadleaf jungle with
branch-supported near crowns, ring-varying lobe contours, secondary drooping lower-edge leaves,
restrained secondary trunk bends/girth variation, tapered two-segment bent support branches,
sparse hanging leafy lianas on two near variants, distance-faded
procedural bark fissures, rounded far crowns,
per-tree LOD, pointed-leaf near bushes with a
compact far tier, and forest-floor plants including upright-broadleaf and folded split-leaf near
variants with per-instance LOD/range selection,
speed-responsive player bending plus curved, two-sided daylight shading, per-instance blade
stature, a sparse juvenile underlayer, and six restrained seed-bearing stems in the instanced
near-grass mesh,
fuller segmented near-palm leaflets, procedural near-trunk scars, restrained coconut clusters,
transform-derived green-only per-instance frond tint, sparse hanging dry fronds on two near
variants, and staggered per-instance near/far LOD,
157 land rocks and 14 emergent reef outcrops with transform-derived per-instance tint, irregular
near crowns, fixed-sun facet shading,
near-only strata/lichen, and a low fixed-sea-level wet band, 13 fallen logs, 31 sparse beach-
driftwood pieces with a three-limb forked near variant,
an authored three-piece cove arc, and per-instance large-prop range/LOD, 63 fallen palm
fronds with curved asymmetric leaflet silhouettes, 265 near-field shell/pebble/dry-wrack clusters,
including a five-rock/two-log authored
clearing-landmark composition, 24 simulated
procedural birds with slow radial route drift, banked turns, intermittent glides, and layered near
trailing feathers, eight fish schools,
with depth-following pitch, banked turns, forked tails, 51/12-vertex CPU-lit near/far geometry,
single-surface fins, restrained depth/caustic colour response, and near head detail, one occasional
non-hostile offshore shark with matching pose, silhouette, and water-column treatment, seven GPU-skinned
procedural monkeys with bounded calm-state head attention and near bone-following fingers/toes,
fixed sunny sky
with layered horizontal high wisps,
five fogged distant islets with 11 simplified palm silhouettes,
one bounded terrain-conforming contact-shadow batch for near palms, canopy trees, land rocks,
wood, and monkeys,
animated shore-following foam, and procedural surf/breeze/jungle, two sparse wave accents, player
water-movement audio, two water entry/exit accents, paired distance-scheduled sand/soil land
footsteps, movement-driven shader-integrated water
ripples, short hysteretic entry/exit surface bursts, a procedural near-surface swimming veil with
depth haze, restrained surface scatter, and offshore-current modulation, a deeper
current-responsive undertow mix in the existing water loop,
and a pooled
positional voice with two bird and two monkey phrases. That package
builds and passes application lint. The current x86_64 runtime gate constructed this complete scene,
reported its biome-aware grass path, and completed a GLES 3 first draw under KVM. The inspected
overview establishes integrated rendering, including the corrected translucent touch HUD, but is
not detailed enough to qualify every layer's close appearance.
The current sources also include the original 20.0 m by 5.2 m broken wooden shipwreck, its twelve
beachward debris pieces, and its single portable 3,948-triangle draw. The refined hull uses a
denser curved cross-section, localized missing boards, keel/rudder, broken rails, hull openings, a
broken foremast, irregular splintered debris, and a portable aged-wood/waterline GLSL ES 300
material. Earlier dedicated x86_64 shipwreck runs on the API 35
`Medium_Phone` AVD used KVM 12 and host AMD Radeon 780M gfxstream,
required both exact shipwreck markers, and produced clean matching GLES 3 frames with and without
the debug overlay. The inspected overlay-free 2400 by 1080 frame retains the complete broken hull,
exposed ribs, partial deck, cabin, snapped mast, debris, ocean overlay, and translucent controls.
The recorded startup split was 2170.241 ms world, 4.046 ms framework, 3916.735 ms content,
31.262 ms pre-draw, and 82.595 ms first draw, for 6204.880 ms total. No fatal, shader, or
`GL_INVALID_OPERATION` diagnostic appeared. This qualifies functional translated-host rendering,
not physical-device performance or thermal behaviour.
The same accelerated environment passed a strict `--viewpoint crabs` run with all thirty agents,
five visible near silhouettes in one draw, the mobile quality marker, and a clean inspected 2400
by 1080 frame. Startup measured 776.182 ms world, 1.782 ms framework, 1512.457 ms content,
24.508 ms pre-draw, and 21.288 ms first draw, for 2336.217 ms total. The default arm64 package was
then restored and the reproducible x86_64 intermediates removed.
The current sources additionally include eighteen deterministic clearing-edge butterflies and
five deterministic near-shore sea turtles. The butterflies retain terrain-relative altitude
bounds, five palettes, and one near/far batch; the turtles retain seabed/surface/shore bounds,
cruise/forage/surface states, articulated flippers, and a separate one-draw near/far batch. On the
same KVM 12/host-gfxstream boot, strict current-source `butterflies` and `turtles` views passed.
The inspected butterfly overlay reported 11/18 visible (seven near, four far) in one draw. The
clean turtle frame showed one near turtle beneath the transparent ocean in one draw; the repeated
run completed first draw in 2527.896 ms. No fatal, shader, or `GL_INVALID_OPERATION` diagnostic
appeared. This qualifies translated-host function and composition, not physical-mobile cost.
The refined shipwreck then passed a new strict x86_64 KVM/gfxstream run requiring its exact
aged-wood/waterline shader marker. The inspected clean 2400 by 1080 GLES 3 frame retains the curved
hull, missing boards, irregular debris, transparent water overlay, and touch controls with one
wreck draw and no fatal, shader, or `GL_INVALID_OPERATION` diagnostic. The emulator booted in
18.606 seconds and reached first draw in 3592.612 ms before a clean shutdown; this remains
translated-host functional evidence rather than physical-device performance or colour
qualification.
The latest strict shipwreck run additionally required the exact large-ship silhouette marker. It
selected `OPENGLES3`, retained one 3,948-triangle wreck draw, and reached first draw in 2436.391 ms.
The full-start AVD boot completed in 18.752 seconds.
Its inspected 2400 by 1080 frame clearly preserves the deck rails, four near-side hull openings,
broken foremast, keel/rudder silhouette, and more strongly scattered beach debris with clean touch
controls. No fatal, shader, or `GL_INVALID_OPERATION` diagnostic appeared. This remains translated-
host functional evidence rather than physical-device cost qualification.
The subsequent fixed-jungle qualification requires the new inland humus/moss material marker.
Its API 35 x86_64 KVM 12/host-gfxstream run selected `OPENGLES3`, reached first draw in
2755.377 ms, and produced a clean inspected 2400 by 1080 scene matching the desktop and browser
composition. An unrelated Pixel Launcher ANR dialog covered the helper's first screenshot after
the game had already qualified; selecting the system `Wait` action and recapturing the still-
focused game produced the retained unobstructed frame. No game fatal, shader, or
`GL_INVALID_OPERATION` diagnostic appeared. This remains translated-host functional evidence,
not a physical-device fragment-cost result.
The subsequent fixed-grass run required the narrower asymmetric/occasional bent-tip diagnostic.
The same KVM 12/host-gfxstream AVD selected `OPENGLES3`, reached first draw in 2943.958 ms, and
produced an inspected 2400 by 1080 scene with the intended thinner varied silhouettes and clean
touch-control blending. No fatal, shader, or `GL_INVALID_OPERATION` diagnostic appeared. This is
functional translated-host evidence rather than sustained physical-mobile performance evidence.
The latest single-ABI arm64 debug qualification build produced a 19,082,004-byte APK with a
16,271,720-byte stripped `libmain.so`, passed `:app:lintDebug` with no issues, and verifies with APK
signature scheme v2. It includes the narrowed,
noise-mottled portable footpath, broad reused-noise clearing-floor patch breakup, inland
leaf-humus/cool-moss substrate breakup, close sand granulation, sparse soft-edged pebble/shell
flecks,
and shoreline-raked ripples, irregular
low rounded near-palm root ridges, irregular near-canopy buttress roots, transform-derived green-only
jungle foliage variation, irregular near fallen-log broken-end contours with restrained radial
end-grain cracks, reused crossing-wave ocean
colour plus shallow-ridge light, short sunward crests, and view-aligned grazing reflection bands,
and restrained near-monkey crown/cheek fur tufts with
bone-following fingers/toes, the transient hardware-keyboard run/walk
confirmation, reduced CPU-lit fish/shark
geometry, the shared run-capable input resolver used by hardware keyboards, and shared-buffer
terrain cell batching. Its four persistent audio beds remove their generated mean before PCM16
quantization without adding another source or payload. It also validates every indexed procedural
mesh plus the monkey fourteen-bone hierarchy/palette, clips, tracks, and key transforms before GPU
upload. A fresh x86_64 build
of the same sources and then-current `9fbe34468c69` CNA pin passed two KVM/host-GPU GLES 3 first-
frame gates. The ordinary overview completed in 2179.350 ms, while the fixed offshore-current view
completed in 1555.412 ms and visibly reported normalized current 0.7 with exact `DRIFT 0.3/1.2`.
Both inspected 2400 by 1080 captures retain the complete scene and clean translucent touch
controls; no fatal, shader-compilation, or `GL_INVALID_OPERATION` diagnostic appeared. The default
arm64 package was then restored. The current `97783d1e090c` x86_64 package subsequently passed the
strict KVM/gfxstream GLES 3 and audio gate at the fixed east clearing. Its inspected 2400 by 1080
frame includes the broad reused-noise clearing treatment and clean touch controls; first draw
completed in 1594.265 ms total. A second ordinary launch injected a sustained movement-zone touch,
moved the player to `z=-204.9`, and played eight distance-scheduled footsteps. Pressing Home then
resuming retained PID 4218, recreated the surface in a 35 ms hot launch, and paused/restarted the
same AAudio stream. The emulator was stopped cleanly and the default arm64-only package above was
restored. Public terrain queries additionally fail closed before non-finite
or out-of-domain values can reach procedural noise-cell conversion. The APK verifies with
signature scheme v2, contains only the selected ABI, and remains the same size after an immediate
no-change incremental assembly.

A later current-source fixed-palm x86_64 run required the transform-derived frond-tint diagnostic
through Android's platform log. Its inspected 2400 by 1080 frame retains the four palm silhouettes,
varied green crowns, unchanged brown trunks/roots/coconuts, the 1280 by 720 logical viewport, and
clean translucent touch controls. The complete scene opened 44.1 kHz stereo AAudio, reported
`OPENGLES3`, and reached first draw in 1605.876 ms total (499.728 ms world, 1.544 ms framework,
1039.218 ms content, 20.983 ms pre-draw, and 44.402 ms first draw) without a fatal, shader, or
`GL_INVALID_OPERATION` diagnostic. This is KVM/gfxstream functional evidence, not a physical-device
colour or vertex-cost qualification. The emulator was stopped cleanly and the arm64-only package
above was restored, rebuilt, linted, ABI-checked, and signature-verified.

The following current-source fixed-prop x86_64 run required the near fallen-log broken-end marker
through Android's platform log. Its inspected 2400 by 1080 frame visibly matches the WebGL 2
non-planar contour, growth-ring material, grass contact, 1280 by 720 logical viewport, and clean
touch controls. The complete scene opened 44.1 kHz stereo AAudio, reported `OPENGLES3`, and
reached first draw in 1567.410 ms total (503.696 ms world, 4.056 ms framework, 995.605 ms content,
21.922 ms pre-draw, and 42.131 ms first draw) without a fatal, shader, or
`GL_INVALID_OPERATION` diagnostic. This is KVM/gfxstream functional evidence, not physical-device
mesh cost or appearance qualification. The emulator was stopped cleanly and the arm64-only
package above was restored, rebuilt, linted, ABI-checked, and signature-verified.

The subsequent radial end-grain crack pass is required by the same strict Android gate and its
default arm64-only package builds/lints cleanly. The 18,747,212-byte APK contains a
15,936,928-byte stripped `libmain.so`, only `arm64-v8a`, and a valid v2 signature. Its dedicated
GLES 3 appearance was deliberately deferred to a batched x86_64 visual session together with the
palm and folded-frond passes; the newer batch described below now qualifies it.

The later selected-canopy liana pass is also required by the strict Android platform-log gate. Its
18,750,124-byte arm64-only APK contains a 15,939,840-byte stripped `libmain.so`, passes application
lint with zero issues, and verifies with signature scheme v2. The established fixed-jungle capture
predated this mesh detail, so the newer batch below supplies its dedicated GLES 3 evidence.

The current batched KVM/gfxstream session closes all five deferred natural-detail gates. Fixed
`palms`, `fronds`, `props`, and `jungle` runs used PIDs 3334, 3529, 3678, and 3826 and completed
first draw in 1581.837, 1589.120, 1548.131, and 1571.279 ms total, respectively. Every run required
the staggered palm leaflet, palm facet-light, folded frond, radial end-grain crack, and hanging
liana markers, reported `OPENGLES3`, opened a running 44.1 kHz stereo AAudio stream, and reported
the 1280 by 720 logical backbuffer. The four inspected 2400 by 1080 frames match the corresponding
WebGL 2 compositions without a fatal, shader, or `GL_INVALID_OPERATION` diagnostic. These are
functional translated-host visual checks, not sustained or physical-mobile performance evidence.
The emulator was stopped cleanly, the arm64-only APK above was restored and revalidated, and about
1.1 GiB of reproducible x86_64 CMake/object intermediates was removed while retaining the compact
logs and screenshots.

The later sparse dry-frond pass used another strict fixed-`palms` run on the same API 35
`Medium_Phone` KVM/gfxstream path. It required the exact hanging-dry-frond marker, reported
`OPENGLES3`, opened the complete scene, and reached first draw in 2413.984 ms total (755.250 ms
world, 2.066 ms framework, 1541.921 ms content, 21.782 ms pre-draw, and 92.965 ms first draw). The
inspected clean 2400 by 1080 frame preserves irregular brown remnants only beneath selected crowns
alongside the existing living leaflets and translucent touch controls. No fatal, shader, or
`GL_INVALID_OPERATION` diagnostic appeared. The emulator was stopped; the then-current 19,079,172-byte
arm64-only APK and 16,268,888-byte stripped `libmain.so` were restored through the shared cache,
lint passed, only `arm64-v8a` is packaged, signature scheme v2 verifies, and the five reproducible
x86_64 intermediate directories were removed. This is functional translated-host visual evidence,
not sustained or physical-mobile performance qualification.

The subsequent near-canopy layering pass used a strict fixed-`jungle` run on the same API 35
KVM/gfxstream target. It required the exact irregular layered lower-crown marker, selected
`OPENGLES3`, and reached first draw in 3070.691 ms total (1205.416 ms world, 4.393 ms framework,
1785.833 ms content, 19.900 ms pre-draw, and 55.149 ms first draw). The inspected clean 2400 by
1080 frame retains varied lower crown edges, secondary drooping leaves, the complete ground layers,
and translucent touch controls without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. The
emulator was stopped; the 19,080,676-byte arm64-only APK and 16,270,392-byte stripped `libmain.so`
were restored through the shared cache, lint passed, only `arm64-v8a` is packaged, signature scheme
v2 verifies, and the five reproducible x86_64 intermediate directories were removed. This is
functional translated-host visual evidence, not sustained or physical-mobile performance.

The following near-trunk/branch pass repeated that strict fixed-`jungle` run on the same API 35
KVM 12/host-gfxstream target. It required the exact irregular-trunk/bent-support-branch marker,
reported `OPENGLES3`, and reached first draw in 2255.331 ms total (694.128 ms world, 2.885 ms
framework, 1484.222 ms content, 20.835 ms pre-draw, and 53.261 ms first draw). The inspected clean
2400 by 1080 frame retains the desktop/Web composition and touch controls without fatal, shader,
or `GL_INVALID_OPERATION` diagnostics. The emulator was stopped; the 19,082,004-byte arm64-only
APK and 16,271,720-byte stripped `libmain.so` were restored through shared `/rv/cnaccache`, lint
passed, only `arm64-v8a` is packaged, signature scheme v2 verifies, and the five reproducible
x86_64 intermediate directories were removed. This remains functional translated-host evidence,
not a physical-device appearance, thermal, or sustained-performance qualification.

An earlier current-source x86_64 pass explicitly enabled the strict audio gate after persistent-
loop DC centering. Logcat proves bounded sequential source staging, the DC-centering path, slow
non-amplifying ambient gain variation, a running 44.1 kHz stereo AAudio stream, `OPENGLES3`, and
the exact 1280 by 720 logical viewport. It reached first draw in 1681.732 ms total (515.618 ms
world, 1.996 ms framework, 1099.860 ms content, 19.109 ms pre-draw, and 45.149 ms first draw).
The inspected 2400 by 1080 screenshot retains the complete scene and translucent touch HUD without
a fatal or shader diagnostic. A stale Quick Boot snapshot crashed the emulator during the first
attempt; preserving the AVD and restarting with `-no-snapshot-load -no-snapshot-save` produced a
clean 14.908-second KVM boot. This is emulator audio-path evidence, not audible quality or latency
qualification. The default arm64 package was restored afterward.

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

The current x86_64 KVM/gfxstream pass also enabled the land-footstep gate. After the qualified
GLES 3 frame and all 16 procedural audio resources were reported through logcat, the smoke helper
injected a sustained left-zone movement gesture. The player advanced into the jungle and emitted
the exact first-footstep playback diagnostic while AAudio remained active. Startup measured
919.172 ms world, 6.511 ms framework, 2065.384 ms content, 19.974 ms pre-draw, 73.976 ms first
draw, and 3085.017 ms total. The inspected 2400 by 1080 screenshot retains the complete scene and
clean touch HUD, with no fatal, shader, or `GL_INVALID_OPERATION` diagnostic. This proves control,
scheduling, and mixer playback on the emulator; it does not qualify subjective audible quality or
physical-device latency. The emulator was stopped and the arm64-only package restored afterward.

A later current-source x86_64 pass combined the strict GLES 3, audio, footstep, and swimming
gates. After a forward movement-zone gesture produced a real land footstep, a nine-second reverse
gesture crossed the same south beach and shallows. Logcat recorded the first water-transition
accent and then the authoritative walking-to-swimming state change 1.224 seconds later. The
inspected 2400 by 1080 screenshot visibly places the player in the ocean facing the complete beach
and jungle, with the near-surface treatment and clean translucent touch controls. Startup completed
in 1679.190 ms total (574.107 ms world, 2.586 ms framework, 1043.779 ms content, 20.960 ms
pre-draw, and 37.759 ms first draw). This proves one deterministic touch shoreline crossing and
audio event on KVM/gfxstream; it does not qualify physical-device finger feel or audible balance.

A subsequent x86_64 KVM/gfxstream water-view pass requires the new sunward-crest diagnostic through
Android's platform log rather than hidden SDLActivity stdout. The fixed 2400 by 1080 frame retains
the subdued directional surface relief, calm horizon, 0.6-strength crossing burst, and clean
translucent touch HUD. It reports `OPENGLES3` and the exact 1280 by 720 logical viewport, reaches
first draw in 3997.896 ms total, and contains no fatal, shader, or `GL_INVALID_OPERATION`
diagnostic. The emulator was stopped cleanly and the arm64-only package was rebuilt and linted.
This is gfxstream functional evidence, not physical-mobile appearance or performance evidence.

A current fixed offshore-current x86_64 KVM/gfxstream pass additionally requires the exact
grazing-reflection diagnostic through Android's platform log. The inspected 2400 by 1080 frame
reports `OPENGLES3`, the 1280 by 720 logical viewport, normalized current 0.7 and drift 0.3/1.2,
and visibly retains restrained low-angle surface breakup. Strict audio qualification also passed.
First draw completed in 1677.445 ms total (574.042 ms world, 2.317 ms framework, 1045.394 ms
content, 20.736 ms pre-draw, and 34.956 ms first draw) without fatal, shader, or
`GL_INVALID_OPERATION` diagnostics. The emulator was stopped cleanly and the arm64-only package
above restored. This is functional translated-host evidence, not physical-device appearance,
thermal, or performance qualification.

A subsequent fixed close-grass x86_64 KVM/gfxstream run requires the sparse near seed-head
diagnostic through Android's platform log. The inspected 2400 by 1080 frame visibly retains several
restrained green/gold heads among the varied juvenile and mature blades, with clean touch-overlay
blending. It reports `OPENGLES3`, the exact 1280 by 720 logical viewport, and a 7148.253 ms cold
first draw total without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. The slower single
emulator launch is functional evidence rather than a performance claim. The emulator was stopped
cleanly and the arm64-only package was restored, rebuilt, linted, and signature-verified.

The current fixed reef x86_64 KVM/gfxstream run requires the irregular near-rock crown,
fixed-sun facet-light, and transform-derived stone-tint diagnostics through Android's platform log.
Its clean inspected 2400 by 1080 frame matches the WebGL 2 crown silhouette, readable low-poly
planes, restrained warm/olive instance tone, and dark/cool sea-level band. It reports `OPENGLES3`,
the exact 1280 by 720 logical viewport, and a 1604.367 ms cold first draw total (491.149 ms world,
3.307 ms framework, 1074.928 ms content, 19.546 ms pre-draw, and 15.437 ms first draw) without
fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This is gfxstream
functional evidence, not a physical-mobile fragment-cost claim. The emulator was stopped cleanly
and the arm64-only package was restored, rebuilt, linted, and signature-verified.

The current fixed-bird x86_64 KVM/gfxstream run requires the layered near trailing-flight-feather
diagnostic through Android's platform log. Its clean inspected 2400 by 1080 frame visibly retains
the broader light/dark near-wing silhouette against the sky and matches the WebGL 2 composition.
It reports `OPENGLES3`, the exact 1280 by 720 logical viewport, and a 2192.963 ms cold first draw
total without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This is gfxstream functional
evidence, not a physical-mobile animation-cost claim. The emulator was stopped cleanly and the
arm64-only package was restored, rebuilt, linted, and signature-verified.

The subsequent 42-blade close-grass pass used the same accelerated GLES 3 path. Two fixed `grass`
runs required the exact density marker in Android's platform log; the clean overlay-free 2400 by
1080 capture visibly retains separated curved blades, the sparse seed heads, and denser near-field
cover without opaque touch-control backgrounds or obvious mesh corruption. The second cold launch
reported `OPENGLES3`, the 1280 by 720 logical backbuffer, and 2937.204 ms total to first draw
(1024.182 ms world, 3.649 ms framework, 1818.057 ms content, 19.443 ms pre-draw, and 71.873 ms
first draw). No fatal, shader, or `GL_INVALID_OPERATION` diagnostic appeared. The emulator was
stopped cleanly and the arm64-only package above was restored and linted. This proves translated
GLES 3 composition, not physical-device cost or ergonomics.

The Android default selects the centralized `mobile` quality profile. The earlier platform-log
marker records 112 m grass range, 72% contact-shadow range, 82% wildlife draw range, a 128 by 128
ocean grid, 143,624/163,840 generated-texture bytes, and 6/6 permitted audio sources. A fixed
close-grass KVM/gfxstream run passed the strict marker gate twice, reported `OPENGLES3` at the
1280 by 720 logical backbuffer, and produced an inspected clean 2400 by 1080 capture. The second
launch reached first draw in 2703.072 ms. The profile changes distant work only: the 42-blade near
mesh, every wildlife population, every shader layer, textures, and complete soundscape remain.
This is functional translated-host evidence, not a physical-device frame-time or thermal claim.

The static canopy-light field added 4 MiB to this inventory: that checkpoint
reports 4,337,928/4,358,144 generated-texture bytes. Arm64 assembly and lint pass for this change.
The subsequent low-jungle receiver extension also passes arm64 assembly/lint without changing
that texture budget.
The boar CPU-light receiver also compiles and passes lint, retaining an additional 1 MiB CPU
field without changing GPU texture bytes. Its Android runtime is not yet verified.
Monkey footprint sampling through the stock skinned material also passes arm64 assembly/lint,
reusing that same CPU field; Android runtime verification remains pending for both receivers.
The subsequent boar coat atlas added 65,536 bytes; that checkpoint expected
4,403,464/4,423,680 generated-texture bytes. Arm64 assembly/lint passed with shared ccache and two
workers; this material's Android runtime is not yet verified.
The later grass-grounding map adds 232,324 bytes, bringing current source inventory/budget to
4,635,788/4,656,004 bytes. It uses a portable RGBA8 texture, not floating-point render targets.
The subsequent shared boar body/leg pose and tapered geometry also pass arm64 assembly/lint;
their Android runtime remains unverified while the GPU incident gate is active.
The close jungle leaf material also passes arm64 assembly/lint with its custom 52-byte vertex
layout and nine attributes including instancing. It adds no textures or submissions; Android
runtime and physical-device performance remain pending.
The earlier runtime evidence above predates that addition; Android runtime validation of canopy
lighting is still pending. Do not infer it from a successful package build.

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

## Historical runtime evidence (before the GPU incident)

The pre-incident qualification pass used the API 35 `Medium_Phone` Google Play x86_64 AVD with Android
Emulator 36.5.11, `-accel on`, and `-gpu host`. `emulator -accel-check` reports KVM version 12 as
usable. Gfxstream translates GLES through the host AMD Radeon 780M. The guest remains at its
physical 1080 by 2400 / 420 dpi configuration and presents the landscape application surface at
2400 by 1080. CNA renders the requested 1280 by 720 logical backbuffer and scales that 16:9 image
into a centred 1920 by 1080 presentation region; the remaining side bars belong to the physical
surface and do not increase 3D shading resolution.

The current APK loaded `libSDL3.so` and `libmain.so`, created an RGBA8888 GLES 3 surface, opened a
44.1 kHz stereo AAudio stream, reported renderer `OPENGLES3`, and completed its first draw. The
current overview measured 728.549 ms for world generation, 2.625 ms for framework initialization,
1354.746 ms for content creation, 23.770 ms before the first draw, 69.660 ms for the first draw
body, and 2179.350 ms total. The fixed offshore-current view measured 521.314, 2.319, 972.728,
19.533, and 39.518 ms for those phases, respectively, or 1555.412 ms total. These one-run figures
prove startup and are not a performance comparison. The previous unaccelerated QEMU TCG pass took
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
landmark itself, and the one-run timing is not a performance comparison. It describes the
then-qualified baseline rather than the newer `97783d1e090c` build/lint qualification.

The current-pin viewport qualification adds the authoritative logical backbuffer to both the F1
panel and the first-frame platform log. A fixed monkey launch on the same API 35 KVM/host-GPU AVD
reported `OPENGLES3` and `1280x720 logical backbuffer`; the inspected 2400 by 1080 physical
screenshot shows the same `VIEWPORT 1280X720` value and 31.5 FPS / 29.1 ms at that instant. First
draw completed in 1439.657 ms total. Both runtime gates now require exactly 1280 by 720 rather than
accepting a missing or inferred logical resolution. No fatal, shader-compilation, or
`GL_INVALID_OPERATION` diagnostic appeared. This validates resolution accounting and functional
presentation on gfxstream, not sustained performance on a physical mobile GPU.

A current fixed-jungle launch after adding transform-derived foliage tint passed the strict GLES
3 plus audio gate and required the exact platform-log diagnostic. The inspected 2400 by 1080 frame
retains all four jungle layers, unchanged brown trunks, the 1280 by 720 logical viewport, and clean
translucent touch controls. It completed first draw in 1586.194 ms total (487.110 ms world, 1.601
ms framework, 1035.035 ms content, 18.751 ms pre-draw, and 43.697 ms first draw) without a fatal,
shader, or `GL_INVALID_OPERATION` diagnostic. The emulator was stopped cleanly and the default
arm64-only package restored; this is functional gfxstream evidence, not a physical-GPU cost claim.

SurfaceFlinger presentation samples at the same initial scene improved after terrain visible-cell
run batching from 40.903 ms mean (24.45 frames/s, 71 of 126 intervals over 40 ms) to 34.163 and
32.590 ms mean (29.27 and 30.68 frames/s, respectively, with 6 and 1 intervals over 40 ms). This is
useful same-AVD before/after evidence, but it includes compositor/emulator effects and is not a
mobile GPU frame-time claim. Physical-device input latency, touch ergonomics, audio-focus
interruption, sustained memory, thermals, and lifecycle recovery remain open on real hardware.

A current fixed `litter` x86_64 KVM/gfxstream launch passed the strict GLES 3 and audio gate after
the near forest-floor mesh gained three sparse folded fallen leaves. The game reported the exact
reuse diagnostic, `OPENGLES3`, a 1280 by 720 logical backbuffer, and a 1699.166 ms total cold first
draw. The inspected 2400 by 1080 frame retains the subtle brown leaf silhouette below its source
plant, surrounding vegetation, and clean translucent touch controls without a fatal, shader, or
`GL_INVALID_OPERATION` diagnostic. The emulator was stopped cleanly and the default arm64-only
APK was restored, linted with zero issues, and v2-signature verified. The restored package is
18,742,572 bytes and its stripped `libmain.so` is 15,932,288 bytes. This qualifies functional
gfxstream composition, not sustained performance or a physical mobile GPU.

A current fixed `grass` x86_64 KVM/gfxstream launch passed the strict GLES 3 and audio gate after
12 of the 34 near blades gained a geometric V-fold ridge. The game reported the exact fold
diagnostic, `OPENGLES3`, low-latency audio, a 1280 by 720 logical backbuffer, and a 1791.332 ms total
cold first draw. The inspected 2400 by 1080 frame retains clean blade tips, restrained fold
lighting, surrounding density, and translucent touch controls without a fatal, shader, or
`GL_INVALID_OPERATION` diagnostic. The emulator was stopped cleanly; the default arm64-only APK
was restored, linted with zero issues, and v2-signature verified. It is 18,744,028 bytes and its
stripped `libmain.so` is 15,933,744 bytes. This is functional gfxstream evidence, not a sustained
performance or physical-mobile qualification.

A current fixed `driftwood` x86_64 KVM/gfxstream launch passed the strict GLES 3 and audio gate
after variants zero and one replaced their flat branch fins with tapered three-dimensional limbs.
The game reported the exact all-variants diagnostic, `OPENGLES3`, low-latency audio, a 1280 by 720
logical backbuffer, and a 1549.511 ms total cold first draw. The inspected 2400 by 1080 cove frame
retains all three distinct silhouettes and clean touch controls without a fatal, shader, or
`GL_INVALID_OPERATION` diagnostic. The emulator was stopped; the arm64-only APK was restored,
linted with zero issues, and v2-signature verified. It is 18,743,708 bytes and its stripped
`libmain.so` is 15,933,424 bytes. This remains functional emulator evidence rather than physical-
mobile performance qualification.

The subsequent topology-preserving driftwood end-cap correction and constant-only east-clearing
contrast retune passed one batched x86_64 KVM/gfxstream qualification. The fixed `driftwood` run
reported the exact irregular-ring marker and completed first draw in 1522.465 ms total; its
inspected 2400 by 1080 frame retains three clean closed silhouettes. A separate fixed `east` run
completed in 1521.558 ms total, and its inspected frame retains a restrained but clearly warmer
olive-gold clearing floor. Both runs reported `OPENGLES3`, low-latency audio, and the 1280 by 720
logical backbuffer without a fatal, shader, or `GL_INVALID_OPERATION` diagnostic. The emulator was
stopped and the arm64-only package was restored, linted with zero issues, and v2-signature
verified. The APK is 18,744,428 bytes and its stripped `libmain.so` is 15,934,144 bytes. These are
functional emulator checks rather than physical-mobile appearance or performance qualification.

The later near-palm leaflet asymmetry pass builds and lints in the default arm64-only package,
which is 18,745,116 bytes with a 15,934,832-byte stripped `libmain.so` and a valid v2 signature.
The Android runtime gate requires its exact staggered-leaflet marker. Its GLES 3 appearance was
intentionally deferred rather than recreating about 1.1 GiB of temporary emulator intermediates
for a vertex-position-only follow-up; the current batched palm capture now qualifies it.

The following near-only fixed-sun palm shading pass also builds/lints for arm64 and is covered by
the next runtime marker gate. That arm64-only APK was 18,745,740 bytes with a
15,935,456-byte stripped `libmain.so`; lint reports zero issues and signature scheme v2 verifies.
Its Android runtime appearance is qualified by the current batched palm capture; physical-mobile
cost remains unmeasured.

The subsequent folded fallen-frond pass also compiles/lints for arm64 and is now required by the
next Android runtime marker gate. That arm64-only APK was 18,746,380 bytes with a
15,936,096-byte stripped `libmain.so`; lint remains clean and signature scheme v2 verifies. Its
GLES 3 appearance is qualified by the current batched frond capture together with both palm
refinements; physical-mobile cost remains unmeasured.

The current persistence-qualified x86_64 run booted the API 35 `Medium_Phone` AVD with KVM 12 and
host-GPU gfxstream in 31.843 seconds. PID 3937 selected high touch-look sensitivity, wrote the
schema-1 settings record, retained the same process across Home/resume, and restored its surface
and AAudio stream. After force-stop, PID 4112 loaded `touch high` from app-private CNA storage and
requalified `OPENGLES3`, 1280 by 720 logical rendering, and all strict audio diagnostics. Initial
and cold-relaunch first-draw totals were 2500.229 and 2454.681 ms. All three inspected captures are
clean 2400 by 1080 frames. The emulator was stopped, the default arm64-only APK was restored, lint
reported no issues, and signature scheme v2 verified. The APK is 18,861,028 bytes and contains a
16,050,744-byte stripped `libmain.so`; the temporary x86_64 intermediates were then removed. This
qualifies functional persistence and lifecycle recovery on the emulator, not physical-device
storage durability, ergonomics, audio focus, thermals, or driver coverage.

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
logical viewport, first-draw timing, biome-grass, near-seed-head, near-bird-feather, and sunward-
ocean-crest plus grazing-reflection diagnostics, the selected near-grass geometric V-fold, the
narrower asymmetric/occasional bent-tip close-blade path, the
all-variants tapered driftwood branches, their body-ring-matched end caps, and the near-rock
crown/facet plus stone-tint diagnostics. Every strict run also requires the exact 20 m broken
ship/12-debris construction marker and the renderer marker proving the exposed-rib/partial-deck/
snapped-mast path. The helper rejects
fatal/shader errors, and saves both logcat and a device screenshot:

```bash
tools/run_android_runtime_smoke.sh \
  "$ANDROID_SDK_ROOT/platform-tools/adb" \
  android/app/build/outputs/apk/debug/app-debug.apk
```

The default evidence directory is ignored `build/android-runtime-smoke/`. The gate proves a live
GLES 3 frame and process; it does not by itself qualify touch ergonomics, audio, lifecycle recovery,
performance, memory, or thermals. Override the deliberately generous software-emulator timeout with
`ISLAND_ANDROID_SMOKE_TIMEOUT_SECONDS` when testing faster hardware.

Screenshot acceptance also uses `android_capture.py` (host Python/Pillow). Each initial,
hot-resume and cold-relaunch capture gets a bounded 30-second presentation wait, rejecting
empty, malformed or visually degenerate images with the shared native capture thresholds.
The explicit `litter` camera uses the reviewed `shaded-detail` relative-contrast profile;
all other cameras use `scene`. The last explicit camera argument selects the profile,
including `--overview`; failed pixels never trigger a fallback profile. See
`terrain-surface-frame.md` for calibration and limitations.
Each PNG has a sibling `.validation.json` containing metrics, errors and attempt count.
This closes a false positive observed on 2026-10-11: a startup/first-draw log was present,
yet the immediate screenshot was entirely black. Drawing diagnostics alone are not proof
of display presentation. Image checks do not establish semantic correctness or exclude
system overlays; inspect accepted images too. Earlier runs did not have this image gate.

`CNA-ISLAND-002` exposed another false positive: a complete background image despite failed
stock-skinned shader linking and missing monkeys. The application now mirrors bounded stderr
lines to the platform log; all four runtime checkpoints reject stock VS/FS/link failures and
nonzero guest GL diagnostics. Scene-content review remains required. The monkey workaround
is not implemented yet; do not infer wildlife acceptance from the earlier image gates.

Set `ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1` for an audio-qualified run. In addition to the normal
GLES 3 requirements, this waits for the exact sequential-generation/upload diagnostic emitted only
after all sixteen effects were created, the exact persistent-loop DC-centering diagnostic, and the
exact slow non-amplifying ambient-gain diagnostic. It proves that the current generation, staging,
and runtime-mix paths reached CNA's audio objects; it does not prove audible balance, device
latency, or total process-memory usage:

```bash
ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1 \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk
```

Set `ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS=1` as well to require the land-footstep resources,
inject a sustained movement-zone touch gesture after the first qualified frame, and wait for the
exact first-playback diagnostic:

```bash
ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS=1 \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk
```

Set `ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING=1` to continue from the interactive south-beach spawn
through the shallows with a reverse movement-zone gesture and require an actual
walking-to-swimming state transition. When the audio gate is also enabled, the same check requires
the water-transition accent to have started. It can be combined with the footstep gate to validate
both traversal directions in one process. Fixed `--viewpoint` arguments are rejected because they
deliberately disable player movement; overview, capture, smoke, and benchmark modes are rejected
for the same reason:

```bash
ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING=1 \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-swimming
```

Set `ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY=1` to exercise the top-right three-dot
selector. The helper parses both SDL's physical window and the logical backbuffer, maps the target
through the same aspect-fit viewport (including pillarbox or letterbox margins), and holds a
stationary contact long enough for the game to observe its Pressed state. It requires the exact
high/1.35-times and no-mouse-capture platform diagnostics. Renderer-independent tests separately
cover the complete standard-to-high-to-low cycle, hold debouncing, look-role isolation, bounded
multipliers, and selection preservation across resize. It composes with traversal and lifecycle
modes:

```bash
ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE=1 \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-touch-sensitivity
```

The smoke helper disables user settings by default so fixed cameras and input automation cannot
contaminate a player's preferences. Set `ISLAND_ANDROID_SMOKE_DISABLE_USER_SETTINGS=0` and
`ISLAND_ANDROID_SMOKE_REQUIRE_SETTINGS_PERSISTENCE=1` to qualify storage explicitly. Persistence
mode also requires the touch-sensitivity and lifecycle gates: it selects high, requires the save
diagnostic, force-stops the app, and requires a new process to load high from Android app-private
storage. It can be combined with the strict audio gate:

```bash
ISLAND_ANDROID_SMOKE_DISABLE_USER_SETTINGS=0 \
ISLAND_ANDROID_SMOKE_REQUIRE_SETTINGS_PERSISTENCE=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1 \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-settings
```

Set `ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE=1` to extend a qualified run with both lifecycle
paths. The helper sends Home and brings the activity forward, requires the original PID plus SDL
pause, surface destruction/recreation, nonzero window dimensions, resumed focus, and (under the
audio gate) AAudio pause/start. It then waits for presentation to settle before capturing the
resumed frame. Finally it force-stops the package, requires the original PID to disappear, starts
a new process, and requires a fresh qualified GLES 3 frame. The cold audio gate repeats all three
resource/mix diagnostics. Separate logs, launch timings, and screenshots are retained for the hot
resume and cold relaunch. All traversal and lifecycle checks can run together:

```bash
ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING=1 \
ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE=1 \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-lifecycle
```

This verifies deterministic functional recovery on the identified emulator. It does not qualify
long suspension, OS-initiated process eviction, external audio-focus interruption, or lifecycle
behaviour across the physical Android device/driver matrix.

For a fixed debug QA camera, pass a comma-separated native argument array. The application ignores
this extra in non-debuggable packages:

```bash
ISLAND_ANDROID_SMOKE_ARGUMENTS=--viewpoint,shipwreck,--debug-overlay \
  tools/run_android_runtime_smoke.sh \
  "$ANDROID_SDK_ROOT/platform-tools/adb" \
  android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-shipwreck

ISLAND_ANDROID_SMOKE_ARGUMENTS=--viewpoint,crabs,--debug-overlay \
  tools/run_android_runtime_smoke.sh \
  "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-crabs

ISLAND_ANDROID_SMOKE_ARGUMENTS=--viewpoint,butterflies,--debug-overlay \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-butterflies

ISLAND_ANDROID_SMOKE_ARGUMENTS=--viewpoint,turtles,--debug-overlay \
  tools/run_android_runtime_smoke.sh \
    "$ANDROID_SDK_ROOT/platform-tools/adb" \
    android/app/build/outputs/apk/debug/app-debug.apk \
    build/android-runtime-smoke-turtles
```

Every strict run also requires the thirty-agent crab, eighteen-agent butterfly, and five-agent
sea-turtle population/renderer markers. The species cameras expose nonzero visible populations
and their single-draw batches in the retained platform logs or F1 captures.

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
