# Octopuses

Four peaceful procedural octopuses now occupy deterministic reef and rocky-shallow dens. Placement
requires authoritative shallow seabed between 0.9 and 7 metres deep and scores reef suitability,
coastal rockiness and low beach suitability. Each animal remains within a 4.8–6.4 metre home.

The controller cycles through den, crawl, forage and short swim states, returns before leaving valid
habitat, hides when the observer comes within 2.7 metres and maintains both seabed and water-surface
clearance. Mantle pulse and independent arm phase remain renderer-independent. Full, reduced and
dormant distance tiers bound work; invalid input fails dormant.

The one-draw near model uses rounded multi-ring mantle/head volumes, compact amber eyes with
horizontal pupils and eight continuous tapered diamond-section arms with independent curl plus sparse pale sucker marks. Three muted
reef palettes and a reduced far silhouette keep the feature portable. The rejected first native
capture used a pointed five-ring mantle and disconnected triangular arms. After replacing those
with an eight-ring/sixteen-side mantle and continuous volumes, the inspected final native evidence
is `build-consumer/software-captures/20261015T071621.426076Z-octopuses` (106 colour bins, 31.20
luminance deviation, 176.62 spatial contrast). An intermediate oversized eye pass was rejected as
too cartoon-like before the smaller horizontal-pupil treatment was accepted.

`IslandOctopusGroupTests` runs two exact colonies for 120 seconds and verifies deterministic
movement, four shallow dens, bounded homes, actual travel, seabed/surface clearance, activation
accounting and invalid-input dormancy. The world report exports four animals and validates with
fingerprint `f662f6e9ac0f9c45`. The focused Chrome/SwiftShader audit
`build/browser-software/20261015T071824.635104Z-chrome-octopuses` passes 60 live intervals with
a clean WebGL context and an inspected matching image. ARM64 Android assemble/lint passes without
an emulator or KVM; the refreshed 24,363,299-byte APK has SHA-256
`00895083b4cff088b89e1a91be621602e3cd9421f151d63a923c6accfc21ddac` and its stripped
`libmain.so` is 19,320,728 bytes. Physical Android rendering remains part of the release-wide
device qualification rather than this species gate.

## Rear-swept mantle and smooth skin-light pass (2026-10-15)

A new coastal-animal audit found the octopus recognisable but visually dominated by a tall,
uniformly orange balloon-like mantle. The long arms were geometrically closed yet projected as
thin ribbons beside it. The mantle is now lower, broader and extended rearward behind a flatter,
wider head. The unchanged eight-by-sixteen ellipsoid tessellation receives analytic per-vertex
normal light, a continuous upper-to-side pigment blend and restrained low-frequency variation,
removing the conspicuous whole-ring colour steps without a shader or texture lookup.

Each of the eight arms now begins from a wider root, retains more vertical diamond-section depth,
spreads slightly farther and tapers to a still-delicate tip. Independent controller curl, pale
sucker rows, eyes and horizontal pupils are retained. This is a coordinate and vertex-colour pass:
the exact 2,892/24 near/far inventory, 3,100-vertex fixed capacity, four-agent population, one
batch, habitat, behavior, hide response, pulse and LOD remain unchanged.

Inspected native baseline `20261015T155650.913276Z-octopuses` and accepted current-binary
llvmpipe/pixman evidence `20261015T161248.570827Z-octopuses` show the lower rear-swept mantle,
continuous surface relief and stronger arm roots. The accepted 1280x720 frame passes with 119
colour bins, 33.22 luminance deviation, 187.73 spatial contrast and an empty kernel delta. Native
and Web gates pin the profile and exact geometry diagnostics so a stale executable cannot satisfy
the focused view.

## Elongated mantle and raised eyes (2026-10-15)

A later current-binary animal audit still found two conspicuous cartoon cues: the mantle projected
as an almost circular second ball and each eye was a flat gold button on the head surface. The
mantle is now narrower, taller and longer toward the rear. A closer head/mantle pigment transition
reduces the seam without removing the animal's two anatomical masses. Each eye receives a closed
eight-sided ocular bulb embedded in the head plus a fitted gold iris, wider horizontal dark pupil
and small pale catchlight.

The ocular geometry adds 294 vertices / 98 triangles per close animal, changing the exact near/far
inventory from 2,892/24 to 3,186/24. Existing slack means the conservative four-animal buffer grows
by only 800 vertices, about 13 KiB. Population, one opaque draw, dens, controller, arm curl, mantle
pulse, hiding and LOD are unchanged; a new runtime check enforces the exact close count. Inspected
baseline `20261015T161248.570827Z-octopuses` and accepted llvmpipe/pixman evidence
`20261015T202752.644913Z-octopuses` show the round mantle and button eyes replaced by a more organic
rear profile and raised horizontal-pupil eyes. The final 1280x720 frame passes with 117 colour bins,
33.20 luminance deviation, 188.71 spatial contrast and an empty kernel delta. Native and Web gates
pin both the new inventory and eye diagnostic. Focused Chrome/SwiftShader evidence
`20261015T203024.741359Z-chrome-octopuses` passes 60 finite intervals, inspected current and
after-simulation frames, GL error zero, a live context and an empty final kernel delta. Android
qualification remains grouped with later integration.
