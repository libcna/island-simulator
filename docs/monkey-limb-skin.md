# Monkey limb-to-torso skin refinement

## Change

The old limb skin started with a .105-model-unit open ring centred at each shoulder/hip,
fully attached to the upper limb. Its broad rotating rim and cone-like silhouette were
visible in close tree-contact captures. `MonkeyLimbProfile` now uses ten longitudinal
sections instead of nine, a .040 proximal radius embedded inward/upward in the torso,
and a narrower .083 maximum muscle radius further down the limb. The first ring follows
the torso bone, the next blends torso/upper limb, and the remaining rings blend upper/
lower bones as before. Each vertex still has at most two influences. The collar frame
is orthonormal and normals account for radius taper instead of treating every section
as a cylinder. Foot/hand contact transforms and the 14-bone skeleton are unchanged.

Extra mesh cost per silhouette: 44 vertices / 80 triangles near, 28 vertices / 48 triangles
far. Draw count, shader/uniform budget and textures are unchanged. These remain overlapping
surfaces, not a welded anatomical shoulder mesh; a visible material/shape junction remains.

## Evidence and rejected first revision

Native `20261011T142749.956942Z-monkey-tree` passes its automated gates but the inspected
peak image exposed an angular collar rim. That first revision was not accepted visually.
The final root moves farther inward (half the root X coordinate) with a .040 radius;
CPU checks require the entire proximal ring's lateral envelope below .14 for all three
thickness variants. Other fixtures cover valid weights, two-influence limit, non-collapsed
rings, torso attachment and unchanged wrist endpoint.

Final `20261011T143018.362367Z-monkey-tree` passes all eight stage gates; grounded and
maximum torso-rise images were inspected. The exposed proximal rim is hidden, the upper
arm is less bulky and hands/hind feet retain their contacts. Ordinary walking-view capture
`20261011T143134.265841Z-monkeys` passes and was inspected too. All three kernel deltas are
empty. These are sampled poses, not proof of every intervening animation frame or every
camera angle. Junction visibility, simplified anatomy/face, stiff tail and full climbing
remain open. No Android/Web runtime acceptance of this geometry change is claimed.

Final native build and 37/37 non-display groups pass in 60.06 seconds
(`build/monkey-limb-collar-final-tests.log`). Asset gate: 24 records, 71 generator sources,
10 project files, zero external files. Stop latch unchanged; all launched jobs finished.
