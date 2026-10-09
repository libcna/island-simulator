# Connected split ground leaves

## Scope

Ground-plant variant 2 used detached lobe patches with wind weights differing from
the central strip. Its tip formula ended below the plant origin; the far tier
substituted unrelated fronds. This pass replaces that species with a continuous
indexed surface. Placement, populations, other species, texture/shader paths and
framework sources are unchanged.

`SplitGroundLeaf` defines five deterministic source leaves with varied angle,
length, width, lift, phase and pigment. Each has a petiole and three deep but
connected lobe pairs. A smoothstep outline, arched centre and cupped/twisted cross
section produce the blade. Root wind weight is zero; every attachment is part of
the shared mesh, so independently weighted detached patches cannot open cracks.
Existing leaf material coordinates and sampled unit normals provide daylight,
veins and tip pigment without a new shader, texture or vertex layout.

Far retains sources 0, 1, 3 and 4 with the exact same profile, phase and core knots.
Near uses 33 rows/five columns; far uses nine rows/three columns. The first native
review rejected a 17-row near mesh: endpoints and midpoints alone sample a
smoothstep at its straight-line values, leaving pointed diamond shoulders despite
the curved formula. Quarter-interval samples now represent actual rounded edges.
A regression checks those generated row positions rather than just the formula.

## Cost and limits

Near: 825 vertices / 1,280 triangles, formerly 220 / 140. Far: 108 / 128, formerly
24 / 16. Runtime reports total jungle static vertex payload **2,653,040 bytes**,
up 35,828 bytes; index storage grows by 7,512 bytes. Existing instanced batches and
draw counts are retained. Vertex processing increases; physical-device performance
has not been measured during the hardware incident stop.

The profile stays above its local plant-origin plane, not a per-vertex terrain
surface. Sloped terrain can still intersect a leaf. Far is polygonal and omits one
leaf: shared identity is not a guarantee of invisible LOD changes. Normals describe
the rest shape, not a full wind-deformed differential. Neither this pass nor its
tests establish complete vegetation realism.

## Validation

`SplitGroundLeafTests.cpp` checks 21,105 positions/normals, bounded above-origin
geometry, attached roots, closed tips, finite unit normals, independent tangent
orthogonality, deep lobes, invalid inputs, connected indexed topology, unique far
identities and exact shared LOD positions. Graph connectivity does not prove
absence of all geometric self-intersections. Runtime retains finite-attribute,
index-range and footprint validation.

Native llvmpipe ground captures (1280x720, normal player-eye camera targeting
variant 2), under `build/software-captures/`:

- `20261009T091532.019393Z-ground`: old detached patches, before this pass.
- `20261009T092808.128042Z-ground`: connected 17-row first iteration; shoulders
  still angular, prompting the quarter-sampling refinement.
- `20261009T092938.961460Z-ground`: final 33-row near mesh. Inspected matched
  view shows attached lobes, rounded shoulders and continuous curved surfaces.
  Grass spacing, nearby bushes and terrain still look stylized.

The final comparison is `build-probe/island-split-ground-comparison.png`; original
PPMs remain in the evidence directories. Wind is active, so these are matched
camera views, not frozen pixel diffs. Both post-change runs pass image statistics,
shader/inventory gates and kernel monitoring with empty live/final deltas.

Native, Chrome, Firefox and Android smoke gates now require the new geometry
diagnostic to reject stale executables. The focused Web vegetation matrix includes
the existing `ground` camera as its seventh view.

Contained Chrome/SwiftShader run
`build/browser-software/20261009T093110.666658Z-chrome-vegetation/` passes all seven
views, 60 presentation intervals each, GL error zero and no context loss. Browser
host PID 4003132 exits; live/final kernel deltas are empty. Both the post-simulation
target and dense-jungle images are inspected. The target retains connected surfaces
under active wind, though two snapshots are not exhaustive animation/LOD review.
The validated Web package is 13,877,911 raw / 6,962,710 gzip bytes. Full browser
input/lifecycle gates were not repeated for this geometry pass.

Both Android ABI assembly/lint passes complete (x86_64 3 s; restored default ARM64
4 s). Isolated API 35 KVM/SwiftShader run
`build/android-software/20261009T093932.612353Z-ground/` passes with app PID 1813 /
entry TID 1886 and first-draw total 10,536.045 ms. The inspected 1280x720 frame
retains the connected lobes and mobile overlay. Kernel deltas are empty, emulator
exit zero. APK hashes/sizes are in `build-android.md`; only rolling generated APKs
are replaced. No renewed touch/lifecycle/traversal or ARM64 runtime qualification
is implied. Software startup timings are not phone performance measurements.

Final 32 non-display CTest groups pass in 25.35 s, including the package validator.
Script syntax and diff-whitespace checks pass. No owned graphics process remains;
the final kernel interval is empty and the hardware stop-latch hash is unchanged.
Builds use existing trees, shared ccache and two workers. Historical incident
evidence is preserved; no physical GPU was qualified by this pass.

## Next review

Portable target and jungle images are inspected. Next review distant silhouette
changes during travel and near shrub construction.
Keep the unresolved AMDGPU incident separate: software images are not a driver
fix, a hardware stress qualification or permission to remove the stop latch.
