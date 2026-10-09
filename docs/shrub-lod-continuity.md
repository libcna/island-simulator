# Shrub outline continuity and walking review

Date: 2026-10-09. This extends [shrub construction](shrub-structure.md), not the
unresolved AMDGPU diagnosis. All new rendering checks use software implementations.

## Measured traversal before the geometry correction

Opt-in input telemetry now records the previous draw's actual submitted shrub
identities and near/far tiers, alongside grass populations and canopy identities.
IDs use millimetre X/Z placement coordinates. The reader rejects duplicate IDs,
malformed data, stale telemetry and disagreement with near/far counts. Shared IDs
changing tiers between adjacent samples are switches; disappearing/reappearing
plants alone are not. CPU fixtures cover both cases, missing data and invalid IDs.

The existing east-clearing route uses actual captured mouse/input ownership and
W/S movement, with seven measured stops on an 18 m outward/return walk. The
contained SwiftShader run `20261009T102134.869003Z-chrome-grass-motion` records:

- 18.3634 m outward, 1.0033 m return error, 94 telemetry samples;
- 47 grass population states, 10 canopy and 29 shrub identities changing tier;
- 97 distinct compositor frames without recording truncation;
- live WebGL context, zero GL error, empty live/final kernel deltas; host PID 4029491.

Evidence is under `build/browser-software/`. Step 3 and frames 9/11/12 around the
`-8881:-18933` switch are inspected; the latter form
`build-probe/island-shrub-walk-switch.png`. The shrub is small/occluded among grass.
These images **do not isolate a visible pop**. Many submitted shrubs are behind or
occluded from the camera because culling is cell-based. Telemetry-to-compositor
association is approximate; the bounded recording is not every rendered frame.
Do not present 29 submitted switches as 29 visible transitions or a smoothness score.

## Geometric finding and correction

The former far leaf had three longitudinal rows: root, widest point, tip. Its
diamond planform retained only 76.3106% of the near oval's projected area. This
quantity follows from the actual outline profile and does not depend on interpreting
the walking screenshot. It is an individual blade's local-plane area, **not** the
occluded shrub's on-screen coverage or a claimed 24% screen-space improvement.

Both tiers now retain all seven longitudinal outline rows, including identical
positions, profile normals, wind phase/weight and endpoints. Near retains the
central rib (three transverse columns); far joins the two edge columns directly.
Closed endpoints still have only one vertex. All source leaves, supporting shoots,
placement, collision footprint, culling distances, shaders, instances and batches
remain unchanged. There is no alpha blend, extra pass or per-frame mesh generation.

Tests sum the actual generated triangles projected onto each of the 162 leaves'
local planes, requiring near/far area agreement within 0.001%. They verify that
every near boundary coordinate occurs in far and samples the same geometry/motion,
along with existing 37,422 finite/bounded samples, nondegenerate triangles, index
ranges, attachment and invalid-input checks. This does not prove identical images:
the missing interior cup/rib changes interpolation, and shoot radial detail still
differs. Wind normals are rest-shape normals, not a full deformed-surface solution.

## Geometry cost

Near leaves remain 17 vertices / 20 triangles. Far changes from 5/4 to **12/10**.
Its triangle cost is half the near leaf's, but 2.5 times the former far leaf's.

| Variant | Near vertices / triangles | Previous far | Current far |
|---|---:|---:|---:|
| 0 | 915 / 1,150 | 315 / 330 | 630 / 600 |
| 1 | 1,098 / 1,380 | 378 / 396 | 756 / 720 |
| 2 | 1,281 / 1,610 | 441 / 462 | 882 / 840 |

Shared jungle vertex payload increases by 58,968 bytes to **2,893,748 bytes**;
indices increase by 5,832 bytes. Each submitted far shrub adds 270–378 triangles,
depending on variant. This is a quality/cost tradeoff, not a free optimization.
Physical GPU timing and sustained mobile performance remain unqualified.

## Validation

Incremental native build and all 32 non-display CTest groups pass in 20.42 s.
Contained llvmpipe capture `20261009T102650.266107Z-bush` passes shader/image gates,
and its actual mesh inventory matches the table. The 1280x720 image is inspected
at `build-probe/island-shrub-outline.png`; live/final kernel deltas are empty.
This close target uses near detail, so it checks scene integrity, not an isolated
far/near comparison. The historical baseline and all original incident evidence
remain intact.

Final contained Chrome/SwiftShader traversal
`20261009T102949.809142Z-chrome-grass-motion` passes: 18.1906 m outward,
0.2444 m return error, 106 samples, 50 grass populations, nine canopy and 28 shrub
identities changing tier (61 shrub transitions, including return travel). All 108
recorded frames are distinct without truncation. GL error is zero, context is live,
live/final kernel deltas are empty, and browser host PID 4033661 exits normally.
Frames 0/20/40/60/80/107 are inspected together in
`build-probe/island-shrub-outline-walk.png`. This is a sampled whole-scene review,
not an isolated screen-space before/after metric; movement and wind differ from
the earlier run. Full browser input/lifecycle and eight-view gates were not repeated.
The refreshed Web package validates at 13,895,553 raw / 6,968,513 gzip bytes.

Android x86_64 and restored ARM64 assembly/lint pass (3 s / 4 s). Contained
API 35 KVM/SwiftShader `20261009T103304.055335Z-bush` passes with app PID 1845 /
entry TID 1906, first-draw total 10,688.124 ms. The 1280x720 image is inspected;
live/final kernel deltas are empty and the owned emulator exits zero. Current APK
hashes are in `build-android.md`; only rolling generated packages were replaced.
This fixed view does not repeat touch/lifecycle/traversal gates, and ARM64 remains
build/lint-only evidence. All builds use the existing trees, shared ccache and two
workers. CNA/Sharp Runtime and original incident evidence remain unchanged.

Final 32 non-display groups pass in 20.86 s, including refreshed package validation
and the absent/reappearing-shrub parser regression. Script syntax and diff checks
pass. Host checks find no owned graphics survivors; the final kernel query since
12:16 CEST is empty and the hardware stop-latch hash is unchanged. 359 GiB remains
free (browser evidence 199 MiB, native captures 573 MiB, Android software 1.7 GiB).

Remaining work: targeted unobstructed near/far image pairs, other travel headings,
ground-layer LOD, procedural repetition and sparse floor cover. Do not claim
seamless all-angle LOD or AMD hardware safety from this pass.
