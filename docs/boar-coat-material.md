# Boar coat material — live integration, visual refinement open

## Tapered upper limbs, compact hooves and tracking audit — 2026-10-13

The close side view still exposed each leg as a narrow uniform tube attached to the
large torso. Each upper limb now uses two connected fur-textured sections, tapering
from a 0.135 m body-rooted radius through 0.095 m to the existing 0.057 m knee. This
retains the fixed-length articulated pose and per-foot terrain fit while giving the
shoulder/haunch junction more mass. The two near cloven-hoof lobes are narrower,
12 mm lower and 15 mm shorter, so they no longer read as oversized black balls.

The accepted change adds 192 vertices / 64 triangles per near animal and 120 vertices /
40 triangles per far animal, within the existing bounded upload and unchanged two opaque
draws. An earlier attempted pair of separate furred shoulder/haunch ellipsoids was
visually rejected: their closed oval boundaries and rotated hair mapping produced obvious
side patches. That implementation was removed before this checkpoint.

Release and Debug `IslandBoarHerdTests`, `IslandBoarCoatTextureTests` and
`IslandBoarPoseTests` pass. The final deterministic contained llvmpipe audit
`20261013T050251.961664Z-boars` passes at 1280×720 with 211 colour bins, 18.69
luminance deviation, 89.32 spatial contrast and an empty kernel delta. All eight staged
poses keep the subject visible and were inspected; the side view shows continuous tapered
upper limbs and smaller split hoof silhouettes.

A new focused browser `boars` scope runs only this viewpoint, requires the current limb
diagnostic, two live material draws, 60 finite intervals, an intact SwiftShader context
and two distinct images. Its first run correctly exposed that the fixed camera lost the
moving animal. The QA camera now follows the first live boar and continuously retargets
it. Final Chrome/SwiftShader evidence `20261013T050501.611529Z-chrome-boars` passes with
`glError=0`, `contextLost=false`, two different image hashes and an empty final kernel
delta. Both images were inspected: the animal remains centred while turning from a side
pose toward the camera. Android runtime and further continuous joint/gait realism remain
open, so visual acceptance is not claimed.

## Broader head and profile-readable tail — 2026-10-13

The bridge-to-nose scale is now 0.55 instead of 0.70, moving the authored tip
from z1.060 to z0.934 while retaining the cheek, eye and ear anchors. Six forward
body stations have a wider cheek/forehead transition and a slightly fuller shoulder,
so the textured head no longer narrows into the earlier long rodent-like wedge.
The former straight ellipsoid tail is replaced by four connected tapered fur segments.
Their five-point curve rises from the rump, curls laterally and returns forward/down,
making the short hook visible both from the side and from above; a small gait-driven
twitch stays continuous around the cycle.

`IslandBoarPoseTests` now covers the revised muzzle length, minimum cheek widths,
tail attachment, segment lengths, side-profile return, cycle continuity and non-finite
fallback. Focused release boar herd/coat/pose tests pass. Actual isolated llvmpipe
capture `20261013T011157.531270Z-boars` passes at 1280x720 with 211 colour bins,
luminance deviation 18.69 and spatial contrast 89.30; live and final kernel incident
logs are empty. Eight deterministic forage-to-walk poses were captured. A matched
stage-two comparison against `20261013T010653.451649Z-boars` was inspected: the head
is shorter/full-bodied and the tail reads as a compact raised hook rather than the
old horizontal rod. Fur remains clearly visible, but coarse limb junctions and the
procedural low-poly anatomy still prevent final realism acceptance.

Debug build and its core/herd/coat/pose groups pass. Release WebAssembly builds, and
the actual Chrome/SwiftShader wildlife-foliage run
`20261013T012751.079805Z-chrome-wildlife-foliage` passes all four views with 60
intervals each, including the boar view; every view reports `glError=0`, no context
loss and an empty final kernel delta. The first browser attempt exposed a stale
one-draw harness expectation even though the renderer and its material assertion both
correctly reported two opaque draws; the gate now requires the exact two-draw
contract shared with native capture. The actual Web boar before/after-simulation
images were inspected. Android runtime qualification is not claimed by this entry.

## Fine-bristle source v2 — 2026-10-12

Built-in ImageGen produced a new original opaque 1254-square source at
`assets/art-studies/boar-bristle-albedo-v2.png`; exact prompt is the sibling
`boar-bristle-albedo-v2-prompt.txt`. No external reference images were used.
The previous source remains preserved. Source SHA256 is
`79490e2046ba87f0e5260e39981f0e2e29909a2b2ed32326a4b2462b5cd50c15`.
The source and actual reduced 256-square texture were inspected. Existing material
preparation produces153886bytes, SHA256
`0b292105904b1337de14a8fdb2323a934aef61f8bf2d3b7066a991fac3fcf880`.
Native/Web source selection and Android preparation now select v2; dimensions,
mips, memory, UVs and draw counts are unchanged. Manifest/license records include
source and prompt. Asset and pixel-preparation tests pass34.01s; native build passes.

Actual software capture `20261012T071024.005936Z-boars` passed with an empty kernel
log and byte-identical pose JSON to the previous material run. Enlarged stage0
`build/boar-v2-after.png` was inspected: finer, less high-contrast body bristles,
but head UV stretching and simplified face/joints remain. Not final visual
acceptance. No full-suite or Web/Android runtime qualification for v2.

## Runtime integration

### Shorter nasal region and short muzzle fur

The first nasal pass compressed only the region in front of z0.78 by0.70, moving
the tip from1.060 to0.976m. The later 2026-10-13 pass above uses0.55 and a0.934m tip.
Cheeks/ear locations and all limb/foot anchors remain unchanged. Body rings, fitted
mouth, outer eye rim, nose disc, nostrils and tusks use the same mapping;
nose/nostril/tusk depth dimensions scale consistently. Rooting coefficients and
vertical positions are unchanged.

`BoarFurAlong` is continuous at the bridge but reverses into a denser interior source
window toward the nose. The muzzle uses over six times the longitudinal UV density
of the body, so nasal hair is shorter instead of long trunk-scale streaks. Regression
checks tip/identity/UV continuity/range/density plus existing fitted face triangles,
all rooting poses and foot contact. Focused pose/herd/coat PASS11.19s.

Actual contained064822.203943Z-boars PASS332bins/std44.06/spatial195.44/kernel[],
image `build/boar-muzzle-after.png` inspected. Nose-region fur is finer; the head
still has coarse cheek streaks and angular ear/limb transitions. This camera's animal
pose changed with wall time, so it is not an exact pose-matched comparison. A fixed
boar capture clock and multi-pose review remain desirable before broad visual claims.
No portable build/runtime, no new texture/allocation/draw count in this refinement.

The game now requires `Content/wildlife/boar-bristle.png`, prepared at256square,
with9runtime mip levels/349524RGBA8bytes. Missing content fails rather than falling
back to the rejected procedural coat. Body and upper legs use the RGB source with
near-neutral variant tints; eyes/nose/hooves and remaining solid anatomy keep their
own colours. One shared upload is partitioned into two opaque BasicEffect draws.
Every triangle's material marker is checked before partition; old white-strip atlas
coordinates are no longer sampled. `stable_partition` may use bounded temporary CPU
storage up to the existing36000vertex batch; no new permanent GPU vertex allocation.

Interior UVs use longitudinal0.1..0.9 and mirrored angular0.1..0.9 windows, reusing
the existing MonkeyFurUv mapping. Fold lines remain at top/bottom rather than broad
sides; this avoids assuming seamless source edges. Old procedural coat code/tests
remain historical utility, not a runtime fallback.

Replacing128square/no-mip65536bytes with the new349524byte material adds283988bytes.
Generated-only textures now4586636bytes; total runtime9130456bytes and full/mobile
quality caps9150672bytes. Native install, Web preload and Android preparation include
the required source/output; compile/package changes alone are not portable runtime proof.
Browser/Android exact-budget checks have been updated without weakening thresholds.

First actual contained native063826.490440Z-boars was inspected in
`build/boar-bristle-live-first.png`, PASS326bins/std44.03/spatial195.59. Fur is visible,
but conspicuous long streaks on the head and plain upper legs were exposed. Upper
legs then received matching fur UVs/tint. Muzzle scale/stretch, body/leg joints and
naturalness remain open; this is NOT completed boar appearance.
Final contained064239.018898Z-boars PASS324bins/std44.05/spatial195.52/kernel-after[];
`build/boar-bristle-live-final.png` actually inspected. Upper legs now share the coat,
while eyes/nose/hooves stay solid; required material and1/6visible/two draws confirmed.
Fur reads clearly but the long muzzle still produces conspicuous streaks and the
coarse limb junctions remain. No overall realism acceptance.
Native build/inclusive55CPU PASS136.47s, including actual boar-source host/C++ pixel
comparison and exact budget checks. Logs build/boar-material-{final-build,full-tests}.log.
JS smoke-script syntax and Android shell syntax pass. No Web/Android build/runtime.

## Source creation history

The operator rejected the current smooth, procedural-looking boars. The actual
062718.488953Z native image still shows a plain flank despite a refined dorsal
silhouette. This step creates a project-owned coarse-bristle source for that defect.

Built-in ImageGen produced the opaque1254×1254 source
`assets/art-studies/boar-bristle-albedo-v1.png` (4,135,795bytes).
Exact final prompt: `assets/art-studies/boar-bristle-albedo-v1-prompt.txt`.
No reference images or external assets were supplied. Original tool artifact:
`/home/robertvokac/.codex/generated_images/01a129bd-2c19-7192-a43c-adcfe79c0fb9/exec-ad125c01-bfe7-436f-a8bb-755f957f65f7.png`.
The source was copied unchanged into the project; hashes/provenance are recorded
in assets/manifest.json and ASSET_LICENSES.md, without an invented CC0 licence.

Both generated source and actual256-square area-reduced preview were inspected.
Visible overlapping coarse grey/brown/tan bristles have substantially more individual
strand variation than the existing grayscale multiplier. Direction is predominantly
horizontal with oblique short clumps. Some highlights/contrast and clump pattern may
still be too prominent on the animal; only a live comparison can establish suitability.
No seamless repeat claim: avoid tiling until edge behaviour has been evaluated.

Preview preparation uses the existing unmodified host tool:

```sh
python3 tools/prepare_material_texture.py assets/art-studies/boar-bristle-albedo-v1.png build/boar-bristle-preview-256.png --size 256
```

Preview154913bytes, SHA256
`7dc6d99d98c9e2c5ef9801971dee828b4de72ea3785ee2a9959b09ed454bdc1e`.
This is a preview only, not a packaged runtime texture or successful game capture.

## Integration checklist recorded at source creation (historical)

- Replace the current128-square grayscale body multiplier with required file-backed
  RGB coat content and mip levels; do not silently fall back to the rejected material.
- Avoid multiplying the brown source by the old dark brown vertex coat. Retain
  meaningful per-animal variation and shared canopy/sun lighting.
- Keep eyes, nose, hooves and other solid anatomy out of the fur texture. The old
  white-strip atlas is not present in the new image; use a deliberate material split.
- Map source direction/scale coherently along the body with interior source windows;
  inspect seams, stretching and distant filtering rather than assuming tileability.
- Update native install, Web preload, Android preparation and exact texture budgets.
- Inspect actual close and distant before/after island views; then qualify portable
  build/runtime. An attractive texture swatch is not a realistic animal by itself.

No runtime code, GPU allocation, packaged content or displayed boar changed in the
earlier source-only step. Current runtime changes are recorded above; visual work remains open.
Asset manifest validation and prepared-material regression tests PASS9.06s
(`build/boar-bristle-source-tests.log`). No native/Web/Android runtime test in this step.
## Repeatable live capture sequence

The `boars` file-capture viewpoint now advances simulation/render animation at
0.1 seconds per update without wall-time catch-up. It follows only subject
translation and writes eight poses at frames 8 through 64. Normal gameplay is
unchanged. The capture harness checks stage order, clocks, finite/ranged state,
visibility, and image presence; these checks do not establish artistic quality.

Native software run `20261012T065903.868384Z-boars` passed, with an empty kernel
incident log. Actual stages 0/2/4/7 were inspected: rooting transitions to walking,
but coarse coat streaks, angular ears and foliage-obscured rear views remain.
Native build and
43 harness tests passed; Web/Android were not rebuilt for this change.

Repeat run `20261012T070112.556340Z-boars` produced byte-identical pose JSON and
final PPM to the first run. Near ears were then refined with 16-point corner-cut
outlines (64 triangles each, versus 32), retaining eight far triangles. Outer rim
and back now use the existing fur texture; inner cups remain solid. This adds
192 vertices per near animal but no texture, draw or GPU capacity increase.
Closed-mesh, mirror, normal and pose-envelope checks pass (0.64 seconds).

Actual final `20261012T070439.874400Z-boars` passed with an empty kernel log and
unchanged pose JSON. Matched enlarged stage-zero images were inspected: outer
ears integrate better with the coat, but the body is still overly streaked and
the cheek/inner ear remain simplified. No full visual acceptance or portable
qualification is claimed.
