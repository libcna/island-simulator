# Western stream course — live bed and first water pass

## Independently eroded left and right banks — 2026-10-16

The current ordinary-height environment comparison retained the wet-bank habitat, physical
cross-section and animated flow, but selected the lower course's paired edges: centreline bends
and shared pools/pinches still left every local cross-section mirrored. `StreamRoute` now exposes
one deterministic bank-scale function with different incommensurate rhythms for the physical left
and right sides. The effect fades completely at the source and mouth and remains within about
±21%. `StreamBedPatch` measures signed distance along the local segment normal and applies the
appropriate bank scale; `StreamWaterMesh` applies the same scale to its corresponding side. The
result creates alternating dry ledges and locally fuller banks without separating water from the
collision-authoritative bed.

No water vertex, index, texture, attribute or draw was added: water remains 3,429 vertices / 6,080
triangles in one transparent draw. The changed refinement footprint compacts the existing bed from
7,580 vertices / 15,050 triangles to 7,525 / 14,940. `IslandStreamCourseSurvey` checks every water
triangle against the actual bed, reports 14.4106 cm minimum visible depth and a bounded 0.685523 m
deepest cut. Inspected four-phase native evidence `20261016T101848.725497Z-stream` passes at
1280x720 with 373 colour bins, 53.92 luminance deviation, 251.35 spatial contrast and an empty
kernel delta. Core, rendered-terrain, terrain-material and all 97 browser/capture harness tests
pass. Inspected Chrome/SwiftShader evidence `20261016T102312.904694Z-chrome-stream` passes two
different-time frames, 60 finite positive intervals, GL error zero, a live context and kernel
`[]`. Physical Android appearance, longer free traversal and subjective stream-audio review remain
open.

## Depth-graded water and broken current threads — 2026-10-14

The environment audit found that the complete stream still read as a muted grey-green strip from
ordinary eye height. The geometry, incised banks and exposed bed were already sound, so this pass
changes only the existing portable water fragment shader. Centre depth now biases the translucent
water toward a cooler green-blue while the shallows retain warmer bed influence. Narrow moving
current threads, irregularly gated along the course, complement the existing sparse riffles and
broken bank sheen. A first restrained tuning was rendered and rejected because it remained nearly
indistinguishable; a stronger version was then broken into larger gaps to avoid a regular ladder
pattern.

Geometry remains 3,429 vertices / 6,080 triangles in one culled transparent draw, with no new
texture, attribute, simulation or collision work. Inspected four-phase llvmpipe evidence
`build-consumer/software-captures/20261014T032331.834177Z-stream` passes with 356 colour bins,
54.24 luminance deviation and 249.45 spatial contrast. The bed remains visible while moving
current/riffle accents make the water surface materially distinct from the damp ground.
Focused Chrome/SwiftShader evidence
`build/browser-software/20261014T032735.069130Z-chrome-stream` passes with two inspected
different-time frames, 60 finite intervals, GL error zero, an intact context and no new kernel GPU
fault. Stream survey, Core tests and 112 harness unit tests pass. The ARM64 debug APK assembles and
lint passes without starting an emulator.

## Embedded bank stones — 2026-10-13

The ordinary eye-height stream view still exposed long smooth stretches beside the newly incised
banks. Eighteen deterministic stones now sit irregularly along both sides of the 74 m middle and
lower reach. Their offset follows the local route normal and the shared varying half-width, then
starts beyond the replacement bed's 1.0 m bank blend. Terrain height, slope, shore distance, the
reserved player route and nearby trees reject unsuitable candidates. Every accepted stone stays
inside the moist bank microhabitat.

This is deliberately a prop-layout pass rather than a stream-specific renderer. The stones reuse
the existing three rock variants, material, instanced batches, culling and contact-shadow batch.
They are non-solid, so a prominent foreground stone cannot unexpectedly block a narrow crossing;
the collision grid remains at 805 static obstacles. The world gains exactly 18 rocks (184 to 202)
and no texture, shader, mesh, vertex stream, wildlife update or draw path. WorldReport records the
new `streamBankRocks` population and fingerprint `301ea795731c0732`.

Release `IslandStreamCourseSurvey` and Core tests prove the bounded deterministic population,
non-solid flag, local bank offset and moist habitat. Native llvmpipe evidence
`20261013T173931.739334Z-stream` passes at 1280x720 with 350 colour bins, 54.37 luminance
deviation, 249.45 spatial contrast and an empty kernel delta; the base frame was inspected against
the preceding no-stone frame. The new stones are visibly seated in the wet shoulders, break up
their smooth silhouette and do not form a regular crossing.

Chrome/SwiftShader evidence `20261013T174218.337615Z-chrome-stream` passes 60 finite intervals,
two inspected separated-time frames, `glError=0`, an intact context and no new kernel GPU fault.
ARM64 debug packaging and lint pass without launching or modifying the emulator. Longer free
traversal, Android runtime imagery and subjective positional stream-audio review remain open.

## Shared pools and pinches — 2026-10-13

The incised eye-height view exposed one remaining construction cue: channel width increased almost
monotonically from source to mouth even though the centreline meandered. `StreamRoute::HalfWidth`
is now the single CPU source for a smooth base width plus two incommensurate low-frequency terms.
Their influence fades at both ends, so the authored 0.45 m source and 1.50 m mouth connections are
unchanged. Across one-metre stations the width stays within 0.45–1.555 m and differs from the old
profile by -0.328–+0.174 m, producing readable pinches and small pools without high-frequency
saw teeth.

`StreamCourse`, the replacement bed, water mesh and collision support consume that same function.
The inland moisture/drainage CPU field calls it directly, while its GLSL counterpart carries the
identical equation for terrain material. Mangrove setback now includes local channel width, so
wider lower pools move stilt roots outward rather than weakening drainage exclusion; the compact
colony retains exactly 15 trees. The intentional placement response produces WorldReport
fingerprint `ec24c4c7db7f26ad` with 270 canopy trees, 1,770 understory plants, 582 bushes, 3,009
ground plants, 184 rocks and 33 fallen fronds.

No water vertex, terrain vertex, index, draw or texture was added: water remains 3,429 vertices /
6,080 triangles in one draw, and the adaptive bed remains 7,580 / 15,050 in its existing draw.
`IslandStreamCourseSurvey` and Core tests now prove deterministic reconstruction, endpoint widths,
the 0.45–1.56 m safety range and material positive/negative variation. Native llvmpipe evidence
`20261013T172213.838983Z-stream` passes with 345 colour bins, 54.49 luminance deviation, 249.45
spatial contrast and an empty kernel delta; the base plus four time-separated phases were
inspected. The ordinary eye-height view shows a narrower middle reach opening again toward the
bay, while keeping exposed wet substrate and physical banks.

Chrome/SwiftShader evidence `20261013T172616.845838Z-chrome-stream` passes two inspected distinct
frames, 60 finite intervals, `glError=0`, a live context and an empty kernel delta. ARM64 debug
packaging and lint pass without launching or changing the emulator. This width-only stage's
fingerprint was subsequently superseded by the bank-stone pass above. Longer free traversal,
Android runtime imagery and subjective stream-audio review remain open.

## Incised physical banks — 2026-10-13

The previous ordinary view still let the lower course read as a translucent strip laid on
nearly flat ground. This bounded pass deepens only the existing collision-authoritative
replacement patch. Water now lies 36 cm below the original centreline ground instead of 15 cm,
the wet bed is 28 cm below that water, and the outer bank reaches unchanged terrain through a
compact 1.0 m blend instead of 2.0 m. A shaped cross-section retains at least 14.1482 cm of real
bed clearance below the visibly opaque part of the ribbon. Maximum measured incision is
0.67725 m, below the existing 1 m construction guard.

The narrower affected band reduces the adaptive replacement from 8,696 vertices / 17,282
triangles to 7,580 vertices / 15,050 triangles. Terrain normals, daylight and `PlayerMotion`
support are still derived from this same patch. The portable terrain shader reuses the shared
GLSL route geometry function to reveal restrained soil strata only on genuinely sloping bank faces;
moisture noise breaks the response so it does not become a parallel painted outline. No new
texture, terrain draw, water draw or collision representation was added. The stream QA camera
now uses normal player eye height instead of a 3.5 m overview, exposing the bank profile that a
walker actually sees.

`IslandStreamCourseSurvey` now rejects inadequate bank freeboard, an overly broad blend and less
than 13 cm clearance beneath visible water away from source/outlet fades. Current native evidence
`20261013T160928.554419Z-stream` passes at 1280x720 with 344 colour bins, 54.48 luminance
deviation, 250.29 spatial contrast and an empty kernel-fault delta; all four separated-time
images were inspected. Chrome/SwiftShader evidence
`20261013T161228.307093Z-chrome-stream` passes 60 finite intervals, two distinct inspected
images, GL error zero, an intact context and no new kernel GPU fault. ARM64 assemble/lint also
passes without launching an emulator. More irregular longitudinal bank erosion, embedded stones,
long free traversal, subjective audio review and Android runtime imagery remain open.

## Readable shallow-water edges and riffles — 2026-10-13

A fresh current-binary four-phase sequence `20261013T150001.158057Z-stream` was
inspected before this pass. The bed, damp biome and downstream motion were present, but
the intentionally restrained translucent surface read almost like a grey-green wet path
from the ordinary camera. This pass improves water readability without widening the
channel, adding foam, changing terrain/collision or introducing another draw.

Each water vertex now carries its normalized -1..1 lateral coordinate in addition to
local route tangent, cumulative course distance and opacity. The portable vertex grows
from 28 to 32 bytes. The shader uses this coordinate for a narrow, tapered bank-edge
sheen inside the existing alpha falloff. A second low-frequency mask breaks faster
downstream wavelets into scattered shallow riffles; it never displaces geometry or
crosses the surveyed bank. Fresnel colour and base transmission increase modestly so
the water is identifiable while the wet bed remains visible. Geometry stays at 3,429
vertices / 6,080 triangles in one frustum-culled transparent draw. The CPU stream survey
now rejects nonfinite or out-of-range lateral coordinates.

The resulting native sequence `20261013T150812.724753Z-stream` passes at 1280x720 with
347 colour bins, 49.59 luminance deviation and 228.43 spatial contrast. All five base/
phase images were inspected as a montage: the route now reads as continuous shallow
water to the bay, and the broken highlights move between stages without becoming a
solid white stripe. Its kernel delta contains no GPU fault. Stream survey plus Core
tests pass.

Current Chrome/SwiftShader evidence `20261013T151145.333849Z-chrome-stream` passes 60
finite intervals with `glError=0`, an intact context, two distinct time-separated images
and no new GPU kernel fault. Both images were inspected and retain the subtle moving
riffles and exposed substrate. Android ARM64 assemble/lint also passes without launching
the emulator. Richer physical bank relief, longer free traversal, subjective audio
listening and Android runtime imagery remain open.

## Route-following translucent current — 2026-10-13

The water surface no longer evaluates every ripple against one island-wide straight
flow vector. Each generated vertex now carries the normalized tangent of its actual
local route segment plus cumulative course distance. A 28-byte portable vertex feeds
those values directly to the desktop GLSL 330 / GLES 300 shader. Its two small normal
waves and sparse current highlights therefore advance downstream through bends without
moving geometry through the bed or banks. The fresnel base and opacity were reduced so
the dark wet substrate remains visible below the shallow fresh water instead of reading
as an opaque blue-green ribbon. Geometry, indices, draw count and collision are unchanged:
3,429 vertices, 6,080 triangles and one frustum-culled transparent draw.

`IslandStreamCourseSurvey` now additionally rejects nonfinite or non-unit flow vectors,
upstream tangents and non-increasing/out-of-range cumulative distance. The isolated
native sequence `20261013T001913.275256Z-stream` records four deterministic 0.8--3.2 s
stages, each with one draw and the exact triangle budget. All four images differ and the
inspected montage shows a restrained highlight moving along the exposed lower course;
the final 1280x720 scene passes with 346 colour bins, 49.71 luminance deviation and
228.96 spatial contrast, with no kernel fault.

The focused Chromium 155 / SwiftShader WebGL 2 run
`20261013T002850.683872Z-chrome-stream` also passes with two inspected separated-time
frames, distinct captured hashes, `glError == 0`, no context loss and an empty kernel
delta.

The focused native input run
`software-stream-crossing/20261013T003730.637725Z` then uses an actual P debug
placement and held W input rather than a controller-only route. Sixteen samples move
from -3.39144 m signed across the southern bank through 0.341693 m nearest-route
distance to +2.83405 m on the northern bank. Locomotion remains Walking throughout;
every eye height agrees with the same nine-point maximum footprint support used by
`PlayerMotion` within 4.8e-7 m. The inspected before/after frames show the visible
water crossing and arrival in the wet-bank vegetation, with an empty kernel delta.
Android runtime, longer free traversal and richer physical bank relief remain open.

## Shared meandering route — 2026-10-12

`StreamRoute` now owns the height-independent centreline used by `StreamCourse`,
the replacement bed, water mesh, CPU/GLSL moisture field and authored wet-bank
plants/logs. Two smoothly enveloped low-frequency bends move the middle course
between -2.1626m and +1.4484m from the former straight line while returning
exactly to the existing source and sea outlet. Course distance is now the actual
polyline length, 101.879m, rather than a constant straight-line estimate.

The bed projects vertices onto the nearest local route segment; the first X-only
approximation was rejected because `StreamSurvey` found water within 1.5cm of the
bed on an inside bend. A bounded +/-6-segment search retains the corrected result
without scanning all 98 segments per vertex. Final geometry is 8696 vertices / 17282
triangles, deepest cut .429558m; water remains 3429 vertices / 6080 triangles.
The survey verifies source/outlet connection, material bends in both directions,
monotonic water, portable indices, winding, every sampled water triangle above the
actual bed, boundary preservation and player support.

Final StreamSurvey/Core/RenderedTerrain/TerrainMaterial tests pass 4/4 in
100.29s. World fingerprint is
`583c5a9ba1150779`. Actual isolated llvmpipe capture
`20261012T092831.265962Z-stream` passed 391 colour bins, std50.48/spatial233.38
with an empty kernel-after log. `build/stream-meander-final.png` and the before/after
comparison were inspected: the lower course visibly bends and one damp-bank log is
now readable, without a terrain/water separation artifact. This improves but does
not finish stream art: animated-flow sequence review, richer bank relief and
Web/Android/performance qualification remain.

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

### Stream-bank microhabitat composition

The next bounded pass reuses the three existing ground-plant silhouettes to place
39 marked mixed fern/broadleaf plants in irregular one-sided colonies along the
lower damp banks. Candidate colonies can be omitted, choose either bank, vary
their setback and scatter individual plants within the colony; the first paired
station layout was rejected after its actual image visibly resembled planted rows.
Two marked fallen logs reuse the darker existing bark variant and follow the bank
roughly without bridging the flowing channel. Layout tests enforce deterministic
identity, moisture/drainage, route/tree clearance, support and population bounds;
the world fingerprint is `965c50c3c8a40e47` (3023 total ground plants, 723 forest
companions, 11 total logs).

Final native build and StreamSurvey/CoreTests pass (2/2, 51.31s). Actual isolated
llvmpipe capture `20261012T090622.678487Z-stream` passed 392 colour bins,
std50.81/spatial238.05 with an empty kernel-after log. The inspected image
`build/stream-bank-irregular.png` confirms that paired rows are gone and the damp
foreground has varied colonies and gaps. This qualifies only the first bank
composition: the water course is still conspicuously straight/simple, the two
logs are not a strong landmark in this view, and motion/Web/Android/performance
qualification remains.

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
candidate water/bed heights and smoothly widening half-width0.45..1.5m. At that initial survey
stage, upper water lay15cm below original ground and the bed25cm below water; the current
incised-bank stage at the top of this document supersedes those dimensions. A minimum0.2%
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
At this historical prototype point, source transition, downhill flow shading,
wet-bank material, vegetation/prop clearance, movement and audio still required
implementation and actual image review. The integrated stages above supersede that
state and consume the same shared course rather than a separate approximation.
