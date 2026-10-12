# Boar coat material — generated source, not live yet

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

## Required integration

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

No runtime code, GPU allocation, packaged content or displayed boar changed in this
source-only step. The full visual task remains open.
Asset manifest validation and prepared-material regression tests PASS9.06s
(`build/boar-bristle-source-tests.log`). No native/Web/Android runtime test in this step.
