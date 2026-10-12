# Spiders — implementation and verification

Status: CPU ground population/motion and first mesh only. **No live spiders or webs are installed
in IslandGame yet.** Both ground spiders and webs between plants remain required.
This is not a rendered model, movement, visual or platform acceptance claim.

## Ground colony simulation (not integrated into IslandGame)

`SpiderColony` constructs the12 habitat homes against the full static world obstacle
set. Its renderer-independent states expose an authoritative `SpiderGroundPose`,
including the rendered-triangle body basis, eight local leg solutions, eight world
foot positions and stance flags. Ground basis transforms are shared with the intended
renderer; it must not reconstruct a different stateless gait.

Each home retains its pre-cleared0.60m patrol disk plus0.14m body envelope. Quiet
decisions deterministically alternate rest/crawl and headings, with inward intent
near the boundary. Crawl speed is0.035m/s horizontal, retreat0.065m/s, turning0.6rad/s.
An observer within2.5m in3D triggers non-hostile retreat; translation waits until the
heading is within60degrees of the direction away. At the patrol boundary translation
stops but turning remains possible. No escaping the reserved disk, teleports or
invisible colliders. High/nonfinite observers do not trigger ground retreat.

Updates clamp to100ms and subdivide to20ms. World-space stance feet stay fixed while
the body moves/turns. At most two feet swing, leaving at least six supports; the most
displaced eligible foot is selected above3.5mm error. A120ms smoothstep swing has an
8mm sine-squared lift and ground-aware height; moving landing targets lead by4mm,
resting ones return to neutral. Feet sit0.1mm above rendered ground. The IK solver
must reach every accepted body/foot pose. Candidate legs are sampled at five points
per segment for radius-aware ground clearance and140mm horizontal envelope. This
is not continuous triangle/terrain intersection or complete self-contact proof.

`SpiderColonyTests` builds actual terrain/palms/jungle/props/wreck/hut obstacles,
compares two deterministic colonies over30s at20ms, requires each animal to travel,
checks six supports, exact retained stance anchors, reconstructed rendered endpoints,
bounded speed/home disk and actual detailed-mesh terrain clearance at1s intervals.
Current focused test passes6.05s: per-animal travel0.625706–0.940732m,
110443 consecutive stance pairs with no changed world position,28773 swing samples,
minimum sampled mesh clearance0.0000991821m. A further10s fixed-observer test requires
nondecreasing observer distance and actual retreat travel; invalid/high observer and
invalid dt checks pass. Sparse mesh samples and CPU metrics are not visual gait proof.
Acceleration, support-polygon stability, terrain-normal transitions, limb self-contact,
longer/other routes and actual native/Web/Android presentation still need review.
Current inclusive native build and51/51 CPU tests pass55.93s, logs
`build/spider-colony-current-{build,tests}.log`. No actual renderer/browser/emulator
or portable build was run for this simulation step.

## First mesh (not yet rendered in the game)

`BuildSpiderMesh` consumes the eight solved leg chains. Two smooth-normal ellipsoids
form the front body and abdomen; three tapered, capped tubes follow each leg's
root/hip/knee/foot. The lower segment ends at a point, so there is no flat-ground
penetrating foot cap. Two short two-segment palps sit in front. Near geometry uses
12-sided/8-row bodies and6-sided limbs; far uses8-sided/6-row bodies and4-sided limbs.
Near-only eight small eye volumes have their centre rays fitted to the actual front
body triangles, not the larger analytic ellipse, with0.6mm embedding. Near-only
2.4mm bristles start at actual body tessellation vertices, with0.18mm root radius.
These are geometric details, not a fur texture or photorealistic material claim.
Cuticle/dorsal marking/eye/hair material tags await renderer shading.

Near/far meshes contain4248/1632 vertices (1416/544 triangles), guarded by a10000
vertex hard ceiling. CPU tests exercise both LODs over nine raised/longitudinal target
sets and check finite positions, unit normals, nondegenerate outward winding, required
parts, reduced far detail, flat-ground clearance and140mm horizontal habitat envelope.
This is not uneven-terrain clearance or limb self-contact proof. No live batch/LOD
selection, in-game camera, motion capture or portable runtime is claimed yet.

Inclusive native build and50/50 CPU tests pass61.84s after the polygon-fitted eye change
(`build/spider-mesh-current-{build,tests}.log`). The earlier pre-eye-fit50/50 suite
passes69.78s (`build/spider-mesh-{build,tests}.log`). A subsequent inclusive build adds
`island-spider-mesh-preview`; all three spider groups pass1.10s after that build.
The tool exports a CPU painter-sorted SVG with fixed demonstration colors and flat
per-face lighting, not the game's shading/depth path. `build/spider-mesh-preview.png`
(rasterized from its SVG) was actually inspected: eight-legged/two-body silhouette is
recognizable from elevated front and side, but faceting and sparse bristles remain.
This preview is deliberately not material/photorealism or live-island acceptance.

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
