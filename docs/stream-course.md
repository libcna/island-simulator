# Western stream course — live bed and first water pass

## Live water integration — 2026-10-12

`StreamWaterMesh` follows the surveyed water elevation, inside 80% of the carved
channel half-width, starting after the source incision blend. Its 3429 vertices
and 6080 triangles use one static 16-bit indexed buffer pair and one frustum-culled
transparent draw. Edge/source/outlet opacity tapers avoid hard ribbon boundaries.
The desktop GLSL330/GLES300 shader animates downstream ripple normals and restrained
Fresnel/sun highlights without moving the mesh through the bed. It converts XNA
clip depth to OpenGL depth, matching the existing terrain/ocean shaders.

Native build passed (CPU affinity0–3, -j2, shared ccache). StreamSurvey/CoreTests
passed61.32s before the final shader-only correction; the survey checks upward
winding and sampled triangle interiors against the actual bed. Actual isolated
llvmpipe capture `20261012T081415.552173Z-stream` passed348colour bins/std23.82/
spatial114.83 with empty kernel-after log. The image
`build/stream-water-depth-fixed.png` was viewed: the lower water course now visibly
reaches the bay, but reads as a soft straight green strip, not a finished natural
stream. Wet substrate/banks, placement clearance, less uniform edge treatment,
lighting and actual motion-sequence review remain. No Web/Android/performance or
art acceptance is claimed. Earlier captures exposed the missing clip-depth
conversion; a closer forest capture failed its image contrast gate and is retained.
Final incremental build and survey also pass0.14s after adding finite-value,
alpha-range, bed-domain and portable-index-capacity guards.

The sections below are chronological implementation evidence, not current-state
claims that water or terrain integration is still absent.

### Shared wet corridor pass

The first environment-field migration adds a 10–25m damp influence around the
stream while keeping concentrated drainage narrow. Grass and solid vegetation no
longer occupy flowing water; surrounding jungle layers and fallen wood respond to
moisture. Terrain uses damp earth, moss and sparse stone cues. Water centre,
width and opacity now vary deterministically while remaining inside the carved
bed. Focused CPU tests pass43.07s. Actual isolated llvmpipe capture
`20261012T084525.578398Z-stream` passed388colour bins/std50.99/spatial239.36 with
an empty kernel-after log. `build/stream-wet-biome-final2.png` was inspected: the
open channel is readable to the bay, though its form and banks remain visibly
procedural and require dedicated fern/deadwood composition and further shaping.

## Live bed integration — 2026-10-12

TerrainRenderer now excludes all original coarse quads whose centres lie in the
grid-aligned patch rectangle and draws the replacement with the existing terrain
material. Separate16-bit buffers avoid exceeding the global terrain vertex index
limit; one additional frustum-culled draw is used when the patch is visible.
Patch vertices use their new height, slope and daylight. Its CPU surface is owned
by TerrainRenderer; PlayerMotion borrows it after renderer installation and uses
the same sampled heights for walking footprint support and uphill checks.
The renderer must outlive controller queries; no destruction-time query occurs.

Native build and StreamSurvey/CoreTests pass45.58s, including player footprint
placement along the actual stream bed. Camera-only incremental build also passed.
Actual isolated llvmpipe capture `20261012T075212.867607Z-stream` passed350colour
bins/std41.25/spatial210.68, kernel log empty. The actual image
`build/stream-bed-live.png` was inspected: subtle dry cut, still obscured by existing
plants/tree. Not a finished stream. Water, wet materials, vegetation/prop clearance,
wildlife grounding, contact shadows, packed terrain-height consumers and underwater
surface queries still need the shared patch. No full-suite/portable qualification.

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
