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
palms stay upright. Not yet wind-aware: canopy crowns, windward wave height and foam,
dune orientation, sky wisp direction and the wind bed in the soundscape; these
are the remaining parts of item 4.

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
wave/foam, canopy and ambient sound are still not wind-aware.

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
