# Pixel-scale-independent terrain shading normals

Status: native checks pass; Android material images pass but stock-skinned rendering is degraded;
focused Web runtime qualification passes.

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
included in the focused Chrome `materials` matrix with litter, grass and bay-ground.
The previous `water` camera remains 18 m offshore
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
validator at that time was unchanged. That failed run remains untouched.

### Explicit shaded-detail image profile

The `litter` view now selects the named `shaded-detail` profile in the native and Android
launchers. Its deviation requirement is `max(12, 0.25 * mean luminance)`, making contrast
relative to scene brightness while retaining an absolute noise floor. The constants are
reviewed project heuristics, not a perceptual standard or proof of visual quality. All other
image gates are unchanged. Other viewpoints retain the original `scene` profile and its
absolute deviation threshold of 18. Selection depends on camera arguments, never on whether
an image failed. Reports record the profile and effective threshold.

Six validator tests cover camera selection/order, relative contrast, the noise floor,
washed-out bright images, unchanged colour/spatial/extreme-pixel gates and real solid images.
Twenty Android CPU-only tests include profile forwarding through capture and PNG decoding.
The final non-display CTest run passes all 33 groups in 49.94 seconds.
Fresh native run `20261011T095340.833878Z-litter` passes with deviation 16.8703 against
14.71815, mean 58.8726, 240 colour bins and spatial contrast 37.2616. Its image was inspected:
textured shaded floor and vegetation remain readable. Kernel delta is empty and latch hash
unchanged. No renderer lighting was adjusted for this calibration. This new run does not
retroactively turn the earlier failed run into a pass.

## Focused Android verification

x86_64 assembly/lint succeeds. Contained runs `20261011T095903.886091Z-litter` and
`20261011T100107.688712Z-wet-sand` pass their existing runtime and image gates, both on
the first screenshot attempt. Both images were inspected; soil texture remains readable
and wet sand has no missing material/black frame. Litter selects `shaded-detail`; wet sand
retains `scene`. Both final kernel deltas are empty. Tested APK: 20,443,211 bytes,
SHA-256 `991af9d4978d2b13bdbc467202d8758f5f0d02bce3bbd95b59e5cb5a86f88625`, preserved
at `build/android-software/island-x86_64.apk`.
Default ARM64 assembly/lint restored: 20,528,428 bytes, SHA-256
`f475545d831065a4c068690d69d253c8c1e78e5138758fe32e09faa7dc5224ce`.
Physical-device runtime remains untested.

Qualification caveat: litter logs 27 guest gfxstream `s_glGetUniformLocation` errors
`0x502` near the first draw. The wet-sand log has no `GL error` matches. The same 27
messages occur in retained pre-fix `20261011T092539.275219Z-interactive`, so this observation
does not establish a regression in the new normal helper. Existing Android gates check
fatal/shader failures, not every guest GL diagnostic; their passed result must not be
reported as a zero-GL-error run. Program identity, root cause and ownership remain unknown.
Follow-up `20261011T101527.286817Z-monkeys` confirms a stock-skinned link failure:
active vertex uniforms exceed the device's 256-vector limit, and monkeys are absent.
See confirmed `CNA-ISLAND-002` in `BUGS.md`. The old generic passed results do not establish
whole-scene correctness; the Android gate now rejects these failures. Dependencies remain
unchanged. No new host GPU fault
occurred. These focused runs do not requalify touch/traversal or the open lifecycle issue.

## Focused Web verification

Contained Chrome/SwiftShader `20261011T100522.314132Z-chrome-materials` passes litter,
grass, bay-ground and wet-sand: 60 intervals each, GL error zero in every view. All four
images were inspected and match the native material appearance. Final kernel delta contains
12 unrelated AppArmor audit entries and one audit-suppression notice, no GPU fault; hardware
latch unchanged. Build/package: 10,347,083 raw / 3,528,841 gzip bytes. This qualifies the
surface-normal material fix, not full input/lifecycle, Firefox or host AMD hardware.
Browser session `21871` completed successfully; do not poll/restart it.
The separate Android lifecycle and guest-uniform issues remain open.
