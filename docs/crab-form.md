# Beach-crab form pass

## Scope

This 2026-10-12 pass addresses the explicit request to improve the visual form of the
island's crabs. It preserves the existing thirty agents, six debris-associated beach
colonies, behaviour, habitat constraints, culling, LOD distance and single dynamic draw.
No downloaded model or texture is used.

## Geometry change

The former near model used a pointed fan for the shell and one double-sided triangle
for each complete leg. In a close capture this read as a flat polygon with hairline
limbs and arrow-shaped claws. The replacement uses:

- four elliptical height rings and ten sectors for a rounded, broad carapace;
- restrained alternating upper-shell panels and distinct darker side plates;
- two crossed, tapered surfaces for both segments of all eight walking legs;
- a thickened arm and octahedral palm for each claw, followed by two separated fingers;
- short volumetric eye stalks with small dark eyes;
- paired mouthparts and two subtle lateral carapace teeth.

The exact near budget is 594 vertices / 198 triangles and the far silhouette remains
12 vertices / 4 triangles. The renderer checks both counts while assembling every visible
animal. Worst-case dynamic capacity is 17,820 vertices (285,120 bytes at 16 bytes per
`VertexPositionColor`), although ordinary culling and the 38 m near threshold make that
case deliberately conservative. All visible near and far animals still share one upload
and one draw call.

The `crabs` audit camera is now 1.75 m seaward of its subject and 0.62 m above the sampled
beach. This deliberately allows the normal non-hostile retreat behaviour rather than
freezing an artificial display pose, while making palm-sized anatomy inspectable.

## Native evidence

The pre-change ordinary view is retained at
`build-consumer/software-captures/20261012T211908.604459Z-crabs`; it shows the five-crab
colony mostly as small flat reddish marks. The closer diagnostic baseline
`20261012T212204.895592Z-crabs` exposes the pointed shells, single-surface legs and spike-like
eyes.

Final isolated pixman/llvmpipe evidence is
`build-consumer/software-captures/20261012T212449.173818Z-crabs`. The 1280 by 720 gate passes
with 166 colour bins, 28.25 luminance deviation and 175.93 spatial contrast. The inspected
frame shows five near crabs in one draw with rounded shells, jointed legs, palms, split
claws and shortened eye stalks. `kernel-live.jsonl` and `kernel-after.json` contain no new
kernel messages. No AMD GPU was used.

## Remaining limits

The animals remain deliberately low-poly and vertex-coloured. They do not yet have a
filtered shell microtexture, self-shadowing, burrows or individually curled distal leg
segments. WebGL 2, Android GLES 3, physical-device performance and interactive motion review
remain open, so the broader aquatic-wildlife polish item remains in progress.

## Fixed-sun facets, distal toes and socketed eyes (2026-10-13)

Fresh baseline `20261013T024856.545338Z-crabs` confirmed that the rounded shell was recognizable,
but its panels remained uniformly flat, the second leg segment ended as a hairline and each eye
read as a black diamond on a stalk. All crab triangles now receive the shared portable CPU
fixed-sun/sky-fill facet response used by other wildlife. Every walking leg gains a short,
alternately splayed, tapered third segment from foot to toe. Each eye stalk ends in a shell-colour
octahedral socket with a smaller dark globe projected slightly forward/up, instead of exposing the
entire black octahedron.

The eight toes add 96 vertices and the paired outer eye sockets add 48. Exact near geometry is now
738 vertices / 246 triangles per crab; far geometry stays 12 / 4. Worst-case capacity is 22,140
vertices or 354,240 bytes at 16 bytes per `VertexPositionColor`. Population, 38 m LOD threshold,
behaviour, habitat, culling and the single dynamic draw are unchanged. Runtime still checks the
exact per-agent near/far vertex counts.

The first retained after-image `20261013T025101.002806Z-crabs` passed and was inspected against the
baseline. Final current-camera native evidence `20261013T025356.261202Z-crabs` also passes: five
near crabs in one draw, 191 colour bins, luminance deviation 30.38, spatial contrast 186.15 and an
empty kernel delta. The image shows stronger shell volume, socketed eyes and visibly bent distal
feet; thin planar claw fingers and low-poly surfaces remain.

A new focused `crabs` browser scope requires the current detail marker, a nonzero near population
from all 30 agents in exactly one draw, 60 finite intervals, two screenshot files, SwiftShader,
zero GL error and an intact context. The QA camera follows only the third crab's translation so a
slow software run retains its moving subject. Parent validation requires different hashes for the
two images. Actual `20261013T025538.029727Z-chrome-crabs` passes; both frames were inspected and
retain the moving colony, hashes differ, and the kernel delta is empty. This does not qualify
continuous gait, hardware performance, current Android rendering, claw curvature or a filtered
shell microtexture; those remain open, and the Android emulator/KVM was not touched.

## Solid pincer fingers (2026-10-13)

The current five-animal audit still exposed both fingers of every claw as coincident, double-sided
triangles. Each finger is now a closed tapering triangular prism: separate upper/lower faces, three
lit edge quads and reduced tip thickness preserve the open pincer while giving it a visible solid
edge. This changes no behavior, habitat, LOD threshold, population or draw count.

Four fingers add 72 vertices, bringing the exact near tier to 810 vertices / 270 triangles per
crab. The conservative all-near capacity is 24,300 vertices / 388,800 bytes; the far tier remains
12 vertices. Inspected llvmpipe evidence
`build-consumer/software-captures/20261013T111127.629747Z-crabs` passes with 195 colour bins, 30.83
luminance deviation, 189.41 spatial contrast and an empty kernel delta. Thin planar fingers are no
longer an open defect; smoother curvature, shell microtexture, Android execution and
physical-device cost remain open.

Focused Chrome/SwiftShader evidence `20261013T111847.667938Z-chrome-crabs` passes. The two
inspected frames retain separated solid claws on the moving colony and have different SHA-256
identities; all 60 finite intervals complete with `glError=0`, a live context and an empty kernel
delta. Current Web execution is therefore qualified; Android, continuous gait, smoother curvature,
shell microtexture and physical-device cost remain open.

## Lower lateral silhouette and stronger limbs (2026-10-13)

The retained close view still made the animals resemble round-bodied spiders: the shell rose too
high above narrow legs and the claw palms were too small to dominate the front silhouette. A
fourth, no-cost proportion pass lowers every carapace ring, widens the lateral shell extent and
slightly reduces its front-to-back depth. All eight walking legs now start farther out, end closer
to the body and use thicker crossed sections. Both octahedral claw palms are larger while the
existing separated solid fingers remain unchanged.

This is a coordinate-only change. Exact near geometry remains 810 vertices / 270 triangles, the
far tier remains 12 / 4, and the conservative all-near capacity remains 24,300 vertices / 388,800
bytes. The thirty-agent population, habitat, behaviour, culling, 38 m LOD threshold and one
dynamic draw are unchanged. Native and browser capture gates now require a dedicated silhouette
diagnostic in addition to the prior anatomy marker, preventing a stale executable from qualifying.

Inspected llvmpipe evidence
`build-consumer/software-captures/20261013T170649.755712Z-crabs` passes with 192 colour bins,
30.24 luminance deviation, 184.04 spatial contrast and an empty kernel delta. Compared with
`20261013T111127.629747Z-crabs`, the colony has visibly lower, wider shells, sturdier shorter legs
and more readable claw palms.

Focused Chrome/SwiftShader evidence `20261013T170831.603758Z-chrome-crabs` passes with two
inspected different-hash moving poses, 60 finite intervals, `glError=0`, a live context and an
empty kernel delta. ARM64 debug packaging and lint also pass without starting or changing the
Android emulator. Current Android runtime rendering, continuous gait review, smoother shell
curvature, filtered shell microtexture and physical-device performance remain open.

## Smooth low carapace and solid legs (2026-10-14)

The next visual audit found that the ten-sector shell still exposed large triangular colour panels
and the crossed leg strips remained visibly flat. An initial smooth version was rendered and
rejected because its high round dome looked beetle-like. The accepted version uses eighteen radial
segments, a lower and substantially wider four-ring profile, per-vertex radial daylight and ten
small irregular shell marks. All twenty-four walking-leg sections are now tapered rectangular
volumes rather than crossed sheets. Claws, eyes, thirty-agent simulation, 38 m LOD boundary, far
silhouette and one-draw batching remain unchanged.

The exact near budget is 1,326 vertices / 442 triangles per crab; conservative all-near capacity
is 39,780 vertices / 636,480 bytes. Inspected llvmpipe evidence
`build-consumer/software-captures/20261014T031332.711101Z-crabs` passes with 197 colour bins, 34.00
luminance deviation, 193.78 spatial contrast and no new kernel GPU fault. The accepted frame reads
as a lower, broad-bodied shore crab with materially solid legs; the discarded high-dome iterations
are retained only as local build evidence. Focused Chrome/SwiftShader evidence
`build/browser-software/20261014T031556.664431Z-chrome-crabs` passes with two inspected animated
frames, 60 finite intervals, GL error zero, an intact context and no new kernel GPU fault. The
ARM64 debug APK assembles and lint passes without starting an emulator.

## Debris-colony separation and honest QA framing (2026-10-14)

A new current-binary audit exposed a composition problem rather than another mesh defect. The five
members around each debris anchor accepted the first valid points on almost the same small circle;
the best-isolated animal across all thirty still had only 0.387424 m to its nearest neighbour. The
old close camera also triggered all five retreat vectors at once. Their overlapping broad shells
therefore hid the already implemented low profile, legs and claws.

Debris-guided initialization now rejects homes less than 1.12 m from an earlier member of the same
five-crab colony. It keeps the exact six selected debris anchors, 3.25 m anchor radius, shoreline
surface/slope constraints, home radii and deterministic search. The QA view selects the animal
with the greatest nearest-neighbour distance (current subject 28 at 1.28294 m) and gives only this
inspection simulation a distant observer, so foraging remains active without camera-induced herd
retreat. Ordinary gameplay still supplies the real player and preserves the existing non-hostile
scuttle response.

Geometry, thirty-agent population, palettes, 38 m LOD, one dynamic draw and audio are unchanged.
Baseline `20261014T155226.177045Z-crabs` and final `20261014T160445.974296Z-crabs` were inspected;
the latter passes at 325 colour bins, 26.46 luminance deviation and 136.09 spatial contrast and
shows individually readable low broad shells instead of one piled mass.

## Live colony separation and current habitat gate (2026-10-15)

The aquatic-wildlife audit found a new failure in the current debris/burrow composition. Home
points still began at least 1.12 m apart, but autonomous foraging had no live agent separation, so
several broad shells, legs and claws converged into one pile. The native supervisor also retained
the obsolete isolated-subject regex after the camera intentionally changed to show a shared burrow;
browser/Web gates already used the new habitat composition.

Every accepted movement candidate now remains at least 0.72 m from all other live beach crabs.
A blocked candidate turns toward home with a deterministic lateral bias and reduces speed rather
than teleporting or stopping the colony. The thirty agents, six five-member colonies, home points,
burrows, player-triggered scuttle, habitat limits, mesh, materials, LOD and one draw are unchanged.
The long native CPU route samples all pairs every simulated second for 400 seconds and rejects an
overlap. A shared runtime marker pins the separation policy in native and Web capture gates. The
native validator now checks the current subject-to-burrow composition rather than asking for a
removed isolated-animal diagnostic.

The initially failed but visually useful frame is retained at `20261015T144744.062847Z-crabs`.
Current evidence `20261015T150132.598386Z-crabs` passes at 1280x720 with 279 colour bins, 41.08
luminance deviation, 204.90 spatial contrast and an empty kernel delta. The inspected same-camera
comparison keeps all five animals around their debris-linked burrow while restoring individually
readable shells and limbs. This proves the bounded native route and one current composition, not
physical-device performance or collision-free motion under every possible future controller.

## Closed shallow underside (2026-10-16)

A later small-fauna comparison retained the low eighteen-sided top shell, solid limbs and live
colony separation but found that a crab turning its broad axis edge-on still exposed an open lower
ring and read as a thin red card. The eighteen-point lower rim now closes around one shallow
recessed abdominal centre with eighteen dark fixed-sun-shaded triangles. The accepted low crown,
lateral corners, legs, claws, eyes and motion are unchanged.

This adds 54 vertices / 18 triangles per visible near beach crab, raising the exact tier from
1,338/446 to **1,392/464**. The conservative all-thirty-near capacity grows by only 25,920 bytes to
668,160 bytes at the existing 16-byte vertex format. Coconut crabs reuse this base beneath their
larger armor and rise from 2,538 to 2,592 vertices each. Population, 38 m LOD, culling, materials,
simulation and the shared one-draw submission are unchanged.

Inspected native `20261016T005728.064865Z-crabs` passes at 263 colour bins, 43.12 luminance
deviation, 226.10 spatial contrast and an empty kernel delta. Independent coconut-crab guard
`20261016T010220.118617Z-coconut-crabs` passes at 283/44.19/216.91 with an empty kernel delta,
confirming its exact derived geometry remains valid. Focused Chrome/SwiftShader
`20261016T005949.422541Z-chrome-crabs` passes two distinct inspected motion frames, 60 finite
positive intervals, GL error zero, a live context, zero target-close timeouts and an empty final
kernel delta. All 103 browser/capture harness tests pass.
