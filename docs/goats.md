# Feral island goats — implementation in progress

**No goats are present in the running game yet.** This document records the first
placement, ground-pose, behavior and mesh foundations, not a completed species or visual acceptance.

## Deterministic open-slope homes

`GoatHabitat::Find` proposes six homes using the rendered terrain triangles, jungle
layout and the complete supplied collision grid. The reference test supplies palms,
canopy trunks, props, shipwreck proxies and the hut, matching live world obstacles.
The search checks3025 bounded lattice candidates, each with deterministic ±2m jitter
from seed `0x6f617431`. It prefers higher accepted sites, preserves stable ties and
requires eight metres between homes and at most40m from the herd's first home.

Acceptance requires:

- inland terrain at least6m high and35m inside the shoreline;
- centre normal Y in0.90..0.997 (sloped ground, excluding flat floors and steep cliffs);
- a1.25m body-clearance disc outside every static obstacle, enlarged after measuring
  the grazing horn envelope; analytic centre normal Y must also be at least0.90,
  matching the body-frame tilt bound rather than only the rendered triangle normal;
- centre/eight perimeter terrain samples above6m with normal Y≥0.90 and no sampled
  height difference exceeding0.55m from the centre;
- separation from canopy trunks by9m×tree scale, understory by1.25m+1.8m×plant scale,
  and bushes by1.25m+1.2m×bush scale.

These are conservative layout proxies, not measured goat anatomy or a proof that no
leaf overlaps any future model. They must be checked against the eventual goat mesh.
No vegetation is removed, no fallback ignores obstacles and no invisible collisions
are added. If a complete group cannot be found, the search returns no result.

Current reference sites (x,y,z metres, rounded):

| x | y | z |
|---:|---:|---:|
| -64.2131 | 38.4079 | 70.6373 |
| -72.1349 | 36.9021 | 56.3421 |
| -71.7322 | 35.7253 | 46.1564 |
| -72.1036 | 35.4725 | 80.1896 |
| -81.2493 | 33.7004 | 70.7155 |
| -80.0528 | 32.9791 | 81.4526 |

The tests check deterministic reconstruction, exact rendered height, independent
obstacle/vegetation distances, separation/herd bounds, rejection of blocked homes,
a completely blocked island, nonfinite input and offshore input. The first test build
found mismatched obstacle-container types; explicit spans corrected the test assembly.
The accepted-site criteria were not weakened to obtain six sites.

## Grounded walking-pose foundation

`GoatPose` provides a four-beat walking cycle with68% stance, an authored1.2m/s walk
reference, up to25cm stride amplitude and12cm swing lift. Its provisional two-link
leg chains have fixed39/38cm lengths,55mm hoof height and a25mm forward sole-centre
offset. Front/rear bends differ. These are authored modeling dimensions, not a claim
that two links reproduce every anatomical joint of a goat.

The body frame uses smooth analytic terrain orientation and rendered-triangle root
height. Each hoof target is fitted along body-up with18 bounded bisection iterations
against actual rendered triangles. Unreachable targets remain explicitly flagged;
bone lengths do not stretch to hide them. Nonfinite inputs, invalid leg indices,
negative speeds and invalid scale/mesh-range queries are rejected.

`GoatPhaseAdvance` calibrates phase from travelled distance, speed and scale so a
constant-speed straight root translation cancels local stance travel on a flat surface.
Its result is modulo one cycle. This is NOT persistent world-space foot locking:
turns, acceleration, changes in ground basis and continuous slope traversal still
need simulation-level tests and visual review.

Tests exercise13,824 fitted poses across all six homes, eight headings,32 cycle phases,
three speeds and three scales (0.75/1/1.1). All43,776 sampled stance sole centres match
rendered ground within0.15mm, with no unreachable target or stretched bone. Further
tests cover full-cycle/wrap continuity, flat constant-speed no-slip calibration at
three scales, invalid inputs and explicit unreachable-ground reporting. A centre
contact check does not prove that an entire cloven hoof footprint clears all triangles.
Final native build and all43 non-display groups PASS71.25s after this pose addition.
WorldReport remains valid with unchanged fingerprint `b3fde347f77eb5f8`; no renderer,
emulator, KVM or dependency changes. Portable builds/runtime and visual review of
goats remain pending because no live goat renderer/population exists yet.

## CPU herd behavior foundation

`GoatHerd` now constructs six simulated agents from the accepted homes, including
one0.75-scale juvenile. It is not instantiated by `IslandGame`. Deterministic timed
decisions alternate grazing, walking and resting; a finite observer within7m in3D
triggers non-hostile retreat. An observer100m above the herd does not trigger retreat.
The grazing pose eases in only after slowing down. Speed is capped at1.2m/s, with
bounded acceleration/deceleration and at most1.8rad/s turning.

Each valid update consumes at most0.1s in at most five20ms substeps. Static solids and
the same canopy/understory/bush exclusion envelopes are indexed once. Proposed steps
check endpoint/midpoint ground suitability, a containing midpoint collision disc
covering the swept body disc, other goat discs,14m individual home bounds and the40m
herd region. Each accepted endpoint also requires a reachable fitted leg pose. Stride
phase advances only from accepted3D travel, not attempted motion against an obstacle.
Blocked agents stop and request a bounded alternative turn, with a short avoidance
interval so direct-away observer steering cannot immediately cancel it.
There are no teleports, forced penetrations or attacks.

Regression evidence includes90s of duplicated identical-input simulation, all six
agents moving4.75–48.47m, grazing/walking/rest transitions, unchanged initial placement,
no overlaps and agreement with the original non-indexed placement predicates. Nearby
observer clearance reaches8.44m; the added head-on obstacle case reaches7.74m without
crossing its body-clearance boundary. Tests also cover high/invalid observers, invalid
and capped frame deltas, turn limits, and sampled hoof contacts along moving routes.

This is not runtime performance qualification or visual behavior acceptance. Abrupt
blocked-speed changes, persistent world-foot locking, full hoof/mesh contact,
distance-based update throttling and long all-angle observation remain to review.
Midpoint ground sampling is not a mathematical proof for every unsampled terrain
point. The final model must still fit the authored collision/body envelope.
Native build and all44 non-display groups PASS73.92s; herd regression3.90s. WorldReport
remains valid with unchanged fingerprint `b3fde347f77eb5f8` and no live goats reported.
No renderer, emulator, KVM or dependency changes; hardware stop latch unchanged.

## Mesh foundation and CPU shape review

`GoatMesh` builds local-space triangle lists with smooth normals, UVs, material and
anatomical-part tags. Body, deforming neck, head, paired curved horns, ears, beard,
tail and independently fitted legs include split hoof geometry. Grazing lowers and
rotates the head while the neck follows; this is not a verified feeding animation.
Near/far meshes have6252/3516 vertices, each below the8192-vertex guard. There is no
runtime renderer, coat texture or final material yet.

`IslandGoatMeshTests` checks both LODs across16 walking phases and11 grazing amounts:
finite coordinates, unit normals, UV ranges, nondegenerate consistently wound faces,
flat-ground clearance, required anatomy/eye LOD and leg-root coverage by actual torso
triangles. For each sampled vertex, the maximum horizontal projection over all tilt
azimuths up to acos(0.90) is bounded; the largest measured radius is1.17903m at scale1.
The1.25m collision disc covers that sampled envelope and the herd's smaller juvenile,
not arbitrary larger scales or every continuous animation state. Whole-hoof terrain
penetration, persistent planted feet and full body clearance on slopes remain open.

Native tool `island-goat-mesh-preview` writes a CPU-only orthographic SVG, converted
locally to `build/goat-mesh-preview.png` for inspection. The initial inspected image
exposed detached foreleg caps: the chest was too short. Extending the chest and
burying the upper limb attachment corrected the visible gap; the new actual-triangle
root test guards it in both LODs. The revised two-pose image was inspected as well.
It still shows coarse ears, tiny eyes and angular limbs/hooves. Temporary flat
triangle shading exaggerates facets; it is not the game's interpolated lighting,
depth buffer, fur, shadowing or a live animation audit. This is not visual acceptance.

After the clearance change, current90s herd travel is17.36–32.09m per agent; observer
clearance is8.69m and the head-on obstacle case7.95m. The fifth home changed as listed
above; older behavior metrics in this document describe the preceding1.1m envelope.
All13,824 home poses/43,776 stance contacts still pass. Complete native build and
45/45 non-display tests PASS59.51s (`build/goat-mesh-complete-tests.log`). No GPU,
emulator, Android/KVM settings or dependency changes; hardware stop latch unchanged.

## Remaining implementation

- [~] Fixed-length terrain-fitted goat legs: numerical foundation above implemented.
  Whole cloven-hoof contact, persistent stance behavior and the final body envelope
  still need validation with the actual model and live motion.
- [~] First head/body/ears, horns/beard and cloven-hoof mesh with CPU shape review above.
  Improve anatomy and add readable coat; actual in-game visual acceptance remains open.
- [~] Grazing/walking/rest/retreat CPU controller and bounded collision-aware movement
  implemented above. Browsing details, gait transitions and visual acceptance remain.
  The40m grouping radius is NOT a traversable-area guarantee; initial sites do not
  establish connectivity or safe paths between them.
- [ ] Actual population/update/renderer integration, lighting/contact shadows and budgets.
- [ ] Live diagnostic counts and WorldReport coverage only after goats really exist.
- [ ] Actual unobscured native/Web/Android appearance and motion audits; CPU placement
  tests and a successful build do not satisfy this acceptance requirement.

No species count, WorldReport fingerprint, rendering, physics or gameplay behavior
changes merely by adding this currently unreferenced placement component.

Native build and all43 non-display test groups PASS70.22s. WorldReport remains valid
with fingerprint `b3fde347f77eb5f8` and unchanged live wildlife counts. No renderer,
Android emulator, KVM or dependency changes were involved; no portable/runtime visual
qualification is claimed. The AMD hardware stop latch remains unchanged.
