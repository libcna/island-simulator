# Western stream course — surveyed layout, not live water

The existing `westValley` centreline is `z = -55 - 0.23*x`. A read-only CPU survey
from x=-65 downstream to x=-163 reaches the sheltered western shoreline near
(-163,-17.51). The 99 one-metre-x stations span100.559m horizontally; unmodified
analytic ground decreases from9.82824m to-0.00209m with no sampled uphill step.
At the mouth the rendered surface is0.00824m, illustrating why analytic water
placement alone is insufficient.

`StreamCourse.hpp` records the route, cumulative distance, original ground,
candidate water/bed heights and smoothly widening half-width0.45..1.5m.
Upper water lies15cm below original ground, bed25cm below water. A minimum0.2%
fall is imposed where needed, and the lower reach clamps to sea level, making
the mouth a short tidal/backwater reach rather than water below the ocean.
The unmodified west-corridor profile needs at most0.4m incision; construction
rejects a profile requiring more than1m rather than silently making a canyon.
The route is deterministic and does not yet alter terrain, placement or rendering.

`island-stream-survey` validates repeated layout identity, increasing station
distance, nonincreasing finite water, positive depth, width limits and a sea-level
outlet. It also emits a raw survey CSV: its `water_candidate` column deliberately
shows the *unclamped* diagnostic candidate, not the final sea-connected layout.
Native build and `IslandStreamCourseSurvey` pass0.07s. Evidence:
`build/stream-west-profile.csv`, `build/stream-west-profile-summary.log`.

## Next integration requirements

Local geometry foundation now exists in `StreamBedPatch.hpp`: a coarse-grid-aligned
replacement rectangle with8x finer spacing (~0.354m), parabolic wet bed and2m bank
blends. Source incision fades in over2m; outlet influence fades4m past the course.
It starts from the exact existing rendered surface and never raises it. Boundary
vertices retain that surface exactly. Shared `SampleHeight` evaluates the same
triangle split as the mesh, including inclusive boundary roundoff handling.

The current full rectangular prototype has36273vertices/71680triangles and maximum
cut0.433233m. This is too costly relative to the narrow channel: adaptively retain
coarse geometry outside the cut region before live integration. CPU checks verify
all vertex heights, every triangle centroid against the query, positive/upward
winding, unchanged boundary, non-raising carve, centreline depth>=10cm after source
fade and outside-domain rejection. Focused test passes0.10s. No runtime mesh upload,
terrain removal, player collision integration, water rendering or visual acceptance
is claimed; the original terrain still renders unchanged.

### Adaptive reduction

The next version retains8x detail only in coarse cells containing changed vertices
(including both cells beside a modified shared edge). Unchanged cells use a central
fan on the original triangle diagonal. Only fan edges bordering refined cells split
into eight segments. Unreferenced grid vertices are compacted; scalar grid heights
remain for the same bounded constant-time query (36273floats/145092bytes).

Current output:8510vertices/16910triangles, down from36273/71680 (~76% reduction),
with unchanged0.433233m maximum carve. Added edge-incidence checks require each
interior edge to occur exactly twice with opposite direction; only the patch outer
boundary may have single-use edges. This catches cracks/T-junctions and overlapping
edge topology. All existing vertex/centroid/height/boundary/depth checks still pass.
Native build and focused survey pass0.10s. No renderer/physics integration or runtime
performance claim yet. Evidence: `build/stream-adaptive-{build,tests,summary}.log`.

The global terrain grid is about2.83m across, coarser than the upper wet channel.
Provide local channel/bank geometry and a coherent authoritative carve/collision
representation; do not cover an uncut coarse heightfield with a blue strip. Avoid
z-fighting or rendering both the old terrain and a floating bed over it. Stream
water mesh must meet ocean level at the outlet and remain bounded by banks.
Source transition, downhill flow shading, wet-bank material, vegetation/prop
clearance, movement and audio still need implementation and actual image review.
The shared environment fields must later consume this same course, not a separate
approximation. No stream is visible in the game yet.
