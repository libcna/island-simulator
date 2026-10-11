# Monkey fur material

## Shorter fur and non-collapsing side UVs (2026-10-11)

The first source and cosine UV projection produced large wood-like streaks on the actual
monkey. The derivative of `.5+.4*cos(angle)` vanishes on side-facing flank vertices: a
narrow strip of the image is spread over that surface. `MonkeyFurUv::Around` now uses an
angular mirrored interior window, with folds at top/bottom instead of the broad sides.
Head UVs also use angular coordinates rather than planar X/Y projection. Body, limbs,
head and tail retain skin-attached rest UVs; no repeat/seamless claim is required. CPU
tests cover side density, range, circumference closure and longitudinal endpoints.

New source `assets/art-studies/monkey-fur-albedo-v2.png` is the unchanged built-in ImageGen
output `01a129bd-2c19-7192-a43c-adcfe79c0fb9/exec-64a1789b-5a13-41e9-bf2d-a0d42e3bef38.png`.
Exact final prompt is `assets/art-studies/monkey-fur-albedo-v2-prompt.txt`. Generated without
reference inputs, actually 1254-square opaque RGB8 despite the 1024 prompt. SHA-256 and
provenance are registered in the asset manifest and ASSET_LICENSES.md. A first attempted
revision (`exec-f27ea7ba-8883-4926-80a7-7b13f5ef2764.png`) still had long flowing strands
and was rejected; it is not a runtime asset. Historical v1 remains unchanged. CMake and
Android Gradle now prepare v2 at the same 512-square runtime path; mip/texture memory,
shaders, palette, draw count and texture-loading diagnostics are unchanged.

Native evidence `20261011T141731.945186Z-monkey-tree` passes all eight stage gates; peak
image inspected against `20261011T141011.681705Z-monkey-tree`: coarse flank/head ridges
are replaced by finer, shorter fur detail. Ordinary moving-view capture
`20261011T141831.862271Z-monkeys` also passes/inspected. Both kernel deltas are empty.
This is a visible material improvement, not a realistic-anatomy acceptance: bulky limb
attachments, simplified facial anatomy, geometric silhouette and stiff tail remain.
Static frames do not prove full animation quality, LOD transition quality or climbing.

Native build and 37/37 non-display groups pass in 48.27 seconds
(`build/monkey-fur-v2-tests.log`). Asset gate: 24 procedural records, 70 generator sources,
10 project files, zero external assets. Web runtime qualification remains outstanding.

Android x86_64 build/lint passes (10 s). Isolated KVM/SwiftShader GLES evidence
`20261011T142013.159547Z-monkeys` passes on its first presentation attempt; the actual
1280x720 screenshot was inspected and shows the same finer coat. Kernel delta is empty.
Retained APK `build/android-software/island-monkey-fur-v2-x86_64.apk`, 21,158,731 bytes,
SHA-256 `fa80bf806c7540f9fafc71a010b38909d5730ab4e7f5ba92d09eeef7f0abde2b`.
This ordinary view does not qualify Android tree-contact poses, lifecycle or real hardware.
Default ARM64 build/lint restored successfully (9 s), runtime untested; APK 21,233,236
bytes, SHA-256 `d6078c5ef061eb3127e2c7193758f8ef835a230ba37a05bcc7740970c12cb0aa`.
Stop latch unchanged; all launched jobs finished, no host/emulator configuration edits.

## Historical v1 provenance

Built-in ImageGen (not CLI/API fallback), generated without reference images. Exact
prompt: `assets/art-studies/monkey-fur-albedo-v1-prompt.txt`. The original output
`01a129bd-2c19-7192-a43c-adcfe79c0fb9/exec-21714962-5c96-485b-9ad1-2835b81941a6.png`
was copied unchanged to `assets/art-studies/monkey-fur-albedo-v1.png`.
Actual size is 1254 by 1254, opaque RGB8, despite the requested 1024-square prompt.
Both source and prompt hashes are recorded in `assets/manifest.json`.

## Implementation

Existing bounded host area reduction prepares `Content/wildlife/monkey-fur.png`
at 512 square for native, Web preloading and Android assets. The original stays intact.
The shared opaque-image loader constructs ten mips (1,398,100 RGBA8 bytes).
Missing or invalid fur is a startup error, not an undiagnosed plain-colour fallback.

Rest-mesh UVs travel with skinning. Longitudinal body/limb/tail coordinates follow
each mesh part; transverse mapping uses a mirrored interior window, avoiding direct
repeat sampling of an image whose seamlessness has not been established. Head mapping
is projected in its rest frame. The texture modulates diffuse lighting relative to its
mean, bounded to 0.45–1.8; face and eyes explicitly disable fur modulation. Existing
canopy shade, specular and fog remain independent.

## Verification and remaining scope

Native incremental build passes. Contained software capture
`20261011T114648.821520Z-monkeys` passes the strict close-subject and fur-load gates;
the 1280x720 image was inspected. Fur variation is visible on the flank/head instead
of a constant brown fill; the face and limb junctions remain visibly stylized. Kernel
delta is empty. This static frame does not prove animation quality or absence of
stretching. Android and Web qualification of this changed model/material is pending.
Mirrored windows and per-part density still need art direction. This is not silhouette
fur, a completed monkey overhaul, terrain-aware planting, or tree climbing. Those and
the full island audit remain open.

Native non-display suite: 36/36 pass (69.53 s). After adding the fur source to the
actual-image CPU pixel-oracle comparison, material/shader recheck passes 2/2 (9.98 s).
Android software-launcher CPU tests: 21 pass; native launcher tests: 29 pass. Asset
manifest validation and Web runner syntax pass. None constitutes Android/Web runtime
qualification. Host GPU stop latch is unchanged; no dependency or emulator edits.
