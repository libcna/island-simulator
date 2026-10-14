# Forest stands, emergent trees and saplings

2026-10-10, from the macro-geography analysis items 7 and 8: the canopy was an even 14.5 m
jittered lattice with mild cluster noise, so from above the forest read as a park of separate
trees of one height.

## What changed (`JungleLayout.cpp`)

- Candidates every 10.5 m over the same area (grid radius 28) so crowns can touch inside stands.
- A contrast-shaped stand field (`smoothstep` of 0.72 broad 61 m + 0.28 fine 23 m cluster noise
  around 0.40-0.64) drives acceptance: `density = (0.52 + 0.48 interior) * (0.045 + 0.60 stand)`.
  Dense stands approach closure, open ground between them stays open, and a thin scatter of
  solitary trees remains.
- Emergents: trees with `stand > 0.60` and a 9 % hashed chance are promoted in a second pass
  only when at least two accepted neighbours stand within 20 m, to scale 1.60-1.85 (about 19-24 m
  overall). Their collision radius follows the trunk (`0.43 x scale`). Crown lobes, the canopy
  light occluders, per-tree culling boxes and contact shadows all scale with the tree.
- Saplings: a separate 10% candidate channel inside the stand field converts a bounded subset of
  later placements to scale 0.38--0.58 (roughly 3--6 m). A sapling must retain an established
  neighbour within 18 m. The default world contains 16; no tree, mesh, buffer or draw is added.
  Their root collision shrinks conservatively with the trunk. Monkeys do not select them as
  closed-canopy climbing trees, and the seven qualified climbing-home indices remain adult.

Resulting default population: 277 canopy trees (was 281), 22 emergents, 223 trees with at least
three neighbours within 20 m, 9 solitary. Downstream layouts that avoid trunks shifted slightly:
579 bushes (14 at the clearings), 3,116 ground plants (64 clearing, 827 forest colony, 2,225
island-wide unchanged), 199 colonies, 159 land rocks, 12 logs, 31 driftwood. Tests pin the stand
structure (>= 60 % grouped, <= 15 % solitary, 3-15 % emergent, emergents only inside stands)
and the new counts; the Web/Firefox/Android harness count pins were updated.

## Evidence and limitations

Native llvmpipe `build/software-captures/20261010T150916.889020Z-jungle`, `150924.874906Z-overview`,
`150932.482781Z-east` (pass, inspected): the overview shows clumped masses with open gaps and
emergents breaking the canopy line. Masses are still made of the same eight crown forms and the
forest edge is not yet shaped by wind or soil; terrain macro-geography (item 1) remains open.
Platform runs: Chrome/SwiftShader vegetation scope `build/browser-software/20261010T152203.104225Z-chrome-vegetation`
passes all eight views. The Android emulator crash first attributed to this pass was later
traced to the cumulus loop in the sky shader (`docs/sky-atmosphere.md`); the stands and the
forest-floor colonies were innocent and are unchanged. Android results after the sky fix are
recorded in `PLAN.md`.

The later sapling pass is evidenced by current native overview/jungle/east captures
`build-consumer/software-captures/20261014T203722.209283Z-overview`,
`20261014T203727.476082Z-jungle` and `20261014T203732.668788Z-east`. A frozen view of the first
sapling passes in both forced tiers at `20261014T204128.524263Z-canopy` and
`20261014T204133.968191Z-canopy`. Chrome/SwiftShader evidence is
`build/browser-software/20261014T205103.677192Z-chrome-canopy-light`: one inspected jungle frame,
60 positive finite intervals, GL error zero and no context loss. The ARM64 debug APK assembles and
contains the diagnostic; device runtime remains unclaimed. The core regression first caught, then
prevented, loss of the ordinary monkey climb path. Standing dead and broken trunks remain outside
this pass.
