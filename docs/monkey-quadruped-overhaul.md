# Monkey overhaul — in progress, not complete realism acceptance

The operator explicitly rejected the upright brown primitive assembly and requested actual
fur texture, monkey-like locomotion and tree climbing. These are separate necessary parts of
one unfinished overhaul. The shader portability repair did not satisfy them.

## Live first anatomy/ground-pose pass

- Horizontal, nine-section continuous torso/neck loft replaces the upright torso ellipsoid.
- Reduced head, narrow orbital skin, lower projecting muzzle, small eyes, paired nostrils and
  a mouth line replace the large two-lobed pale mask and button nose.
- Each arm/leg is one nine-ring tapered surface across its two joints, with blended skin
  weights at the elbow/knee. Hands and feet now extend along the ground; four near digits.
- Four phased ground-support/return trajectories drive two-link IK. The simulation advances
  phase from actual horizontal displacement divided by the scaled cycle distance, not from
  wall time or requested speed. Stationary/blocked agents do not advance stride phase.
- Gait amplitude eases in/out; the trunk no longer inherits the old bipedal bob/lean. Existing
  clip/crossfade machinery still supplies head/tail motion. This is transitional integration,
  not a claim that the former arm-swing clips are now anatomically correct climbing clips.
- GLSL uses two bone weights; the palette remains 14 matrices. No textures or draws added.
  Enlarged conservative culling sphere includes the longer horizontal body/tail.

Native `20261011T113355.740100Z-monkeys` passes the close-submission/image gates and was
inspected. The animal is visibly quadrupedal with a horizontal body, no longer the upright
teddy-bear silhouette. Kernel delta is empty. This is one pose, NOT motion-quality acceptance.

Native incremental builds pass. Non-display suite: 35/36 groups pass in 67.36 s; the CPU
supervisor fixture failed its four-second startup-marker deadline. An unchanged standalone
rerun passes all 38 supervisor checks (CTest 1.95 s). Both logs are retained, and no safety
assertion was weakened. The long troop regression now covers deterministic/bounded gait
state and no phase advance while stationary. Asset manifest validation passes (64 sources).

## Known deficiencies / next required work

1. Fur is STILL flat palette colour. Integrate an actual documented raster fur texture with
   stable anatomical mapping, mipmaps/filtering and portable texture budgets.
2. IK currently assumes a flat local support plane. Fit individual hand/foot supports to the
   rendered terrain, pitch the trunk accordingly, and verify sole orientation/stance locking
   through turns and transitions. Current slope contact must not be called solved.
3. Trunk/limb roots still overlap instead of forming a single manifold whole-body skin. Inspect
   joint silhouette and weighting in close moving views; refine pinching and attachment flow.
4. Replace inherited run/head/tail clip compromises with coordinated primate movement; inspect
   timed image sequences. Ground phase is distance-driven, but turning and flight/bounding
   behaviour are not final.
5. Tree approach, trunk grips, ascent, supported arboreal rest/movement and descent remain
   UNIMPLEMENTED. Use actual tree geometry and reachable contacts, not vertical teleportation.
6. Update body-shaped contact shadows/collision extent for the horizontal silhouette.
7. Qualify this anatomy/pose revision on Android and Web; prior shader-fix captures do not
   qualify it. The complete island audit and other plan items remain open too.

No host GPU, dependency, KVM or emulator configuration change is involved.
