# Spiders — implementation and verification

Status: **12 ground spiders are now integrated in IslandGame. Webs are still absent.**
The first actual native software image was inspected, but its generic image-contrast
gate failed. Natural motion, visual realism and portable qualification remain open.

## Generated setae material

Built-in ImageGen produced the project-authored, reference-free opaque study
`assets/art-studies/spider-setae-albedo-v1.png` (1254 square); its exact prompt is
`assets/art-studies/spider-setae-albedo-v1-prompt.txt`. Original and prompt are retained
with hashes in assets/manifest.json and provenance in ASSET_LICENSES.md. This is an
AI-generated material study, not a species photograph or a seamless texture claim.
The existing preparation tool produces a 256-square runtime image, SHA256
`06b5295893a201a4f83523bf5194c69089af7b723ad186eb4d22a59f6ace1f89`.

Cuticle/markings use interior UVs (0.06..0.94), defined in mesh-local coordinates;
leg UVs follow each articulated segment. Eyes and geometric bristles remain solid.
Tests check finite interior UVs and identical UVs across nine articulated poses.
The same stock BasicEffect now submits textured skin and untextured details in two
near draws. Geometry counts remain4248/1632. The120000-vertex buffer uses24-byte
VertexPositionColorTexture entries (2880000 GPU bytes); two staging vectors each
reserve that payload. The required image has9 mip levels/349524 RGBA8 bytes.
Runtime texture total is8846468 bytes; both quality-profile caps include the addition.
CMake native install/Web preload and Android preparation include the material, but
packaging source changes alone do not qualify Web/Android runtime.

The prepared image was inspected: dense short brown/grey marks remain at256px.
First actual eight-stage run050946.511021Z exposed stretched rear-body hairs from
top-down UV projection. Replaced that with latitude and continuously folded longitude
inside the image, retaining pose-invariant coordinates. Final actual llvmpipe run
`build/software-captures/20261012T051053.143139Z-spiders` has1/12visible,2draws,
contact-shadows=1, complete motion data and kernel-after[]. All eight crops inspected
in `build/spider-setae-curved-montage.png`: rear streaking is reduced and the body has
surface detail instead of flat brown. Deep shade, faceting and stiff-looking segment
silhouettes remain; sparse images do not establish natural continuous motion.
**Outer image validation still FAILED** luminance variation, colour bins and spatial
contrast. No gate/profile thresholds changed. Webs and portable acceptance stay open.
Inclusive native build/full52CPU passed74.94s before the final UV-only correction;
post-correction native build and focused mesh regression passed0.05s. Logs:
`build/spider-setae-tests.log`, `build/spider-setae-curved-{build,tests}.log`.

## First live integration and actual native inspection

### Shaded close-view investigation after setae integration

Actual contained run `build/software-captures/20261012T051411.829809Z-spiders`
records ground direct-light transmission0.00488045858 and body-centre
transmission0.00512093166. Shared RGB canopy tint is
(0.462765306,0.532406807,0.621945977). The first home is therefore nearly fully
occluded from direct sunlight in the current canopy model, not unlit by ambient.
SpiderRenderer uses the same .61 ambient/.37 sun/.10 upward-fill expression as
GoatRenderer, multiplied once by the shared tint; BasicEffect lighting is disabled,
and its default diffuse/emissive/alpha are unity/zero/one. No additional spider-only
lighting multiplier was found. This does not establish correct exposure, colour
space, ecological placement or visual acceptance of the common lighting model.

The harness now saves `spider-lighting.json` before image validation, rejects
missing/duplicate/nonfinite/out-of-range/inconsistent diagnostics, and independently
checks the shared tint transfer.41 device-free harness tests pass, including eight
bad lighting records and fully shaded/half/full transmission. Native build passes.
All eight stage images and the final PPM compare byte-identically to051053.143139Z;
the earlier actual visual inspection therefore still applies. Kernel delta is empty.
The outer contrast gate remains FAILED, with no thresholds or viewpoint exceptions.
Next lighting work should compare different real habitats and an open-daylight control
before changing the shared ambient/exposure model; do not brighten only this audit
camera, move spiders out of habitat solely to pass, or label this a lighting fix.

IslandGame constructs SpiderColony after the full static collision grid/hut, updates
it with the real camera observer and sends the authoritative pose to SpiderRenderer.
The renderer uses one opaque vertex-color BasicEffect batch, CPU normal/daylight and
shared canopy transmission. Near meshes apply within4m, far to18m; a0.16m frustum
sphere surrounds each body. Capacity120000 VertexPositionColor vertices (1,920,000
GPU bytes plus the same reserved CPU staging payload), with population/upload guards.
No custom shader, texture or special GPU feature. Grounding shadows now use the
existing contact-shadow batch (details below).
Visible counts appear in the HUD and benchmark; draw totals include the batch.
WorldReport now covers this live population (see the reporting section below).

The spiders viewpoint targets the first normal habitat with a low close camera
(.36,.28,.45m offset,0.5rad FOV) and follows its translation. It is inside the normal
observer retreat radius; the simulation is not frozen or forced into a special pose.
One-time platform-mirrored render proof must report1..12 visible spiders in one draw
for the native capture harness. This first view uses the ordinary four-frame capture,
not a deterministic multi-stage crawling audit.

Actual contained llvmpipe run
build/software-captures/20261012T043243.618050Z-spiders rendered1/12 in1draw,
kernel-after[] and a1280x720 image (inspected via build/spider-live-first.png).
The spider is visibly present on textured forest floor, with eight leg silhouettes,
two-part body, eyes and sparse bristles. It remains too smooth/plain, and contact
shadows are missing. Deep shade makes it low contrast. **Outer run FAILED** because
the generic image validator rejected luminance variation, color bins and spatial
contrast. No thresholds/profile exceptions were changed to turn that into a pass.
This is actual rendering evidence, not clean visual/platform qualification. That
capture precedes the subsequent HUD/help/benchmark-count-only edits.
Current inclusive native build and51/51 CPU groups pass59.58s, logs
build/spider-live-current-{build,tests}.log. No Web/Android build or run in this step.

Next: refine the body pauses observed in the first motion sequence below, surface/hair/contact
refinement and explicit review of shaded small-animal capture acceptance; then actual
Web/Android qualification. Plant-attached webs are still a separate required task.

## Actual native motion sequence

Spider capture now uses a capture-only100ms simulation update and matching render clock,
with fixed-timestep accumulation disabled, like the existing goat/snake audits.
Ordinary gameplay is unchanged. Eight images are read back at frames8,16,...64,
corresponding to0.8..6.4s. Camera translation follows the first normal spider; its
near observer still triggers the actual retreat behavior. No forced poses or routes.
Each stage logs update/time, visible count, body position/heading/speed and all eight
world foot positions/stance flags at nine significant digits.

The harness requires complete ordered stages, the exact clock, finite/limited speed,
turn and between-sample travel, eight distinct feet inside the body envelope, six
supports and actual body/foot displacement. It saves spider-motion.json and requires
all eight image files. Device-free preflight now has39 passing tests, including14
corrupted spider-motion sequences. Sparse samples cannot prove continuous stance
locking, terrain contact, self-contact or natural-looking gait.

Actual contained run build/software-captures/20261012T044137.963028Z-spiders satisfies
the motion checks:1/12visible throughout, six supports at all stages, body displacement
about0.17m and heading change about1.77rad. All eight actual frames were inspected in
build/spider-motion-montage.png: rotation and crawling are visible, but the untextured
smooth body and sparse hairs remain weak. Logged speed is zero at stages0,1 (initial
turning),4,6,7, despite positive cumulative translation later in the sequence. This
motivates a finer body pause/foot scheduling audit; sparse images alone do not prove
how those pauses look continuously.

Kernel-after[]; **outer run again FAILED** the generic image-contrast/color gates.
No threshold/profile changes, no clean visual/platform qualification claim. Next work
must address the observed material/contact and motion limitations, not merely repeat
this same dark camera. Web/Android remain untested for live spiders.
Current inclusive native build and52/52 CPU groups pass59.42s, logs
build/spider-motion-{build,tests}.log. World fingerprint4c38d1adbddc9c9e is unchanged.

## Grounding shadows

BuildSpiderContactShadow adds one soft60×29mm-radius body ellipse (18% centre opacity)
at local z=-12mm and3.5mm-radius foot ellipses (30%) only for stance feet. These are
compact ambient grounding approximations, not directional body/leg silhouettes.
New SmallAnimal ContactPatchDetail permits1..200mm radii and0.3mm vertical bias;
Compact/Coastal retain their original50mm..8m contract and25mm bias. All footprints
are clipped at the actual terrain grid/triangle boundaries. A matching rendered-surface
cache supplies corner heights, avoiding repeated procedural terrain evaluation.

The existing single transparent ContactShadowRenderer batch includes these patches
within8m (quality-scaled), with population/upload guards and4096vertices/spider reserve
(12×4096×16=786432 additional GPU/staging bytes each). Nearby body/feet/stance changes
invalidate its cache precisely; distant spider movement does not force a refresh.
HUD and the mirrored render proof expose spider grounding counts. The native capture
harness requires a positive spider contact-shadow count;40 preflight tests pass,
including seven corrupt/missing shadow-proof cases.

SpiderColonyTests checks shadow vertex/opacity budgets, exact surface bias and triangle
interior grounding at1s intervals over all12 moving spiders, and verifies that raised
feet remove their footprints. Focused core+colony tests pass39.89s; existing large-prop
shadow contracts/counts remain unchanged.

Actual contained run build/software-captures/20261012T045511.802447Z-spiders:
1/12visible,1bodydraw,1groundedspider; motion data pass, kernel-after[].
All eight images inspected via build/spider-shadow-montage.png; raw matching stage0
before/after crops inspected in build/spider-shadow-comparison.png. Under-body shading
is subtly darker, but deep forest shade limits readability; smooth plain material and
sparse hairs still dominate the visual deficit. Outer contrast/color gate stillFAILED.
No gate/profile exceptions or clean visual/platform acceptance claim. No Web/Android
build/run. Next substantial visual work: body surface/hair detail, not another
unchanged-image audit.
Inclusive native build and52/52 CPU groups pass88.30s, logs
build/spider-shadow-{build,tests}.log. World fingerprint4c38d1adbddc9c9e is unchanged.

## Population reporting

WorldReport constructs the same SpiderColony after the full obstacle grid/hut.
wildlife.groundSpiders reports12; groundSpiderPopulation exports initial positions,
homes with canopy indices, heading/speed/behavior, orthonormal body basis and all eight
world foot anchors, stance/reach-limit flags and local root/hip/knee/foot chains.
This section uses seven decimal places to preserve submillimetre appendage data.
websImplemented:false explicitly keeps missing web habitats visible.

The world validity pass rechecks actual habitat acceptance, finite state, home
agreement, grounded stance endpoints and reconstructed mesh endpoints. The fingerprint
now includes homes, body state/basis and complete initial foot/leg state. Host reference
fingerprint4c38d1adbddc9c9e replaces0a98cb638125508f due to this added coverage, not a
placement/simulation/mesh change. Saved actual output: build/spider-world-report.json.

WorldSpiderReportTests.py independently parses real generated JSON, verifies count,
home spacing, canopy references, finite orthonormal basis, eight distinct feet and
fixed18/50/65mm chain lengths, then reconstructs world feet from the exported transform.
Eleven corrupted cases exercise wrong count, missing spider, nonfinite heading,
overlapping homes, a false claim of implemented webs, missing foot, displaced anchor,
stretched bone, invalid basis, missing stance and invalid canopy index. Focused test
passes5.15s. This is initial-state reporting coverage, not a terrain/visual/motion oracle.
Inclusive native build and52/52 CPU tests pass64.55s, logs
build/spider-report-{build,tests}.log. No display/portable build or run for this report step.

## Ground colony simulation

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
displaced eligible foot is selected above3.5mm error. A80ms smoothstep swing has an
8mm sine-squared lift and ground-aware height; moving landing targets lead by12mm,
resting ones return to neutral. Feet sit0.1mm above rendered ground. The IK solver
must reach every accepted body/foot pose. Candidate legs are sampled at five points
per segment for radius-aware ground clearance and140mm horizontal envelope. This
is not continuous triangle/terrain intersection or complete self-contact proof.

### Body pause correction

An8s/20ms audit compares direct retreat and the actual close-camera observer offset.
It counts only translation-eligible samples (heading within1.03rad of away, root inside
0.55m of home, after initial settling), excluding intentional turn/boundary stops.
The original120ms/4mm/unsigned-error scheduler paused165/389 direct samples and117/298
turning samples; next-translation IK was reach-limited in165 and113 respectively.
This confirmed a real limb-reach wait rather than an image illusion.

Selection now uses backward lag only along the forward axis during movement, plus
lateral error for turning. A foot planted ahead is retained instead of immediately
lifting again just because its absolute distance from neutral exceeds3.5mm. Rest
still uses symmetric error. Increasing landing lead to12mm cut pauses to66/389 and
56/298. Shortening the swing to80ms then produced0/389 and0/298, with8s travel
0.518799/0.390786m. The committed regression requires zero eligible pauses for these
two reference routes. All six-support, fixed-anchor, reach, mesh-clearance and habitat
bounds remain unchanged; this is not a guarantee of every future route or naturalness.
Baseline/intermediate evidence remains build/spider-pause-{baseline,directed,fast}.log.

Current30s quiet test travels0.643225–0.984048m per animal,111499 unchanged consecutive
stance pairs,26020 swing samples, minimum sampled detailed-mesh ground clearance
0.0000991821m. Initial population/fingerprint are unchanged.

Inclusive current build and52/52 CPU groups pass58.03s, logs
build/spider-pause-current-{build,tests}.log. Actual post-fix llvmpipe sequence
build/software-captures/20261012T044909.500088Z-spiders passes motion-data checks:
six supports throughout, body displacement0.272804m, stage2..7 speeds0.064899..0.065013m/s
after the two initial turning-only samples. All eight actual frames were inspected
in build/spider-pause-fixed-montage.png. Rotation/crawling and updated foot poses remain
visible; sparse images still do not prove continuous naturalness or all routes.
Kernel-after[]; outer image-contrast/color gate stillFAILED. Plain material, sparse
hairs and missing contact shadows remain unchanged; no thresholds were relaxed.

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
