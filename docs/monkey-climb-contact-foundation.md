# Canopy contact foundation for monkey climbing

The canopy renderer and future grip solver now share trunk ring heights and vertex positions
in `CanopyProfile`. The contact query samples the actual near/far mesh triangle, including
root flare, bend and irregular cross-section, rather than an ideal cylinder. It returns a
mesh-local position and outward triangle normal. Live forehand reaching now uses this
query, as described below; whole-body attachment, climbing and descent remain unimplemented.

An omitted wind time selects the rest mesh. Supplying the renderer's time deforms each
triangle vertex with the trunk's wind weight/phase before interpolating the contact. This
ordering matters: applying nonlinear wind directly to an interpolated point would miss the
rendered triangle plane. Triangle normals are recomputed after deformation. The CPU equation
mirrors the vertex shader and consumes `JungleWind` amplitudes; shader-equation changes must
update the CPU equation and its independent regression fixture together.

The caller must select the tree's actual LOD and apply its uniform scale, yaw and translation
to the contact (rotation only for the unit normal). The query is parameterized by rest-ring
height and angle, not by a world-space ray. Branches, separate buttress meshes and crowns are
not included. Crossing LOD boundaries during attachment needs an explicit continuity policy.
This API alone does not establish reachable grips, limb IK, collision-free body poses,
turning stance locks, a route to the tree or safe dismount terrain.

The initial rest-surface revision passed the native build and all 37 non-display test groups
(58.79 seconds). Tests cover both triangles in every trunk cell, all eight variants, both
LODs, angular wrapping, unit/perpendicular normals and invalid coordinates. Contained native
capture `20261011T130518.614818Z-jungle` passed and was inspected: trunks/crowns remain
present, grounded and continuous in this view; kernel delta is empty. This is a geometry
refactor check, not evidence of monkey climbing or an island-wide realism audit. The view
still exposes stylized flat leaf clusters and repetitive sparse ground cover.

Wind regression additionally samples three times and checks deformed triangle barycentres
and normals, plus independent vertex-shader-equation fixtures. Current Android/Web runtime
qualification, grip visualization and an actual complete climbing cycle remain pending.
The final native build also passes. Its contained jungle capture
`20261011T131221.540034Z-jungle` passes and was inspected with the same continuous trunk
silhouettes and empty kernel delta. Since monkeys do not yet use the query, this image
checks renderer integration, not animated hand-to-bark contact accuracy.
The final wind-enabled revision passes all 37 non-display test groups (102.14 seconds;
`build/canopy-grip-wind-tests.log`). Hardware qualification stop latch remains unchanged.

## Ground approach to actual canopy placements

`IslandGame` installs actual `JungleLayout::Canopy()` placements into `MonkeyTroop`, after
world collision proxies. A periodic wander decision can select the nearest suitable tree
within 14 metres. The target is a staging point on the near side, outside the flared trunk
plus body clearance, inside the monkey's home range. Candidate straight paths require
terrain checks at intervals no larger than 25 cm and exact segment-to-expanded-circle
clearance against every solid proxy. This is not general pathfinding; obstructed candidates
are rejected and ordinary wandering remains available.

`ApproachTree` uses the existing quadrupedal ground gait, slows travel while turning and
caps steps at target distance. Every movement step now checks its swept segment against
solid proxies, including reduced-rate simulation; no tree obstacle is disabled. Within
12 cm of the staging point, `InspectTree` stops and smoothly faces the selected tree.
It currently returns to ordinary wandering after a short pause. A timeout or blocked step
abandons the attempt. Nearby players interrupt it with the existing escape behavior.
Replacing placements or obstacles cancels selected tree identities; invalid placements are
rejected before installation. Debug target lines and walking/observing counts include the
new states without representing them as climbing.

The long deterministic real-island CPU simulation installs identical actual canopy sets
into two troops and requires observed approach AND arrival. It checks matching identities,
arrival tolerance/zero speed, flared-root stand-off, interruption, replacement cancellation,
invalid input, terrain support, home bounds and sampled swept obstacle clearance. The first
approach-only core run passed in 35.49 seconds; it predates the swept-step strengthening.
Visual arrival sequences, attachment transition, planted grips and a complete ascent/rest/
descent cycle remain outstanding, as do current Android/Web qualification.

Contained native `20261011T131911.555963Z-monkeys` passes and was inspected; empty kernel
delta. The close quadrupedal monkey is visible with fur and ground-facing palms. This fixed
early capture shows ground locomotion, not a tree approach or arrival; the behavioral claim
above relies on the long simulation, not this picture. Coarse wood-like fur, simplified face
relief and shoulder joins remain visible defects rather than accepted realism.
The final approach/swept-collision revision passes all 37 non-display groups (85.59 seconds,
`build/monkey-tree-approach-tests.log`). Hardware stop latch is unchanged; no dependency or
emulator configuration changes were made.

## Spatial limb solving and bark pose model

`MonkeyGroundGait::SolveLimb` now handles arbitrary three-dimensional targets with a bend
hint, fixed upper/lower lengths and explicit unreachable-target clamping. Ground fitting
uses it too. The renderer replaces sagittal-only segment rotation with a rigid shortest-arc
rotation; the sole frame also handles a normal parallel to its usual forward direction.
This prepares circumferential grips without changing the 14-bone/four-contact palette.

`MonkeyClimbPose::Sample` is a CPU pose model, NOT yet called by the live troop/renderer.
It samples four requested targets on wind-deformed near/far trunk triangles, applies actual
tree scale/yaw/translation, adds palm clearance and solves each limb in a body frame with
forward up the trunk and dorsal direction away from bark. The torso is 8 cm closer to the
grip plane than its ground pose. A 22 cm stride and 80-percent support duty with staggered
limb phases retain at least three supports. Signed vertical travel must drive phase at
`CycleDistance * monkeyScale`; supporting contact parameters then stay fixed on the bark
while the body advances. Swing contacts lift away from bark. Bent/narrow trunks can exceed
reach: callers must reject/adjust those poses, never treat clamped targets as real grips.

CPU fixtures cover straight and bent trunk heights, eight forms, both LODs, two yaws,
three tree scales and three monkey scales. They check fixed bone lengths, reconstructed
world-space targets, actual bark clearance and non-sliding supports at fixed wind time.
Spatial segment tests include parallel and opposite directions; palm tests include vertical
normals. This is analytic contact evidence, not a visually verified climbing cycle.

Next integration must solve the attachment transition before enabling climbing: the current
ground staging point deliberately stays outside the root flare plus body clearance and is
too far away for a forehand grip. Add a safely checked closer stance, plant forehands while
hind feet remain on terrain, raise/rotate the torso, and transfer hind contacts to bark.
Reverse this for descent/dismount. Do not simply interpolate the entire animal into the air
or bypass the selected trunk's collision. Live gait phase, actual tree LOD and renderer wind
time must agree with the model. Body/branch clearance and a complete visual motion sequence
remain required, along with current portable runtime qualification and fur/anatomy refinement.

Native build and all 37 non-display groups pass (60.58 seconds;
`build/monkey-spatial-grips-tests.log`), including 46,080 climb-pose fixtures and asset
validation with the new generator header registered. Native ground capture
`20261011T132740.781674Z-monkeys` passes/inspected with empty kernel delta: the spatial
segment rotation retains quadrupedal limb geometry and ground-oriented palms. Grass partly
obscures contacts and the known coarse fur/flat face/shoulder defects remain. This image
does not show the CPU climbing pose model or establish attachment/climbing quality.

## Live ground-to-bark forehand transfer

`MonkeyTreeReach.hpp` now supplies the live inspection pose. Approach stand-off is the larger
of the flared-base radius plus 42 cm, or the upper-trunk radius envelope plus head/body
clearance scaled to the monkey. Both nominal forehand targets must be reachable before a
candidate tree is selected. Arrival tolerance is tightened from 12 to 4 cm. Inspection lasts
six seconds: first settle/face the trunk, transfer one hand then the other, retain both on
bark, then return them one at a time. The hind limbs keep their rendered-terrain pose.
The transfer schedule never swings both forehands simultaneously. This still ends by
wandering away, NOT by raising the body or climbing.

The renderer rechecks both targets against the actual terrain-fitted roots, falling back to
the ground pose when either is unreachable. Hands use spatial IK and normal-aligned palms.
Importantly, the renderer grounds canopy placements with `GroundJunglePlacement`: original
layout Y is not the final rendered tree Y. The grip query now receives this same corrected
placement. Wind time comes from the same draw clock as the jungle. Ordinary canopy LOD
distance selection is shared through `CanopyNearLodDistance`; the special forced canopy-LOD
inspection override is not qualified for this new hand-contact view.

New native QA viewpoint `monkey-tree` runs bounded ordinary troop simulation until some
monkey naturally reaches the paired-contact part of inspection. It then holds the troop
state for a close static contact inspection; tree wind remains live. It does not teleport
the monkey or create a test-only tree. The regular `monkeys` moving view is unchanged.
The capture gate additionally requires the selected visible subject, not another troop
member, to have both reachable bark grips applied. A positive submission marker still
requires actual pixel inspection.

The first run `20261011T133719.762250Z-monkey-tree` failed because it required the first
troop member to reach a suitable tree within the bounded simulation. Evidence is retained;
no image was produced and kernel delta is empty. Selecting the first naturally inspecting
member instead produced `20261011T133928.741268Z-monkey-tree`: pass, image inspected, empty
kernel delta. Both front palms visibly meet the trunk, with hind feet on the floor (partly
occluded by grass). Head remains outside the trunk. Boxy shoulder/elbow anatomy, wood-like
fur and simplified face are still clear defects. This held pose does not verify the moving
transfer sequence, full attachment, climbing or descent.

Native build and all 37 non-display groups pass (57.42 s;
`build/monkey-tree-reach-final-tests.log`); the 29 software-capture unit tests include
missing/zero/wrong-subject grip-marker rejection. Asset validation: 24 procedural records,
69 generator sources, 8 project files, zero external files. Stop latch unchanged. Current
Android/Web runtime qualification and a complete visual climbing cycle remain open.

## Reaching elbow pole correction

The held-contact audit exposed a bad bend direction, not just a coarse mesh silhouette.
A bounded first-contact diagnostic records model-space roots/joints/palms. In
`20261011T134622.185496Z-monkey-tree`, left shoulder Y was 0.567373, elbow Y 0.779067,
and palm Y 0.604692: retaining the ground gait's rearward bend hint as the palm moved
forward pitched the elbow upward. That capture passes but retains the visual defect;
kernel delta is empty. Framework sources were inspected read-only, not modified.

`MonkeyTreeReach::BendHint` now interpolates from the ground's rearward pole to a downward
reaching pole using the hand-transfer weight. The same palm target and fixed-length solver
remain. A 202-pose regression covers both arms throughout the transfer, requiring reachable
contacts and elbows below the shoulders. Native build and all 37 non-display groups pass
(60.58 seconds; `build/monkey-elbow-pole-tests.log`).

Final `20261011T135100.198476Z-monkey-tree` passes and was visually compared: the backward
angular protrusion is gone, the forearms meet the same bark contacts, and elbows now bend
below the shoulder-to-palm line. Recorded left elbow Y is 0.391113 with unchanged root and
palm coordinates. Empty kernel delta, unchanged stop latch. Shoulder integration remains
bulky and the wood-like fur/flat face defects remain. These are held-pose before/after
images, not a complete moving contact-transfer sequence or portable qualification. Body
attachment, ascent/descent and the larger island/wildlife plan remain unfinished.

## Eight sampled transfer stages from one natural inspection

Native `monkey-tree --capture` now replays the same bounded ordinary simulation from its
initial troop state, identifies the selected subject/tree/inspection start, and retains
eight actual states: before reaching, left transfer, right transfer, paired contact, left
return, right return, nearly released, and after inspection. No authored pose timer or
teleported animal substitutes for the simulation. Each state is rendered for four frames
and saved as `capture.ppm.tree-stage-N.ppm`. After stage 7 the capture returns to the held
paired-contact reference for the canonical image at frame 36. This reset is intentional,
not continuous playback. Contact shadows use the same sampled state as the monkey mesh.

The capture runner requires ordered stages, one subject and inspection, increasing clocks,
the actual renderer-applied hand weights and final tree release. All eight images also
pass the ordinary scene gate. Negative tests reject missing/duplicate/reordered stages,
changed subject/tree, inconsistent inspection clocks, non-finite clocks, wrong weights and
premature or absent release. These checks do not replace visual inspection.

Evidence `20261011T135905.681863Z-monkey-tree` passes with subject 4 and tree 118. Inspection
samples are at .133333, 1.46667, 2.13333, 2.8, 4.8, 5.46667 and 5.86667 seconds; the last
sample releases the tree. All eight images plus the final reference were inspected:
sequential transfers and returns are visible, hind feet stay grounded, and the head remains
outside the trunk. Grass partly hides ground palms. Bulky shoulders, wood-like fur, flat
face and a stiff tail remain visible defects. Wind is live draw-time wind, not historical
simulation wind. These sampled phases do not prove every intervening frame is smooth.

Native build and 37/37 non-display groups pass (`build/monkey-tree-sequence-tests.log`,
78.35 s); kernel delta is empty. Android/Web are not requalified. Full body attachment,
ascent, rest, descent and dismount remain unimplemented; do not call this tree climbing.

## Initial torso loading with mixed bark/ground support

`MonkeyTreeReach::RaiseTorso` rotates the body roots about the hind-hip axis while solving
all four limbs against their unchanged planted contacts. The renderer applies the same
rigid tilt to pelvis/spine/head/tail before replacing limb matrices with the fixed-contact
solutions. A smooth .22-radian lift occurs only while both hands are fully on bark, and
returns before either hand releases. Full-excursion reach is checked before lifting; an
unreachable excursion retains the previous grounded inspection pose. Hind feet do not
transfer to bark yet. This is the beginning of attachment, NOT climbing.

Native `20261011T140816.894255Z-monkey-tree` passed; its inspected partial-lift image and
diagnostic showed .14256 radians. Stage 3 now samples >=3.3 seconds instead of >=2.7 so
the maximum lift is visible. Final `20261011T141011.681705Z-monkey-tree` passes the eight
stage gates and has an empty kernel delta. Peak and returning-hand images were inspected:
torso/head rise while palms and hind feet retain contact, then lower before release.
Peak diagnostic is .22 radians; left shoulder Y .702676, palm Y .604693. Blocky shoulders,
wood-like fur, simplistic face and stiff tail remain defects. No continuous-animation,
full climbing or Android/Web qualification is claimed.

CPU regression samples 221 torso angles, requiring preserved contacts, fixed segment
lengths, unchanged hind roots and rising shoulders, plus rejection of unreachable/out-of-
range poses. The first test run failed because `220*.001f` slightly exceeded the exact
maximum float; the fixture now samples `(frame/220.0f)*.22f` and retains strict bounds.
Final native build and 37/37 non-display groups pass in 60.44 seconds
(`build/monkey-torso-loading-final-tests.log`); direct 30 capture-unit tests pass.
GPU stop latch SHA-256 remains unchanged. All launched jobs finished.
