# Juvenile ground broadleaf

## Scope and implementation

Ground-plant variant 1 previously used six upright leaves with only four triangles
per lamina. It had no explicit leaf-material tag, and its far tier became four
unrelated fronds. This pass changes **only that ground species**: neither the
larger understory plants nor bushes nor split/litter ground variants are redesigned.

`GroundBroadleaf` defines six deterministic leaf identities with unequal dimensions,
small azimuth variation, pigment and phase. Far retains sources 0, 1, 3 and 4 exactly,
including their petioles. Near samples seven rows/five columns; far samples three
rows/three columns of the same tapered, cupped lamina. The leaf starts exactly at
the existing petiole endpoint. Their shared wind weights agree at that attachment.
Analytic-profile normals and normalized material coordinates enable the existing
filtered vein, age and daylight response instead of the untagged flat fallback.

No new shader, texture, vertex format, per-frame buffer, plant placement, draw pass
or framework change is needed. The BasicEffect fallback gets the geometry but not
the custom leaf lighting/material. Far geometry is still coarse, and omitting two
leaves is not an invisible LOD morph. Other ground species still have mismatched
near/far identity and angular silhouettes; this pass must not be described as fixing
all vegetation.

The revised near mesh has 234 vertices / 300 triangles (previously 54 / 36), far
52 / 40 (previously 24 / 16). Total jungle static vertex payload is 2,617,212 bytes,
up 10,816 bytes. Index storage and per-frame vertex processing are separate costs.
The extra geometry follows the existing species/LOD batches, not one draw per leaf.
Physical-device cost remains unqualified.

## Tests and visual review

`GroundBroadleafTests.cpp` samples 12,726 positions/normals across all six source
leaves: bounded finite geometry, exact petiole attachment, closed taper endpoints,
unit normals, independent tangent-orthogonality checks, cupped cross sections and
invalid-input rejection. Far source identities are exact and unique. Runtime mesh
validation checks finite attributes, index bounds and the existing placement footprint.

All 32 non-display groups pass after the new camera is integrated (23.13 s).
The new command-line negative regression initially expected the wrong wording for
a missing capture path. It was corrected to the existing application diagnostic;
the application error behavior did not change. CTest now expects 25 invalid
invocations, including parsing `ground-broadleaf` before rejecting the missing path.

Native contained llvmpipe captures under `build/software-captures/`:

- `20261009T085035.836020Z-ground`: pre-change general ground view. It primarily
  targets split-leaf variant 2, **not** the revised broadleaf.
- `20261009T085336.254646Z-grass`: revised general scene, clean render and shader
  diagnostics. A comparison crop at (0,120), 410x280, selected a neighboring bush;
  `build-probe/island-ground-broadleaf-comparison.png` is **not evidence** of this
  species' improvement. Retain this note to avoid misinterpreting the image.
- `20261009T085644.169508Z-ground-broadleaf`: dedicated new camera selects variant 1
  at normal player eye height, without hiding other plants or changing the scene.
  Its inspected 1280x720 frame shows the curved blades, fine material veins and
  attached stems. It is a current-state review, not a matched before/after image
  or a proof of perceptual realism. Grass and coarse neighboring bushes remain
  visibly stylized.

The dedicated view is available as `--viewpoint ground-broadleaf` and in the browser
smoke shell. Native/Web checks require its actual target-variant diagnostic, not
merely a generic content-loaded marker. It is added to the focused Web vegetation
matrix. No hardware GPU run is authorized by these examples; use the monitored
software launchers during the open AMDGPU incident.

## Web software validation

The contained SwiftShader six-view vegetation matrix
`build/browser-software/20261009T090051.635173Z-chrome-vegetation/` passes grass,
jungle, fronds, east clearing, clearing edge and ground broadleaf, each with 60
presentation intervals. All report zero GL error and no context loss; live/final
kernel deltas are empty. The ground-broadleaf image after simulation is inspected
and retains the curved targeted plant. Wind is active; these images are not a
frozen pixel comparison. This matrix does not renew the full input/lifecycle suite
or measure physical-device frame times. Package: 13,883,396 raw / 6,964,284 gzip bytes.

## Android software validation

Both ABI assembly/lint runs pass (5 s each), with default ARM64-only packaging
restored. The isolated API 35 KVM/SwiftShader view
`build/android-software/20261009T090756.767342Z-ground-broadleaf/` passes, app PID
1773 / entry TID 1880, first-draw total 14,121.283 ms. The inspected 1280x720 frame
shows the same targeted plant and touch overlay. Kernel deltas are empty; the
owned emulator exits zero. Touch/lifecycle tests were not repeated, and ARM64
runtime is unverified. Current APK identities are in `build-android.md`.

The final native dense-jungle capture `20261009T091041.337804Z-jungle` also passes
and is inspected, with empty kernel deltas. All 32 non-display groups pass again
in 20.34 s. No owned graphics processes remain and the hardware stop latch is
unchanged. These are software scene checks, not a fix or qualification of the
unresolved AMDGPU incident. Builds use the existing trees, shared ccache and two workers.

## Next work

Inspect the far representation during real camera travel; source identity alone is
not a screen-space coverage measurement. Improve split ground leaves and near bushes,
which remain prominent angular forms, then review overall density/composition. Preserve
the same plant-specific QA discipline when choosing comparison crops.
