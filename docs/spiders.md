# Spiders — implementation and verification

Status: CPU ground-habitat foundation only. **No live spiders or webs are installed
in IslandGame yet.** Both ground spiders and webs between plants remain required.
This is not a model, movement, visual or platform acceptance claim.

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
