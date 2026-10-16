# Spiders — implementation and verification

Status: **200 ground spiders and six branch-attached canopy webs are integrated in IslandGame.**
Ground-spider continuous naturalness and further anatomy polish remain open; the live web batch
has passed inspected native llvmpipe and Chrome/SwiftShader WebGL2 runs.

## Tapered dorsal abdomen lance — 2026-10-16

The renewed close audit found that the fitted eyes, textured setae and articulated gait remained
readable, but the large abdomen still collapsed into one almost uniform brown sphere. The existing
near-body vertices now select a muted warm dorsal material through a width that narrows toward both
ends of the abdomen. It remains inside the existing setae-textured body draw; no vertex, texture,
buffer, simulation, shadow or draw call was added, and the far mesh is unchanged.

The current pre-change `spiders-bright` evidence `20261016T075636.847579Z-spiders-bright` was
inspected but rejected: its 39 quantized colour bins missed the dedicated close-natural minimum of
40. No threshold was lowered. Accepted native evidence `20261016T080245.750749Z-spiders-bright`
uses the same camera and passes at 42 bins, 10.48 luminance deviation and 16.64 spatial contrast.
All eight moving stages retain the tapered pattern, one grounded animal, two material draws and
one contact shadow with an empty kernel delta.

Inspected Chrome/SwiftShader `20261016T080545.683671Z-chrome-spiders` passes both the four-pose
ground-spider scene and the independent live-web scene: each runs 60 finite positive intervals,
reports `glError=0`, retains a live context and ends with kernel `[]`. Seven focused Spider CTest
groups and all 119 software/browser/Android harness unit tests pass. This is portable software
evidence, not physical Android or AMD performance qualification.

## Tiered eye cluster and front-quarter audit — 2026-10-14

The near mesh replaces eight equal-size black eye beads with a tiered cluster: four small anterior
eyes, a modestly larger posterior median pair and two lateral eyes. The existing low-resolution eye
facets carry a muted amber upper-forward catchlight; no vertices, texture, draw call or far-LOD work
was added. The first high-value catchlight study was rejected after inspection because the median
pair looked artificial. The accepted cluster stays dark and reads as embedded glossy eyes.

The `spiders-bright` camera now orbits the animal's front quarter in its actual heading convention
and updates its aim as the spider turns. For this inspection mode only, colony activation samples an
observer three metres above the subject, preventing a camera that continually follows the face from
causing an endless retreat turn. Gameplay still uses the real player position. Native llvmpipe
evidence `build-consumer/software-captures/20261014T124838.634321Z-spiders-bright` passes the reviewed
close-natural-detail image gate and eight-stage motion audit. All stages were inspected; the subject
rests, turns and crawls while the face remains visible. Focused Chrome/SwiftShader evidence
`build/browser-software/20261014T125208.907939Z-chrome-spiders` passes the ground and web scenes,
and its four ground poses plus final web frame were inspected.

The native capture validator now consumes the complete contiguous 0..199 habitat comparison rather
than the historical twelve rows. Its dedicated close-natural profile keeps hard luminance, colour
and spatial-contrast floors for the intentionally uniform forest-floor close-up; it is selected by
viewpoint name, never inferred from a failing image. Android ARM64 assemble/lint passes without
launching an emulator or device.

## Live wind-following canopy webs — 2026-10-13

The six previously CPU-only qualified sites now render in `SpiderWebRenderer`. Each retains the
existing 220-node/434-strand connected graph and samples its three real branch anchors every
frame, so the support triangle and every barycentric strand move with the same canopy wind as
the tree. Camera-facing 1.45–2.6mm translucent silk ribbons keep the graph readable without line
width extensions; distance/frustum culling limits all visible sites to one dynamic upload and one
alpha-blended, depth-read draw. At most 15,624 vertices are reserved and none of the 200 ground
spiders gains additional simulation work.

The dedicated `spider-web` inspection camera selects the best regularity/outward-offset site and
stands on the outside of its tree rather than inside the crown. Actual native llvmpipe evidence
`build-consumer/software-captures/20261013T010514.795446Z-spider-web` reports 1/6 visible,
434 strands and one draw. Its 1280x720 image was inspected: the complete radial/circular spiral
and triangular branch frame are visibly attached between three supports; kernel delta is empty.

The current Emscripten build also passes the extended focused Chrome 155/SwiftShader WebGL2 run
`build/browser-software/20261013T005158.377358Z-chrome-spiders`. Both the existing moving ground
spider and the new web view pass with `glError=0`, no context loss and empty kernel delta. The two
inspected web images visibly differ as wind moves the attached frame while preserving topology.
WorldReport now records `websImplemented=true`, six sites, support identities and rest anchors;
the independent JSON validator rejects missing/inconsistent web inventory. Individual-leaf and
neighbouring-plant clearance, web occupants, Android runtime and long-walk discovery remain open.

## Dormant work and wake-up repair — 2026-10-12

The initial population expansion still replayed up to fifteen detailed foot/terrain steps for
each dormant animal every thirtieth update. Staggering spread this cost across frames but did not
eliminate it. A CPU-only measurement of the real 200-animal colony found 3.372ms mean per update
in the reported flight position, where all 200 animals were dormant and none could be rendered.

Dormant animals now preserve their complete planted pose, behavior and intent without simulation
or accumulated catch-up time. Returning to full detail discards any pending reduced-tier interval,
so approaching a resident resumes one normal tick without a visible jump. Nearby residents still
use the original terrain-constrained gait, six-foot minimum support and two-foot swing limit.
The updated regression audits nearby gait and terrain contact plus activation of every one of the
200 residents. The initial test exposed a reduced-to-full catch-up jump; that transition is now
explicitly covered. The older dormant-motion result below describes the superseded implementation.

## Two-hundred-animal population and tiered simulation — 2026-10-12

The requested live population is now exactly 200 ground spiders, deterministically
placed at least 4.5m apart in the existing qualified forest/brush habitat. Each keeps
its articulated feet, patrol state and movement intent. Per-frame work is bounded by
full/reduced/dormant distance tiers and staggered update divisors; accumulated time is
substepped when an animal's turn arrives so movement and foot constraints remain stable.

The renderer chooses at most the nearest 24 visible spiders for its existing two material
draws, and the shared shadow batch chooses at most the nearest 16. Thus the new population
does not reserve or upload geometry for all 200. Focused habitat and motion tests pass;
the dormant-tier audit observed 102/200 animals move over the long deterministic run and
22.1356m aggregate travel. WorldReport exports all 200 articulated initial states and
passes its independent corruptions; shared fingerprint is `b0f0ae75a88ce3c3`.

Actual isolated llvmpipe `20261012T143753.699776Z-spiders` was inspected and reports
1/200 visible in two draws with one contact shadow. The subject remains recognisable and
grounded. The deliberately close, dark forest-floor image still fails the generic scene
contrast gates, so this is population/integration evidence rather than final appearance
acceptance. Web/Android, long-walk encounter review and controlled performance comparison
remain open.

## Population expansion — 2026-10-12

The live deterministic population doubled from twelve to 24 without changing its
8m minimum home separation or qualifying canopy/ground/obstacle rules. Renderer and
shared contact-shadow capacities derive from the population constant, so the larger
world remains bounded and adds no draw call. The focused 30-second regression proves
every spider travels 0.63–0.98m while retaining six or more planted feet and valid
terrain clearance. WorldReport exports all 24 articulated initial states and passes
its independent eleven-corruption validator; fingerprint is `02d44fab27a0b823`.

Actual llvmpipe `20261012T121332.031522Z-spiders` reports 1/24 visible, two material
draws and one contact shadow. Eight inspected stages show a recognizable spider moving
and turning 0.38m. The tightly cropped dark forest-floor image still fails the generic
luminance, colour-bin and large-scale-contrast gate, so this is population/integration
and motion evidence rather than final visual acceptance. Web/Android, long-walk
encounter frequency and measured performance remain open.

## World-space web site proposals — not live or foliage-qualified

### Exact support-branch crossing rejection

Site selection now additionally rejects any strand crossing the seven actual shared
branch side meshes at wind times 0, 4 and 13. The segment/triangle predicate handles
ordinary crossings and coplanar overlap using double-precision plane/barycentric
calculations and an AABB broad phase. Only intentional endpoint contact with the
owning support is trimmed by 0.1mm in tree-local units; the rest of that support is
still tested. This is zero-thickness surface clearance, not an expanded wood volume.

The stronger check changes the default proposals to trees229,191,84,139,105,223;
even tree229 uses a different frame. Earlier tree lists below are historical.
Both web test groups pass2.40s, including seven synthetic intersection cases and
twenty additional wind instants per accepted site independent of selection times.
Logs: build/web-clearance-final-{build,tests}.log. No renderer or portable run.
Leaves, trunk, vines, nearby plants, branch end volumes and continuous-time clearance
remain unqualified. Live integration is deferred behind the renewed operator priorities.

### Connected strand geometry (CPU only)

`SpiderWebGeometry` constructs a fixed graph from a site's three rest anchors:
three frame corners, a centre, 24 boundary attachments and an eight-turn spiral.
The default sites each use 220 nodes and 434 strands. Frame edges are subdivided
at radial attachments, and every spiral/radial crossing shares a node. This avoids
disconnected endpoints or independently moving coincident lines. Corner hits reuse
the frame corner instead of creating zero-length boundary segments.

The spiral uses circular directions in the rest triangle's actual plane and a
steadily increasing radius inside the frame; it is not a stack of triangular rings.
Stored barycentric nodes deform affinely with the three sampled branch anchors.
This is a bounded, taut strand scaffold, not a physical silk simulation or a claim
of species-specific web anatomy. There are no GPU triangles, transparency/line-width
choices, live integration, occupied webs or new visible island content in this step.

Tests check connectedness, unique/nonzero edges, exact corner attachment, convex
frame containment, finite interpolation, strand/node budgets, rest spiral radii,
40 wind samples for each of six proposed sites, 24 additional oriented/asymmetric
frames, and invalid rest/moving frames. Actual rendering and individual vegetation
intersections remain required; CPU graph acceptance is not visual acceptance.
Inclusive native build and all55 CPU groups PASS110.25s, geometry test0.56s;
`build/web-geometry-final-build.log` and `build/web-geometry-full-tests.log`.
No renderer or portable build/runtime was run for this unintegrated graph.

`SpiderWebLayout::Propose` selects at most six three-branch frames from existing
canopy instances. Deterministic hashed tree ordering avoids the initial sequential
selection's concentration on one side of the island. Sites retain the actual branch,
triangle and barycentric attachment identities. `Sample` applies matching local wind,
instance scale/yaw and the same rendered-terrain grounding as JungleRenderer.

Candidate frames are bounded to 0.08–1.5 square metres and 0.3–2.5 metre edges,
with centres at least two metres above rendered ground and twelve metres apart.
Interior sample points exclude the trunk envelope and coarse foliage-core ellipsoids
at three wind times. This is NOT individual-leaf, other-branch or neighbouring-plant
intersection qualification, nor continuous-wind clearance. No vegetation is removed
and no unsafe fallback forces the requested count.

The default world proposes six trees: 1, 229, 138, 23, 184 and 189. Regression tests
verify repeatability, distinct supports, spacing/height, finite area/span bounds over
twenty wind times, independent barycentric reconstruction and inverse instance
transforms, actual anchor motion and four invalid-input cases. Focused test passes
0.36s; inclusive native build and all 54 CPU groups pass, 89.68s
(`build/web-layout-final-build.log`, `build/web-layout-full-tests.log`).

This API is not yet consumed by IslandGame or WorldReport. There are still no visible
webs, new web occupants or web rendering costs. No renderer was launched for this
proposal-only step, and no Web/Android runtime qualification is claimed. Next: bounded
strand geometry and stronger clearance checks, then live rendering/occupants and
actual close/moving-camera inspection. The proposals alone do not complete webs.

## Web attachment foundation — no live webs yet

CanopyBranchProfile.hpp/.cpp now exposes the actual seven bent support branches
per canopy form. JungleRenderer consumes this shared profile for both LOD tiers:
three six-sided rings,18 vertices/24 outward triangles, the original taper, bend,
wood coordinates and wind weights/phases. No extra foliage/GPU geometry or web
population was added. This removes a renderer-private geometry copy before choosing
web sites; it does NOT yet select valid gaps or place any web.

SampleCanopyBranchAnchor stores an attachment as triangle index plus barycentric
coordinates. It deforms the three vertices with the existing local wind model before
interpolating, rather than applying nonlinear wind at an approximate cylinder point.
The result is still tree-local: the future web must apply the real tree instance
transform, use matching wind time and retain the branch/triangle identity. Open end
caps are not invented. World-space gap clearance, plant intersections, wind-stable
strand geometry, occupancy and actual rendering remain required.

BranchAnchorTests covers all8forms×7branches×24triangles×5times=6720 attachments,
independent double-precision wind/interpolation checks, outward/nondegenerate triangles,
exact vertex attachment at rest, and seven invalid variant/cluster/coordinate/time cases.
Native build and branch/light-field/visibility focused tests PASS3.17s. Actual contained
llvmpipe jungle capture054755.576838Z-jungle PASS, kernel-after[]; the1280x720 image
was inspected in build/web-branch-anchor-jungle.png. Supports/foliage are present with
no obvious gross geometry break. This overview is NOT a visual proof of moving web
attachment, exact before/after equivalence or overall realistic jungle acceptance.
No Web/Android build/runtime in this step. Webs are still absent in IslandGame and
WorldReport must continue reporting websImplemented=false.
Inclusive53 CPU groups pass163.64s (`build/web-branch-anchor-full-tests.log`).

## Generated setae material

### Near limb-profile refinement

Near leg/palp sections now use eight instead of six radial sides and an intermediate
ring. The radius is the previous linear taper multiplied by
`0.72 + 0.28*sin(pi*t)`: narrower articulations, fuller middle, still inside the old
collision taper. Smooth normals use the profile derivative. Exact segment endpoints,
bone lengths, foot anchors and zero-radius ground tips are retained. Far geometry
remains1632 vertices; near count9840 versus7920 (+24.2% upload/geometry work).
Existing10000/animal and120000/batch capacities, texture and two near draws unchanged;
only160 vertices/animal budget headroom remains. Portable performance is unmeasured.

Mesh regression independently checks every leg vertex against the union of original
bone taper envelopes across nine poses, in addition to normals/winding/UVs/bounds.
Final native build+mesh/colony tests PASS9.59s, including actual posed terrain checks.
Logs build/spider-limbs-final-{build,tests}.log; earlier profile/envelope checks also pass.
Actual final llvmpipe054117.864733Z-spiders-bright: all eight images inspected in
build/spider-limbs-final-montage.png; earlier same-stage before/after comparison in
build/spider-limbs-comparison.png shows subtly rounder, slimmer articulations.
Still separate simple bone sections, not a fully anatomical multi-segment leg model;
joint seams and the folded body-texture pattern remain.1/12visible/2draws/contact1,
habitat+motion+lighting checks pass/kernel[]. Outer image contrast stillFAILED;
no thresholds or lighting changed. No Web/Android build/runtime for this refinement.

### Comparison with an existing fully sunlit habitat

`--viewpoint spiders-bright` selects the first maximum body-centre canopy
transmission among the twelve existing initial spiders. It changes only the camera
subject, retaining the same offset/FOV, normal observer retreat, following and
deterministic capture clock. No animal relocation, canopy/material/light modification
or image-threshold exception. All twelve candidate transmissions and the selected
index are logged; the capture harness independently verifies ordered/finite candidates
and the first-maximum selection and saves spider-habitats.json.42 harness CPU tests
include eight corrupted comparisons and a tie requiring the lower index.

Actual native llvmpipe run053333.460326Z-spiders-bright selected index1:
initial body transmissions in index order were .00512093,1,.0309411,1,1,1,1,
.408079,1,1,.0575958,.722916. Seven are fully sunlit according to the canopy model;
association with a canopy tree is not actual shade. First-draw ground/body transmission
and RGB tint all equal1. Eight actual images inspected in build/spider-bright-montage.png.
The body/material are more readable than in habitat0, but angular legs and the folded
albedo pattern are clearer defects. Sparse stages still do not prove continuous gait.
The generic image gate stillFAILED: deviation11.6962<18, bins48<64, spatial
contrast19.7302<30; mean luminance53.2637. Motion/material/lighting checks pass,
kernel-after[]. This is comparison evidence, not visual acceptance or a lighting fix.
It does not justify brightening the shared island light solely to satisfy a close crop.

Original-view regression capture053432.291661Z-spiders has all eight stages and final
PPM byte-identical to051626.813603Z, kernel[]. Its outer gate stillFAILED as before.
The new bright camera has not been built/run on Web or Android in this step.
Current native build and inclusive52CPU groups PASS80.86s; logs
build/spider-bright-{build,preflight,tests}.log.

### Actual Android GLES3 inspection — image gate failed

The isolated Android harness now accepts `--viewpoint spiders` and requires the
same live population/material/contact-shadow/lighting proof as native.23 CPU preflight
tests pass, including six corrupted spider logs; shell syntax passes. x86_64
assembleDebug+lintDebug passes. APK validation verifies exact prepared material bytes,
including the141381-byte spider image (same SHA06b52958... as native/Web).

Actual `build/android-software/20261012T052659.737424Z-spiders` reached mobile
OPENGLES3/SwiftShader,1/12visible/2draws/contact1 and the same body light transfer.
The retained log independently passes the new checks. Both the intermediate and final
actual1280x720 image were inspected: textured rounded spider is present, with the
same dark background/angular limbs. This is one retained presentation, NOT a continuous
motion/lifecycle or mobile-performance audit. **Outer run FAILED** after17 image
attempts: luminance deviation9.4266<18, colour bins28<64, spatial contrast23.9878<30.
No threshold/profile exception was added. Parent result remainsfailed; kernel-final[],
owned emulator exit0. No AMD hardware, KVM configuration or AVD configuration changes.

Tested APK retained as `build/android-software/island-spider-setae-x86_64.apk`,
SHA256 `8c8e006bbaebaba7cd9d8d161afab74ed55d772d20c77dc6d9ee697b95f83ec7`.
Then normal ARM64 assembleDebug+lintDebug was restored successfully; lib/arm64-v8a/
libmain.so and spider asset verified. Current Android output SHA256
`6dda4d1761f9acde07d9c9d8a29e11d2a22ef5c517b53ee62a7a2bf8927f5d99`.
Logs `build/spider-android-{preflight,x86-build,arm64-build}.log`. ARM64 build success
is not an ARM-device runtime test. Shaded lighting/acceptance and webs remain open.

### Actual WebGL2 material/pose smoke audit

Focused `spiders` mode now exists in the browser harness and page. It retains an
initial image, four intermediate poses separated by8 requestAnimationFrame callbacks,
and a final image after60 measured intervals. These are real-time poses, not native
deterministic100ms stages. Independent parent checks require the population/material/
shadow proof, valid canopy transfer, live SwiftShader context/glError0,60positivefinite
intervals, four distinct local PNG paths and actual files. No continuous gait or
native image-contrast acceptance is implied.21 browser CPU preflight tests pass,
plus41 native-harness tests and JS/shell syntax checks.

Incremental WebGL2 build/package passes; prepared spider image hash matches native.
First actual run052041.937861Z-chrome-spiders FAILED: old smoke expectations still
pinned pre-spider texture totals8496944/8517160. Corrected to the independently
CPU-pinned8846468/8866684 in focused/fullChrome,Firefox and Android smoke scripts.
No thresholds were weakened; original failed report remains. Firefox/Android were
not run, so updating their expected budget is not platform qualification.

Actual repeat `build/browser-software/20261012T052259.613764Z-chrome-spiders`
parent/child PASS, SwiftShader,1/12visible/2draws/contact1, glError0, context retained,
kernel-final[]. All six images inspected in `build/spider-web-montage.png`.
The textured rounded body and changing leg poses render, but angular limbs, folded
body pattern and dark surroundings remain. Mean rAF interval510.258ms, range
133.4..1016.6ms (60 samples): software diagnostic timing, NOT target-GPU performance
or smooth-motion acceptance. Native contrast failure is unchanged. Package payload
11803579 bytes, gzip4677410 bytes. Android spider runtime, other habitats, webs and
continuous visual acceptance remain open.

### Subsequent near-body refinement

Near body ellipsoids now use24 sides/12 rows instead of12/8. This also increases
vertex-rooted bristle coverage; eyes still fit the actual tessellated front surface.
Leg/palp sections and the entire far LOD are unchanged. Near/far counts are now
7920/1632 vertices, within the existing10000-per-animal/120000-batch capacities;
no texture, buffer-capacity or draw-count increase. Near upload and mesh-generation
work increase86.4%; portable frame-time acceptance remains unmeasured.
Mesh regression measures maximum face-centre radial deviation from the intended
ellipsoids:0.512275mm near versus3.14511mm far, requiring near<0.7mm and<40%of far.
This is a geometry approximation metric, not screen-space or aesthetic acceptance.
Pose-stableUV, normals, winding, habitat bounds and far1632budget remain tested.
Native build and mesh/colony tests PASS8.90s; new mesh regression PASS0.06s.
Actual llvmpipe run051626.813603Z-spiders: all8images inspected in
build/spider-silhouette-montage.png; same-stage enlarged before/after inspected in
build/spider-silhouette-comparison.png. Body outline is rounder, but stiff angular
limbs, dark view and visible folded-texture pattern remain.1/12visible/2draws,
contact1/motion+lightingchecks pass/kernel[]. Outer image contrast stillFAILED.
No threshold changes, hardware or portable runtime. Webs remain absent.

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
