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

## Mixed ground/bark attachment trajectory — CPU only

`MonkeyTreeAttachment.hpp` provides the missing transition between the current .22-radian
torso-loading pose and the existing vertical climbing pose. It is NOT wired into troop
behavior or rendering yet. Live monkeys still stop at the previously qualified inspection.
The ten-second trajectory has ten smooth zero-end-velocity segments:

1. Bring hips forward and raise the torso with all four contacts fixed.
2. Move the left hand to a higher bark grip.
3. Move the right hand to a higher bark grip.
4. Step the left hind foot forward on the ground.
5. Step the right hind foot forward on the ground.
6. Rotate/translate the body to the vertical frame with all four contacts fixed.
7. Transfer the left hind foot to bark.
8. Transfer the right hind foot to bark.
9. Move the left hand onto its exact initial climbing-cycle contact.
10. Move the right hand onto its exact initial climbing-cycle contact.

At most one contact moves; the other three remain fixed while roots move and fixed-length
IK resolves the limbs. Swing contacts lift .08 model units along their blended normal.
Reversing time traverses the same positions for dismount. Inputs are caller-supplied
grounded staging points, actual queried bark contacts/normals and final climbing hip frame.
No cylinder/flat-ground replacement is hidden in the planner. `IsReachable` samples 1,001
poses before any support is released, rejects clamping/non-finite data and invalid normals.
It is a bounded reach check, NOT a continuous collision or dynamic-wind proof.

The initial trial raised hands before moving the torso. Real trunk tests failed: at the
conservative approach distance, the higher target exceeded arm reach. The retained failure
is `build/monkey-attachment-fixtures.log`. Moving the body first fixes the ordering.
The revised fixtures use all eight trunk forms, both mesh LODs, three tree scales, three
monkey scales and two wind times: 208 candidate paths accepted, 80 rejected, with accepted
paths for every form/LOD. End roots and contacts coincide with `MonkeyClimbPose`, avoiding
a jump when that solver takes over. The independent 1,001-pose fixture checks bone lengths,
stationary supports and at least three supports. Invalid times/normals and an unreachable
intermediate foot target are rejected. The real-tree fixture uses flat staging terrain;
arbitrary terrain, yaw, obstacles and body clearance remain caller/integration obligations.

Native build and all 37 non-display groups pass (48.43 s,
`build/monkey-attachment-final-tests.log`). Asset inventory is 24/72/10/0. No graphics run
was needed for this CPU-only addition, and no visual or portable runtime claim is made.
Next: ground/collision-check actual staging feet; integrate an attachment state that keeps
the selected tree and returns safely if interrupted; apply the frame to body/limbs/normals,
culling/shadows consistently; then bounded ascent/rest/descent/dismount and real capture
sequences. Keep tree wind and contact phases coherent. Do not enable an unqualified path
just because its sampled reach check passes.
Final targeted regression also matches entry roots, joints and contacts to the current
`RaiseTorso(.22)` pose (`build/monkey-attachment-continuity-tests.log`, passed).

## Actual-island attachment-site preparation — still CPU only

`MonkeyAttachmentSite::Prepare` now constructs those planner inputs from a naturally
arrived monkey's origin, heading and scale, its selected canopy, rendered terrain and
world obstacle list. It regrounds the tree on rendered triangles rather than trusting
layout Y. Ground feet use sampled terrain height/normal; forehands and final climbing
contacts use the selected wind-deformed trunk mesh. Heading errors over .01 radians,
steep staging ground, unsuitable height and unreachable trajectories are rejected.
No caller position or heading is silently snapped to make a candidate pass.

After 1,001 reach samples, 501 clearance samples check all contact centres, four
normal-aligned sole/toe footprint corners per limb, and three torso/head sphere probes.
The footprint uses the sole renderer's projected forward direction and 1e-8 fallback
threshold. Terrain and bark are checked geometrically; nearby obstacle circles retain
contact padding. Only the selected tree's matching canopy proxy is exempted, because
its actual bark is checked separately. These are conservative sampled probes, NOT
continuous collision detection or full limb/body-mesh collision qualification.

The ordinary deterministic island simulation supplies 64 actual arrival candidates;
21 pass and 43 are rejected (16 initial reach, 2 height, 25 trajectory reach) in the
footprint regression. Accepted staging feet are independently checked against rendered
terrain. An extra obstacle at a staged foot rejects the path. Stale tree layout Y does
not change the grounded result; bad heading and zero monkey scale are rejected.
The current candidates use their real yaw and terrain, but fixed wind time 3 and near
trunk meshes; this is not exhaustive slope/yaw/wind/LOD or moving-wind qualification.

The helper is deliberately not called by the live troop or renderer yet. Integration
must preserve tree identity, safely reverse/descend on interruption, keep wind-aware
contacts coherent, and transform skin, sole frames, culling and shadows together.
Then audit actual attachment/ascent/rest/descent/dismount on all supported platforms.

Final native incremental build and all 37 non-display test groups pass (57.77 s,
`build/monkey-attachment-site-verified-tests.log`), including the final 1e-8 sole-frame
fallback threshold. Asset validation: 24 records, 73 generator sources, 10 project files,
zero external files. GPU stop latch unchanged; no graphics run or visual claim for this
CPU-only change. All launched jobs finished.

## Whole tree excursion and interruption clock — CPU only

`MonkeyTreeExcursion.hpp` composes the ten-second attachment with eight seconds of
ascent, three seconds of four-grip rest, eight seconds of descent and ten seconds of
reverse attachment. Eight complete climbing cycles raise the monkey 2.2 times its scale
in world metres above its attachment position. Height and gait phase are coupled;
smoothstep travel gives zero vertical speed at both ends, and integer phase at the
summit leaves all four limbs supporting. The single ground-relative coordinate frame
carries transformed roots, joints, contacts, normals and torso hip/pitch throughout.
The exit remains the original .22-radian planted inspection pose, not an immediate
walk pose: the existing grip-release/torso-lowering transition is still required.

All 21 accepted actual-island attachment sites also pass full-excursion reach sampling
at 0.01-second intervals. Independent checks cover roots, joints and sole continuity
at all four internal stage boundaries, fixed bone lengths, at least three supports,
four supports during summit rest, and ascent/descent contact retracing. Initial focused
Core run passed in 35.69 s (`build/monkey-excursion-tests.log`).

The interruption clock latches a return request. During attachment/ascent it reverses
the same trajectory back to its planted start; during rest it skips only the identical
rest pose; during descent/detachment it finishes the return. It does not clear the
selected tree or turn an airborne animal into a walker. Seven interruption times cover
all phases and check no initial contact teleport and bounded return to original grips.
An interruption currently reverses velocity immediately; smoothing this is still open.

Not integrated into live troop/renderer yet. Current preflight holds wind time and mesh
LOD fixed, and checks climb reach, not whole-ascent collision. Before live activation,
add moving-wind retargeting, upper-body/limb/terrain/obstacle clearance along ascent,
safe tree replacement handling, consistent renderer/sole/shadow/culling transforms and
actual whole-cycle visual review. These checks must not be replaced with a reach-only
acceptance claim. Existing eight-stage inspection captures do not qualify this cycle.

Final incremental build and all 37 non-display groups pass in 58.77 s
(`build/monkey-excursion-final-tests.log`), including the interruption clock regressions.
Asset inventory 24/74/10/0; unchanged GPU stop latch; all jobs finished. No new graphical
evidence is claimed for this CPU-only change.

## Wind-aware excursion retargeting — CPU only

`MonkeyTreeExcursion::Retarget` rebuilds bark-dependent attachment keys at a supplied
wind time and trunk mesh LOD. Initial and raised hands use fresh triangle queries;
final hands, hind feet and body frame match a freshly sampled climb entry. Grounded
hind feet and staging contacts/normals remain unchanged. The live-wind `Sample` overload
retargets attachment/detachment and queries current wind directly during climbing/rest.
It does not rerun the expensive site search, and does not imply collision acceptance.

The first regression exposed three failures when the far mesh moved an initial palm
beyond reach: the two-link solver clamped the sole and the rebuilt plan silently called
that a grip. Retargeting now explicitly rejects unreachable initial/raised-torso and
climbing-entry poses. The retained failing log is `build/monkey-excursion-wind-tests.log`.
Tests independently establish initial reach before expecting acceptance or refusal.
Changing LOD is therefore not automatically safe for an already attached monkey: runtime
integration must keep the supporting tree mesh coherent or qualify a continuous transfer,
never swap to a failed retarget or drop to a ground pose.

Tests exercise three wind times and both mesh queries, verify fresh hand targets and
unchanged ground feet, and ensure wind actually changes contacts. Valid transition
fixtures compare roots/joints/soles across stage boundaries while wind advances. A full
39-second advancing-wind near-mesh sample checks reach and bone lengths at every accepted
natural site. This is sampled coverage, not every wind phase or continuous collision
proof. Live troop/render integration and actual graphical qualification remain open.

After the refusal fix, final incremental native build and all 37 non-display groups pass
(38.52 s, `build/monkey-excursion-wind-final-tests.log`). Asset inventory remains 24/74/10/0;
GPU stop latch unchanged, all jobs finished. No graphics process was launched.

## Whole-excursion clearance preflight — CPU only

`MonkeyExcursionClearance::Validate` now follows the whole 39-second trajectory at
0.02-second intervals with advancing wind. Each pose checks four sole centres and
normal-aligned toe/sole corners, four interior samples on each upper and lower limb,
and three torso/head spheres. Upper/lower limb probes use .055/.035 model-unit radii;
these are coarse probes, not exact skinned sleeves. Terrain uses rendered triangles;
the selected trunk uses its current wind-deformed surface with angular queries relative
to the bent trunk centre. Other objects use existing conservative vertical circular
proxies. Only the selected tree's matching canopy proxy is exempted. Nearby obstacles
are filtered once with a bound including trunk lean and monkey size.

Failure returns the category, excursion time and probe index; an unreachable retarget
is rejected, never silently replaced with a ground pose. All 21 accepted actual-island
attachment sites pass this preflight at initial wind time 3. The first targeted Core run
passes in 34.44 s (`build/monkey-excursion-clearance-tests.log`). Negative fixtures insert
an obstacle on the bark-contact path, shift the entire pose/tree .02 m into unchanged
terrain, and supply an invalid frame. They must be refused as obstacle, terrain and
invalid-input failures respectively.

Limits: finite time/shape probes, not continuous swept collision, full mesh/tail collision
or all possible wind phases. The static world proxies overestimate some elevated objects.
This completes the initial full-cycle CPU preparation, NOT the live feature. Next wire
the existing prepared path and clock into troop/renderer, retain coherent supporting-tree
LOD, transform body/sole frames and culling/shadows together, then inspect actual in-game
attachment, ascent, rest, descent, interruption and release. Existing inspection captures
cannot stand in for that visual audit.

Final incremental build and all 37 non-display groups pass (54.09 s,
`build/monkey-excursion-clearance-final-tests.log`), including the negative fixtures.
Asset gate 24/75/10/0, stop latch unchanged, all jobs finished. No graphics run yet.

## Live troop/renderer integration and first actual cycle audit

Ordinary `MonkeyTroop` now attempts the prepared excursion once per natural inspection
after 3.2 seconds, only at full simulation distance and away from a nearby observer.
It samples the current wind time, rejects failed attachment/full-cycle clearance, and
stores the accepted immutable plan with a reversible clock. The original ground origin
is retained for transform continuity. During an excursion the ground walk clock and
heading remain fixed; the animation clock continues. A nearby player requests return
instead of replacing airborne support with `RunAway`. After return, 2.8 seconds of the
ordinary torso-lowering/hand-release inspection finishes before wandering or fleeing.
Canopy/obstacle replacement during attachment/release is rejected before mutating the
world snapshot; callers must let attached animals return before replacing supports.

The renderer applies current-wind excursion limbs, sole normals and full torso rotation/
translation to the 14-bone palette. Body position drives culling/lighting and the contact
shadow cache/position; ground-shadow opacity fades with elevation. Supporting trees stay
on the near mesh through attachment and release, avoiding the demonstrated coarse-LOD
unreachable grip. At most seven occupied trees gain this exception; it is not a global
LOD override. Live preflight currently runs synchronously and needs frame-cost profiling.

`--viewpoint monkey-climb` captures eight naturally reached stages from one replayed
excursion without teleporting an animal or fabricating a pose. It retains the summit as
the final reference frame. The existing `monkey-tree` view explicitly disables climbing
for its separate inspection-only regression; normal play leaves climbing enabled.
The software harness requires fixed subject/tree, expected ascending cycle times, more
than 1.5 m actual rise, return height and completed ground release; all eight images also
pass the unchanged image-content gate. New negative harness fixtures reject missing,
wrong-identity, wrong-phase, no-ascent, no-descent and unreleased sequences.

First native llvmpipe capture `20261011T154734.814785Z-monkey-climb` passes all eight
stages. Actual original-resolution summit and eight-stage contact sheet inspected:
subject 3 climbs tree 187 from body Y 12.7403 to 15.046, descends to 12.7416 and releases.
The body stays connected and visible; near-side hands/feet meet the trunk, while the
opposite contacts are partly occluded. It is not continuous-motion/all-angle proof.
Audit exposed a skyward-facing head on the vertical torso; a bounded counter-pitch now
aims the muzzle toward upcoming grips, with zero correction at the attachment boundary.
The first capture is retained as pre-correction evidence. Anatomy/facial silhouette,
visible joint transitions and stiff tail remain realism defects, not accepted as final.

Native integration suite: 37/37 non-display groups pass (82.01 s,
`build/monkey-live-climb-final-tests.log`); live tests require actual climbing/summit,
deterministic identity/clock, safe proximity return and rejection of support deletion.
The capture harness has 31 passing CPU tests. No current Web/Android climbing, physical
GPU performance, continuous animation or all-angle qualification is claimed.

Head correction rebuilt and final native `20261011T154923.604121Z-monkey-climb` passes
all eight gates. Original-resolution summit and full sequence inspected: muzzle now
faces toward upcoming grips, ascent/descent/release retained; same near-side contact and
far-side occlusion limits. Both kernel deltas empty. Asset gate 24/75/10/0, GPU stop latch
unchanged, all launched jobs finished. No Android/KVM or dependency changes.

## Android GLES climbing qualification

The `monkey-climb` stages now report even without a desktop `--capture` filename.
After all eight stages, frame 36 reports a held-summit presentation marker. Android's
existing bounded readiness gate additionally waits for that marker for this viewpoint;
its Python verifier requires the same subject/tree, phase times, ascent, descent and
release as the native harness. A first frame or material-loaded message is insufficient.
New negative Android log fixtures reject no cycle, no held summit and an omitted phase.
No renderer/image thresholds were relaxed, and other viewpoints retain their gates.

x86_64 build/lint passes (6 s). The existing isolated KVM/SwiftShader emulator passes
`20261011T155424.149773Z-monkey-climb` with GLES 3, PID 1874 / render TID 1915. All eight
stage log checks pass; screenshot accepted on its first presentation attempt. The actual
1280x720 held-summit PNG was inspected: connected body/limbs, fur and head counter-pitch
match native, visible near-side palms/feet meet bark; opposite contacts remain occluded.
Touch controls are visible but were not exercised in this fixed-camera run. This proves
the staged render diagnostics and inspected summit, not eight Android image inspections,
continuous gait, lifecycle recovery, physical-device speed or ARM64 runtime behavior.
Startup first draw 5601.262 ms / total 22765.587 ms includes QA warmup and software rendering.
Final kernel delta is empty; no shared adb, emulator configuration, KVM or dependency edits.

Retained emulator APK: `build/android-software/island-live-climb-x86_64.apk`, 21,251,403
bytes, SHA-256 `fc3c575c2b8d3461bf55faefcc7ffa44d6310ff3fd02847d05be4c4e577a8ed4`.
Default ARM64 build/lint restored (6 s): 21,313,652 bytes, SHA-256
`3434ee4d48a3db55f83a0b59a0ad30fce10fe78812ef2b11ac56959247b327d5`; three ARM64 libraries
and prepared fur present, runtime untested. Android harness 22 tests and native capture
harness 31 tests pass. Current Web climbing qualification and realism/performance work
remain open.

Native diagnostic-change rebuild and all 37 non-display groups pass (62.97 s,
`build/monkey-climb-platform-markers-tests.log`). Asset gate 24/75/10/0, GPU stop latch
unchanged, all jobs finished.

## Focused WebGL climbing gate

The contained Chrome harness now has a separate `monkey-climb` scope and launch mapping.
It waits for all eight staged renders and the held-summit marker, then captures the
summit and measures the unchanged 60 requestAnimationFrame intervals with GL error zero,
intact context, required materials and renderer diagnostics. The parent supervisor
independently parses the full-cycle identity/time/height/release invariants using the
same tested validator as native/Android. This is staged diagnostics plus held-summit
images, not continuous-motion or all-angle qualification. The ordinary `monkeys` scope
retains its separate walking-pose image sequence.

Existing Release Web build/package refreshed: 11,062,826 raw / 4,161,779 gzip bytes.
Prepared fur SHA remains `dc65039f7c3875ec2f33d930afc895b47f0443dc82b84c75fa7a056b8f0293ed`.
The first run `20261011T160143.966968Z-chrome-monkey-climb` failed a 40-second stage wait
after seven of eight healthy stages. Its terminal failure and empty kernel delta are
retained. The bounded wait is now 80 seconds; no phase, frame-count or pixel gate was
removed. The second run `20261011T160312.460229Z-chrome-monkey-climb` passes functional
checks, but actual screenshot inspection found the debug overlay covering the animal.
That image is NOT accepted for visual qualification. The QA launch now omits the overlay,
matching the ordinary monkey view, and a regression asserts that unobscured launch mapping.
Browser harness 14 CPU tests and Node syntax check pass.

Final `20261011T160632.181230Z-chrome-monkey-climb` passes in Chrome/155.0.8059.39,
host PID 239924, contained SwiftShader: eight cycle invariants, held-summit marker,
60 intervals, GL error 0, context intact, final kernel delta empty. Both actual 1280x720
unobscured summit PNGs (before/after intervals) inspected: fur, connected limbs and head
counter-pitch match native/Android; visible near-side grips meet bark. Opposite grips
remain occluded, and both images hold the same summit pose rather than document continuous
climbing. The two earlier attempts remain as rejected/limited evidence. Asset gate
24/75/10/0, GPU stop latch unchanged, all launched jobs finished. No dependency or emulator
configuration changes. Remaining work includes preflight frame cost, continuous/all-angle
motion and interruptions, face/joint/tail realism, and the wider island plan.
