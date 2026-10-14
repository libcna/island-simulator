# Bird landing and perching

## Near anatomy and wreck-perch audit — 2026-10-14

The original live perch state was functionally correct but its near renderer still used an
eight-triangle pointed body with planar wing sheets. At the former five-metre QA distance, the
wreck mast partly obscured it and the animal read as a small origami triangle. The accepted near
form keeps the same state machine and one dynamic batch, but uses a closed twelve-sided body loft,
a multi-ring head, an ochre four-face beak and paired lateral eyes. Per-face fixed-sun colour on
the body and head restores volume without a texture or additional draw. The maximum near budget is
498 vertices per bird; the 24-agent batch remains bounded and the far LOD is unchanged.

The detail camera now uses a close front-three-quarter view of the deterministically seeded wreck
bird and frames the real spar without looking through the mast. Baseline
`build-consumer/software-captures/20261014T130525.972281Z-birds` and accepted
`20261014T131932.005299Z-birds` both pass the ordinary image validator and were inspected at full
resolution. An intermediate eight-sided-head version at `20261014T131127.467822Z-birds` was
rejected as toy-like. Native Release, WebGL and Android ARM64 assemble/lint pass; portable runtime
appearance and a close flying-pose sequence are not yet claimed.

The existing 24-bird flock now has a bounded `Fly -> Approach -> Perched -> Depart` cycle.
Only shore and canopy route families use it, and at most two birds may leave normal flight at
once. High birds retain their uninterrupted distant routes. Approach uses bounded turn, bank,
descent and terrain clearance; a perched bird fixes its feet to the selected support, folds its
wings, holds a subtle calm pose, and departs with a short climbing flap before rejoining its
original route family.

`BirdPerchLayout` derives eight deterministic anchors from geometry already in the world: five
points are real vertices on broadleaf support branches, two are the authored caps of solid land
rocks, and one is the surface of the shipwreck's rendered cross-tree beam. The initial world starts
with one shore bird on the wreck and one canopy bird on a branch, after which all eligible birds
use the same scheduled state machine. The near renderer blends its existing wings into a folded
silhouette and adds two small leg/toe silhouettes while contact is active. Far geometry only folds
the wings. No new texture, draw call, persistent allocation, collision object or animal is added.

Core regression covers deterministic anchors, the exact 5/2/1 support inventory, terrain
clearance, the two-agent cap, all four states, folded wings, support-coordinate locking and exact
body-to-feet height. Isolated llvmpipe evidence
`20261012T153950.288712Z-birds` passes and was inspected: the fixed bird camera shows the folded
bird contacting the wreck beam with the full ship context. The earlier flight view also remains
valid. A moving visual sequence, wind-following correction for branch anchors, longer interactive
observation and actual Web/Android runtime remain before final completion.
