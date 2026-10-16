# Shrub structure and individual foliage

## Design

The subsequent [outline-continuity pass](shrub-lod-continuity.md) replaces the initial
far diamond below with the complete near outline. The inventory and validation in
this document describe the original construction pass; current costs are in that follow-up.

The previous three bush meshes used four to six solid, faceted cluster volumes
and two oversized pointed leaves per cluster. Their far tier became two plain
volumes. The targeted baseline shows why these read as floating green stones
rather than foliage. This pass changes only bush meshes: all 578 placements,
their scales, reserved walking footprints, culling distances and instanced draws
remain unchanged.

`BushProfile` defines five, six or seven curved woody shoots per variant, with
four pairs of leaves and a terminal leaf on each. Length, width, pitch, phase,
pigment and shoot dimensions vary deterministically. Variant shapes range from
broader oval blades to narrower leaves. Profiles feed the existing jungle leaf
and wood material; there are no new textures, shaders or renderer abstractions.

Near and far retain **every shoot and leaf identity**. Near samples seven
longitudinal rows; far three. Closed endpoints use one vertex each, giving
17/5 vertices and 20/4 nondegenerate triangles per near/far blade. This removes
four degenerate triangles and four redundant vertices per leaf per tier from
the initial grid implementation. Shoots keep all attachment knots, with five
near or three far radial sides. A fixed horizontal reference avoids abrupt
cross-section rotation as the shoot tangent changes.

Leaf roots equal their shoot-centre attachment and share its wind phase/weight.
Rooted shoots have zero base wind. Branch radial vertices are close to, but not
identical to, the leaf's central attachment; the existing position-dependent wind
field is not a skeletal deformation. Thin intersections remain preferable to
disconnected floating leaves, but this is not a proof of watertight botanical joints.

## Camera and tests

`--viewpoint bush` selects variant 0 at player eye height, 3.8 m away. The first
camera-only attempt, `20261009T094603.018474Z-bush`, failed because requiring an
empty 3.8 m circle near the primary path left no eligible target. The corrected
selector tests eight real sightlines against understory and collision obstacles,
without moving or hiding scene objects. It reports its target and fails explicitly
if none is available. It does not guarantee that every surrounding plant is outside
the frame. A command-line regression parses the new mode before a missing-path
error; the invalid-invocation gate now has 26 cases.

`BushProfileTests.cpp` samples 37,422 points/normals across every leaf, checking
finite bounds including wind margin, rooted shoots, positive leaf height, attached
roots, matching wind, unit/tangent-orthogonal normals, shared LOD positions and
invalid inputs. Both tiers' actual coordinate/index tables are tested for bounds,
budgets and nonzero triangle area. Runtime retains generated-mesh attribute/index
checks, wood-normal validation and the existing 2.1 m placement-footprint guard.

Matched native llvmpipe evidence under `build/software-captures/`:

- `20261009T094855.171292Z-bush`: old clusters with the corrected targeted camera.
- `20261009T095133.346097Z-bush`: initial individual-leaf mesh. Inspected comparison
  clearly separates the shrub's leaves and supporting shoots from the unchanged
  neighboring juvenile broadleaf. This still contains redundant endpoint triangles;
  it is retained as an intermediate, not the final mesh.
- `20261009T095617.826626Z-bush`: final compact mesh and stable shoot frames.
  Matched comparison is inspected at `build-probe/island-bush-comparison.png`;
  original PPMs remain in their evidence directories. Wind is active, not frozen.

All successful captures have empty kernel deltas and clean graphics/material
gates. The failed camera attempt also has an empty final kernel delta. Native
build and all 32 non-display test groups pass in 20.41 s. This includes CLI and
software-supervisor tests, not hardware qualification.

## Cost and remaining limits

The mesh identity is static and shared by instances. Removing solid volumes does
not add transparent overdraw or per-leaf draw calls, but individual foliage has
more submitted triangles than the old cheap blobs. Final runtime inventory:

| Variant | Near vertices / triangles | Far vertices / triangles | Source leaves |
|---|---:|---:|---:|
| 0 | 915 / 1,150 | 315 / 330 | 45 |
| 1 | 1,098 / 1,380 | 378 / 396 | 54 |
| 2 | 1,281 / 1,610 | 441 / 462 | 63 |

The old near models had 220/320, 275/400 and 330/480; all far models 36/48.
Total jungle static vertex payload is 2,834,780 bytes, +181,740 bytes. Index
storage grows by 23,904 bytes (calculated from the triangle inventories). The
targeted view still submits 61 bushes, 19 near and 42 far, with unchanged batching;
its triangle workload is higher. The final closed-end topology saves 67,392 static
vertex bytes compared with this pass's first individual-leaf implementation.
Physical GPU timing is not inferred from these memory totals or a successful
software capture.

## Portable software validation

Contained Chrome/SwiftShader eight-view matrix
`build/browser-software/20261009T095827.308359Z-chrome-vegetation/` passes, including
the new `bush` camera. All scenes report 60 presentation intervals, zero GL error
and no context loss; live/final kernel deltas are empty. Post-simulation bush and
dense-jungle frames are inspected. Browser host PID 4015739 exits normally. This
does not renew the full input/lifecycle matrix or qualify moving LOD transitions.
The validated Web package is 13,893,639 raw / 6,968,980 gzip bytes.

Both Android ABI assembly/lint passes complete (x86_64 4 s; restored default
ARM64 5 s). Isolated API 35 KVM/SwiftShader run
`build/android-software/20261009T100800.243131Z-bush/` passes with app PID 1865 /
entry TID 1968, first-draw total 10,910.550 ms. The 1280x720 target frame is
inspected; live/final kernel deltas are empty and the owned emulator exits zero.
APK sizes/hashes are in `build-android.md`. No renewed touch/lifecycle/traversal
or physical-device performance claim; ARM64 is build/lint-only evidence.

Final 32 non-display CTest groups pass in 20.30 s. Script syntax and diff-whitespace
checks pass; no owned graphics process survives. Final kernel interval is empty,
and the hardware stop latch is unchanged. All builds reused existing trees, the
shared ccache and two workers. No CNA/Sharp Runtime source or incident evidence
was changed by this geometry pass.

## 2026-10-15 independent leaf-roll follow-up

A new eye-height audit found that the oval outlines and crooked supporting shoots were readable,
but every leaf's transverse axis still used the world-horizontal frame. Overlapping pairs therefore
formed flat shelves. Each existing leaf now has a deterministic bounded roll around its own
longitudinal direction. Paired leaves tilt in opposite directions with smaller shoot-specific
variation; terminal leaves use a restrained independent roll. The existing cupping then follows
that rolled frame. Near and far tiers call the same profile, so positions, normals, wind attachment,
source identity and outline still match exactly. Counts, buffers, instances, culling, textures and
draws do not change.

The profile test now requires meaningful positive and negative roll, projects actual triangles
onto the rolled leaf plane, verifies nondegenerate equal-coverage LODs and rejects invalid roll.
`IslandLeafBladeTests` and `IslandCoreTests` pass. Exact-final native llvmpipe evidence is
`build-consumer/software-captures/20261015T094530.825739Z-bush`: 252 colour bins, 42.88
luminance deviation and 188.09 spatial contrast. The full image and a target crop were inspected
against `20261015T094013.615051Z-bush`; the crown has clearer volume without a larger footprint.

Chrome/SwiftShader evidence is
`build/browser-software/20261015T094944.252248Z-chrome-bushes`: 60 finite positive intervals,
GL0, live context, exact current diagnostic and clean kernel supervision. Its full-resolution image
was inspected. The Web package is 11,431,979 bytes of Wasm, 2,230,701 bytes of data and 280,439
bytes of JavaScript. ARM64 `assembleDebug` and `lintDebug` pass in four seconds with two workers
and no emulator/KVM. The APK is 24,456,059 bytes with SHA-256
`74ac87fcb53b03303134fe0f8ef74e4cd53bd881f942b5eb8beb3f2765454a9c`; stripped `libmain.so`
is 19,413,488 bytes. Wind-deformed normals, repeated canopy crowns and physical-device appearance
remain open.

## Deterministic paired-leaf gaps — 2026-10-16

The renewed eye-height audit accepted the crooked supports, cupped/rolled leaves and pigment range
but selected the repeated paired tiers: every shoot still carried every lateral position and the
target read as three continuous horizontal rosettes. Each shoot now omits one deterministic
lateral leaf at a differently rotated tier while always retaining its terminal crown leaf. The
same presence predicate builds near and far meshes, so retained leaf identity, root, normal and
wind phase remain exact across LOD.

The three variants retain 40/48/56 leaves instead of 45/54/63. Their near inventories fall from
915/1,098/1,281 vertices and 1,150/1,380/1,610 triangles to 830/996/1,162 vertices and
1,050/1,260/1,470 triangles. Far inventories similarly fall to 570/684/798 vertices and
550/660/770 triangles. The 569 placements, cells, buffers, textures, shader, wind, culling and draw
counts do not increase.

Inspected native baseline `20261016T081540.234939Z-bush` and accepted
`20261016T082546.886647Z-bush` show open gaps between the retained asymmetric tiers and more visible
supporting shoots without making the shrub sparse. The accepted frame passes at 257 colour bins,
43.45 luminance deviation and 195.71 spatial contrast with an empty kernel delta. Focused
Chrome/SwiftShader `20261016T082857.948662Z-chrome-bushes` passes 60 finite positive intervals,
`glError=0`, a live context and kernel `[]`. Five focused native groups and all 96 browser/capture
harness tests pass.

## Remaining visual work

The later outline-continuity pass gives far leaves the complete near outline and equal projected
coverage, but no invisible moving LOD transition is claimed. Rest-shape normals do not follow full
wind deformation. Physical GPU cost, sustained phone performance and the unresolved AMDGPU failure
remain separate qualification requirements.
