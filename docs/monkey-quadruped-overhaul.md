# Monkey overhaul — in progress, not complete realism acceptance

The operator explicitly rejected the upright brown primitive assembly and requested actual
fur texture, monkey-like locomotion and tree climbing. These are separate necessary parts of
one unfinished overhaul. The shader portability repair did not satisfy them.

## Current short face, fur-broken tail and grasping extremities — 2026-10-13

The current near/far body has 2,049/797 vertices and 3,124/1,208 triangles per
silhouette. The thirteen-section trunk now separates a fuller rump and shoulder/chest
from a narrower waist instead of carrying one barrel radius along its length. The
larger cranium is separated from a shorter, broader connected facial relief; visible
ears and a forward-hooked, fur-textured tail with eight raised clumps strengthen the
primate silhouette. Near
palms/soles are wider, the four forward digits are shorter and thicker, and each has a
short lateral opposable digit. All extremity vertices retain the existing rigid sole
frame, so ground-normal fitting and live bark grips move the complete hand rather than
leaving decoration behind. No bone, texture, shader, draw call or population was added.

The current nine-state native tree sequence
`20261013T015052.930547Z-monkey-tree` and six-image Chrome/SwiftShader movement run
`20261013T015419.973741Z-chrome-monkeys` pass and were inspected. The shorter fuller
head and stronger hooked tail remain connected through sampled motion. Limb/trunk
surfaces still overlap rather than forming one manifold skin, the front face remains
simple and continuous natural gait is not accepted. This is another bounded anatomy
pass, not completion of monkey realism.

## Live first anatomy/ground-pose pass

- Horizontal, initially nine-section and now thirteen-section continuous torso/neck loft
  replaces the upright torso ellipsoid.
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

1. Actual raster fur is now integrated (`docs/monkey-fur-material.md`), native-inspected;
   refine anatomical density and qualify filtering in motion on Android/Web.
2. Individual ground targets now sample the rendered triangle surface; the trunk is vertically
   adjusted within the common reachable interval and fixed-length IK refits each limb.
   Impossible cliffs explicitly report lost contact rather than stretching bones. Independent
   palm/sole orientation is now native-tested (`docs/monkey-sole-alignment.md`). Still needed:
   trunk pitch/roll, multi-point support and stance locking through turns/transitions. A solved
   ankle target/tangent plane alone does NOT prove every digit is on the triangle surface.
   Current slope contact must not be called fully solved.
3. Trunk/limb roots still overlap instead of forming a single manifold whole-body skin. Inspect
   joint silhouette and weighting in close moving views; refine pinching and attachment flow.
4. Replace inherited run/head/tail clip compromises with coordinated primate movement; inspect
   timed image sequences. Ground phase is distance-driven, but turning and flight/bounding
   behaviour are not final.
5. Tree approach, reachable wind-deformed bark grips, bounded ascent, supported summit and
   descent are now live and natively/Web/Android exercised. Broader branch-to-branch travel,
   canopy rest/foraging and continuous natural climbing quality remain open; do not mistake
   the fixed QA excursion for a complete arboreal repertoire.
6. Update body-shaped contact shadows/collision extent for the horizontal silhouette.
7. Qualify this anatomy/pose revision on Android and Web; prior shader-fix captures do not
   qualify it. The complete island audit and other plan items remain open too.

No host GPU, dependency, KVM or emulator configuration change is involved.

## Individual rendered-terrain targets — native verification

The first contact stage samples the actual cached terrain triangles under each yaw/scale
transformed gait target. `FitGround` adjusts trunk height within the intersection of the
four reachable vertical intervals and solves both fixed-length links again. Swing clearance
is retained. Unreachable cliffs produce an explicit reach-limit diagnostic and preserve
bone lengths; loss of contact is not hidden as successful planting.

Native capture `20261011T115107.984389Z-monkeys` passed and was inspected; no reach-limit
diagnostic, empty kernel delta. All 36 non-display groups pass (82.60 s). Additional real
island fixtures cover seven spawn locations, eight yaws and forty gait phases; gait/shader
recheck passes 2/2. Synthetic fixtures check slopes, trunk/root attachment and unreachable
cliffs. These tests prove target geometry, not a fully planted palm/foot mesh. The limitations
above, motion review and Android/Web qualification remain open.

## Focused Web moving-view audit

The focused monkey runner now saves four intermediate screenshots, each after eight
additional animation frames, as well as the existing start/end images and sixty measured
frame intervals. It records all filenames in `monkeyMotionFrames`. This is bounded visual
evidence, not an automatic claim of biologically correct locomotion or perfect foot contact.

Inspected start and intermediate frames from
`20261011T115524.501474Z-chrome-monkeys` show the textured model remaining visible while
heading/location and limb poses change. Concrete remaining defects: the pale rounded
orbital/muzzle patches still read as a toy mask from the front; shoulder/trunk attachment
still looks assembled; the animated tail makes angular bends between its three segments.
Grass obscures some feet, so these images cannot establish absence of sliding or complete
sole contact. Preserve these as open defects rather than treating a renderer pass as realism
acceptance. Full runtime verdict is recorded separately after supervision terminates.

This run TERMINATED at the 360-second supervisor deadline: **not qualified**. All four
intermediate poses and the start image exist and were inspected, but the final interval/GL
report and endpoint image are missing. Final kernel delta contains only virtual-keyboard
registration and perf sampling throttling, no GPU fault. Retain the failed run unchanged.
The added 32 frames and four screenshots extend a slow software-rendered workload; next
attempt should use a bounded 600-second deadline, retaining all sixty measured intervals
and the same graphics gates. The runner now additionally persists a partial state checkpoint
after every intermediate image; `complete:false` cannot be mistaken for acceptance.

Repeat `20261011T120413.407382Z-chrome-monkeys` with a 600-second bound PASSES:
sixty intervals, GL error zero, context intact, no application error or limb-reach warning.
All six images (start/four intermediate/end) inspected. Final kernel contains ten unrelated
AppArmor audit messages, no GPU fault. This qualifies the fur/quadruped/terrain-target
revision built before the following tail change, not the later tail geometry or full realism.

## Continuous tail skin — first pass

Three disconnected rigid tubes are replaced by one tapering longitudinal mesh, with eighteen
near/nine far spans. Cubic Hermite sections have shared tangents through the existing three
bone anchors; smooth adjacent skinning weights remove rigid seams without adding bones,
textures or draw calls. One continuous interior fur window follows the whole tail. A pure
profile regression covers finite nondegenerate frames, bounded radii/weights, palette
indices and position/tangent/weight continuity at both internal anchors. Native build and
focused profile/gait test pass. Live image and full-suite verification are recorded below.

Native `20261011T121030.039628Z-monkeys` passes; inspected frame has continuous taper
without the old tube joins. Empty kernel delta. Asset validation passes with 65 sources.
Full non-display suite passes 36/36 (79.24 s).
This one near-static tail pose does not establish deformed silhouette quality across its
whole cycle. Android/Web current-tail checks remain open; the earlier successful Web run
used the previous tail mesh. Face, limb junctions, palm/sole contact and climbing remain open.
