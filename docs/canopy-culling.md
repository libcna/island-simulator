# Conservative per-tree canopy visibility

## Implementation

`JungleRenderer` retains its cell and distance rejection, then tests a world-space bounding box
for each remaining canopy tree using CNA's existing `BoundingFrustum::Contains`. It rejects only
`Disjoint`, keeping intersecting crowns even if their trunks are outside the view. All 281 tree
placements, meshes, LOD thresholds, materials, collision proxies and the full-world light bake
remain unchanged. This is not occlusion culling and is not a GPU incident fix.

`CanopyInstanceBounds` uses the maximum horizontal radius and Y envelope of both mesh tiers.
The measured local mesh envelope already includes 0.16 m horizontal and 0.01 m vertical wind
margins. A yaw-invariant cylinder bounds the uniformly scaled instance, and its world AABB gets
a 0.01 m rounding cushion. The containing cell is extended by this exact box. Only canopy
instances gain a stored box, not every low plant. Invalid or overflowing inputs throw.

`JungleWind` shares all four displacement amplitudes between CPU bounds tests and generated
GLSL 330 / GLSL ES 300 constants. Static assertions require their full sine envelope to fit
the margins. Generated mesh upload checks finite positions/wind and weights in [0,1]. Wind
behavior is unchanged, including shader literal rounding. The unanimated stock-effect fallback
is also contained. New parameters do not weaken any GL/kernel error checks.

## CPU evidence

`IslandCanopyVisibilityTests` links CNA math but never creates a window, device or GL context.
It checks 172,800 synthetic cylinder vertices under varied yaw, scale, wind extremes and camera
orientation. All 53,316 clip-visible samples have retained boxes; 169 definitely disjoint boxes
are rejected. An independent homogeneous point test uses the shader's pre-GL 0..w depth range.
Additional checks exercise an offscreen trunk with an intersecting crown, near/eye-plane
crossing, bad input/overflow and locale-independent, exactly round-trippable shader constants.
This supplements the analytic envelope and validation of actual generated vertices; it is not
exhaustive GPU readback over every possible camera pose.

## Frozen-image evidence

Before rebuilding, preserve the reference executable identity:

- Game SHA-256: `93693c5c1d10793899c50cae6af9b6474e3596a3371058468813f014cc2e8aae`
- CNA shared runtime SHA-256: `a96dcaf304021c980ae8772adbf61fab263f5382246ec07ec776f8098100405a`

The following directories are under `build/software-captures/`. These use the explicitly labeled
single-tree frozen inspection mode, forced llvmpipe and private pixman, serially with live kernel
monitoring. Each before/after pair passes `cmp`; there is no pixel tolerance. All live/final kernel
deltas are empty and there are no GL errors or material fallbacks.

| Fixed view / target / tier | Before | After | Submitted canopy before -> after |
|---|---|---|---|
| East / `-87120:-15369` / near | `20261009T010205.339228Z-east` | `20261009T011533.085211Z-east` | 183 -> 116 (76/107 -> 42/74 near/far) |
| East / same / far | `20261009T010209.311080Z-east` | `20261009T011537.054830Z-east` | 183 -> 116 (75/108 -> 41/75) |
| Grass / `-18000:-45227` / near | `20261009T011318.989090Z-grass` | `20261009T011540.809493Z-grass` | 275 -> 236 (61/214 -> 42/194) |
| Jungle / `99368:83948` / far | `20261009T011322.954576Z-jungle` | `20261009T011544.775233Z-jungle` | 109 -> 65 (62/47 -> 37/28) |

The SHA-256 of each matching PPM pair is respectively:

- East near: `0524a491f32899c80fa20afe47d4082bff9a8fccc467622985fb9050b5c0f798`
- East far: `ec9d3945a4e1b62a8b426398e38d6a8c4cff4aada5c7e34e9a7c5b910035ff83`
- Grass: `567a27dfcd6f8d60aae69d2026400ee17adda42c7b0d31a46d51bfb093d3988c`
- Jungle: `2410d22ee7b4a39e5b90b2004f473291365b7b83982b78e1f4773eec58bc3756`

Near-east/grass/jungle report 67/39/44 additional frustum rejections. Other plant populations and
25/28/28 jungle draws are unchanged. Using the cheaper near variant's 4,328 triangles as a lower
bound, these views avoid at least 178,568/101,272/126,288 submitted triangles respectively.
These counts are not measured hardware frame-time savings.

## Normal animation and traversal

Normal native jungle capture `20261009T011901.058728Z-jungle` passes with zero GL/fallback errors
and an empty kernel delta. Serial SwiftShader grass/jungle/fronds/east (Chrome PID 3821108)
passes 60 presentation intervals per view, with nine empty kernel checks and no context loss.
The normal views submit 236/65/226/116 trees, rejecting 39/44/44/67 after cell/range checks.

The software real-input route (Chrome PID 3821826) covers 17.9334 m outbound and returns within
0.645 m, passing unchanged movement/pointer-lock/return gates. It records 91 samples, 43 grass
LOD populations and 18 observed tier switches across nine submitted trees. The capped recording
contains 99 distinct frames/2,681,082 bytes over 26.700 s without truncation/error; all frame
hashes are verified. Ten empty kernel checks include browser exit, and GL/context/fallback
checks pass. The farthest full-resolution stop and frames 003–008 (inspection sheet
`build-probe/island-canopy-culling-motion-review-3821826.png`) show no gross disappearing crowns
in that portion. Compositor/telemetry association is approximate; not every frame or tree was
visually inspected. Earlier reports and stationary frames are preserved as `.before-per-tree-frustum`.

All 26 non-display test groups pass. Native/Web builds and Android arm64 assembly/lint pass;
no emulator or device was launched. Exact frozen comparisons cover these cameras and shader
time one, not all possible edge trajectories. Physical GPU performance and Android rendering
remain unqualified while the AMD incident latch is active. No latch or dependency source changed.
