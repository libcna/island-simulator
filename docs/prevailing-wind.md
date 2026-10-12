# Prevailing wind

2026-10-10, from the macro-geography analysis item 4: one fixed trade wind that several systems
share, so the island reads as a real place with a windward and a leeward side.

## What changed

- `PrevailingWind.hpp`: the wind blows from the east toward the west (`DirectionX = -1`).
  `wind::Exposure(x, z, shoreDistance)` is 0 in the sheltered interior and 1 on the exposed
  windward coast: a windward side factor (`LeewardFloor` 0.25 on the lee shore, rising to 1
  facing into the wind) times a coastal factor fading between 12 and 75 m from the shore.
- Palms lean away from the wind by `PalmLeanRadians(exposure)`, up to 0.17 rad (about ten
  degrees), as a world-space rotation after yaw in the instance transform, so the lean does
  not depend on the authored facing. Collision proxies stay at the base. `PalmPlacement`
  now records its signed shore distance for this; the deterministic placement is unchanged.
- Tests pin exposure bounds and ordering (east coast > west coast > interior), the lean limit,
  the retained shore distance and rejection of non-finite input.

## Evidence and limitations

Native llvmpipe `build/software-captures/20261010T161635.857589Z-palms`, `161642.176454Z-beach`,
`161649.189940Z-overview` (pass, inspected): east-coast palms lean visibly to the west, interior
palms stay upright. Not yet wind-aware: canopy crowns and dune orientation. Ocean/foam,
sky wisps and the surf/breeze mix are covered by the later stages below.

## Coastal-debris integration — 2026-10-12

`IslandEnvironmentField::SampleCoastalDebris` now combines the same prevailing-wind exposure
with the shared sheltered-bay, mangrove-mouth and rocky-shore identities. It returns normalized
exposure, shelter and accumulation controls. `NaturalPropLayout` consumes the accumulation for
driftwood, shell/pebble/dry-wrack detail and coastal fallen-frond density. Ten supported,
tree/route-cleared driftwood pieces and 24 spaced shell/wrack clusters form a deterministic
irregular deposition belt in the western bay using only existing geometry.

The default world measures sheltered/exposed counts of 11/4 driftwood, 47/26 beach-detail
clusters and 2/0 coastal fallen fronds. Total inventories remain bounded at 33 driftwood,
217 beach details and 35 fallen fronds; all six debris-guided crab colonies remain valid.
Core tests pin the field ordering, finite failure behavior, exact placement determinism,
support/clearance and at least 2:1 driftwood plus 5:3 detail bias. WorldReport is valid with
fingerprint `ec61dbf7e9ab7adb`.

Inspected isolated llvmpipe captures `20261012T140703.574060Z-bay-ground` and
`20261012T140806.665876Z-headland` pass. The bay frame visibly contains the accumulated logs,
wrack and small shell cues; the headland retains the sparse rocky character. These are native
software checks only. Web/Android, interactive coastal walking and performance comparison remain;
At that stage wave/foam, canopy and ambient sound were not yet wind-aware; the later
sections below supersede the wave/foam and ambient portions of that limitation.

## Slow fair-weather cloud drift — 2026-10-12

The existing eleven cumulus masses and layered high wisps now share one slowly advancing
sky phase coupled to the east-to-west trade wind. `SkyCloudDrift` advances 0.00035 rad/s
(about 1.2 degrees per minute) and wraps only after a complete shader-periodic turn, so
long sessions do not reset to a different cloud shape. This changes one uniform and the
existing procedural samples; it adds no geometry, texture, draw call, weather state or
day/night system. The stock-effect fallback remains the static gradient because it has no
procedural clouds to animate.

Core tests pin the initial phase, 0.21-radian ten-minute displacement, complete-turn wrap
and non-finite-clock fallback. Isolated llvmpipe overview
`20261012T144704.173888Z-overview` passes (377 colour bins, 39.79 luminance deviation,
207.94 spatial contrast); the inspected frame retains the sunny horizon, wisps and cloud
masses, and the runtime confirms the new shader uniform path. A true separated-time image
comparison plus Web/Android runtime remain open, so ecosystem-stage item 10 stays `[~]`.

## Windward ocean, foam and ambience — 2026-10-12

`CoastalWindResponse` now converts the same trade-wind direction into a bounded coastal
value that is neutral away from shore, approximately 0.25 in the leeward western bay and
1.0 on the exposed east coast. The ocean shader uses it to scale broad displacement and
all analytic surface slopes from 0.78 to 1.22. Ocean breaker foam and the separate shared
shoreline ribbon scale from 0.75 to 1.25. Tide-pool water remains neutral and calm.

The procedural surf and breeze beds reuse the same response through a restrained
0.82–1.18 gain scale. This adds no audio source, PCM asset, draw or geometry; jungle and
player-water layers remain unchanged, and existing crossfades smooth movement between
coastal sectors. Core tests prove east/west ordering, exact bounds, neutral far-field and
finite fallback. Inspected llvmpipe east/bay/headland captures
`20261012T145730.795098Z-east`, `20261012T145824.100449Z-bay` and
`20261012T145932.019541Z-headland` pass and show no shader or foam artifact. The nominal
east camera faces mostly inland, so it is not accepted as comparative water evidence;
controlled matched east/west motion and audible runtime review plus Web/Android remain.
