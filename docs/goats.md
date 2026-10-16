# Feral island goats — implementation in progress

**Six goats are now integrated into the running game; a generated short-coat material
has replaced the initial untextured baseline.** Native software-rendered scenes have been inspected; this is not a
completed species or visual/motion acceptance. Earlier milestone evidence below
describes foundations before integration; the current runtime section supersedes
their historical no-renderer/no-live-population statements.

## Deterministic open-slope homes

`GoatHabitat::Find` proposes six homes using the rendered terrain triangles, jungle
layout and the complete supplied collision grid. The reference test supplies palms,
canopy trunks, props, shipwreck proxies and the hut, matching live world obstacles.
The search checks3025 bounded lattice candidates, each with deterministic ±2m jitter
from seed `0x6f617431`. It prefers higher accepted sites, preserves stable ties and
requires eight metres between homes and at most40m from the herd's first home.

Acceptance requires:

- inland terrain at least6m high and35m inside the shoreline;
- centre normal Y in0.90..0.997 (sloped ground, excluding flat floors and steep cliffs);
- a1.25m body-clearance disc outside every static obstacle, enlarged after measuring
  the grazing horn envelope; analytic centre normal Y must also be at least0.90,
  matching the body-frame tilt bound rather than only the rendered triangle normal;
- centre/eight perimeter terrain samples above6m with normal Y≥0.90 and no sampled
  height difference exceeding0.55m from the centre;
- separation from canopy trunks by9m×tree scale, understory by1.25m+1.8m×plant scale,
  and bushes by1.25m+1.2m×bush scale.

These are conservative layout proxies, not measured goat anatomy or a proof that no
leaf overlaps any future model. They must be checked against the eventual goat mesh.
No vegetation is removed, no fallback ignores obstacles and no invisible collisions
are added. If a complete group cannot be found, the search returns no result.

Current reference sites (x,y,z metres, rounded):

| x | y | z |
|---:|---:|---:|
| -64.2131 | 38.4079 | 70.6373 |
| -72.1349 | 36.9021 | 56.3421 |
| -71.7322 | 35.7253 | 46.1564 |
| -72.1036 | 35.4725 | 80.1896 |
| -81.2493 | 33.7004 | 70.7155 |
| -80.0528 | 32.9791 | 81.4526 |

The tests check deterministic reconstruction, exact rendered height, independent
obstacle/vegetation distances, separation/herd bounds, rejection of blocked homes,
a completely blocked island, nonfinite input and offshore input. The first test build
found mismatched obstacle-container types; explicit spans corrected the test assembly.
The accepted-site criteria were not weakened to obtain six sites.

## Grounded walking-pose foundation

`GoatPose` provides a four-beat walking cycle with68% stance, an authored1.2m/s walk
reference, up to25cm stride amplitude and12cm swing lift. Its provisional two-link
leg chains have fixed39/38cm lengths,55mm hoof height and a25mm forward sole-centre
offset. Front/rear bends differ. These are authored modeling dimensions, not a claim
that two links reproduce every anatomical joint of a goat.

The body frame uses smooth analytic terrain orientation and rendered-triangle root
height. Each hoof target is fitted along body-up with18 bounded bisection iterations
against actual rendered triangles. Unreachable targets remain explicitly flagged;
bone lengths do not stretch to hide them. Nonfinite inputs, invalid leg indices,
negative speeds and invalid scale/mesh-range queries are rejected.

`GoatPhaseAdvance` calibrates phase from travelled distance, speed and scale so a
constant-speed straight root translation cancels local stance travel on a flat surface.
Its result is modulo one cycle. This is NOT persistent world-space foot locking:
turns, acceleration, changes in ground basis and continuous slope traversal still
need simulation-level tests and visual review.

Tests exercise13,824 fitted poses across all six homes, eight headings,32 cycle phases,
three speeds and three scales (0.75/1/1.1). All43,776 sampled stance sole centres match
rendered ground within0.15mm, with no unreachable target or stretched bone. Further
tests cover full-cycle/wrap continuity, flat constant-speed no-slip calibration at
three scales, invalid inputs and explicit unreachable-ground reporting. A centre
contact check does not prove that an entire cloven hoof footprint clears all triangles.
Final native build and all43 non-display groups PASS71.25s after this pose addition.
WorldReport remains valid with unchanged fingerprint `b3fde347f77eb5f8`; no renderer,
emulator, KVM or dependency changes. Portable builds/runtime and visual review of
goats remain pending because no live goat renderer/population exists yet.

## CPU herd behavior foundation

`GoatHerd` now constructs six simulated agents from the accepted homes, including
one0.75-scale juvenile. It is not instantiated by `IslandGame`. Deterministic timed
decisions alternate grazing, walking and resting; a finite observer within7m in3D
triggers non-hostile retreat. An observer100m above the herd does not trigger retreat.
The grazing pose eases in only after slowing down. Speed is capped at1.2m/s, with
bounded acceleration/deceleration and at most1.8rad/s turning.

Each valid update consumes at most0.1s in at most five20ms substeps. Static solids and
the same canopy/understory/bush exclusion envelopes are indexed once. Proposed steps
check endpoint/midpoint ground suitability, a containing midpoint collision disc
covering the swept body disc, other goat discs,14m individual home bounds and the40m
herd region. Each accepted endpoint also requires a reachable fitted leg pose. Stride
phase advances only from accepted3D travel, not attempted motion against an obstacle.
Blocked agents stop and request a bounded alternative turn, with a short avoidance
interval so direct-away observer steering cannot immediately cancel it.
There are no teleports, forced penetrations or attacks.

Regression evidence includes90s of duplicated identical-input simulation, all six
agents moving4.75–48.47m, grazing/walking/rest transitions, unchanged initial placement,
no overlaps and agreement with the original non-indexed placement predicates. Nearby
observer clearance reaches8.44m; the added head-on obstacle case reaches7.74m without
crossing its body-clearance boundary. Tests also cover high/invalid observers, invalid
and capped frame deltas, turn limits, and sampled hoof contacts along moving routes.

This is not runtime performance qualification or visual behavior acceptance. Abrupt
blocked-speed changes, persistent world-foot locking, full hoof/mesh contact,
distance-based update throttling and long all-angle observation remain to review.
Midpoint ground sampling is not a mathematical proof for every unsampled terrain
point. The final model must still fit the authored collision/body envelope.
Native build and all44 non-display groups PASS73.92s; herd regression3.90s. WorldReport
remains valid with unchanged fingerprint `b3fde347f77eb5f8` and no live goats reported.
No renderer, emulator, KVM or dependency changes; hardware stop latch unchanged.

## Mesh foundation and CPU shape review

`GoatMesh` builds local-space triangle lists with smooth normals, UVs, material and
anatomical-part tags. Body, deforming neck, head, paired curved horns, ears, beard,
tail and independently fitted legs include split hoof geometry. Grazing lowers and
rotates the head while the neck follows; this is not a verified feeding animation.
Near/far meshes have6252/3516 vertices, each below the8192-vertex guard. There is no
runtime renderer, coat texture or final material yet.

`IslandGoatMeshTests` checks both LODs across16 walking phases and11 grazing amounts:
finite coordinates, unit normals, UV ranges, nondegenerate consistently wound faces,
flat-ground clearance, required anatomy/eye LOD and leg-root coverage by actual torso
triangles. For each sampled vertex, the maximum horizontal projection over all tilt
azimuths up to acos(0.90) is bounded; the largest measured radius is1.17903m at scale1.
The1.25m collision disc covers that sampled envelope and the herd's smaller juvenile,
not arbitrary larger scales or every continuous animation state. Whole-hoof terrain
penetration, persistent planted feet and full body clearance on slopes remain open.

Native tool `island-goat-mesh-preview` writes a CPU-only orthographic SVG, converted
locally to `build/goat-mesh-preview.png` for inspection. The initial inspected image
exposed detached foreleg caps: the chest was too short. Extending the chest and
burying the upper limb attachment corrected the visible gap; the new actual-triangle
root test guards it in both LODs. The revised two-pose image was inspected as well.
It still shows coarse ears, tiny eyes and angular limbs/hooves. Temporary flat
triangle shading exaggerates facets; it is not the game's interpolated lighting,
depth buffer, fur, shadowing or a live animation audit. This is not visual acceptance.

After the clearance change, current90s herd travel is17.36–32.09m per agent; observer
clearance is8.69m and the head-on obstacle case7.95m. The fifth home changed as listed
above; older behavior metrics in this document describe the preceding1.1m envelope.
All13,824 home poses/43,776 stance contacts still pass. Complete native build and
45/45 non-display tests PASS59.51s (`build/goat-mesh-complete-tests.log`). No GPU,
emulator, Android/KVM settings or dependency changes; hardware stop latch unchanged.

## First live integration and native scene audit

`IslandGame` constructs `GoatHerd` after the complete collision grid (including the
hut), updates its six states and submits `GoatRenderer` in the normal scene. It is
not restricted to a QA camera. Renderer uses terrain-fitted poses, world-transformed
smooth normals, CPU daylight/canopy tint and stock untextured vertex-colour BasicEffect
with fog. Distance/frustum culling, near/far LOD at30m and a quality-scaled120m limit
bound work. One dynamic batch reserves49,152 vertices (786,432 bytes of position/colour
vertex storage); the same capacity bounds staging/upload. Texture allocation is zero.
Six full-detail current meshes use37,512 vertices, not the entire reserved budget.
Per-frame CPU mesh allocation/lighting and herd simulation still need profiling and
distance throttling. This is not a hardware performance claim.

Goat body contact ellipses join the existing terrain-conforming shadow batch with
reserved capacity, pose-aware cache invalidation and count accounting. They are not
hoof-specific or directional body shadows. Console, benchmark-visible counts and
debug overlay include goats; WorldReport now constructs the same initial herd and
exports six initial positions/headings/scales. Its fingerprint becomes
`0a98cb638125508f` from `b3fde347f77eb5f8`. Independent JSON tests cover count, finite
positions, separation/herd bounds, one juvenile and five corrupt-report cases.
Procedural provenance is recorded in the manifest and ASSET_LICENSES.md.

`--viewpoint goats` follows translation of the first live goat, with a telephoto
camera initially outside the7m retreat trigger. Native contained llvmpipe run
`build/software-captures/20261012T013223.609582Z-goats` passed, submitted3/6 goats in
one draw and retained an empty kernel delta. The actual1280×720 image was inspected:
the central goat is unobscured and another is partly hidden in grass. Horns/head/body
are readable, but flat beige material, tiny point eyes, crude ears and straight
angular limb segments remain conspicuous. Foot grounding is not established by this
single image; continuous all-angle walking/grazing review is still required.
No AMD hardware was used; Web/Android builds and runtime audits are pending.

Native build and46/46 non-display groups PASS61.98s in the initial integration run.
Final current-tree rebuild and full46 groups PASS70.70s, retained in
`build/goat-live-verified-tests.log`; all owned processes terminated normally.
This proves neither realistic fur nor accepted animal motion. The first capture is
a baseline for improvement, not completion of the user's goat request.

## Goat coat source and material integration

Built-in ImageGen produced original opaque artwork in
`assets/art-studies/goat-fur-albedo-v1.png`; its exact prompt is retained in
`assets/art-studies/goat-fur-albedo-v1-prompt.txt`. The source was visually inspected:
warm ivory/grey-beige fine hair with mild directional variation, no silhouette/text.
This is AI-generated project artwork, not a photograph/scan or a claim of an exclusive
license. Both files are hash-gated in the manifest and attributed in ASSET_LICENSES.md.
Original3,986,448-byte PNG stays unchanged. The existing deterministic area reducer
prepares a256-square139,062-byte PNG; runtime loads9mips totaling349,524 RGBA8 bytes.
Missing or invalid material is a named startup error, not silent plain-colour fallback.

`GoatCoatUv` maps into the interior6–94% of the source. Circumferential coordinates
mirror continuously through a cosine window rather than wrapping unverified source
edges; body hair flows head-to-tail and limb/head coordinates flow toward their tips.
This is an authored mapping, not anatomically qualified groom direction at every
joint. The first runtime texture capture exposed unequal station-index stretching:
loft UVs now follow physical longitudinal distance and sweep UVs follow path arc length.
The body-distance regression guards this; circumferential compression and joint seams
still need close/moving inspection.
Mesh tests cover interior ranges, seam continuity and directional ordering; prepared
artwork is compared pixel-for-pixel with the existing C++ area-filter oracle.

Coat geometry and bare horn/hoof/eye/nose parts share one dynamic upload but draw in
two opaque stock BasicEffect passes. Bare parts do not sample the coat image. No
custom shader, transparent shells or additional skeletal uniforms are introduced.
Vertex layout now includes UVs:49,152×24=1,179,648 reserved GPU vertex bytes. The main
and solid staging vectors each reserve that capacity; this CPU overhead remains a
profiling target. Current near/far geometry counts are unchanged.

CMake stages/installs the texture and preloads it for Web; Android's existing material
task stages the same path. Runtime texture payload is now8,496,944bytes, with an explicit
349,524-byte addition to both full/mobile policy (8,517,160bytes). Generated-only
payload stays4,652,172bytes. These exclude driver padding, decoder/shadow copies and
framebuffers; changing the accounting is not a mobile memory/performance qualification.
Web/Android runtime qualification remains pending; build evidence is recorded below.

Native textured baseline `20261012T014053.532859Z-goats` and corrected length mapping
`20261012T014251.605524Z-goats` both passed contained llvmpipe capture, with actual images
inspected and empty kernel deltas. The latter confirms3/6 goats in2draws, coat PNG
loaded and8,496,944/8,517,160 texture accounting. Hair variation is visible on body,
head and legs; bare parts remain dark/untextured. The source's directional groups and
mirrored mapping still read as overly broad streaks/swirl around the neck at this
distance; point eyes, crude ears and joint anatomy remain unfinished. This is an
improvement over the plain baseline, not accepted realism or motion quality.
Final native build/full46 non-display groups PASS63.57s; initial material run also
passed46/46 in68.52s. Original, first textured and corrected captures are retained.
WebGL2 build/package passes:11,551,789bytes raw/4,497,328gzip. ARM64 Android assembly
and lint pass10s; APK SHA256 `e2363bdbb5ceeb7063f63f4a75ff6bc40fdb10f50cf3f148489cfe2be40a791f`.
APK includes139,062-byte goat PNG; prepared native/Android PNG SHA256 both
`66ecf3800685de5613d5d155d58f264179945b70fe4a087c122c81d5036ad316`.
Chrome/Firefox/Android texture-accounting pins now match the new inventory, Android
APK preflight requires the coat asset. Android/browser CPU preflight2/2 PASS2.29s;
both JavaScript launchers and shell launcher pass syntax checks. No actual browser
or Android goat runtime is claimed, and no emulator/KVM configuration was touched.

## First multi-stage motion audit and hoof support correction

Native `--capture ... --viewpoint goats` now uses one0.1s simulation step per presented
frame and64frames, retaining8images at0.8s intervals. This deterministic clock is
capture-only; ordinary play and the existing snake capture clock are unchanged.
The ordinary herd/controller is used without teleporting agents or forcing behavior.
Following-camera translation keeps the first animal in view while its heading changes.
The log records position/speed/stride phase/grazing state and update count. The validator
requires ordered finite states, the exact clock, visibility, bounded travel, walking
phase change and a grazing transition;12corrupted fixtures are rejected. It is not a
visual naturalness oracle or a continuous stance-locking proof.

Actual llvmpipe runs `20261012T015104.568755Z-goats` (before hoof correction) and
`20261012T015711.270483Z-goats` (after) passed with empty kernel deltas. Both8-stage
montages were inspected; the corrected stage3 leg detail was additionally enlarged
and inspected (`build/goat-contact-stage3-detail.png`). The first goat raises its head,
turns and walks down the slope, with2.44m net displacement over the sampled interval.
Initial grazing amount0.8 becomes0 and stride phases advance. The final log also
passes the strengthened validator after its travel/phase bounds were added.
These sparse stills do NOT establish smooth real-time motion from every viewpoint.

The audit prompted a new check of actual near-hoof mesh vertices along the existing
90s six-goat CPU route, sampled once per simulated second, rather than only sole centres.
It failed at1.75476mm terrain penetration (`build/goat-hoof-audit.log`, retained).
This was an intentional CPU regression-test failure, not an AMD/GPU incident.
Each fitted leg now carries the rendered support normal in body coordinates. The hoof
mesh rotates minimally about its sole-centre pivot to that normal, including its shading
normals; the limb IK/bone lengths, sole centre, gait and world placement are unchanged.
Worst sampled penetration falls to0.804901mm, within the unchanged1mm vertex tolerance.
Residuals near adjacent terrain triangles are not zero. Analytic tilted-plane tests
cover both LODs, unit normals and invalid normal rejection. Arbitrary mesh/terrain
triangle interiors, full continuous trajectories and persistent planted-foot locking
are still not proven by the vertex samples.

Visual priorities from the enlarged review: limbs remain overly angular and uniform,
hooves look blocky, eyes tiny; mirrored coat flow forms a conspicuous swirl on the rump.
Those larger appearance defects are not excused by the sub-millimetre contact result.
Final native build/full46 non-display groups PASS106.71s; capture preflight after the
last validator changes PASS0.19s. No new Web/Android build/runtime for this hoof/clock
change, no emulator/KVM/dependency changes, and the hardware stop latch is unchanged.

## Flank coat correction and verification-preset repair

The body/head loft uses x=rx*cos(angle), y=centre+ry*sin(angle). The original coat
mirror used cos(angle), folding the image through the middle of each visible flank.
Body/head now use ring height (sin); the mirror lies on back/belly ridges instead.
Sweep-part mapping, source image, longitudinal direction, geometry, allocations and
draw count are unchanged. New tests check cardinal coordinates and matching-height
left/right sides, in addition to the seam/range/direction checks.

Native llvmpipe `20261012T020419.131486Z-goats` passed the8-stage live sequence with
empty kernel delta. The actual montage and a same-stage enlarged before/after pair
(`build/goat-flank-comparison.png`, old left/new right) were inspected. Side hair now
flows more coherently along the body; the central mirrored whorl is reduced. Rump
taper distortion, coarse strand grouping, angular legs and other anatomy remain open.

Verification correction: the first full-suite run for this change used the old curated
native build target list, which omitted the three goat test binaries. Their existing
binaries could therefore pass without compiling the latest test edits. This was proven
by GoatMeshTests.cpp being newer than its executable; that initial green log is NOT
evidence of the new flank regression, and older preset-only runs cannot alone establish
that their latest goat test edits were compiled. Explicit goat-target builds and actual
runtime captures remain separately recorded evidence.
Debug/sanitizer build presets now select the configured `all` target (CNA's unrelated
tools remain EXCLUDE_FROM_ALL); linux-core tests exclude only the display label instead
of a stale test-name allowlist. New preset regression rejects four narrowed/unsafe
configurations. The corrected build explicitly compiled GoatMeshTests.cpp and rebuilt
the preview; the subsequent actual linux-core preset passes47/47 in87.05s, including
the latest tilted-hoof-plane and flank-map checks. Sampled hoof clearance remains
-0.000804901m. No sanitizer run or fresh Web/Android build/runtime is claimed here.

## Limb volumes and lower cloven hooves

Both LODs now retain six longitudinal limb stations: distinct front/rear upper
volumes and a tapered lower segment follow the existing two-link chain. Joint
positions, bone lengths, sole pivots, gait and herd routes are unchanged. Near/far
counts are now6636/3756 vertices (formerly6252/3516); six near animals use39816 vertices.
The per-animal8192 and batch49152 guards, allocations, textures and two draws remain
unchanged. Hoof walls are70mm high rather than110mm, with paired width96mm rather
than114mm and a minimum8mm toe gap. The flat sole remains55mm below the ankle and
retains the individual terrain-normal rotation.

Regression checks measure actual upper-ring radii in both LODs and bound hoof wall
height/width, alongside existing winding, normals, support-plane and terrain tests.
Current inclusive native build plus linux-core passes47/47 in54.47s; logs are
`build/goat-leg-hoof-build.log` and `build/goat-leg-hoof-tests.log`. The90s sampled
herd route has minimum actual near-hoof vertex clearance-0.000736237m, within the
unchanged1mm tolerance; this does not establish zero penetration everywhere.

Contained llvmpipe `20261012T022306.275860Z-goats` passes all eight motion stages;
kernel-after.json is empty. Both `build/goat-leg-hoof-comparison.png` (previous
flank-only model left/current right) and `build/goat-leg-hoof-motion-montage.png`
were actually inspected. Smaller hooves remove much of the oversized-boot silhouette;
upper limb volume improves modestly, but angular joints, crude ears/eyes and coat
taper distortion remain visible. Sparse frames do not qualify continuous stance
locking or natural gait. No current Web/Android build/runtime qualification, hardware
GPU run, emulator/KVM change or dependency change. AMD incident remains unresolved.

## Focused software WebGL audit

The contained Chrome supervisor now accepts `--scope goats`; the page starts the
normal `--viewpoint goats` game, not a replacement demo. It captures the initial
view, four intermediate images separated by eight requestAnimationFrame callbacks,
and a final image after60 measured frame intervals. These are real-time samples,
not the deterministic native capture clock or a continuous stance-locking test.
The gate requires SwiftShader, a live error-free WebGL2 context, loaded256px/9-mip
coat, visible1–6/6 goats and both coat/bare-part draws. The outer supervisor also
validates the report's population/material/context and four distinct pose filenames.
Twenty BrowserSoftwareTests pass, including corrupt goat reports for missing coat,
zero population, missing draw, duplicate/missing images and dead/error contexts.

Actual run `build/browser-software/20261012T023517.624374Z-chrome-goats` passed on
Chrome155.0.8059.39/SwiftShader:60 intervals, glError0, contextLost=false, outer
supervisor passed and final kernel delta empty. All six images (initial, four poses,
final) were actually inspected, intermediate images together in
`build/goat-web-motion-montage.png`. Coat and reduced hooves render; the animal turns
and changes position. Rear coat taper has a conspicuous star pattern; ears remain
crude, eyes poorly readable and limbs angular. This is functional WebGL evidence,
not accepted realism, continuous gait, hardware performance or AMD crash resolution.
Updated Web package11559073 raw/4499299 gzip bytes. No Android build/runtime this turn.

Reproduction (existing Web build, no hardware access):

```sh
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope goats --timeout 600'
```

## Physical-height body coat projection

The actual native/Web rear views exposed a star-shaped fur squeeze: normalized ring
height still mapped the full source width onto each shrinking rump ring. Body UVs
now use physical longitudinal position and physical height (`(y-.52)/.56`) before
body bob, including cap centres. GoatCoatUv consumes that body coordinate directly;
head/sweep mappings are unchanged. This removes ring-radius-dependent hair spacing
without modifying geometry, animation, assets, draw calls, vertex layout or memory.
Tests cover actual body vertices in both LODs/all sampled poses, physical z/height,
bob compensation, source window and the separate non-body circumferential seam.

Inclusive native build and47/47 CPU tests PASS94.68s, logs
`build/goat-height-uv-{build,tests}.log`. Contained native8-stage sequence
`20261012T024124.340743Z-goats` PASS, empty kernel delta. All eight images and the
same-stage rear before/after were actually inspected in
`build/goat-height-uv-montage.png` and `build/goat-height-uv-comparison.png`.
The rear star pattern is removed in that comparison, but the planar projection
produces overly horizontal rear hair bands; directional rear mapping and a finer
coat remain open. No final realism/continuous-gait acceptance. Latest actual Web
audit above predates this mapping change; no fresh Web/Android build/runtime here.

## Thin cupped ears

The original circular ear tubes are replaced by a seven-station leaf with paired
front/back surfaces and a closed12mm-thick rim. Front centre is recessed relative
to its edges; the existing bare Skin material occupies the interior, not an overlay.
Both LODs retain the tapered longitudinal shape (five/three transverse samples).
Head attachment, grazing transform and body bob are shared with the existing head;
the rest of the anatomy/animation is unchanged. No texture, shader or draw added.
Tests require symmetric nonempty interiors, forward-facing skin, attachment/length
and depth bounds, plus the existing all-pose winding/normal/budget validation.

Actual contained native `20261012T024656.529510Z-goats` passes eight stages with empty
kernel delta. The enlarged same-stage comparison `build/goat-ear-comparison.png`
and all eight images in `build/goat-ear-montage.png` were inspected. The forward
view has thinner readable ears with a contrasting inset; rear still has furred backs.
They remain angular, while point eyes and crude leg joints remain larger anatomy
defects. No full goat realism, continuous animation or portable acceptance claimed.

The first CPU run failed one supervisor fixture because its marker was absent after
the4s startup deadline (test_supervisor_sigkill_stops_detached_descendants). Evidence
is retained in `build/goat-ear-tests.log`; this is not a GPU fault. The runtime capture
was concurrent with that CPU suite, so the repeat runs without it. The new ear-specific
assertions were added after the first build and are compiled explicitly in
`build/goat-ear-current-build.log`; only the subsequent run qualifies those assertions.
That current full47-test run passes in80.77s (`build/goat-ear-current-tests.log`),
including the supervisor in1.78s; its startup limit was not relaxed. Current meshes
are7068/3996 near/far vertices (+432/+240), six all-near42408, below the unchanged
8192/animal and49152/batch guards. Sampled hoof clearance remains-0.000736237m and
world fingerprint0a98cb638125508f. No new Web/Android build/runtime or hardware run.

## Surface-fitted near eyes

The near model now has a52×28mm oval dark lid border,44×22mm ochre iris and30×5mm
horizontal pupil, replacing the tiny black loft. The three contours and centre fit
the actual head triangle surface by side-ray intersection after grazing deformation;
their1.5/3.5/4.5/5.5mm side offsets give shallow relief. Head normals are interpolated.
The Iris material is a new bare vertex colour, added to renderer and CPU preview
palettes; it shares the existing bare-part draw. No new texture/shader/draw/buffer.
Near/far7140/3996 vertices, six near42840, below unchanged budgets.

Tests require near-only eye/iris materials and check every eye triangle's vertices
and centroid against the underlying head at11 grazing amounts: clearance must remain
between0.1 and7mm. This is sampled triangle contact, not a complete raster-depth or
continuous surface proof. Existing pose/budget/winding/unit-normal checks remain.
Full native47/47 PASS66.81s (`build/goat-eye-{build,tests}.log`); goat mesh test1.12s.
Hoof clearance/world fingerprint unchanged. Actual contained native8-stage capture
`20261012T025612.436707Z-goats` PASS/kernel[], same-stage enlarged comparison
`build/goat-eye-comparison.png` and all stages in `build/goat-eye-montage.png`
actually inspected. Iris/dark border are more readable than the previous dot; pupil
detail remains limited at this camera scale. Orbital head shape, muzzle, coat direction,
joint anatomy and continuous gait remain open. No current Web/Android qualification.

## First actual Android goat view

The isolated Android software supervisor accepts `--viewpoint goats` and requires
the exact coat/mip marker plus1–6/6 visible goats and two draws. The shell waits for
that evidence before taking its screenshot. IslandGame emits a one-time goat render
proof after the first GoatDetail draw, mirroring actual renderer counts to logcat;
this is not a hard-coded pass or per-frame stream. Twenty-three Android preflight
tests pass, including missing coat/count/draw/mip/proof-marker rejection.

Initial `20261012T030243.275234Z-goats` rendered a correct-looking image but the outer
gate FAILED: ordinary stdout visibility totals were absent from Android logcat.
Its evidence remains; it is not counted as a qualified run. The one-time mirror
above corrects that diagnostic gap without weakening the gate. Actual repeat
`build/android-software/20261012T030631.137902Z-goats` passes outer supervisor,
OPENGLES3/SwiftShader,3/6 visible/two draws, accepted1280×720 image and empty final
kernel delta. Emulator exits0. Both run images were actually inspected: current
height-mapped coat, cupped ears, fitted eyes and smaller hooves render with mobile
touch controls; coarse coat bands and angular legs remain apparent. This is a
single-view functional/visual check, not continuous movement, lifecycle or phone
performance acceptance, and does not qualify the unresolved AMD crash.

Verified x86_64 APK retained as `build/android-software/island-goat-eyes-x86_64.apk`,
SHA256 `b4b4db56ae51573093187ddac86881a9e8d5a00a54114c40207d1cb6300bb88c`.
x86_64 assembly/lint passes; default ARM64 assembly/lint restored and passes7s.
No AVD/KVM/driver/dependency configuration changed. Existing launcher removed a stale
owned AVD lock only after its no-live-emulator check; it did not alter shared AVDs.
Latest actual Web audit still predates height mapping, ears and eyes.

## Continuous CPU stance audit — confirmed failure, not accepted gait

`build/island-goat-stance-audit` now measures world-space sole-centre movement at
20ms intervals. It reconstructs the complete world obstacles including the hut,
runs all six actual herd agents for30s with a distant observer and another30s with
a fixed nearby observer, and fits the same terrain pose used by the renderer.
Adjacent samples count only when the same foot is in stance in both; a swing resets
the anchor. Distances are horizontal x/z, not vertical terrain-fit errors. This is
continuous CPU sampling, not a video or proof about every rendered hoof triangle.

The auditor first checks a known10mm slip/reset fixture and a straight flat0.65m/s
calibration, which cancels body translation to within0.00000012m per step. Two actual
runs are byte-identical (`build/goat-stance-current.jsonl` and `goat-stance-repeat.jsonl`).

| Scenario | Stance pairs | Pairs over1mm | Maximum20ms movement | Maximum distance from stance start |
|---|---:|---:|---:|---:|
| Flat straight calibration |261|0|0.000000119209m|0.000000715256m|
| Undisturbed herd30s |30342|5588|0.122717m|0.948285m|
| Nearby observer30s |29133|5609|0.132466m|0.873171m|

Worst undisturbed step:15.16s, goat3/right rear leg2, speed0.65→0, no heading change.
Worst observer step:15.44s, goat0/right rear leg2, speed0.698→0, heading change-0.036rad.
The source explains a definite mechanism: failed TryMove sets speed=0 immediately,
while EvaluateGoatLeg's stride amplitude depends on speed; changing to zero moves
each hoof back below its root regardless of an existing stance. Turning a blocked
body can also rotate all grounded feet without a swing because phase advances only
with accepted translation. Small sole-height errors and the earlier straight-speed
calibration never established foot locking, and the new data explicitly disproves it.

Next fix must retain per-foot world stance anchors, release/replant feet through a
bounded swing during turns/stops, and preserve fixed limb lengths, terrain support,
collision clearance and determinism. Share the resulting pose between simulation
validation and rendering; do not merely hide the measured displacement or relax
the audit. Rerun both cases plus continuous visual review after implementation.
Tool exit0 means measurements/calibration succeeded, NOT that gait is acceptable.
No game movement or mesh was changed in this audit; no GPU/emulator was used.
Inclusive native build and existing47 CPU tests pass71.12s; logs are
`build/goat-stance-current-build.log` and `build/goat-stance-current-tests.log`.
These passes verify existing invariants, not the newly disproved stance requirement.

## Three-dimensional fixed-length leg targeting foundation

RetargetGoatLeg now solves a target with lateral as well as vertical/longitudinal
offset from the leg root. Both39/38cm bones keep their lengths; unreachable targets
are clamped to the reachable shell and explicitly marked reachLimited. The reference
root, stance flag and sole normal are preserved. A projected fore/hind bend direction
matches the old sagittal solver, with an axis-aligned fallback. EvaluateGoatLeg now
uses the shared solver after its existing ground-height constraint.

Tests cover180 reachable lateral targets, zero-distance/too-far/axis-aligned targets,
nonfinite input, bone lengths, metadata and128 legacy sagittal knee comparisons.
Inclusive native47/47 CPU PASS64.07s (`build/goat-retarget-{build,tests}.log`). Existing
world fingerprint0a98cb638125508f and sampled hoof clearance-0.000736237m unchanged.
The complete stance audit (`build/goat-retarget-stance.jsonl`) remains byte-for-byte
equal to its prior baseline: worst122.717/132.466mm steps are still present. This is
the geometry prerequisite for planted world anchors, NOT a foot-sliding fix. Persistent
per-foot state, world-to-body target conversion and swing/plant transitions remain
to be integrated into herd simulation and rendering. No new runtime visual claim.

## Live persistent world-space foot supports

GoatHerd now retains four world-space sole positions per animal. During stance they
remain fixed; a swing interpolates from the previous support to a terrain-sampled
landing using a180ms smoothstep and a90mm-scaled vertical arch. At most two feet
swing, with at most one new swing per20ms substep. Existing four-beat phase chooses
walking lifts; displacement-based replants let a stopped/turning body reposition
its feet. Landings are checked for fixed-length reachability before adoption.

GoatState stores the authoritative GoatGroundPose. Candidate body translations and
blocked turns check the same contact-fitted legs, and GoatRenderer consumes that
stored pose rather than reconstructing a sliding speed-dependent one. A body-frame
helper avoids solving four unrelated ground targets repeatedly during contact checks;
the renderer's redundant RenderedTerrainSurface cache is removed. No new mesh,
texture, shader, draw call or GPU allocation. Per-foot world orientation is not yet
locked: the hoof may pivot about its fixed sole centre as the body turns.

Initial integration failed head-on avoidance: a temporarily unreachable anchored
foot was treated as a new obstacle, repeatedly changing route intent. TryMove now
distinguishes waiting for a step from geometry blockage, retaining route intent in
the former case. A0.1mm contact separation keeps rounding/triangle-boundary effects
inside the UNCHANGED0.15mm centre-contact and1mm mesh-penetration tolerances.
Intermediate failure logs (`goat-anchors-{tests,recovery-tests,contact-tests,probe-tests}.log`)
are retained; neither those failures nor earlier partial passes are final qualification.

Final inclusive build and47/47 CPU tests PASS111.75s
(`build/goat-anchors-final-{build,tests}.log`). New live-state regressions require
consecutive planted positions within0.1mm, at least two supports, reachable limbs,
inverse transforms and actual mesh inside the1.25m collision envelope. Existing
determinism, invalid inputs, observer/obstacle escape and terrain checks remain.
Ninety-second per-goat travel8.50997–27.7957m; observer/head-on clearances7.57935/7.01376m.
Minimum sampled near-hoof mesh clearance-0.000240326m. Initial world fingerprint
0a98cb638125508f unchanged.

Updated20ms audit uses the SAME authoritative pose as rendering and rejects an
immobilized herd: quiet25563/observer23569 stance pairs, zero pairs over1mm and zero
measured horizontal displacement/drift at float precision. Minimum per-goat30s travel
2.87275/3.09094m. See `build/goat-anchors-final-audit.jsonl`; previous122.717/132.466mm
jumps are absent in these cases, not proven impossible on every future route.

Contained native `20261012T034322.249615Z-goats` passes all eight motion stages with
empty kernel delta; every stage was inspected in `build/goat-anchors-montage.png`.
Walking/grazing/turning still occur with the changed leg poses. Sparse stills do not
qualify continuous natural gait, hoof rotational locking, self-contact or smooth body
acceleration; those remain open. No current Web/Android build/runtime qualification.

## Portable runtime check after planted-foot integration

Current Web build/package passes11589087 raw/4512594 gzip bytes. Twenty browser
preflight tests and JavaScript syntax pass. Actual contained run
`build/browser-software/20261012T035115.028629Z-chrome-goats` passes on
Chrome155.0.8059.39/SwiftShader:60 frame intervals, glError0, contextLost=false,
3/6 goats/two draws and empty final kernel delta. All six images were actually
inspected (initial, four poses in `build/goat-anchors-web-montage.png`, final).
Walking/turning continue with the authoritative pose; recent eyes/ears/height-mapped
coat render too. Rear horizontal hair bands and angular legs remain visible. This
supersedes the old pre-anchor Web check, not continuous visual gait or hardware
performance qualification; the quantitative stance proof remains the native CPU audit.

Current x86_64 Android assembly/lint passes7s; all23 Android preflight tests pass.
Actual isolated `build/android-software/20261012T035719.719637Z-goats` passes the
outer supervisor on OPENGLES3/SwiftShader, with3/6 visible goats/two draws, empty
final kernel delta and emulator exit0. Its1280×720 image was actually inspected:
the current grazing pose, coat, cupped ears, eyes and feet render with mobile controls.
This is one presented view, not Android continuous gait/lifecycle qualification.
Verified APK: `build/android-software/island-goat-anchors-x86_64.apk`, SHA256
`6b3f1133e0b0dfa1bda3580ded05a00c139d2391b1d7a8c39975aca161e0b64c`.
Default ARM64 assembly/lint restored and passes9s; APK SHA256
`d1b4b25ca0b9ef3c2e6cee0a0dbbce2f2586d4d89b1bd5f7a1d1f3b5e98ef98f`.
These packages include code29c23a0. No game-code change, hardware run, dependency
change or Android/KVM configuration change occurred during this qualification.

## Barrel, limb-root and coat-direction refinement

The renewed whole-island animal audit selected the goats after the deer pass. Close views showed
three related defects: the barrel collapsed into sharp cones at both ends, the planted leg chains
emerged as narrow sticks without shoulder or haunch mass, and the otherwise detailed source fur
ran longitudinally across the flank so that it read as engraved horizontal scratches.

The body loft now uses thirteen rings rather than eleven. Two closely spaced terminal tapers keep
the mesh closed while preserving broad rump, barrel, brisket and shoulder volumes. Each leg keeps
the exact same planted root, knee, hoof, fixed bone chain and sole pivot, but gains a seventh
longitudinal station plus larger, body-overlapping shoulder/haunch and upper-limb radii. The body
UV projection is rotated so the source image's vertical fibres follow physical body height; head,
neck and limb sweeps keep their existing longitudinal mapping. The source image, mip chain,
materials, six-agent population, behaviour, sounds, two draws and buffer allocation are unchanged.

Near/far meshes now contain 7476/4188 vertices; six all-near animals use 44856 vertices beneath
the unchanged 8192-per-animal and 49152-batch guards. Updated mesh tests cover the 1.22 m physical
body window, downward height mapping and 75/90 mm front/rear upper-limb volumes across both LODs,
sixteen gait phases and eleven grazing amounts. The focused test reports the unchanged conservative
horizontal bound of 1.17903 m.

The CPU orthographic before/after was inspected, then the actual contained llvmpipe capture
`20261015T142227.065114Z-goats` passed at 1280x720 with 238 colour bins, 58.24 luminance deviation,
252.91 spatial contrast and an empty kernel delta. A same-camera comparison confirmed a fuller
silhouette and vertically falling flank hair rather than the prior bands. This is a focused native
appearance check, not physical AMD/Android qualification or proof of natural motion from every
angle. Hoof rotational locking, self-contact and subjective call listening remain open.

## Lower-leg and hock refinement

A later bear/resident/goat comparison found the goat's lower leg to be the weakest close-view
animal detail. The planted-foot solver already supplied fixed-length root, knee and hoof targets,
but the mesh joined the knee to the hoof as an almost straight narrow tube. The existing
intermediate lower-limb ring now receives a small sagittal offset: 24 mm on front legs and 52 mm
on rear legs in local model space. The cannon/pastern ring grows from 30/35 mm to 38/45 mm and the
ankle attachment from 24 to 28 mm. The actual knee, hoof, support normal and sole pivot do not move.

This is a coordinate/radius-only edit to the existing seven-ring sweep. Near/far counts remain
7476/4188 vertices, the all-near six-goat load remains 44856 vertices, and there is no added
buffer, draw, texture, collision or simulation work. `IslandGoatMeshTests` and `IslandCoreTests`
pass. Same-camera baseline `20261015T171900.703139Z-goats` and accepted capture
`20261015T172630.933666Z-goats` were inspected side by side. The accepted 1280x720 image passes
with 235 colour bins, 58.25 luminance deviation, 252.91 spatial contrast and an empty kernel delta;
it shows a fuller planted lower leg and a visible rear hock break without disturbing the cloven
hoof contact. Continuous all-angle gait and physical Android/AMD appearance remain open.

## Shoulder, waist, neck and herd-colour refinement — 2026-10-16

The next cross-species audit selected the goats as the weakest ordinary close terrestrial animal.
Although the planted legs and fur source were sound, the barrel still read as one pale round mass,
the shoulder entered the neck through a visible short collar and all six animals shared the same
near-white coat. The thirteen body rings now separate a fuller haunch, tucked waist and raised
shoulder. The neck grows from four abrupt circular stations to six closely spaced taper stations
that bury their root in the chest and carry the line continuously toward the head.

The shared neutral hair image now receives six deterministic cream, fawn, grey, warm-brown and
sable vertex tints, plus a restrained darker-belly/lighter-topline response. This creates distinct
feral individuals without another texture, material, draw or state field. The expanded body-height
projection remains based on undeformed physical coordinates, so fur does not swim with body bob or
squeeze around the new shoulder profile. Near/far meshes contain 7,620/4,260 vertices; six all-near
animals require 45,720 vertices beneath the unchanged 49,152 batch guard and remain exactly two
draws.

All five focused native groups (`IslandCoreTests`, goat habitat, herd, mesh and world-report) pass,
as do all 41 browser/capture harness tests. Inspected native eight-stage evidence
`20261015T223701.422223Z-goats` passes at 1280x720 with 218 colour bins, 58.32 luminance deviation,
252.91 spatial contrast and an empty kernel delta. The sequence shows the raised shoulder, waist,
continuous neck and distinct tan/dark animals through walking poses without detached limbs.
Focused Chrome/SwiftShader `20261015T224046.143766Z-chrome-goats` passes four distinct inspected
motion frames, 60 finite intervals, GL error zero, a live context and an empty final kernel delta.
Physical Android/AMD appearance and subjective call listening remain outside this focused pass.

## Helicopter-clearing habitat exclusion — 2026-10-16

A renewed eight-stage audit initially framed the nearest goat through the helicopter's open cabin.
This was not merely a camera problem: its home at `(-54.8407, 48.9365)` was only about 4.59 m from
the helicopter centre, inside the authored 6.20 m clearing. Wildlife avoidance used the 3.20 m
fuselage proxy, which prevented contact with the airframe but still permitted grazing beneath the
doors and rotor disc.

The shared wildlife obstacle now reserves `HelicopterSite::ClearingRadius` rather than the smaller
fuselage proxy. Goat placement and movement add their existing 1.25 m body clearance, so a complete
animal remains at least 7.45 m from the centre. The same obstacle is supplied to the resident,
bear, monkeys, boars, snakes, deer and spiders; their existing collision-aware controllers gain the
same coherent clearing boundary without a new per-frame query. The standalone world report now
uses and reports the same radius, and its goat validator rejects a population member moved into
the clearing.

Accepted native evidence `20261016T054652.958008Z-goats` starts the selected goat at
`(-57.6012, 55.7556)`, about 10.16 m from the helicopter centre, and exposes all eight motion
stages on an unobstructed open slope. It passes at 251 colour bins, 56.87 luminance deviation and
254.51 spatial contrast with an empty kernel delta. Eight focused native groups passed before the
world-report contract was tightened; the rebuilt world-report, habitat, herd and core groups pass
again afterward. Focused Chrome/SwiftShader
`build/browser-software/20261016T055151.536764Z-chrome-goats` shows four distinct inspected motion
frames in the new habitat and passes 60 finite intervals, GL error zero, a live context and an
empty kernel delta. This fixes habitat composition and audit visibility; it does not claim final
all-angle anatomy or subjective call acceptance.

## Surface nostrils and fitted mouth crease — 2026-10-16

The next unobstructed close audit selected the muzzle rather than the already readable furred
barrel, planted legs, horns or beard. The old head ended in one smooth dark nasal volume without
nostril openings or a lip boundary. Detailed goats now add two shallow eight-sided oval nostril
patches just above the existing nose cap and a short three-station lateral mouth crease on each
side. The crease follows the changing lower-head radius instead of floating as a straight line;
both features inherit the existing head grazing transform.

The addition is near-LOD only and shares the existing solid-anatomy pass. Near/far geometry is
7,848/4,260 vertices, so six all-near goats require 47,088 vertices beneath the unchanged 49,152
upload guard. It adds no texture, buffer, draw, simulation or collision work. Seven focused native
groups and all 41 browser/capture harness tests pass. Inspected native eight-stage evidence
`20261016T063041.590394Z-goats` passes at 1280x720 with 253 colour bins, 56.87 luminance deviation,
254.51 spatial contrast and an empty kernel delta. The close crop confirms that the nostrils stay
inside the nasal plane and the mouth follows the lower muzzle without producing projecting beads.
Focused Chrome/SwiftShader `20261016T063240.164159Z-chrome-goats` passes four distinct inspected
motion frames, 60 finite positive intervals, GL error zero, a live context and kernel `[]`.
Physical Android/AMD appearance, continuous all-angle review and subjective call listening remain
outside this focused pass.

## Remaining implementation

- [~] Fixed-length terrain-fitted goat legs: numerical foundation above implemented.
  Whole cloven-hoof contact, persistent stance behavior and the final body envelope
  still need validation with the actual model and live motion.
- [~] Head/body/ears, horns/beard, cloven-hoof mesh, surface nostrils and fitted mouth crease plus
  the focused native/Web appearance passes above are implemented. Further joint anatomy and
  continuous all-angle visual acceptance remain.
- [~] Grazing/walking/rest/retreat CPU controller and bounded collision-aware movement
  implemented above. Browsing details, gait transitions and visual acceptance remain.
  The40m grouping radius is NOT a traversable-area guarantee; initial sites do not
  establish connectivity or safe paths between them.
- [~] Actual population/update/renderer integration, lighting/contact shadows and vertex
  capacity implemented above; CPU/performance budgets and portable runtime remain open.
- [x] Live diagnostic counts and initial WorldReport coverage after runtime integration.
- [~] Unobscured native and Chrome/SwiftShader appearance is current through the latest
  barrel/limb/coat, hock and muzzle passes. Physical Android and continuous all-angle motion remain
  open.

The AMD hardware stop latch remains unchanged. No Android emulator/KVM or dependency
configuration was changed for this integration.

## Local-space feral coat markings — 2026-10-16

A fresh current-binary eight-stage audit accepted the planted limb chain, horns, beard, face and
directional hair source, but the large coat regions still read as one flat tint per animal. The
renderer now modulates only fur vertices with bounded masks authored in undeformed goat-local
coordinates. Every animal receives a narrow dorsal tone; three selected palettes carry unequal
broad flank or shoulder-cape areas, the nearest buck's cape continues through its neck, selected
animals receive a soft pale facial blaze, and three carry darker lower-leg points. Eyes, nose,
mouth, ear skin, horns and hooves remain outside the fur treatment.

The masks are evaluated before the existing pose transform, so body bob, planted-foot motion and
world heading cannot make a marking slide over the animal. They reuse the existing neutral fur
atlas, six palette colours and textured coat draw. Near/far geometry remains 7,848/4,260 vertices,
the all-near population remains bounded beneath the existing 49,152-vertex batch allocation, and
there is no new texture, buffer, draw, state, simulation or collision work.

Seven focused native groups pass, as do all 97 browser/capture harness tests. Same-camera native
baseline `20261016T103705.952734Z-goats` and accepted
`20261016T104401.379617Z-goats` were inspected across all eight motion stages. The accepted
1280x720 result retains distinct movement and passes at 253 colour bins, 56.91 luminance deviation,
254.51 spatial contrast and an empty kernel delta. Focused Chrome 155/SwiftShader evidence
`build/browser-software/20261016T104712.598400Z-chrome-goats` was inspected across four distinct
poses and passes 60 finite positive intervals, GL error zero, a live context and kernel `[]`.
Physical Android/AMD appearance, continuous all-angle gait and subjective call listening remain
outside this bounded material pass.
