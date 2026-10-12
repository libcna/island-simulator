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

The global terrain grid is about2.83m across, coarser than the upper wet channel.
Provide local channel/bank geometry and a coherent authoritative carve/collision
representation; do not cover an uncut coarse heightfield with a blue strip. Avoid
z-fighting or rendering both the old terrain and a floating bed over it. Stream
water mesh must meet ocean level at the outlet and remain bounded by banks.
Source transition, downhill flow shading, wet-bank material, vegetation/prop
clearance, movement and audio still need implementation and actual image review.
The shared environment fields must later consume this same course, not a separate
approximation. No stream is visible in the game yet.
