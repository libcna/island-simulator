# island-simulator handoff

## User priorities — boar and shipwreck visual rework (2026-10-11)

Added explicit unchecked tasks near the top-level wildlife backlog in PLAN.md for better
pig/wild-boar anatomy and bristly coat/skin/snout/hoof textures, and for a more realistic
shipwreck with convincing construction, broken planks, weathered wood and rusty fittings.
The user permits original generated textures where useful. These are planning additions,
not implemented improvements. Require actual close/wide in-game visual review on native,
Android and Web; existing build/material diagnostics do not close either task. Keep the
previous monkey, additional wildlife, hut and resident work open.

## Latest — shorter fur source and angular side mapping, native + Android inspected

Built-in ImageGen source v2 is preserved with exact prompt and provenance under
`assets/art-studies/monkey-fur-albedo-v2*`; v1 untouched. First replacement attempt was
rejected for long strands. CMake/Android now prepare v2 at the existing 512-square path.
`MonkeyFurUv` replaces collapsing cosine side projection with angular mirrored interior
UVs on body/head/limbs/tail; no texture repetition or shader/budget change. CPU density,
closure and range checks added. Native build and 37/37 non-display groups pass (48.27 s).
Asset gate passes 24 records / 70 generators / 10 project files / zero external assets.

Native `20261011T141731.945186Z-monkey-tree` and `20261011T141831.862271Z-monkeys`
pass, peak/walking images inspected; shorter finer coat instead of broad wood-like streaks.
Android x86_64 build/lint passes in 10 s; isolated GLES
`20261011T142013.159547Z-monkeys` passes first presentation attempt and image inspected.
All three kernel deltas empty. Tested APK retained as
`build/android-software/island-monkey-fur-v2-x86_64.apk` (21,158,731 bytes), SHA-256
`fa80bf806c7540f9fafc71a010b38909d5730ab4e7f5ba92d09eeef7f0abde2b`.
Default ARM64 build/lint restored (9 s), 21,233,236-byte APK SHA-256
`d6078c5ef061eb3127e2c7193758f8ef835a230ba37a05bcc7740970c12cb0aa`; ARM64 runtime untested.
All launched jobs finished, stop latch unchanged, no dependency/host/KVM configuration edits.
This qualifies current ordinary model/material on software GLES, not tree-transfer poses,
lifecycle, physical-device performance or continuous animation. Web remains outstanding.
Coarse shoulders, simple face/silhouette and stiff tail remain; hind-foot bark transfer and
ascent/rest/descent/dismount are still missing. Goal and all wider island tasks stay active.

## Earlier — initial torso lift while four contacts remain planted

Live inspection now raises the torso .22 radians about the hind hips only while both
forehands grip bark, returning before hand release. Fixed-target IK preserves all four
contacts; full-excursion reach is prechecked. Hind feet stay on terrain: full attachment,
ascent/rest/descent/dismount are still missing. No shader/bone budget change.
Peak capture stage moved from >=2.7 to >=3.3 seconds. Final native evidence
`20261011T141011.681705Z-monkey-tree` passes all eight image/contact gates, kernel delta
empty; peak and return images inspected. Peak diagnostic angle .22, left shoulder Y
.702676 and palm Y .604693. Earlier partial-lift capture `20261011T140816.894255Z-monkey-tree`
also passes/inspected, empty kernel delta. Fur/shoulder/face/tail defects remain.
See contact-foundation doc for CPU fixtures and the corrected float-boundary test failure.
Final native build and 37/37 non-display groups pass (60.44 s,
`build/monkey-torso-loading-final-tests.log`); direct 30 capture-unit tests pass. All jobs
finished, GPU stop latch hash unchanged, no dependency or emulator configuration edits.
Android/Web remain unqualified for these changes. Goal remains active.

## Earlier — eight-stage native tree-transfer visual audit

The source changes add eight sampled states from the same naturally reached
inspection to native `monkey-tree` capture, plus strict sequence/contact-weight validation.
See `docs/monkey-climb-contact-foundation.md` for capture semantics and limitations.
`20261011T135905.681863Z-monkey-tree` passes; all eight stage images and final reference
were inspected, kernel delta empty. Native build and 37/37 non-display tests pass (78.35 s).
Sequential hand transfer/return is visible; hind feet remain grounded. This is not full
climbing or continuous-frame qualification. Bulky shoulders, wooden-looking fur, flat
face and stiff tail remain. Android/Web qualification is still outstanding. Next implement
body attachment with mixed ground/bark supports, then ascent/rest/descent/dismount; retain
the larger wildlife, hut, resident, boar and wreck tasks. Goal remains active.

## Earlier — reaching elbow pole fixed after actual-pose diagnosis

`MonkeyTreeReach::BendHint(weight)` continuously changes the ground rearward bend direction
to downward during a forehand transfer. Retaining the ground pole had pitched elbows above
the shoulders when palms moved forward. Added a bounded first-contact root/joint/palm
diagnostic to MonkeyRenderer; no shader/bone-budget changes or dependency edits.
Native pre-fix `20261011T134622.185496Z-monkey-tree` records left root Y .567373,
elbow Y .779067, palm Y .604692. Final `20261011T135100.198476Z-monkey-tree` has elbow
Y .391113 with identical root/palm coordinates; image compared, backwards angular forearm
protrusion gone and hands retain bark contacts. Both kernel deltas empty; latch unchanged.
Shoulder blend remains bulky; wood-like fur and flat face still defective.

Native build and all 37 non-display groups pass (60.58 s). New 202-pose transfer regression
requires reachable, downward-bending elbows through the blend. All jobs finished. No
Android/Web runtime qualification, no full moving-sequence audit claimed: these are held
before/after poses. Next: capture the actual sequential hand transfers (same naturally
reached subject/tree, including approach/return), then raise/rotate the body with mixed
ground/bark supports and implement ascent/rest/descent/dismount. Full goal remains active.

## Latest — live forehand-to-bark transfer, native held contact inspected

`MonkeyTreeReach.hpp` provides closer stand-off, sequential forehand transfer weights and
model-space bark targets. `MonkeyTroop` prechecks nominal forehand reach, approaches within
4 cm, stores selected tree and inspection timers. Six-second `InspectTree`: settle, left
then right hand onto trunk, hold, sequentially return. Hind feet remain on terrain. Live
renderer checks actual terrain-fitted reach and applies spatial IK/palm normals only if
both full targets are reachable. No body raising, attachment/climbing/descent yet.

IMPORTANT discovered correction: JungleRenderer uses `GroundJunglePlacement`, overriding
layout Y with rendered-triangle terrain height. Hand queries now use that same grounding,
same draw-time wind and shared ordinary canopy LOD threshold. Special forced canopy-LOD
override is not contact-qualified. `MonkeyClimbPose` full-body model remains CPU-only.

New native `monkey-tree` QA viewpoint warms ordinary simulation (bounded 12000 steps),
selects a naturally inspecting troop member at >=2.7 seconds into inspection, then holds
troop state while wind stays live. Captures must show the selected subject with reachable
paired grips, not just another visible animal. First `20261011T133719.762250Z-monkey-tree`
failed because member zero never reached a suitable tree in the bounded warm-up; retained,
no image, empty kernel delta. Selection of any naturally inspecting member fixed QA setup.
Final `20261011T133928.741268Z-monkey-tree` passes/inspected: both forepalms meet bark,
hind feet remain on terrain (some grass occlusion), head outside trunk. Shoulder/elbow
blockiness, wood-like fur and flat face remain defects. This is a HELD pose, not a verified
moving transfer or full climbing sequence. Regular `monkeys` view remains moving.

Native build and all 37 non-display groups pass (57.42 s), plus direct 29 capture-unit tests.
Asset gate: 24 records / 69 generators / 8 project assets. All jobs finished, stop latch
unchanged; no dependency/host/emulator configuration changes. Android/Web NOT requalified.
Next: visual transfer sequence and body attachment with mixed ground/bark support, then
actual bounded ascent/rest/descent/dismount; improve exposed limb joins and fur/face too.
Full goal remains active; wildlife/hut/resident and entire visual audit remain unfinished.

## Latest — spatial limb solver and CPU bark-grip pose model

`MonkeyGroundGait::SolveLimb` handles arbitrary 3D targets, fixed .31/.33 bone lengths,
bend hints and explicit reach clamping. Ground fitting uses it. Renderer segment rotations
now use a general rigid shortest arc (`MonkeySoleFrame.hpp`), not X-axis-only rotations;
palm frames handle forward-parallel bark normals. Bone/uniform budgets are unchanged.

New `MonkeyClimbPose.hpp` (asset manifest registered) produces local limb poses plus world
contacts/normals/origin/frame from actual tree transforms and wind-deformed trunk triangles.
Model +Z points upward, +Y outward, +X circumferential. Root height offset -.08; stride .22,
support fraction .8, phase advances with signed vertical travel / (.275 * monkeyScale).
At least three supports; supporting tree coordinates stay fixed while root moves. Reach
clamping is diagnostic, NOT contact success. This model is only tested, NOT used by live
monkeys yet. Current live behavior remains ground approach/inspection then wandering.

Native build and all 37 non-display groups pass (60.58 s). 46,080 climbing fixtures cover
two heights (straight/bent), eight forms, both LODs, yaw, independent tree/monkey scales;
fixed lengths, target reconstruction, bark clearance and support locking pass. Segment
opposition/palm singularity tests pass. Native `20261011T132740.781674Z-monkeys` passes and
was inspected: retained ground quadruped geometry, grass partly masks contacts, existing
wood-like fur/flat face/shoulder defects remain. Empty kernel delta, stop latch unchanged.
No current Android/Web qualification; all jobs finished, no dependency/host/emulator changes.

Next must solve attachment: the present staging stand-off is too far for a forehand grip.
Use a collision-checked closer stance, forehands on bark while hind feet stay on terrain,
then raise/rotate torso and transfer hind contacts. Do not simply elevate the whole animal.
Need reachable mixed-support IK, body/root-flare clearance, reverse dismount and actual
visual motion evidence before claiming climbing. See `docs/monkey-climb-contact-foundation.md`.
Full goal/plan stays active and incomplete.

## Latest — actual canopy selection and ground approach (not attachment/climbing)

`MonkeyTroop::SetCanopy` now receives actual canopy placements from `IslandGame` after
obstacles. Periodic wander decisions select a suitable nearby tree and a near-side staging
point outside its flared root plus body clearance. Full segment/proxy clearance and sampled
terrain qualify candidate paths. Every actual movement step now also checks swept circular
collision, without exempting the selected tree. `ApproachTree` slows while turning and
stops within 12 cm; `InspectTree` faces the tree, pauses, then returns to wandering for now.
Timeouts/blocked steps abandon attempts; player proximity triggers escape. Replacing canopy
or obstacles cancels stale tree selection. No attachment/ascent/descent or limb grip exists.

The two repeated real-island test troops now use canopy placements and require actual tree
arrival, matching identities, stand-off, interruption and replacement cancellation. Existing
terrain/home/behavior coverage remains. Native build passes. First approach-only core test
passed (35.49 s); see `docs/monkey-climb-contact-foundation.md` for final evidence.
Native `20261011T131911.555963Z-monkeys` passes/inspected, empty kernel delta; this shows
early ground locomotion, NOT visual approach/arrival. Coarse fur/face/shoulder defects remain.
Android/Web current qualification and complete climbing remain open. Next: attachment/body
rotation and reachable bark grips, then bounded climbing/return with visual motion evidence.
Final build and all 37 non-display groups pass (85.59 s); all jobs finished. Stop latch
unchanged, no dependency/host/emulator configuration changes. Full goal remains active.

## Latest — shared trunk contact geometry, including wind (climbing not connected yet)

`docs/monkey-climb-contact-foundation.md` records the new shared near/far trunk vertices,
ring layout and barycentric triangle contact/normal query. Renderer consumes the same
rest vertices. Optional wind time deforms vertices before interpolation with the rendering
wind equation. Coordinates are mesh-local; tree instance transforms and actual LOD must
be supplied by future callers. Branches/buttresses are not covered. No monkey uses the
query yet: actual approach, attachment, reachable limb poses, ascent/descent and arboreal
movement remain unimplemented, as do moving ground stance locks.

Native build passes. Rest-surface suite passed 37/37 (58.79 s); expanded wind tests cover
all eight variants/both LODs, both triangles in each cell, three wind times, normals,
periodicity, invalid inputs and independent shader-equation fixtures. Native jungle
captures `20261011T130518.614818Z-jungle` and `20261011T131221.540034Z-jungle` pass and
were inspected; empty kernel deltas. These are renderer integration checks, not climbing
evidence. No dependency/host/emulator changes. Android/Web qualification remains open.
Final wind-enabled build and all 37 non-display groups pass (102.14 s). All jobs finished;
hardware stop latch unchanged. The full plan/goal remains active and incomplete.
Next integration: `MonkeyTroop` currently receives only circular collision proxies via
`SetObstacles` (`IslandGame.cpp`), not tree identities/transforms. Supply actual canopy
placements before selecting approach targets; do not infer climbable trees from generic
obstacle circles. `MonkeyBehavior` still has only ground states. Preserve the existing
distance-driven ground gait while adding explicit approach/attach/climb/dismount states.

## Latest — connected monkey face replaces the two pale oval patches (native first pass)

`docs/monkey-face-profile.md` records scope/evidence. One twelve-row cheek/muzzle/chin surface
replaces separate orbital/muzzle ellipsoids; smaller eyes/nostrils, attached curved mouth
strip and muted skin palettes. Same head bone/draw structure. New profile included in asset
manifest (67 sources), CPU profile regression added. Native build and all 37 non-display
groups pass (93.53 s). `20261011T125510.820617Z-monkeys` passes/inspected, empty kernel delta.
Two bright oval patches no longer dominate the face, but it is still simple/low-relief and
needs sockets/lids/cheek transitions. Coarse fur and shoulder attachment remain defects.
All jobs finished, no dependency/host/emulator changes. Android/Web have NOT qualified this
face revision; Web also still lacks current contact/tail/camera qualification. Continue those
checks and actual stance locking/tree gripping/climbing. Whole goal/plan remains open.

## Latest — close side QA camera and Android contact shader verified

The `monkeys` camera now views from forward 0.6, side -2.1, height 1.1, target height 0.52.
No vegetation/behaviour hidden or altered. Candidate `123555.508886Z` passed image checks but
was visually rejected because a shrub masked feet; `123834.120032Z` failed image variance.
Both retained. Final native `20261011T124224.778948Z-monkeys` passes/inspected: front palms
clearly visible along slope, rear partly masked. Empty kernel deltas, unchanged image gates.
Native build and four targeted monkey/CLI groups pass. Prior full sole-frame suite was 37/37.

Android x86_64 assembly/lint and `20261011T124529.157229Z-monkeys` PASS: new `uSoles[4]`
shader packaged, side-view image inspected with visible front palms/rear foot. First image
attempt accepted, no shader/GL/reach-limit errors, empty kernel delta. Tested APK retained;
ARM64 assembly/lint restored and ZIP checked. Hashes/scope in `docs/monkey-sole-alignment.md`.
All jobs finished. No dependency/host/KVM/emulator changes. The intermittent earlier black
presentation remains unresolved. Web has NOT qualified current contact shader/tail/camera.
NEXT: focused Web sequence with 600-second bound; face-mask/shoulder/coarse fur refinement,
turning stance locks and actual tree-contact/climbing. Whole goal and wildlife/hut/resident
backlog remain open. The new close camera makes the toy-like face and coarse/wavy fur obvious.

## Latest — independent palm/sole frames (native first pass, portable recheck required)

See `docs/monkey-sole-alignment.md`. Four contact matrices now orient hand/foot mesh to each
rendered ground normal rather than inheriting lower-limb rotation. Blend-index W tags 1–4
select these matrices; zero remains ordinary skeleton skinning. Fourteen skeleton bones
unchanged; conservative vertex uniform budget now 92 vectors. Sole-height compensation
preserves tangent-plane underside clearance. No extra draws/textures/dependency changes.
Native build and all 37 non-display groups pass (81.08 s), including new CNA-math-only sole
frame tests and shader dialect/budget contracts. Native `20261011T123046.817709Z-monkeys`
passes/inspected: visible geometry, but grass masks some digits. Kernel only perf throttle,
no GPU fault. Asset manifest passes with 66 generator sources. All jobs finished.
NEXT: requalify this new contact shader on Android/Web and use unobscured close/moving
contact views; continue stance locking, face/shoulder anatomy and real tree gripping/climbing.
Prior Android/Web passes do NOT qualify these new contact matrices. Whole goal/plan open.

## Latest — current monkey model qualified on Android; first black presentation unresolved

See `docs/monkey-android-current-model.md` for hashes, both runs and scope. Current ca6b042
x86_64 assembly/lint passes. `20261011T121437.574103Z-monkeys` FAILED: six all-black images
in 30 seconds despite fur-load/first-draw markers. Fresh failure-log retention was added and
CLI presentation wait extended to 60 seconds without changing image/GL gates. Identical APK
`20261011T121912.364377Z-monkeys` PASSES on its FIRST screenshot attempt, image inspected:
current quadruped/fur/terrain-target/tail visible. Empty kernel deltas in both. No claim that
the longer wait fixed the failure; cause remains unknown/intermittent. Both runs retained.
Shell and Python startup gates require the fur-load marker; APK hashes all three textures.
22 Android CPU tests and shell syntax pass. Tested x86 APK retained separately; default
ARM64 assembly/lint restored, three ARM64 libraries and three textures verified. No physical
ARM64/lifecycle qualification. All jobs finished; no emulator/KVM/dependency changes.

NEXT: improve toy-mask face/shoulder form, independent palm/sole alignment and stance locks,
then actual tree-contact/climbing; Web current-tail verification is still pending (its last
passing motion series used the prior tail). Keep whole-plan audit/wildlife/hut/resident open.

## Latest — Web anatomy/fur/contact qualified; continuous tail native first pass

`20261011T120413.407382Z-chrome-monkeys` PASSES with a 600-second bound, all sixty
intervals retained: GL error zero, intact context, no app error or reach warning. Start,
four intermediate and final images inspected. Ten unrelated AppArmor kernel audits, no
GPU fault. This qualifies the existing 05c3824 anatomy/fur/contact renderer, NOT the tail
change authored while that browser was running. Older timed-out evidence remains intact.

New `MonkeyTailProfile.hpp` and `AddTail` replace three disconnected rigid tubes with one
smoothly weighted tapered mesh, eighteen near/nine far longitudinal spans, same three tail
bones and same fourteen-bone palette. No new draw calls/textures. Profile/gait CPU test
passes; native build and `20261011T121030.039628Z-monkeys` pass. Image inspected with
continuous taper/silhouette, empty kernel delta. This static native image does not qualify
tail motion or Android/Web. Asset validator passes with 65 generator sources.
All 36 non-display groups pass (79.24 s); all browser/build/capture/test jobs finished.
NEXT: finish current-tail portable qualification, continue face/shoulder realism and full
palm/sole contact, then actual tree-contact/climbing. Android is still the older shader-only
APK, not these anatomy/material/contact revisions. Do not label the monkeys or whole plan
complete. No dependencies, host graphics settings, KVM or emulator configuration changed.

## Latest — Web moving-view audit captured, runtime deadline NOT passed

Current anatomy/fur/contact Web build succeeds; package refreshed (10,927,110 raw /
4,083,870 gzip bytes). Focused monkey runner adds four intermediate images after eight
frames each, plus original endpoints and sixty measured intervals. Syntax and 13 launcher
CPU tests pass. `20261011T115524.501474Z-chrome-monkeys` TERMINATED at the 360-second
supervisor deadline; do NOT call it qualified. Start + four intermediate images inspected:
visible textured quadruped changes position/heading/limb pose; toy-like face mask, angular
tail and assembled shoulder remain explicit defects. No final GL/interval report exists.
Final kernel delta only keyboard registration/perf throttling, no GPU fault. All jobs finished.
No current job to poll/restart. Keep failed evidence unchanged. Newly added partial checkpoint
writing will preserve page state during the next attempt; it always says complete:false.
NEXT: fresh bounded 600-second focused Web run for the longer sequence, retaining all sixty
intervals and graphics gates; then Android current-model qualification and contact/climbing.
See `docs/monkey-quadruped-overhaul.md`. Goal and whole-plan scope remain unchanged/open.

## Latest — individual rendered-terrain targets for monkey limbs (native first pass)

`MonkeyGroundGait::FitGround` now refits all four fixed-length chains to individual
rendered ground heights, preserving swing clearance and lifting/lowering the trunk within
the common reachable interval. Impossible terrain reports reach limitation and preserves
bones instead of stretching. `MonkeyRenderer` samples its cached RenderedTerrainSurface
at each yaw/scale-transformed sole and applies the body offset to the pelvis hierarchy.
Native build and all 36 non-display groups pass (82.60 s). The added real-island test
checks all seven spawn footprints at eight yaws and forty gait phases; separate gait/shader
recheck passes 2/2. Synthetic tests cover slopes and an impossible cliff. Asset gate passes.
Capture `20261011T115107.984389Z-monkeys` passes and was inspected: visible quadruped/fur,
no reach-limit diagnostic, empty kernel delta. Static frame only, not motion acceptance.
All jobs finished. No dependency/host/emulator changes, GPU stop latch unchanged.

Remaining contact work is explicit in `docs/monkey-quadruped-overhaul.md`: hands/feet still
inherit lower-limb orientation; independent palm/sole alignment, turning stance locks,
trunk pitch/roll and silhouette/contact-shadow refinements remain. Actual tree climbing is
still absent. NEXT: moving-sequence and Android/Web qualification of current anatomy/fur/
contact, then continue full monkey contact/climbing work. Whole plan/goal remains open.

## Latest — first real monkey fur material integrated (native inspected)

See `docs/monkey-fur-material.md`. Built-in ImageGen source and exact prompt preserved
in `assets/art-studies/monkey-fur-albedo-v1*`, hashed in manifest and attributed.
512-square build preparation wired into native/Web/Android packaging; runtime ten mips,
required asset, rest-mesh UVs, body-only mean-normalized diffuse modulation. Face/eyes
excluded. Native build and `20261011T114648.821520Z-monkeys` pass; image inspected,
fur variation visible, but face/limb junctions still stylized. Empty kernel delta.
Native gate now requires fur-load marker as well as a close subject at readback;
focused Web gate requires the marker, Android APK attribution includes the third texture.
Native non-display suite passes 36/36 (69.53 s); direct Android/launcher CPU tests pass
21/29 respectively, asset validation passes and Web runner syntax passes. The actual-fur
pixel-oracle test was added after the full suite started; separate material/shader recheck
passes 2/2 (9.98 s), including the actual fur source. All build/test/capture jobs finished.
Portable runtime qualification and movement review remain pending. No emulator/KVM or
dependency changes. Keep the entire goal active; monkey realism, tree climbing, full
visual audit, remaining wildlife, hut and human resident are NOT complete.

## Latest — quadrupedal mesh and distance-driven ground pose are live (native first pass)

`docs/monkey-quadruped-overhaul.md` records the actual scope and deficiencies. Horizontal
torso/neck loft, continuous dual-weight limb skins and smaller face replace the upright model.
`MonkeyState.gaitPhase/gaitAmount` now drive the ground IK from actual travel, not a clock.
Native build and contained `20261011T113355.740100Z-monkeys` pass; image inspected with a
visibly horizontal quadrupedal animal, empty kernel delta. This is NOT complete monkey realism:
fur texture and climbing are still absent; flat-plane sole supports/turning/contact need work.
Native test run: 35/36 pass; the CPU supervisor fixture missed its four-second startup marker.
Unchanged standalone supervisor recheck passes (1.95 s), recorded in
`build/monkey-quadruped-supervisor-recheck.log`; all jobs finished.
No safety gate was weakened. No native/Android/browser renderer remains running.

NEXT: actual documented fur texture and stable UVs/filtering, then per-limb rendered-terrain
support and tree-contact/climbing. Portable anatomy verification is pending; do not reuse
the older Android/Web shader-fix images as qualification for this changed model. The full
island visual audit, wildlife backlog, hut and resident are all still open.

## Latest — monkey overhaul started, NOT yet integrated into the live model

Operator explicitly rejected the toy-like brown primitives and human-like upright walk.
Requirements now recorded in PLAN: coherent primate anatomy, actual fur texture, quadrupedal
locomotion and climbing actual trees with gripping/contact. Do not substitute a colour change
or shader success for these requirements. Full-island visual audit and all unimplemented
wildlife (including snakes, spiders and goats), hut and newly requested human resident remain
open. The active goal is to implement the whole plan, then repeatedly audit and improve.

First implementation foundation: `MonkeyGroundGait.hpp` provides four phased support/return
trajectories and two-link sagittal IK with fixed segment lengths; idle-to-walk amplitude and
support-phase root-travel cancellation. `IslandMonkeyGroundGaitTests` passes (601 phase samples
at three amplitudes, periodicity, two-or-more supports, sole heights and segment lengths).
Native incremental build passes. This helper is NOT connected to MonkeyRenderer yet: the live
model/animation/images are unchanged. No graphics job is running. No new image was generated.

NEXT: replace the upright rest rig and disconnected primitive body with a coherent horizontal
quadrupedal body, attach this pose solver with speed synchronization and terrain support, then
qualify real captures. Integrate a documented fur texture, then implement approach/grip/climb/
rest/descent on actual tree geometry; verify full cycles, not merely a vertical translation.
ImageGen skill was inspected in the preceding conversation but generation has not started;
read/use the skill when working on the raster fur asset. Dependencies remain read-only.

## Latest — 14-bone monkey shader verified on native, Android and Web

Android x86_64 assembly/lint and `20261011T110417.653056Z-monkeys` pass strict graphics
gates; close monkey visible, image inspected, no shader/guest GL errors, empty final kernel.
Tested APK retained separately and default ARM64 assembly/lint restored; exact hashes in
`docs/monkey-palette-workaround.md`. Native also qualified below. Physical ARM64/lifecycle
remain unqualified. Focused Chrome `20261011T110957.242647Z-chrome-monkeys` passes
60 intervals, zero GL error, intact context; before/after images inspected with visible
close geometry and changed pose/location, empty final kernel. Web package refreshed:
10,366,754 raw / 3,534,595 gzip bytes. 13 browser-launcher tests and Node syntax pass.
ALL jobs finished, including Web sessions 51546/70348; do not poll or restart them.
No temporary diagnostic shader/depth/draw substitutions remain. This is a rendering fix,
NOT a realism milestone: the old model is still visibly toy-like. Next: continue existing
content realism (monkey form/material/contact or terrain backlog), retaining the same safety
boundaries. Full browser input/lifecycle, Firefox and Android lifecycle remain separate open work.

## Latest — native monkey visibility restored; Android/Web still pending

Read the latest section of `docs/monkey-palette-workaround.md`, which supersedes earlier
visibility hypotheses below. QA camera now follows subject translation; replacement shader
now converts XNA clip depth to GL, matching terrain. Normal native close monkey is visible
in `20261011T110027.440307Z-monkeys` (inspected, empty kernel delta). Native 35/35 non-display
tests pass in 74.01 s. Temporary magenta/bind-pose/depth-disabled/screen diagnostics removed.
Native harness additionally requires a close-model submission at capture time; final
`20261011T110247.359170Z-monkeys` passes (2 visible / 1 near / 4 draws), inspected,
empty kernel. 29 software-launcher and 21 Android device-free tests pass.
No dependency/host/emulator settings changed, hardware latch unchanged.
Current Android default APK is still the older ARM64 diagnostic build. Next: x86 build/lint,
strict software close capture, retain tested APK and restore ARM64; then Web verification.

## Update — WIP 14-bone shader; native visibility still unresolved

Uncommitted implementation in `MonkeySkinningShader.hpp`, MonkeyRenderer, shader contract test,
CMake/presets and launcher marker updates. Read `docs/monkey-palette-workaround.md` before
continuing. Native build/first 35/35 CPU groups pass, but replacement captures show no monkeys.
Crucially temporary native STOCK comparison is also empty; its bypass was removed. Do not
claim shader acceptance or infer that the replacement alone caused invisibility.
Latest contained probe `20261011T103724.288807Z-monkeys` finished (session `77749`), CPU
skinned pelvis is correctly near subject, camera above terrain. Other build/test/capture
sessions also finished. No jobs live. Current native binary is replacement + bounded probes.
NEXT: investigate vertex/palette binding or occlusion on native before Android qualification.
Android default remains the restored ARM64 diagnostic build, not the new monkey shader.

## Update — confirmed Android missing-monkey shader failure

Diagnostic monkey run `20261011T101527.286817Z-monkeys` FINISHED (session `51286`).
New stderr mirror captured stock `skinned (vertex-lit) link failed`, active vertex uniforms
exceed 256; the inspected image has no monkeys. Old run.json says passed, but that is a
documented FALSE POSITIVE, not acceptance; preserve it unchanged. Empty kernel delta.
`CNA-ISLAND-002` is now confirmed in `BUGS.md`. Dependencies remain read-only.
Harness now rejects stock VS/FS/link and nonzero guest GL errors at all four checkpoints;
21 Android CPU tests and shell syntax pass. Existing swimming fixture delimiter updated.
Diagnostic x86 APK preserved separately at `build/android-software/island-diagnostic-x86_64.apk`.
Default ARM64 assembly/lint restored (20,533,364 bytes, SHA-256
`931349af0a37149cee31154c6b4e8409a562eb725abf48963f9d599d94d6e6f2`), physical runtime untested.
No jobs remain running. NEXT: implement consumer-only 14-bone monkey shader workaround,
preserving animation, material, canopy shade and fog; qualify close geometry/motion on all
three platforms. Current stock shader declares 72 mat4 bones; game rig has only 14.
Do not hide monkeys, change dependency pins or claim earlier generic image gates prove visibility.

## Update — Web surface-frame materials complete; Android stderr mirror prepared

Browser `20261011T100522.314132Z-chrome-materials` passes all four views, 60 intervals
each, GL error zero. All four images inspected; final kernel 12 AppArmor audit + one
audit-suppression notice, no GPU fault, latch unchanged. Session `21871` FINISHED exit zero.
See `docs/terrain-surface-frame.md`. No full input/lifecycle/Firefox qualification implied.

Added consumer-only Android stderr mirror before game construction: complete lines/chunks,
2 KiB buffer, at most 128 chunks plus a limit notice, original stderr always preserved.
No dependency/rendering changes. Native build and 34/34 non-display tests pass (52.34 s),
including chunking, fragmented unitbuf lines, trailing text, stream restoration and log cap.
New test added to explicit debug/sanitizer preset targets. Android build/runtime verification
is next; capture the actual shader/link message for suspected `CNA-ISLAND-002`.
Android x86_64 assembly/lint passes. Monkey close-view run is ACTIVE in session `51286`;
poll the same handle. Launcher log: `build/diagnostic-mirror-android-monkeys.log`.
After inspection, preserve the diagnostic x86 APK separately and restore default ARM64
assembly/lint. The rolling tested APK still holds the earlier material-qualified build.

## Update — Android guest uniform diagnostics investigated read-only

See suspected `CNA-ISLAND-002` in `BUGS.md`: stock CompileAndLink logs failure but continues
uniform queries; the vertex-lit skinned program has 24+3 queries, matching the 27-message
burst. This is a candidate, NOT confirmed program attribution or a driver/uniform-limit defect.
Next targeted evidence: contained monkey close view and actual link diagnostic, after Web ends.
Browser materials session `21871` is still live; litter and grass screenshots are now present.
Litter image inspected and consistent with native. Do not restart or run concurrent graphics.

## Update — surface-frame Android visual checks; Web material scope prepared

Game revision `3642ac1` / capture harness `90fc6a4`: x86_64 assembly/lint and litter
`20261011T095903.886091Z-litter`, wet-sand `20261011T100107.688712Z-wet-sand` gates pass.
Both images inspected, first screenshot attempt; empty final kernel deltas. ARM64 build/lint
restored, tested x86 APK retained; exact hashes in `docs/terrain-surface-frame.md`.
CAVEAT: litter has 27 guest gfxstream uniform-query 0x502 diagnostics, also seen in the
older `20261011T092539.275219Z-interactive`; wet-sand has none. Root cause/ownership unknown;
passed image/startup gates do not mean zero GL errors. Android lifecycle remains open.
Added Chrome `materials` scope: litter, grass, bay-ground, wet-sand, 60 intervals each;
12 browser-launcher tests and Node syntax check pass. Web runtime qualification pending.
Web build/package complete: 10,347,083 raw / 3,528,841 gzip bytes. Browser materials run
is ACTIVE in session `21871`; poll the same handle, do not restart on observation timeout.
Launcher output: `build/surface-frame-web-materials.log`. Build session `8078` is complete.


## Update — explicit shaded-floor capture profile verified

The litter image discrepancy below is resolved by an explicit `shaded-detail` profile,
not a lighting change or global gate reduction. See `docs/terrain-surface-frame.md`.
Fresh native `20261011T095340.833878Z-litter` passes and was inspected; empty kernel delta,
latch unchanged. The old failed run remains failed and untouched. Six validator and twenty
Android CPU-only tests pass; native/Android launchers select the profile from camera arguments.
Full non-display CTest also passes 33/33 (49.94 s); no jobs remain running.
NEXT: Android/Web material qualification of game revision `3642ac1` remains pending.
No emulator/KVM configuration change, hardware rendering or push is authorized.

## Update — surface normal fix built; litter image gate remains open

`docs/terrain-surface-frame.md`: scale-first CPU/GLSL normal helper now drives sand,
soil clods and wet sheen; new `wet-sand` camera and mappings. Native build and 33/33
non-display tests pass (42.70 s), including 1e-30..1e30 scale/winding invariance.
Fingerprint still `5d0b62ad43a25895`, grass 17,225. Native wet-sand
`20261011T094128.539261Z-wet-sand`, grass `20261011T094154.489156Z-grass`, bay-ground
`20261011T094225.864744Z-bay-ground`, water `20261011T094616.083800Z-water` pass and
were inspected; all kernel deltas empty, latch unchanged. No jobs running.

NEXT: resolve retained litter capture `20261011T094405.569362Z-litter`, which FAILED
the generic image gate: luminance deviation 16.8656 < 18, mean 58.8703, 240 bins,
spatial contrast 37.2447; other gates pass. Inspected image is complete textured shaded
floor, not blank. Do not call it passed. Validator unchanged. Evaluate appropriate
image criteria/reference evidence without adjusting lighting just to satisfy a threshold.
Then Android/Web material qualification for this fix; prior `9f1ee59` results do not
qualify it. Android lifecycle remains open and hardware latch must remain set.

## Update — sand/turf Android and Web qualification complete

Current game revision `9f1ee59`. x86_64 assembly/lint passes. Bay-ground
`20261011T092336.875730Z-bay-ground` passes the image gate and matches native visually.
Interactive `20261011T092539.275219Z-interactive` passes sensitivity/footsteps/swimming
and transition audio after one gesture; image inspected. Both kernel deltas empty,
latch unchanged. Lifecycle deliberately excluded, still degraded. Tested x86 APK
preserved, default ARM64 build/lint restored. Exact sizes/hashes in `docs/dry-sand-relief.md`.

Web build/package: 10,346,158 raw / 3,528,706 gzip bytes. Contained Chrome terrain
`20261011T092826.584773Z-chrome-terrain` passes seven views, 60 intervals each, GL error zero.
Bay-ground before/after and jungle images inspected. No GPU faults; final kernel delta
contains AppArmor audit and audit-suppression notices. Full input/lifecycle/Firefox not rerun.
Browser session `28597` and build `34267` completed; do not poll/restart them.
This qualifies game revision `9f1ee59`, not newer uncommitted normal/camera changes above.
These are software checks, not host AMD or physical-device acceptance.

## Update — dry sand relief native stage complete

`docs/dry-sand-relief.md`: wind-aligned warped ripples plus footprint-filtered grain
shading. Native build succeeds, non-display 33/33 pass (50.10 s); final shader refinements
pass material/CLI/browser-preflight checks. Initial bright foreground rejected and fixed
by correcting the sand normal-length floor. Final bay-ground
`20261011T092129.366965Z-bay-ground` and bay `20261011T092157.241700Z-bay` pass and
were inspected; empty kernel deltas, latch unchanged. No geometry/population changes.
Web terrain suite now has seven views, including `bay-ground`. No jobs running.
NEXT: qualify BOTH this shader and preceding coastal turf on Android (bay-ground and
focused touch/traversal, restore ARM64) and Web (build/package, seven-view terrain matrix).
Earlier beach qualification does not qualify these newer stages. Static native images
do not prove temporal stability. Soil/wet-normal length floors deserve separate review;
they were not changed here. Full environment/geology and Android lifecycle remain open.

## Update — coastal turf native stage complete

`docs/coastal-turf.md`: shared CPU/generated-GLSL sand/rock suppression for grass and
turf, plus hard sandy-apron exclusion before the density floor/route boost (parents and
companions). New `bay-ground` eye-height camera and platform mappings. Native build and
final 33/33 non-display groups pass (52.01 s); CLI help test updated for the new camera.
Captures bay-ground `20261011T091417.768738Z-bay-ground`, grass
`20261011T091436.406649Z-grass`, bay `20261011T091521.122820Z-bay` pass and were
inspected; all kernel deltas empty, latch unchanged. Fingerprint `5d0b62ad43a25895`,
17,225 grass patches (two fewer), other populations unchanged, rendered root gaps zero.
Browser grass count pin updated. No jobs remain running.

NEXT: Android/Web qualification of the new turf/camera revision. The previous beach
results below do not qualify it. Ground-level bay review also identified overly smooth
dry sand as the next material target (filtered wind ripples/grain); that is not implemented.
Full environment migration/geology still open. Android lifecycle remains degraded.

## Update — Web wider-beach validation complete

Web build and package for committed `53a2442` complete: 10,343,092 raw / 3,528,055
gzip bytes. Contained Chrome terrain suite `20261011T085703.088366Z-chrome-terrain`
finished with all six views passing, 60 intervals each. Overview/bay/headland/east/jungle
images inspected; no new GPU faults, Wi-Fi bandwidth/EXT4 unmount/USB disconnect notices.
Full Web input/lifecycle
and Firefox not rerun. No browser job remains running; session `38693` completed.

Do not attribute this completed Web test to the newer coastal-turf revision above.

## Update — dominant bay beach native stage complete

`docs/dominant-bay-beach.md`: shared approximately 36 m sheltered bay / 6.5 m eastern
headland width now controls physical profile and sand material. Final 33/33 non-display
tests pass (53.06 s). Native bay `20261011T084410.221147Z-bay`, headland
`20261011T084654.624120Z-headland`, beach `20261011T084740.315381Z-beach` pass and
were inspected, all empty kernel deltas. Existing beach camera looks inland through
vegetation, so it is not a close sand review. Fingerprint `08ced4994845ae68`; measured
inventory pins updated in core and browser tests. Hardware latch unchanged.
Android x86_64 build/lint and interactive `20261011T085027.124944Z-interactive` pass
touch/footsteps/swimming after two gestures, image inspected. Initial bay run had a black
PNG despite old log-gate success. Added `tools/android_capture.py`: bounded 30-second
image-content validation for initial/hot/cold screenshots; 19 CPU-only Android tests pass.
Repeat `20261011T085244.623709Z-bay` passes strengthened gate and visually matches native.
No new GPU faults (Wi-Fi-only kernel deltas). Tested x86 APK preserved; ARM64 build/lint
restored. See `docs/dominant-bay-beach.md` for hashes. Web work is tracked above.
Earlier rocky-coast runs do not
qualify this newer code. A dedicated ground-level bay view is useful follow-up work.
Android lifecycle remains degraded; do not close the known issue from focused visual runs.
No Android/KVM system configuration changes are needed or requested. No push.

## Update — rocky coast Web qualification complete

Web build/package refreshed: 10,343,035 raw / 3,528,104 gzip bytes. Contained Chrome
terrain suite `20261011T083120.118782Z-chrome-terrain` passes all six views: overview,
bay, headland, beach, east, jungle, 60 intervals each and GL error zero. Bay/headland/east/
jungle images inspected. No new GPU faults; kernel delta contains USB/storage, PCIe and
perf notices. Full Web input/lifecycle and Firefox not rerun. This qualifies `2a660b7`.

Historical source preparation (now completed in the native stage above): changes in
`IslandEnvironmentField.hpp`, `IslandTerrain.cpp`, `TerrainRenderer.cpp` and `CoreTests.cpp`
add variable beach width, approximately 36 m in the sheltered bay and 6.5 m at the eastern
headland, shared by geometry and sand material. The completed browser suite used the immutable
rocky-coast build from commit `2a660b7`; it does NOT qualify this newer source preparation.
Native build/tests/review are now complete; affected Android/Web platforms still need requalification.

## Update 2026-10-11 — integrated rocky coast (native and focused Android qualified)

See `docs/coastal-environment.md`. The first `IslandEnvironmentField` coastal subset drives
actual up-to-7.5 m headland relief, RockyGround classification, palm/canopy/understory/grass
exclusions and shader rock/sand controls. Reuses canopy texture blue/alpha; red/green and
texture/mesh/draw budgets unchanged. This is not the full environment model.
Crab starts now find nearby valid sandy habitat; tests preserve their population/determinism.
Rocky faces intentionally block climbing, with separately tested adjacent beach access.
Final 33/33 non-display groups pass (55.12 s). Bay `20261011T082154.597630Z-bay` and final
headland `20261011T082316.930694Z-headland` pass/inspected, empty kernel deltas.
Fingerprint `b0f79ae14adeb8cd`; inventory pins updated from report. `headland` camera is
available in native/Android launchers and Web template; terrain suite now has six views.
Android x86_64 build/lint and headland `20261011T082539.891561Z-headland` pass, image
inspected. Interactive `20261011T082658.311623Z-interactive` passes touch/footsteps/swimming
after two gestures; lifecycle not rerun and still degraded. Empty kernel deltas. ARM64
assembly/lint restored; physical-device runtime untested. NEXT: finish the running Web
visual/runtime qualification for this new terrain/shader, then
extend environmental controls/dominant beach. Prior coast-shape qualification below is not
current evidence. Android lifecycle remains degraded. No push or hardware rendering authorized.

## Update 2026-10-11 — Web coast qualification complete

Coast Web build and release package refreshed (10,340,739 raw / 3,525,638 gzip bytes).
Contained Chrome terrain suite `20261011T080318.835613Z-chrome-terrain` passes all five
views: overview, bay, beach, east, jungle (60 frame intervals each). Overview/bay/east/jungle
screenshots inspected. No new GPU faults; kernel delta contains perf and Bluetooth/USB
messages. No game changes since coast commit `623c314`. Web input/lifecycle and Firefox
were not rerun. Android lifecycle remains degraded, separately from the passed visual checks.
Read-only SDL inspection found explicit pending pause/resume coalescing, which can explain
missing AAudio calls during rapid Home/resume; see `BUGS.md`, not a confirmed root cause.
Next implementation: `docs/environment-field-design.md` (design only), integrating shared
coastal rockiness/beach suitability through relief, vegetation placement and terrain materials.

## Update 2026-10-11 — bay/headland silhouette (native qualified)

See `docs/coastal-silhouette.md`. Western bay between two headlands plus eastern promontory;
shore radii 158.994–258.680 m. New `bay` QA camera. The first shallow design was rejected
visually; final geometry has world-space recession checks. New coast exposed turtle bank
steering at the home centre, now fixed without weakening its movement regression.
Final 33/33 non-display groups pass (37.13 s). Native final bay
`20261011T074941.874456Z-bay` and turtles `20261011T075118.375540Z-turtles` pass/inspected.
Fingerprint `9dc988ef0ffd9f67`; population and shadow-count pins updated from the report.
Android x86_64 build/lint and bay `20261011T075311.442314Z-bay` pass, image inspected.
Interactive first fails a too-short fixed swimming gesture; bounded repeated-touch gate
added with a device-free shell regression. Repeat `20261011T075739.370693Z-interactive`
passes touch/footsteps/swimming after two gestures but lacks AAudio pause/start evidence
on Home/resume, despite restored surface/focus/same PID. Lifecycle remains degraded;
see the distinct observation under `CNA-ISLAND-001`. ARM64 assembly/lint restored;
physical-device runtime untested. Relevant harness regressions and syntax checks pass.
Web qualification is recorded above. Next is shared coastal
environment/material placement to replace the continuous sand band with rocky headlands
and a dominant beach. Macro-geography items 1–3 remain incomplete. Android lifecycle issue
`CNA-ISLAND-001` remains open. No hardware rendering or push is authorized.

## Update 2026-10-11 — ridge/valley interior

See `docs/ridge-and-valleys.md`. Two offset ridges now reach 40.0192 / 29.0938 m, with a
13.7758 m saddle station and southern/western drainage. Steep faces reveal filtered rock
grain/fractures. Bird clearance now anticipates terrain and clamps the complete altitude.
Final 33/33 non-display tests pass (49.78 s), including summit/valley, trail slope, roots,
wildlife contact and bird clearance. Native final overview `20261011T072739.811196Z-overview`
and east `20261011T072605.084747Z-east` pass and were inspected.
Android x86_64 build/lint and east `20261011T072932.613664Z-east` pass, image inspected.
Interactive `20261011T073115.062445Z-interactive` fails Home/resume with `EGL_BAD_SURFACE`;
identical-APK repeat `20261011T073355.598497Z-interactive` passes. Suspected lifecycle race
is open as `CNA-ISLAND-001` in `BUGS.md`; do not call Android lifecycle reliably qualified.
ARM64 assembly/lint restored. Web build/package refreshed; Chrome terrain suite
`20261011T073720.044111Z-chrome-terrain` passes all four views (60 frame intervals each).
Overview/east/jungle screenshots inspected; no new kernel GPU faults.
The world fingerprint and new inventory pins are in the ridge document.
Next is the shoreline/headland/bay stage, followed by the shared environment field. The full
macro-geography rewrite remains open. No hardware rendering or push is authorized.

## Update 2026-10-11 — narrow footpath

See `docs/natural-footpath.md`: vegetation corridor 12.4 m -> 2.8 m, narrower dirt shading,
full trunk/rock footprint rejection and three-lane collision regression. Native overview/jungle
inspected; all non-display test groups pass (core rerun after the rock fix).
Android x86_64 touch/traversal/lifecycle run `20261011T071027.731091Z-interactive` passes;
ARM64 build/lint restored. Web build and package refreshed. First grass-motion run failed a
15 s wall wait at 5.99/6 m; bounded game-sample wait added. Retry
`20261011T071524.062886Z-chrome-grass-motion` passes seven stages, 93 distinct frames,
GL error zero and no new GPU faults. Full Web lifecycle was not rerun.
New inventory and world fingerprint are in the footpath document; browser pins updated.
The next substantial realism target is the asymmetric ridge/valley terrain
and coast from macro-geography items 1–3. Local path forks/detours remain open too.

## Update 2026-10-11 — reef/seabed material pass

Continued the suggested items 5/16 in `TerrainRenderer.cpp`: depth-limited olive seagrass,
mottled ochre reef patches, pale radial sand channels and a broken outer material crest.
See `docs/reef-seabed.md` for the design, limitations, captures and package identities.
This is a material-only first pass; geometric reef relief and the larger terrain rewrite
remain open. No dependency pins or population/mesh counts changed.

Linux build and 33/33 non-display tests pass. Native before/after overview and Android
SwiftShader reef/overview captures pass and were visually inspected. ARM64 assembly/lint
restored; physical runtime untested. Chrome/SwiftShader terrain suite
`20261011T065909.248161Z-chrome-terrain` passes all four views and the web release is refreshed.
Nothing from this
session has been pushed. The AMD hardware stop latch remains unchanged.

The reef pass is complete as a first material milestone. The trail step selected after it is
recorded above.

The following is the historical 2026-10-10 handoff; use the update above and `PLAN.md` for
the current state.

Written 2026-10-10 (last update at the end of that day's session) for the next agent
(expected: Codex Astra High) starting on a fresh context. The working tree is clean and
everything below is committed and pushed to `origin/develop`. Everything you need to continue is here or linked; `PLAN.md` is the detailed
ledger (newest sections at the top, under "2026-10-10"). All repository text is English.

## 1. Read first: hardware safety boundary (unchanged, still in force)

**Do not run this game on the host AMD GPU. Do not clear the hardware stop latch.**
The 2026-10-08 AMDGPU GPUVM faults caused a MODE2 reset, VRAM loss and a GNOME logout; the
root cause is not identified or fixed. Clean software-renderer runs are not evidence that the
hardware workload is safe.

- Latch: `build/gpu-incident-evidence/QUALIFICATION_STOP.json`, SHA-256
  `c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1` (ignored local state; a
  fresh clone lacking it is not hardware authorization).
- Only run the game through the contained software launchers listed in section 5. Never run
  unrestricted display CTest, never replay GL traces on AMD, never launch the binary directly.
- Incident documentation: `docs/incidents/2026-10-08-amdgpu-gpuvm-reset.md`,
  `capture-evidence-inventory.md`, `mesa-recorder-readiness.md`, `process-containment.md`,
  `maintainer-sharing-summary.md` and `upstream-triage-draft.md` (drafts only; nothing has
  been submitted upstream and pushing this repository is not permission to do so).
- Stop on any new GPU fault, including unrelated host activity. Do not restart a running
  tool job just because an output poll timed out.

## 2. Operator direction (2026-10-10)

1. **The island must look noticeably closer to reality**, for existing and all future
   content. Stylized or crude assets are defects. The operator named the sea turtle as
   "really terrible"; a first pass is done, more is expected (see section 4).
2. Product boundaries are unchanged: peaceful first-person exploration of a ~500 m tropical
   island, fixed sunny daytime, no combat, quests, survival, crafting, inventory, weather or
   day/night. Deterministic simulation. Portable GLSL 330 / ES 300. Measured budgets.
3. Backlogs the operator asked for are in `PLAN.md` (section "2026-10-10 — operator
   direction"): a 16-species nonhostile wildlife list (snakes, spiders, parrots, pigeons,
   crocodiles, frogs, bats, mice, rats, deer, goats, ants, flies, stingrays, dolphins,
   octopuses), a decorative island hut, and a 20-item macro-geography realism list from the
   operator-supplied analysis (Czech original kept outside the repository at
   `../island_improvements.md`). **Monkeys and wild boars stay on the island** by operator
   decision.
4. Order: visual realism of existing content before new species; a new species is not done
   until silhouette, material, motion and contact look believable in close software captures.

## 3. What was done on 2026-10-10 (all committed on `develop`)

- Dependency pins advanced twice to the clean shared checkouts (`dependencies.lock`): Sharp
  Runtime `next` c20373ba, EasyGL f4d6afaf, MetaGL dd3b3005, CNA 97783d1e (unchanged). The
  strict check in `cmake/VerifyDependencies.cmake` refuses to configure when a shared
  checkout moves; **other agents commit to those checkouts during the day**, so if a build
  refuses, review the delta, update the lock deliberately and requalify. Never switch or
  edit the shared worktrees.
- `docs/ground-cover-and-trunks.md`: `GroundCoverField` + `GroundCover::Density` shared by
  grass placement and the terrain turf undercoat (patch field in the canopy-light texture's
  green channel), 20,523 grass clusters; grey-brown canopy trunks with root flare, rounded
  buttresses, moss/lichen, contact occlusion and canopy-shaded wood; slender flared palm
  stems with smooth normals.
- `docs/sea-turtle-form.md`: domed scute-aligned carapace, plastron, beaked head, bending
  paddle strokes (`SeaTurtleProfile.hpp`, tested); still blocky head and tonal-only seams.
- `docs/sky-atmosphere.md`: shared-haze horizon, warm sun glow, eleven fixed distant cumulus
  masses; `environment::FogColor*` constants back the C++ fog sites (GLSL literals are still
  hand-kept at `vec3(0.53, 0.78, 0.88)`).
- `docs/forest-stands.md`: closed canopy stands with a sparse scatter and 22 emergents
  (277 trees). Counts that changed downstream: 579 bushes (14 clearing), 3,116 ground plants
  (64 clearing, 2,225 island-wide), 199 colonies, 159 land rocks, 12 logs.
- Evening, after the operator chose to continue the backlog: sea turtle second pass (ten-segment
  four-ring head, geometric scute seams, wet sheen; `docs/sea-turtle-form.md`) and the first
  prevailing-wind pass (`docs/prevailing-wind.md`: shared east trade wind, coastal windward
  exposure, palms lean up to ten degrees; ocean/foam/crowns/wisps/audio still to do) and
  depth on the horizon (`docs/distant-islets.md`: nearer wooded islet, sandbar, hazed far
  silhouettes), and coconut palm groves with young palms (`docs/palm-groves.md`, 370 palms).
- Harness fixes (`docs/browser-input-ownership.md`): GLSL ES reserves `patch` (a CPU test now
  scans generated GLSL for ES-reserved identifiers); the Web harness holds keys until the game
  logs a new acknowledgement and waits for the release to be observed; the swim walk and
  resume waits are paced by game output because frames on SwiftShader exceed 1 s and
  `PlayerMotion` clamps each step to 0.1 s.

## 4. Open work, in the intended order

1. **Keep going on visual realism** per the `PLAN.md` 2026-10-10 backlog. Highest-impact open
   items: terrain macro-geography rewrite (item 1), varied coast types (2), environment
   field replacing shore-distance bands (3), prevailing wind (4), readable reef (5), natural
   footpath (6), palm distribution (9), directional shadows (11), coastal micro-topography
   (15), underwater habitat (16), small fauna (17), geology (19), wet biome (20).
   Partially done: wind (4), stands (7), emergents (8), palm groves (9), trunks (10), sky (12),
   atmosphere (13); done: islets (14).
2. Sea turtle third pass: hexagonal scute layout, surface reflections, underwater tint.
   Suggested next realism step (designed, not started): readable reef/seabed (items 5/16) in the
   terrain fragment shader only. Measured profile: the bed is 0.7 m deep 20 m offshore, 3 m at
   70 m, 5 m at 110 m, 16 m at 250 m; water alpha is `0.42/m` extinction, so seabed patterns
   read to about 3 m depth. Plan: seagrass meadows (0.5-2.5 m depth, dark olive, broad/medium
   noise), patch reefs 28-78 m offshore (ochre-brown mottled blobs) cut by pale radial sand
   channels (noise along arc length), and a broken reef crest band 70-84 m offshore. Keep it
   straight-line GLSL (see the sky lesson below) and verify on Android.
3. Then species from the wildlife backlog and the island hut, each with deterministic
   simulation, budget, capture viewpoint, regression tests and close-capture review.
4. Requalify Web input/lifecycle and Android touch/lifecycle after relevant changes. ARM64
   physical-device runtime and Linux AMD hardware acceptance remain open. Never promote
   pre-incident results to current qualification.
5. Incident work is a separate approval-gated track (section 1).

**Resolved 2026-10-10 evening:** the Android emulator crash (qemu host RenderThread SIGSEGV
in `jungle`/`east`/`turtles`) was caused by the sky's cumulus loop over a constant array, not
by the forest stands. Bisected file by file and fixed by unrolling (`docs/sky-atmosphere.md`).
Lesson for future shaders: avoid constant arrays indexed by loop counters (and `continue`)
in fragment shaders; the Android emulator's gfxstream/SwiftShader host can crash on them even
when Mesa and ANGLE accept them. Many earlier "colony" experiments in `PLAN.md` are void.

Known soft spots to watch: distant turf undercoat reads as flat olive in open clearings;
cumulus masses are smooth blobs; `lifecycleResumed` in the full Web suite is false (reported
only, not a gate); `IslandGpuQualificationSupervisorTests` has a 4 s fixture timeout and can
fail under `ctest -j4` load while passing at `-j2` or alone.

## 5. Build, test and capture commands (run from the repository root)

```sh
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv     # mandatory, see ../AGENTS.md
cmake --build --preset linux-debug -j2                 # existing build/ tree, do not reconfigure
ctest --test-dir build -LE display --output-on-failure -j2

# Contained native llvmpipe captures (viewpoints: overview beach jungle east clearing-edge
# west grass ground ground-broadleaf bush litter contact movement palms water current distant
# props driftwood reef fronds birds fish shark monkeys shipwreck crabs butterflies turtles)
sg systemd-journal -c 'python3 tools/run_software_capture.py build grass'

# Web (existing build-probe/ tree)
source /home/robertvokac/emsdk/emsdk_env.sh
export EMCC_CORES=2 BINARYEN_CORES=2
cmake --build --preset web-webgl2 -j2
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope litter --timeout 300'
#   scopes: full terrain wildlife-foliage coast grounding vegetation grass-motion litter
#   (the turtle view exists only in --scope full --timeout 1700, about 25 minutes)
tools/package_web_release.sh build-probe

# Android (x86_64 emulator run, then restore the default ARM64 package)
export ANDROID_SDK_ROOT=/home/robertvokac/Android/Sdk
tools/run_android_gradle.sh :app:assembleDebug :app:lintDebug -PislandAbi=x86_64
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint grass'
cp android/app/build/outputs/apk/debug/app-debug.apk build/android-software/island-x86_64.apk
tools/run_android_gradle.sh :app:assembleDebug :app:lintDebug
```

Rules that bit this session:

- Every launcher writes `run.json` with `"result"`; a `tail` of its stdout can look like
  success while the gate failed. Always read `run.json` and keep failed-run directories.
- The owned emulator crashed once (exit -11) on 2026-10-10 and its stale AVD lock files made
  the next launch fail before boot; the harness now removes such locks when no emulator
  process exists. Browser scopes with eight views need `--timeout 1700` at current frame cost.
- Harnesses pin exact diagnostics and counts (`tools/run_software_capture.py`,
  `tools/run_web_smoke.mjs`, `tools/run_firefox_web_smoke.mjs`,
  `tools/run_android_runtime_smoke.sh`). When you change a mesh inventory, population count or
  a diagnostic string, update the pins in the same change, otherwise runs fail with
  "check for a stale executable" or silently on a regex.
- The full Web suite verifies 28 sections; if a run fails, test the harness' `includes`
  literals and regexes against the retained `island-browser-smoke.json` outputs to find the
  stale one quickly (see the PLAN entry for the 2026-10-10 turtle requalification).
- Do not overlap heavy jobs (Web build, browser suite, emulator, Gradle). Two build workers
  for Sharp-Runtime-governed work; `-j4` is fine for the island build itself.
- Never build in `/tmp`; reuse `build/`, `build-probe/`; keep `build-consumer/` and
  `build/gpu-incident-evidence/` untouched.

## 6. Evidence locations (ignored local data, not in Git)

- Native: `build/software-captures/<timestamp>-<viewpoint>/` (PPM, logs, `run.json`,
  kernel deltas). Latest (palm groves): `20261010T174808.840978Z-overview`,
  `174754.658042Z-palms`, `174801.794546Z-beach`; before: `20261010T172806.329415Z-overview`,
  `172812.566297Z-distant`, `172642.513830Z-palms`, `172648.540756Z-jungle`. Earlier:
  `20261010T150916.889020Z-jungle`, `150924.874906Z-overview`,
  `150932.482781Z-east` (forest stands); `145219Z-beach`, `145227Z-overview`,
  `145234Z-palms` (sky); `143756Z-turtles`; `130435Z-grass`, `131112Z-litter` (ground cover).
- Web: `build/browser-software/`. Full suite `20261010T180146.003280Z-chrome-full` (28/28,
  final tree with palm groves; earlier `172921.618192Z`, `142621.752717Z`);
  vegetation scope for the forest stands `152203.104225Z-chrome-vegetation`; litter scopes `145543Z`
  (sky) and `131626Z` (ground cover); grass-motion `132225Z`. Failed runs from the same day
  are retained on purpose (reserved word, key pulses, stale pins).
- Android: `build/android-software/`. Final tree (palm groves), all pass:
  `20261010T175715.080427Z-palms`, `175828.715762Z-jungle`, `175941.257408Z-east`; before the
  groves: `173721.178620Z-jungle`, `173833.723258Z-east`, `174128.122762Z-turtles`. Earlier:
  `145908Z-palms` (sky), `144750Z-turtles`, `133014Z-grass`; the many failed `jungle`/`east`
  runs of the evening are the sky-shader crash bisection. Rolling tested x86_64 APK:
  `build/android-software/island-x86_64.apk`.
- Package sizes and hashes are in the `docs/*.md` file of each pass. ARM64 runtime untested.
- Incident evidence: `build/gpu-incident-evidence/` and historical `build-consumer/`.
  Never clean these as disposable build output.

## 7. Repository and dependency boundaries

- CNA `next` export at `build/dependencies/cna-next` and Sharp Runtime `next`, EasyGL and
  MetaGL `develop` under `../` are strictly read-only. Record actual framework defects in
  `BUGS.md`; do not patch them.
- Original artwork lives in `assets/art-studies/` with provenance in `ASSET_LICENSES.md`
  and `assets/manifest.json`; prepared PNGs are generated at build time (Python 3.10+ and
  Pillow on the host only).
- Commit on `develop`; push only when the operator asks. The push on 2026-10-10 included all
  work up to the forest-stands pass.
