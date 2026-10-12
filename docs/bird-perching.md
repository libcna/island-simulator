# Bird landing and perching

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
