# Feral island goats — implementation in progress

**No goats are present in the running game yet.** This document records the first
placement foundation, not a completed species or a visual acceptance claim.

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

## Remaining implementation

- [ ] Fixed-length terrain-fitted goat legs and hooves; validate the proposed body envelope.
- [ ] Recognizable goat head/body/ears, horns/beard and cloven hooves, with readable coat.
- [ ] Grazing/browsing/walking/turning behavior and terrain/obstacle-safe movement.
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
