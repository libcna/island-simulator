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
palms stay upright. Not yet wind-aware: canopy crowns, windward wave height and foam, driftwood
accumulation, dune orientation, sky wisp direction and the wind bed in the soundscape; these
are the remaining parts of item 4.
