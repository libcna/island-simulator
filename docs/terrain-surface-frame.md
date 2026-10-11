# Pixel-scale-independent terrain shading normals

Status: native build/CPU tests pass; one native image gate remains degraded.

Soil clod lighting and wet-sand sheen normalized the cross product of world-space
pixel derivatives by clamping its squared length to 1e-8. That is not a harmless
zero guard: a perfectly valid flat 2 mm/pixel footprint has cross-product magnitude
4e-6, and the old expression returns a normal of length 0.04 instead of one.
At 1 mm/pixel its length falls to 0.01. Lighting consequently depends on pixel scale,
not only surface orientation. The dry-sand pass already exposed a related bright
foreground and used a smaller floor, but still had a separate implementation.

`TerrainSurfaceFrame.hpp` now provides a CPU reference and GLSL helper. Both divide
the cross product by its largest absolute component before taking the squared norm.
This keeps that norm between one and three for every finite nonzero vector and avoids
underflow/overflow during normalization. Zero has an upward fallback; negative-y
normals flip upward to preserve the terrain's existing winding-independent convention.
The CPU reference rejects non-finite input; the shader consumes finite world derivatives.

Dry sand, soil clods and wet sand use the shared helper. Normalized relief and half-angle
vectors keep their existing guards; this fix concerns the unnormalized derivative cross
product. No mesh, material palette, texture, placement or draw pass is added or changed.
CPU and GLSL use corresponding arithmetic, not a claim of bit-identical implementations.

Regression coverage in `TerrainMaterialTests.cpp`: scales 1e-30 through 1e30,
both windings, unit length and stable orientation, zero fallback and invalid input.
The existing byte-layout/legacy baked-light checks remain unchanged.

`wet-sand` is a new fixed QA view one metre inside the western shoreline, at eye height
with downward pitch. CLI, software/Android allowlist and Web template are mapped. It is
not in an automatic Web matrix yet. The previous `water` camera remains 18 m offshore
and cannot substitute for inspecting a close wet-sand highlight.

## Verification and open image-gate discrepancy

Native build succeeds and all 33 non-display groups pass (42.70 s), including the new
scale/winding tests and unchanged legacy RGB/layout checks. Fingerprint remains
`5d0b62ad43a25895`, 17,225 grass patches; geometry and populations are unchanged.
The following contained native captures pass and were inspected:

- `20261011T094128.539261Z-wet-sand`
- `20261011T094154.489156Z-grass`
- `20261011T094225.864744Z-bay-ground`
- `20261011T094616.083800Z-water`

The grass foreground loses the former artificial brightening while retaining visible
floor texture; dry sand remains consistent. All final kernel deltas are empty and the
hardware latch is unchanged.

`20261011T094405.569362Z-litter` FAILED the existing image gate solely because luminance
standard deviation is 16.8656, below its global 18 threshold. Mean luminance 58.8703,
240 colour bins, spatial contrast 37.2447, no overwhelmingly dark/bright pixels. Image
inspection shows a complete textured shaded floor and vegetation, not a blank frame.
This is not counted as a passed capture. Its kernel delta is also empty. The global
validator is unchanged. Resolve whether its absolute-contrast requirement is appropriate
for this darker close view using independently justified image criteria/reference evidence;
do not silently relabel the failed run or change lighting solely to satisfy the threshold.

Android/Web material qualification remains pending. The completed seven-view Web suite
qualifies prior revision `9f1ee59`, not this fix. No host AMD rendering is authorized,
and the separate Android lifecycle issue remains open.
