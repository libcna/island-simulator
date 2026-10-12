# Monkey fur material

## Renewed rat-like silhouette complaint — 2026-10-12

Actual contained native baseline `20261012T061354.741487Z-monkeys` was inspected
(`build/monkey-rat-review-before.png`): thin pointed tail, projecting narrow muzzle,
long torso and dark simplified face. This confirms visible defects despite existing
fur/rig work; previous implementation is not appearance acceptance.

The tail now retains a fuller fur-bearing radius through most of its length and rounds
to a closed tip, instead of linearly tapering from36mm to3mm. Radius is
`(.032+.022*(1-t))*sqrt(max(0,1-t^12))`; near24rings/12sides, far12rings/8sides.
The terminal ring is closed with a single triangle fan rather than collapsed quads.
Existing three-bone centreline, blending, fur material and UVs remain. Cranial radii
increase from150/160mm to166/176mm; the muzzle is shortened/widened, nostrils and
mouth follow it, the abdomen is narrower and shoulders fuller. No gait/AI change.

Contained native after-view `20261012T061652.692874Z-monkeys` PASS322bins/std36.01/
spatial210.29, kernel-after[]; actual image inspected in
`build/monkey-rat-review-after.png`. Tail has more volume and face is less pointed.
Same camera, not an exact pose/wind/frame match. The tail still reads as a smooth
tube at this distance and limb junctions, facial detail and overall realism remain
open. No visual completion, full movement/climbing or portable qualification claim.

Safety deviation: the initial `ctest -R Monkey` mistakenly included a direct display
test and ran on AMD at08:16CEST. It terminated normally, but was NOT authorized by
the stop latch and is not accepted qualification. Read-only kernel query08:15–08:18
returned no entries. Subsequent checks use contained software/`linux-core` only.
CMake now disables direct display-labelled tests when the retained incident stop
file exists at configuration time; `ctest -N -R IslandGameMonkeyCapture` confirms
Disabled. This is not a runtime guard on direct executable launches. The latch itself
is unchanged. This guard is not a driver fix or permission to resume hardware tests.
Final incremental build and inclusive55 CPU groups PASS100.52s; logs
`build/monkey-silhouette-final-build.log` and `build/monkey-silhouette-full-tests.log`.

## Hooked tail, facial rims and smoother trunk — 2026-10-12

The retained `monkey-tree` sequence made the remaining tail defect clearer than the
single ordinary still: the thicker tube projected nearly straight behind the animal and
read as a dog-like rod. The three existing tail bones now follow an upward curve and the
terminal Hermite point turns forward, forming a continuous hook. The rounded fur-bearing
radius profile, 24/12 near rings/sides, skin weights and animated phase offsets are retained.
Tests require the terminal centre and tangent to turn forward/upward while preserving C1
continuity and normalized two-bone blending.

The body loft now has 13 longitudinal sections instead of nine, with additional bounded
hip, abdomen, shoulder and neck transitions. The cranium grew about 7–8%, and rounded
outer/inner ears are larger. Each dark cornea is now framed by separate skin-coloured upper
and lower eyelid strips (40 sampled points and 40 triangles per monkey face) whose profile
is finite, bilaterally mirrored, nonzero-width and slightly forward of the orbital surface.
This uses the existing face/eye draws, bone palette, fur texture and materials; no texture,
shader, animation state or draw call was added.

Actual contained llvmpipe `20261012T202913.511429Z-monkey-tree` passes all nine images;
selected stages were inspected together. They show the subject walking quadrupedally,
reaching one arm to the trunk and returning, while the hooked tail remains volumetric and
animated. Final metrics are 203 colour bins, luminance deviation 44.72 and spatial contrast
192.54. Ordinary close `20261012T202959.340747Z-monkeys` also passes and was inspected
(164 bins / 29.02 / 113.80). Both kernel deltas are empty. The new hook is a clear
silhouette improvement, but the procedural limbs/trunk and small face detail remain
visibly stylized; this is not final realism or portable qualification.

Release monkey profile/gait, skinning-shader and sole tests pass 3/3; Debug gait and both
native game builds pass. No AMD GPU, Android emulator/KVM or dependency setting was used.

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

Subsequent WebGL check of current fur plus limb collars passes:
`20261011T143701.925390Z-chrome-monkeys`, six images inspected, 60 intervals, GL error
zero, intact context, empty kernel delta. Prepared fur pixels match native by SHA-256.
See `docs/monkey-limb-skin.md` for scope and remaining visual defects.

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
