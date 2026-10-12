# Boar coat material — live integration, visual refinement open

## Runtime integration

### Shorter nasal region and short muzzle fur

The shared `BoarMuzzleZ` now compresses only the region in front of z0.78 by0.70,
moving the tip from1.060 to0.976m. Cheeks/ear locations and all limb/foot anchors
remain unchanged. Body rings, fitted mouth, outer eye rim, nose disc, nostrils and
tusks use the same mapping; nose/nostril/tusk depth dimensions scale consistently.
Rooting coefficients and vertical positions are unchanged.

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
