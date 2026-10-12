# Feral island goats — implementation in progress

**No goats are present in the running game yet.** This document records the first
placement, ground-pose and behavior foundations, not a completed species or visual acceptance.

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
- a proposed1.1m body-clearance disc outside every static obstacle;
- centre/eight perimeter terrain samples above6m with normal Y≥0.90 and no sampled
  height difference exceeding0.55m from the centre;
- separation from canopy trunks by9m×tree scale, understory by1.1m+1.8m×plant scale,
  and bushes by1.1m+1.2m×bush scale.

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
| -80.5767 | 34.0256 | 64.2600 |
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

## Remaining implementation

- [~] Fixed-length terrain-fitted goat legs: numerical foundation above implemented.
  Whole cloven-hoof contact, persistent stance behavior and the final body envelope
  still need validation with the actual model and live motion.
- [ ] Recognizable goat head/body/ears, horns/beard and cloven hooves, with readable coat.
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
