# Spiders — implementation and verification

Status: CPU ground-habitat and articulated-leg foundations only. **No live spiders or webs are installed
in IslandGame yet.** Both ground spiders and webs between plants remain required.
This is not a rendered model, movement, visual or platform acceptance claim.

## Articulated anatomy foundation

`SpiderPose` defines two body ellipsoids in metres (+z forward): the front body
centre (0,.036,.018), radii (.018,.014,.024), and abdomen centre (0,.039,-.025),
radii (.025,.021,.034). These are design dimensions, not a scanned specimen.
Four pairs of legs attach along the front body; left front-to-rear indices0..3,
right front-to-rear4..7. Each chain has root, hip, knee and foot: fixed18mm coxa,
50mm upper and65mm lower links. The coxa points toward the neutral landing point;
the distal two-link solver projects an upward bend perpendicular to the target
direction, with a radial fallback for vertical targets. Neutral knees arch above
the floor and all neutral joint centres fit inside the existing140mm habitat radius.

The solver accepts arbitrary finite 3D landing targets, clamps the reachable shell
with a10-micrometre margin and explicitly marks reach-limited results. Coincident,
vertical and distant targets retain bone lengths; invalid indices/nonfinite or
overflowing distances throw. It does not silently stretch limbs. There is deliberately
no speed-scaled stride here: future simulation must retain world-space stance anchors
and supply their body-local targets to avoid repeating the earlier goat sliding bug.
This code is not yet called by IslandGame, and contains no crawling state machine,
terrain contact, skin mesh, fine hairs, renderer, collision acceptance or web mechanics.

`SpiderPoseTests` checks eight neutral legs, mirrored knees, fixed segment lengths,
216 nearby 3D landing targets,32 singular/far targets and six invalid inputs.
Focused native CPU test and inclusive49/49 CPU suite pass (68.36s total), with logs
`build/spider-pose-{build,tests}.log`. It establishes kinematics only: joint-centre envelope
checks do not establish the later mesh envelope or self-contact safety.

## Ground placement

SpiderHabitat::FindGround proposes12 homes near existing jungle canopy placements.
It evaluates16 bounded candidates per canopy tree (eight directions, two rings at
2.7/4.2 times tree scale), ranks them by a fixed integer hash and greedily selects
homes at least8m apart. Each retains its canopy index for traceable habitat association.
This association is not a ray-traced canopy-shadow or humidity guarantee.

The proposed patrol radius is0.60m plus a0.14m body/leg envelope. The whole disk must
avoid the complete static obstacle grid, including palms, jungle trunks, natural props,
shipwreck and hut. Root/stem exclusions additionally cover non-solid ground plants,
understory and bushes without removing their leaf cover. Nine terrain samples require
dry ground at least2m high, at least30m inside the shoreline, normal.y>=0.95 and no
more than0.25m height change from the centre. Home y is the actual rendered-triangle
height. These are sampled patch constraints, not a continuous route or leaf-volume
intersection proof; runtime movement will still need exact local checks.

No complete set means nullopt, never an unsafe fallback, vegetation clearing or
invisible spider collider. Nonfinite/out-of-bounds coordinates and invalid canopy
indices are rejected before terrain/plant access.

Initial reference-world proposals (x,y,z; canopy index):

```text
-103.321,  6.34669, -38.0958; 127
 -15.1554, 4.86173,-138.629;   25
 -47.5006, 5.15029,-113.109;   39
 -94.7256, 7.97435, 168.572;  258
 -20.5489,10.2619,  -58.1604;  94
-131.682,  4.9408,  -64.6671;  88
  -9.22798,5.53282, -97.7346;  58
 -49.912, 10.2524,  -56.4739; 110
  12.0491, 9.17779, 106.02;   234
 -68.3721, 7.8771,  -55.0346; 109
 -37.3655,38.8024,   31.4377; 176
 -60.8813, 9.16159, -50.1775; 109
```

## Verification and remaining work

SpiderHabitatTests constructs the real terrain/layout/obstacle set, including the
resolved hut. It checks repeated selection equality, actual ground height, independent
obstacle/root/stem distances and separation, invalid inputs, insertion of a new local
obstacle and a completely blocked world. The first focused test passes1.17s. Current
inclusive native build and48/48 CPU tests pass97.63s (SpiderHabitatTests1.58s), logs
`build/spider-habitat-current-{build,tests}.log`. No renderer/emulator/browser was run;
existing live world state/populations were not changed by these CPU proposals.

- [~] Bounded deterministic ground homes: foundation above; actual locomotion still absent.
- [ ] Recognizable two-part body, eight articulated legs and appropriate fine surface detail.
- [ ] Ground-conforming crawling, resting and non-hostile retreat; no foot sliding or tunnelling.
- [ ] Web habitats with anchors attached to actual plant geometry, not floating lines.
- [ ] Web-building/occupancy presentation and bounded web/ground populations.
- [ ] Live game integration, visibility/LOD budgets, counts and world-report coverage.
- [ ] Actual native/Web/Android visual and motion audits. CPU proposals are not live animals.
