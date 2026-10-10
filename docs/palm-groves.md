# Coconut palm groves

2026-10-10, from the macro-geography analysis item 9: 330 palms were spread as a near-uniform
scatter (178 solitary, one with three neighbours inside 12 m) and 134 stood more than 60 m
inland, so the coast had no groves and the forest was salted with palms.

## What changed (`VegetationLayout.cpp`)

- Candidates every 8 m (grid radius 33). A contrast-shaped two-octave grove field (62 m and
  21 m value noise, `smoothstep` around 0.42-0.64) times a coastal weight that is full within
  32 m of the shore and fades over the next 70 m: `density = 0.006 + coastal * (0.012 + 0.36
  grove)`. A tiny floor keeps a few old solitary palms inland.
- Young palms: inside groves (`grove >= 0.45`) an adult recruits a juvenile with 32 % chance,
  2.6-4.4 m away, scale 0.42-0.64, at least 2.2 m from any other palm, on valid coastal ground
  and off the reserved routes. Juveniles get their own collision proxy (`0.31 x scale`).
- Result: 370 palms (282 adults, 88 young), 187 with at least three neighbours within 12 m,
  36 solitary; 3 beyond 90 m from the shore (was 52). The wind lean from
  `docs/prevailing-wind.md` applies to every palm. Downstream props that avoid palms shifted:
  157 land rocks, 11 logs, 32 driftwood pieces, 67 fallen palm fronds.
- Tests require >= 40 % grouped, <= 5 % beyond 90 m, 30+ young palms at <= 35 % of the total,
  juvenile scale/collision ranges and 2.2 m trunk clearance; Web/Firefox/Android harness pins
  now expect 370 palms and the new prop counts.

## Evidence and limitations

Native llvmpipe `build/software-captures/20261010T174808.840978Z-overview`,
`174754.658042Z-palms`, `174801.794546Z-beach` (pass, inspected): the coast reads as groves with
gaps and the interior as broadleaf forest. Not yet done from item 9: fallen or broken palms and
very old leaning giants. Android `palms`, `jungle` and `east` pass
(`build/android-software/20261010T175715.080427Z-palms`, `175828.715762Z-jungle`,
`175941.257408Z-east`); the full Web suite result is recorded in `PLAN.md`.
