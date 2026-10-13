# Ground snake implementation and visual evidence

## Dorsal readability and tongue flick — 2026-10-13

The upper third of the existing olive body now carries a restrained lighter dorsal
saddle while the pale belly and darker flanks remain distinct. This is vertex colour
under the existing scale atlas, so all 50 live animals retain the same one bounded
dynamic draw, texture allocation and 16-nearest render cap. A moving, non-resting snake
can briefly extend a narrow red tongue with a split tip. The flick follows its staggered
locomotion phase, adds four triangles only while active and creates no draw or texture.

Actual llvmpipe `20261013T130651.826608Z-snake-head` was inspected: the fine scales,
near eye and red forked tongue are visible between unmodified grass blades. Its generic
image gate passes with 264 colour bins, 33.29 luminance deviation and 200.57 spatial
contrast. The inspected overhead `20261013T130713.549461Z-snakes` shows the complete
S-curved body more clearly against leaf litter, but still correctly fails the unchanged
generic luminance gate because the close forest-floor composition is dark. This failure
was not hidden by weakening the shared threshold.

Current Chrome/SwiftShader WebGL evidence is
`20261013T131009.763722Z-chrome-snakes`. Both `snakes` and `snake-head` views pass
60 sampled frame intervals, retain four intermediate images, report GL error zero and
an intact context, and have no new kernel GPU fault. Their sampled heads displace
5.613m and 6.274m respectively. The body is visibly in different S-curves and later
moves underneath natural foliage. The intermittent tongue happened to be retracted in
the inspected Web samples; this run therefore proves current portable rendering and
live crawl, not portable tongue visibility or continuous gait quality. The marker for
the new dorsal/tongue model is required independently by native and browser gates.
Android current-population runtime, ordinary player-controlled encounter review and
continuous natural-motion acceptance remain open.

## Fifty-animal population and bounded rendering — 2026-10-12

The requested live population is now exactly 50 ground snakes. Deterministic habitat
selection searches the authored understory more densely and retains at least 14m between
homes, whole-body terrain/obstacle clearance and the existing 18m home patrol radius.
The focused population test exercises all 50 animals and passes its motion, retreat,
terrain and obstacle invariants. WorldReport exports all 50 complete 41-station bodies;
the shared world fingerprint is `b0f0ae75a88ce3c3`.

Population size no longer determines GPU-buffer size. The renderer sorts visible snakes
by camera distance and submits at most the nearest 16 in its existing single draw. The
contact-shadow batch likewise admits at most 16 visible snakes and only nearby snake motion
invalidates its cache. Reduced-tier snakes update on a staggered four-frame cadence. The later
FPS repair removed snake-driven grass deformation: it was unrequested polish whose per-vertex
animated-segment search scaled badly with dense grass. Snakes still crawl, render, hiss and cast
nearby shadows, preserving encounters without coupling the two largest dynamic workloads.

Actual isolated llvmpipe `20261012T143710.846753Z-snakes` was inspected and reports
1/50 visible in one draw with one shadow. The live animal is recognisable and curved;
the generic image gate still rejects this deliberately dark top-down habitat for low
luminance variation. This is population/integration evidence, not final visual-polish
acceptance. Web/Android and a controlled before/after performance comparison remain open.

## Population and first-encounter motion expansion — 2026-10-12

The live ground population is now eight rather than four. Deterministic selection
retains qualified dense-understory-margin homes, full-body clearance and at least 34m
between homes. All eight simulate, cull, render and contribute to the shared contact-
shadow capacity. To retain portable WebGL fragment-uniform limits, the existing grass
interaction upload now chooses the four snakes nearest the camera each frame; this is
a bounded visual interaction subset, not a reduction of the live population.

Startup clocks begin in staggered active phases and the natural rest window is five
seconds rather than nine. This prevents the first focused or player encounter from
showing a straight motionless animal while retaining rest/explore/retreat behaviour.
The focused test covers all eight animals, 10.2246m aggregate focused-head travel and
2,337 frames with opposing lateral bends propagated beyond the neck. WorldReport and
its independent validator export all eight complete 41-station bodies; fingerprint is
`02d44fab27a0b823`.

Actual llvmpipe `20261012T121151.377464Z-snakes` reports 1/8 visible in one draw with
one shadow. All four inspected stages visibly translate and develop different S-curves.
The generic scene gate still fails luminance deviation (13.7352 < 18) despite passing
colour-bin and spatial-contrast thresholds, so this is motion evidence, not final
appearance acceptance. Web/Android, long-walk encounter rate and measured performance
remain open. Positional hissing and tree snakes are not implemented.

## World-report coverage — 2026-10-12

The earlier WorldReport omission is resolved: four ground snakes, initial homes and
headings, and all41 body stations are exported. Reconstruction uses the same complete
obstacle set as startup, including the hut. Count/finite-state/ValidBody checks and
full initial-state fingerprinting are included; tree habitat is explicitly marked
unimplemented. Current host fingerprint `b3fde347f77eb5f8` reflects added audit coverage,
not a live population/placement change. Independent JSON regression checks five corrupt
variants as well as real output. Full42/42 non-display tests PASS68.19s. No renderer,
emulator, KVM or dependency change was involved. This does not close visual, continuous
motion or tree-habitat work. Details: `docs/asset-pipeline.md`.

## Original implementation record

Four live nonhostile ground snakes are now instantiated after the complete world
obstacle grid, updated alongside other wildlife and drawn in normal IslandGame play.
Each body uses 41 terrain-supported stations and a connected 16-sided tube, with a
tapered tail, capped head/nose and small eyes. One bounded dynamic batch (20,000 vertex
capacity, at most 16,032 vertices for all four) uses stock BasicEffect, canopy lighting,
fog, frustum checks and a 70 m distance limit. The distance currently does not scale
with the mobile wildlife profile; mobile visual/performance review remains open.

The original code-generated 64×64 RGBA8 staggered-scale texture adds 16,384 bytes.
Updated exact generated total: 4,652,172 bytes; full runtime texture total 6,399,796,
quality allowance 6,420,012. Numerical test and platform log expectations were updated
by the exact allocation, not weakened. Provenance is in ASSET_LICENSES.md and the asset
manifest (25 procedural records, 77 generator sources, 10 project files, no external
files). Scale detail is procedural prototype art, not an accepted final material.

Use `--viewpoint snakes` for the close ground view. This is an ordinary live scene,
not a frozen replacement pose or hidden population. Draw totals and startup visibility
include snakes. F1 per-species rows, WorldReport population/fingerprint coverage and
dedicated Web/Android snake runtime gates still need extension.

## Actual native image review

All runs used the existing supervised private-display llvmpipe launcher. No host GPU
qualification or emulator configuration was attempted. Each actual image below was
inspected, not inferred from its metrics:

- `20261011T180203.942707Z-snakes`: snake clearly visible, but oversized lozenge scales.
  Close-view scene-contrast gate failed. Increased UV density and softened scale borders.
- `20261011T180405.133764Z-snakes`: wider camera passes generic image checks, but a
  foreground shrub hides the subject. Rejected as useful snake appearance evidence.
- `20261011T180523.790109Z-snakes`: opposite-side camera also passes generic checks,
  but grass hides most of the body. Also rejected for close snake review.
- **Final `20261011T180722.242708Z-snakes`**: restored useful close camera; full tapered
  olive body and much finer scales visible against the ground, head bent away as it
  retreats. Grass still crosses parts of the body. Head/eyes are insufficiently distinct
  at this distance, and the nearly straight starting body needs continuous-motion review.
  Image has 87 color bins, luminance deviation 11.8835 and spatial contrast 17.8797,
  below unchanged generic scene thresholds 18/30. **Automated image gate remains failed**;
  do not describe the final capture as passing because earlier obscured wide views passed.
  Kernel delta is empty. Final PNG is in the run directory beside the retained PPM.

Native build and 40 non-display groups pass (73.86 s); 34 supervisor CPU checks pass.
Final graphics-log replay passes independently of the failed image-contrast gate.
Web Release build/package passes (11,118,476 raw / 4,183,185 gzip bytes); ARM64
assembleDebug/lintDebug passes in 10 s. These are builds, not portable image evidence.
These establish integration and numerical regression coverage, not final appearance.
No tree snakes exist yet. Remaining work: recognizable head/face, continuous body/gait
and final mesh/ground clearance audit, appropriate close-subject capture acceptance
that still rejects blank/missing animals, grass intersections, contact shadows, profile
distance/LOD and reporting completeness, portable actual images. Keep the full snake
task unchecked, including its ground and tree requirements.

## Head refinement (2026-10-11)

Replaced the pointed single-fan nose with three connected rounded/flattened muzzle
sections. The first joins the body exactly; a narrow surface-following mouth seam,
convex amber eyes and dark pupil caps now make the face more readable. Head profile
constants are shared with CPU tests. Tests sample each section through 360 angles
inside the 50 mm spherical clearance envelope, including a margin for the lip, and
bound each complete eye. The final front section was shortened by 1 mm after the
rotation-aware check exposed a slight envelope excess. No body clearance was widened.
Maximum mesh now 18,240 vertices for four snakes, still inside the existing 20,000
allocation. No new texture allocation or external asset.

Added `--viewpoint snake-head`. Actual supervised llvmpipe close frames
`20261011T181314.433311Z-snake-head` and `20261011T181427.154154Z-snake-head` were
inspected; the first informed darker/larger pupils and the fine mouth seam.
Final `20261011T181556.624816Z-snake-head` was also inspected: one near eye, pupil,
mouth and rounded muzzle are recognizable, but grass obscures part of the head and
the body. The head remains a stylized prototype, not final wildlife realism.
All three fail the unchanged generic luminance-variation gate; final kernel delta
is empty and graphics-log replay passes. Do not confuse visible geometric progress
with a passing image gate. Focused population/head CPU tests pass in 8.36 s;
34 supervisor checks pass, asset inventory is 25/78/10/0.
Web Release build/package and ARM64 assembleDebug/lintDebug also pass (Android6s),
without claiming portable runtime images or overriding the native image-gate failure.

Continuous animation, a reliable close-subject QA gate, grass/body interaction,
contact shadows and portable runtime images remain open, alongside tree snakes.

## Continuous terrain-clipped contact shadow (2026-10-11)

Ground snakes now contribute a soft continuous ribbon to the existing contact-shadow
batch, including a tapered head cap and tail fade. Each ribbon triangle is clipped
against the actual rendered terrain triangles before height projection, preventing
bridges across terrain creases. The offset is 4 mm; opacity peaks at 0.23. The existing
population height cache is reused, with no new texture or draw call. Capacity reserves
12,000 vertices per snake (48,000 additional slots); any body station moving over
8 mm invalidates the cached batch. Shadow distance follows the existing quality scale.
F1 now has snake population, shadow and draw counts; its layout still needs a live review.

Population tests sample shadow vertices and two interior points per triangle every
60 frames of the 2,400-update scenario, checking terrain clearance, opacity and vertex
limits. Nonfinite, stretched and degenerate input fixtures must be rejected.

Actually inspected native `20261011T182707.559480Z-snake-head/capture.png`: visible
scaled body with subtle ground darkening, no chain of separate shadow discs. Grass
still heavily occludes the animal and the head faces away in this live pose. This is
not acceptance of final appearance or continuous-motion quality. Startup reports one
visible snake, one snake shadow and one shared contact-shadow draw. Graphics-log replay
passes and kernel delta is empty, but the unchanged generic luminance-variation gate
FAILS. Portable runtime images, grass intersections and motion review remain open.

Full 40 non-display test groups pass (87.01 s), 34 supervisor checks pass and asset
validation passes (25 procedural records / 80 sources / 10 project files / 0 external).
Web Release build/package passes (11,138,018 raw / 4,188,113 gzip bytes); ARM64
assembleDebug/lintDebug passes in 7 s. These builds do not establish portable runtime
appearance. The hardware qualification stop latch remains unchanged.

## Live motion sequence audit (2026-10-11)

The two snake capture viewpoints now retain four frames (rendered frames 4/8/12/16)
from ordinary live simulation, plus all 41 body positions, speed and phase at readback.
The close QA camera follows body-centre translation, preserving orientation; it does
not replace poses or skip simulation. This camera movement also moves the real observer,
so the sequence exercises retreat, not an undisturbed observer or all behavior transitions.
Software frame timing is variable; these are ordered samples, not a fixed-rate video.

First run `20261011T183434.634965Z-snakes`: all four images inspected separately;
head translated 1.923 m, but following only the head let the tail leave the image.
Changed tracking to the mean of the body stations. Final run
`20261011T183616.764190Z-snakes`: inspected the four-frame `motion-grid.png`; the body
remains in frame and the head travels 2.171 m. Both kernel deltas are empty and graphics
logs pass. Both generic image gates FAIL luminance/spatial contrast; thresholds unchanged.
The motion validator passes four ordered finite connected samples with actual displacement,
but does NOT establish natural slithering, continuous mesh clearance or image visibility.
Its missing/reordered/frozen/nonfinite/disconnected/invisible fixtures are rejected.
35 supervisor tests and focused snake tests (6.27 s) pass; native build passes.
Portable builds/runtime for this QA-camera change have not been rerun.

**Actionable visual finding:** movement looks like a largely straight rope being pulled
around a bend, rather than a wave propagating down a snake. Current repeated follower
projection smooths the small head oscillation; investigate retained arc-length head-path
sampling with bounded curvature, while preserving whole-body collision and grounding.
Grass still crosses the mesh. Four sampled poses are not a full temporal-quality pass.
Next work is to fix this visible motion defect, not to mark the snake task complete.

## Retained head-trail slithering (2026-10-11)

Replaced repeated follower projection with arc-length sampling of the accepted head
trail. Body stations are sampled 55 mm apart along that path, then independently
grounded. Accepted samples at least 6 mm apart are retained newest-first and pruned
just beyond the tail, with a defensive 512-record limit per snake. Sub-threshold motion
still uses the current head as the first segment, without adding unlimited tiny samples.
Rejected obstacle/body steps never enter the trail. Existing spacing tolerance, habitat,
self-separation, retreat-distance and whole-segment obstacle checks remain in force.
Head steering oscillation is now 0.8 radians at 6 radians per metre traveled; phase
advances only on accepted movement. No new textures or mesh allocation.

Tests now require repeated opposing bends beyond the first four stations (more than
one radian accumulated in each direction), bound every body station's planar movement,
and compare 2,000 1-ms steps with 200 10-ms steps (station difference below 25 mm).
Full 40 non-display groups pass in 58.03 s; the final additional fine-step focused
test passes in 7.55 s. Supervisor35 and asset25/80/10/0 checks pass.

Inspected actual native four-stage grids:
`20261011T184024.136306Z-snakes` shows improved bends but only 1.693 m head displacement,
less than one body length. Extended QA capture from frames4/8/12/16 to8/16/24/32.
Final `20261011T184233.069497Z-snakes/motion-grid.png` shows alternating S bends along
the body after 3.573 m displacement. Foreground leaves partially occlude the last stage;
grass still intersects the animal. Both motion/graphics log replays pass and both kernel
deltas are empty. Generic image gates still FAIL (final luminance variation), unchanged.
This supports visible improvement, not final naturalness, continuous collision proof,
or acceptance of grass interactions. Portable runtime snake views and tree snakes remain open.

Final native/Web builds and Web packaging pass; ARM64 assembleDebug/lintDebug passes
in 7 s. The first ARM64 attempt caught Clang's dangling-span warning in the prior
audit-camera range-for; retaining the span locally fixes it, with final native/Web
rebuilds afterward. Actual images above precede only that lifetime-warning fix, not a
simulation change. Portable build success is not portable runtime image evidence.

## Ground-grass contact, native stage (still incomplete)

Grass now receives four41-station snake paths as164 vec3(x,z,radius) values plus
body bounds. After player/wind motion, vertices within the body footprint (15mm
margin,140mm soft edge) are pressed toward2mm above the rendered terrain, below
the snake's4mm supported clearance. Existing roots stay planted. This is one existing
grass batch, no extra geometry/texture/draw, and does not modify fallback grass.

The initial per-vertex contact check is now conservatively restricted per grass cell:
CPU AABB overlap sets a four-component uniform mask using the full measured mesh,
slope, wind and player-motion envelope, NOT just patch anchors. Unaffected cells
return before snake loops; overlapping cells retain the original pressure calculation.
Tests compare masked/unmasked pressure across moving-body local grids including cell
edges; touching bounds stay active, separated cells are rejected, invalid bounds throw.
Native focused snake/motion bounds tests PASS2/2 (6.31s), supervisor37 and asset gate
25/81/10/0 PASS. This is a source-level workload reduction, not a measured GPU speedup
or evidence that it caused/fixed the October11 AMD timeout.

Actual contained llvmpipe capture `20261011T200218.123460Z-snakes`: four-stage
motion-grid.png inspected, snake body/S-bends retained, but some oblique grass blades
still intersect the body and foreground leaves occlude later frames. Graphics/motion
log replay passes, kernel-after empty; generic luminance gate still FAILS unchanged.
Earlier pending-contact capture185758.816335Z was also inspected, but the images are
not byte-identical; do not claim a matched pixel regression proof.

Remaining contact work: pressure at vertices alone does not prove triangle-interior
clearance (a blade can span the body with endpoints outside its footprint); assess
whole-blade swept contact, normal deformation and temporal relaxation. Portable actual
views, uniform-budget/performance measurement, tree snakes and final appearance remain
open. The original GPU-crash candidate and its pre-change patch/header stay preserved.
Native/Web builds and Web package PASS (11,160,494 bytes raw /4,195,534 gzip);
ARM64 assembleDebug/lintDebug PASS in5s. No portable runtime was launched for this stage.

## Swept root-to-vertex grass contact

The next change addresses a demonstrated endpoint-only gap: both ends of a slanted
blade can be outside a snake capsule while the intervening blade crosses it. Every
grass blade/turf blade and its seed-head vertices now retain their planted local root.
The custom mesh stride grows40→48 bytes (+7,712 bytes for964 existing vertices),
with ten total vertex attributes including the unchanged44-byte instance record.
Roots are transformed with the same yaw, terrain tangent basis, spread and distance
contraction as the rest of the blade. Contact checks the horizontal root-to-deformed-
vertex segment against each relevant snake segment rather than just the endpoint.
Crossing segments return full pressure; otherwise the closest endpoint-to-segment
distance supplies the existing radius/margin/fade. Per-cell rejection is retained;
no new draw, texture, instance buffer update or shader uniform array is introduced.

CPU regression covers the both-endpoints-outside crossing, reverse direction, parallel
soft edge, separated collinear segments, nonfinite roots, masked/unmasked agreement
and comparison with101 interior point samples on30 test blades. Focused snake and
grass-motion tests PASS2/2 (9.91s); supervisor37 and asset25/81/10/0 gates pass.

Actual contained llvmpipe `20261011T200740.707227Z-snakes/motion-grid.png` inspected:
snake/S-bends remain visible in stages0/2/3; large foreground leaves obscure much of
stage1. Graphics/motion checks pass and kernel-after is empty. Generic luminance
gate still FAILS; no threshold changed. Poses differ from the preceding capture:
the current audit advances simulation with real update time, not fixed per-stage
simulation time. Thus this is shader/runtime evidence, NOT matched before/after
appearance proof. Add deterministic capture timing before using frame-indexed poses
as image regressions. Natural gameplay timing must remain unchanged.

This horizontal swept approximation still does not prove every curved blade triangle
clear of a3D snake, and can press blades whose higher sections pass over it. Normal
deformation, temporal relaxation, true continuous contact and portable actual images
remain open. Increased segment arithmetic has not been GPU-timed. The AMD timeout
remains unassigned/unresolved; this change is visual contact work, not its claimed fix.
Final native/Web builds and package PASS (11,163,384 bytes raw /4,198,207 gzip),
ARM64 assembleDebug/lintDebug PASS in4s. Portable shader execution remains unverified.

## Reproducible native snake capture clock

SnakeDetail/SnakeHeadDetail **with a capture path only** now disable framework wall-time
catch-up and advance all simulated actors by0.1s per rendered frame. Render-time wind/
water animation and monkey absolute time use the same counted clock. Normal gameplay,
manual fixed viewpoints without capture and other capture viewpoints retain their
previous timing. Stages8/16/24/32 explicitly report update counts and times0.8/1.6/2.4/
3.2s. The supervisor rejects missing/extra/malformed stages, unexpected update counts,
nonfinite/wrong times as well as the existing disconnected/frozen/invalid body traces.
The37-test CPU supervisor suite passes, including these new negative fixtures.

Two actual contained llvmpipe runs:
`20261011T201403.216063Z-snakes` and `20261011T201453.821080Z-snakes`.
All four41-station body traces match exactly. All four corresponding PPM files are
byte-identical, with SHA256 hashes in stage order:

```
f4b3d9d5cc59ba968059dfc1f5db3edd904887a2ac5dd94fc2d3f257a4f404b6
9209692d2f2003debd0bcb691b2120df21d6200bd1f3029fee94114691388b75
6d77866ed03306218fe7d26b95e35b028405b249b448a82a249c56a01d3202a3
65f21348d625c41a3dc9ac2b4aa07c95f811752b3d2527e4fffc1dc2d5f1d7f5
```

First run's motion-grid.png actually inspected: the snake is visible in all four frames,
with bends progressing from the head through the initially straight body. Flattened
grass is visible along its path. This does not demonstrate natural temporal recovery,
prove all triangle contacts, or replace continuous-motion/portable audits. Both runs
still FAIL generic luminance variation and large-scale contrast gates; thresholds
unchanged. Both graphics/timed-motion replays pass and kernel deltas are empty.
This establishes repeatability on this software configuration, not cross-driver pixel
identity, appearance acceptance or an AMD-crash fix.
Native/Web builds and package pass (11,162,865 bytes raw /4,197,939 gzip), ARM64
assembleDebug/lintDebug pass in4s; asset gate25/81/10/0 passes. No portable runtime
was launched and the hardware qualification stop latch is unchanged.

## Focused WebGL audit

The contained Chrome supervisor now supports `--scope snakes`: sequential ordinary
`snakes` and `snake-head` viewpoints, four intermediate screenshots per view and
60 additional compositor-frame intervals with WebGL error/context checks. It requires
SwiftShader, visible snake submission, existing shared-material diagnostics and no
fallback/high-severity GL errors. Both pages are closed independently. These are
real-time interactive views, NOT the native capture-clock sequence; screenshot counts
alone do not prove live snake motion or subjective appearance. CPU supervisor suite
has17 tests, including explicit scope, view routing and visible-population requirements;
native software-supervisor37 tests and JS syntax check pass.

Actual evidence: `build/browser-software/20261011T202147.595070Z-chrome-snakes`.
Initial body/head images and both four-pose grids actually inspected. Body view shows
S-bends and scales, but foreground leaves substantially or wholly obscure some later
frames. Head view retains readable eyes/mouth, yet its large irregular scale mapping
does not transition convincingly into the smaller regular body scales. Head appears
stationary across the four sampled images while nearby grass changes; this is not
evidence of successful retreat in that close-observer scenario. Investigate behavior
and camera placement rather than count four screenshots as a gait pass. The head QA
camera currently follows body centroid translation, not head translation.

Open visual/audit defects: improve head/body material continuity, avoid losing the
subject behind foreground leaves in the audit camera without removing live vegetation,
and qualify actual near-observer retreat. No AMD hardware qualification or Android
runtime is inferred from these browser images.
Final supervisor result PASS: both views completed60 intervals with glError0, no
context loss and four intermediate images each; kernel-final[]. This is execution
qualification on Chrome/SwiftShader only, NOT final visual or motion acceptance. No
generic native luminance threshold was changed or used to claim these close views pass.

## Close-frontal retreat deadlock — reproduction history (repair below)

CPU reproduction now matches the initial head-camera observer offset: first snake in
the real vegetation/jungle/prop obstacle layout, observer fixed at head+(0.35,0.25,0.30),
900 updates of1/60s. The whole body remains valid but head displacement is **0m** after
15s. This confirms a simulation failure rather than merely a misleading screenshot.

The original opt-in reproducer (f9e8bc0) failed; its command is retained as an alias
for the current default suite, which now includes the repaired scenarios below:

```sh
build/island_snake_tests --close-observer-regression
```

At the reproduction stage it returned1 with `KNOWN OPEN DEFECT` (without an abort/core).
The regular CTest suite then passed9.48s but did NOT include that regression.
Evidence: build/snake-close-retreat-reproducer.log and
build/snake-close-retreat-restored-tests.log. The first failing test, before it became
an explicit reproducer, is retained in build/snake-close-retreat-tests.log.

Source-level mechanism to address: retreat rejects every head substep that approaches
the observer, while the retained-path sampler uses arc length and ValidBody requires
55mm chords within3%. A sharp initial turn shortens the chord and rejects the candidate;
turning the heading alone does not reshape the neck or its accepted trail. A replacement
circle/path-intersection sampler was tested but REJECTED: it first violated the existing
body-station motion limit, then exceeded the512-sample trail cap after adding a movement
guard. Those experimental simulation edits were fully removed; no renderer/application
build used them. Existing safety tests were not relaxed and the trail cap was not raised.

Next repair must solve close-threat turning with bounded, continuous body/neck motion
and history while retaining terrain, obstacle, self-clearance and non-approach checks.
Do not close this defect using only the older side-observer retreat tests, or replace
serpentine motion with rigid backwards translation just to satisfy displacement.

## Head scale mapping correction, native matched view

Found two UV discontinuities: muzzle longitudinal density was16 repeats/metre versus
the body's1.5/0.055≈27.27, and every nose-fan triangle interpolated to the same(0,0)
pole, spanning up to four circumferential texture wraps. Shared SnakeSkinU/V helpers
now keep the same signed longitudinal density through the neck seam. Each nose
triangle gets its own mid-sector pole U and the correct longitudinal V. Geometry,
triangles, scales bitmap, texture allocation, clearance and simulation are unchanged.
CPU tests check seam/density and every fan triangle's local UV span; normal snake
suite PASS9.79s, supervisor37/assets25/81/10/0 PASS. The known close-retreat regression
is still open and not part of that normal-suite pass.

Matched native head captures before `20261011T204120.519708Z-snake-head`, after
`20261011T204412.087633Z-snake-head`: all four timed body records match exactly.
Both stage0 images and the after directory's head-before-after.png crop actually
inspected: head scales are more regular and closer in density to the neck/body;
eyes/mouth and geometry stay recognizable. Full natural material/anatomy acceptance
remains open. Both graphics logs validate and both kernel deltas are empty.
**Both overall capture runs FAIL the motion gate** because the head remains stationary
in the confirmed close-observer case. No validator was weakened to approve the static
sequence; the paired images support only this localized material correction.
Native/Web builds and package PASS (11,162,882 bytes raw /4,197,939 gzip), ARM64
assembleDebug/lintDebug PASS3s. No new portable runtime; shader/material execution
qualification above is native software only. Hardware stop latch remains unchanged.

## Bounded articulated retreat turn — native repair

When an ordinary retreat substep cannot proceed, a validated lateral neck turn is now
attempted. Side selection increases horizontal distance from the observer; the bend
falls off quadratically over the front12 stations (66cm). The head uses half the
normal movement budget; the remaining half is apportioned over40 joints to gradually
restore55mm spacing without snapping previously curved chords. Every resulting joint
displacement is explicitly bounded, terrain heights recomputed, and the full existing
body/obstacle/self-clearance/home-range checks must pass before commit. Rejected
turns do not change the body/history. Accepted turns reseed43 path points (41body+2
tail extensions); ordinary movement continues with the existing retained head path
and512-sample cap. No rigid backwards translation, relaxed test or larger memory cap.

Two preliminary variants were rejected by unchanged coverage tests: concentrating
rotation into one neck joint traveled only0.233m and lost body waves; distributed
turning without gradual spacing restoration traveled2.499m and also lost waves.
The final variant retains7.902m in that older coverage scenario and2340 opposing-wave
frames. The formerly opt-in close-observer regression is now UNCONDITIONAL in the
normal snake suite, expanded to four offsets and per-update station-motion and
non-approach checks. After900×1/60s, displacements are7.553m at(+.35,+.30),7.760m
at(+.35,-.30),7.900m at(+.5,0), and5.176m at(+3,0). Focused suite PASS9.30s.

The head-detail QA camera now follows the head translation rather than the centroid;
its direction and ordinary threatening observer behavior remain unchanged. Actual
contained llvmpipe `20261011T205902.050693Z-snake-head/motion-grid.png` inspected:
head visibly turns and leaves its initial position, remains framed in all four views,
and bends progress through the neck/body. Timed motion validation now PASSES with
2.394m head displacement between first/last stages (previously0), graphics log valid,
kernel-after[]. Some grass still occludes the neck. The generic luminance/large-scale
contrast gates still FAIL unchanged: not final snake appearance/naturalness acceptance.
This qualifies the reproduced native cases, not every possible escape route, portable
runtime, continuous3D contact or the still-unresolved AMD GPU timeout.
Full native non-display suite PASS40/40 (84.37s). Native/Web builds and package PASS
(11,166,542 bytes raw /4,200,268 gzip); ARM64 assembleDebug/lintDebug PASS7s.
No portable runtime was launched for this repair. Hardware stop-latch SHA unchanged.

## WebGL retreat verification with actual position samples

Ordinary interactive snake QA viewpoints now emit four sparse runtime samples at
rendered frames8/16/24/32: head position, current speed and visible snake count. This
does not add logging to normal gameplay or change simulation; deterministic native
captures retain their existing separate stage records. The browser parent now requires
both body/head reports, exactly four ordered finite samples per view, bounded speeds,
visible submission and at least0.1m displacement from the first sampled head position.
The18-test browser supervisor suite includes rejection of frozen, missing, reordered,
nonfinite and invisible traces. Images are still reviewed separately: moving numeric
coordinates do not establish unobstructed pixels or natural gait.

Actual final run: `build/browser-software/20261011T210926.570528Z-chrome-snakes`,
Chrome155.0.8059.39/SwiftShader, PASS. Both views completed60 measured compositor
intervals with glError0/no context loss; four ordered position samples per view pass
the new parent gate. Displacement from first sample:5.892m body view,5.614m head view.
kernel-final[]; hardware stop latch untouched. Both body-motion-grid.png and
head-motion-grid.png actually inspected: the head view now shows continuing retreat
through changing terrain/vegetation instead of the previously frozen head, with the
head retained in frame and updated scales visible. The wider body's intermediate
images still suffer substantial leaf occlusion, including one fully obscured pose.
That audit-camera issue remains open; this is not final whole-body gait acceptance.
No Android runtime, generic contrast-gate pass, AMD qualification or crash fix claimed.
Native/Web build PASS, Web package11,167,232 bytes raw /4,200,881 gzip; ARM64
assembleDebug/lintDebug PASS4s. Software supervisor37 and assets25/81/10/0 PASS.

## Android qualification attempt — still unqualified

The Android supervisor now supports both snake views and reuses the browser's
four-sample runtime validator. Readiness waits for frame32, and missing, frozen,
unordered, duplicate, nonfinite, invisible and excessive-speed fixtures are rejected.
Android23/browser18/software37 CPU tests and shell syntax pass.

Actual isolated `20261011T212904.738068Z-snake-head` FAILED: SwiftShader boot and APK
launch succeeded, but readiness timed out. Even failure screenshot collection timed
out after10s; the supervisor forcibly terminated its owned emulator (exit-9).
kernel-final[]; no screenshot exists to audit and no Android motion/visual pass is
claimed. No KVM or emulator configuration was changed.

Source review then found the new snake samples only used stdout, unlike the existing
Android climb diagnostics. They now also use the existing platform/persistent-log
mirror and flush stdout, still only four records in the two QA views. This fixes a
diagnostic gap, NOT the unexplained unresponsive guest. Corrected x86_64 build/lint
passed and is archived as `build/android-software/island-snakes-x86_64.apk`; default
ARM64 build/lint restored and passed. The corrected APK still needs actual runtime
verification. The AMD hardware incident remains unrelated/unresolved, not repaired.

### First-frame localization after correcting log mirroring

The corrected archived APK was actually rerun in `20261011T213346.432797Z-snake-head`:
same readiness failure, screencap timeout and forced owned-emulator exit-9, kernel[].
The logging fix alone therefore did not resolve the runtime failure.

Optional CPU pass-entry/exit breadcrumbs now reuse the existing render-pass scopes,
enabled only on frame0 of the two snake QA views. This flag enables no GL diagnostics,
fences or sleeps, and changes no scene content. Instrumented actual Android run
`20261011T213726.011849Z-snake-head` again failed, but completed entry/exit pairs for
clear, sky, terrain, distant scenery, contact shadows, natural props, shipwreck and
jungle. The final marker is `cpu-pass begin frame=0 name=grass` at23:38:19.439 guest
time. No grass return, later draw pass or snake motion sample was recorded before
timeout. Failure screenshot again timed out; owned emulator was forcibly terminated,
kernel-final[]. No image or successful runtime qualification is claimed.

This narrows the CPU-side wait to the grass pass. Shader compilation, uniform uploads,
draw submission and execution remain to be distinguished; it does not establish a
relationship to the original native AMD failure. Preserve contact behavior and all
motion gates while investigating. Android23/browser18/software37 tests still pass.

### Finer trace: grass returns in another failed run

Nested diagnostic scopes now distinguish effect Apply, uniform upload and instanced
binding/drawing, without changing shader source, contact behavior or scene content.
Run `20261011T214147.707723Z-snake-head` FAILED readiness but, unlike the preceding
run, returned from the complete first grass pass in412ms (23:42:36.164–36.576 guest
timestamps). Apply returned within the same millisecond, uniform upload by36.167;
all instanced draw scopes and subsequent IslandGame draw scopes returned. Startup
reported first draw5746.146ms, total17743.021ms. No frame8 motion record followed.

CPU call return does not prove completion of queued graphics work or presentation.
The earlier final grass-entry marker is not proof of a deterministic grass defect.
Failure screenshot again timed out; owned emulator forced exit-9; kernel-final[].
A brief read-only GDB attach to owned host emulator PID751700 retained stacks in
`emulator-stacks.txt` and detached successfully. Missing symbols prevent useful
compiler/driver attribution; attachment can perturb timing, so this is not a clean
performance measurement. No image inspected, no Android qualification or AMD fix.
Android23/software37 CPU tests pass, instrumented x86_64 build/lint PASS3s.

## Conservative per-segment grass-contact rejection

CPU and GLSL now reject a body segment before the four point/segment distances when
the swept blade's XZ envelope is separated from the segment envelope expanded by
maximum endpoint radius +15mm margin +140mm full fade +0.1mm numerical guard. This
is conservative for the existing projected capsule model: no contact/fade distance,
body stations, uniforms, shader path or grass geometry are removed or reduced.
The existing snake-wide and grass-cell rejection remain in place. Actual performance
gain is unmeasured; this does not diagnose or fix either Android or AMD stalls.

An independent double-precision unculled capsule oracle checks24000 deterministic
point/swept queries against straight and actual multi-snake bodies, plus246 near-fade
boundary queries. Tolerance0.0002 pressure units. All prior movement/retreat tests
remain; focused snake suite PASS10.08s. Native build PASS; Android ARM64 build/lint
PASS5s. Actual contained llvmpipe capture `20261011T214716.834158Z-snake-head` has
valid motion stages and kernel-after[]. Its four-stage motion-grid.png was inspected:
head visible and retreating; some foreground grass occludes the neck as before.
All four full-resolution stage images are pixel-identical (ImageMagick absolute-error
count0 each) to `20261011T205902.050693Z-snake-head`. The unchanged generic luminance
and large-scale contrast gates still FAIL; no overall visual acceptance claim.
No Android/Web runtime qualification or hardware incident repair is claimed here.
Full non-display suite PASS40/40 (95.27s). Web build/package PASS,11,178,925bytes raw
and4,203,549bytes gzip. Hardware stop-latch checksum unchanged; all owned jobs terminal.

## Actual Android motion qualification after segment rejection

The two snake QA views now log one Update-resumed marker after the first draw, and
CPU pass breadcrumbs cover the first two frames rather than only the first. This
does not change simulation, shaders, timing limits or motion acceptance criteria.

Actual software GLES runs using the same APK SHA256
`045a1121b58ea5bf2a5a57bc3c02354e132445d1e6ad5ec0886ebebb60a0cc44`:

- `20261011T215120.905025Z-snake-head`: PASS, four ordered live samples,11.304m maximum
  head displacement from the first sample; actual screenshot inspected, head, eyes
  and scales clearly visible.
- `20261011T215307.497535Z-snakes`: PASS functional gate, four live samples,11.320m
  displacement; actual screenshot inspected but a large foreground leaf hides the
  head/upper body. The visible lower body has an S-shaped bend. This is not whole-body
  visual acceptance; the wide QA camera needs unobstructed framing without removing
  foliage from gameplay or the test scene.

Both runs show SwiftShader GLES, kernel-final[], first-attempt screenshot acceptance,
normal owned-emulator exit0 and no rejected graphics diagnostics. Exact APK archived
as `build/android-software/island-snakes-qualified-x86_64.apk`. Earlier failed APK
archive and all failed evidence remain unchanged. Android23/browser18/software37 CPU
tests pass, x86_64 build/lint PASS8s. These successful runs follow the segment filter
but do not constitute a controlled causal A/B experiment for the earlier stalls.
No general stability/mobile performance/native AMD fix or complete natural gait claim.

## Centroid-centered overhead wide QA camera

The wide SnakeDetail camera now starts at the actual mean of41 body stations plus
(0,2m,0.35m), looking toward that centroid; translation tracking was already centroid
based. The close head view is unchanged. No foliage or animals are hidden/moved.
Simulation code is unchanged, but the new observer location naturally changes the
retreat route, so old/new image pairs are not matched-pose rendering comparisons.

Actual native `20261011T215642.556056Z-snakes` four-stage grid inspected: entire body
visible in each stage, lateral bends develop without the old large-leaf obstruction.
Motion gate passes, kernel-after[]; generic luminance/large-scale contrast still fail.
Native build passes. Actual Android `20261011T215811.492358Z-snakes` has all four
valid motion samples (9.372m displacement), but overall run FAILS image acceptance.
Screenshot inspected: most of the body is readable from above, head partly hidden by
a trunk near the upper edge. Luminance standard deviation12.6251 is below unchanged
threshold18; spatial contrast38.3623 passes30. After21 attempts/60seconds no image
was accepted; owned emulator required forced termination-9, kernel-final[].
Do not relabel as an Android pass or full unobstructed-body qualification. Android
x86_64 build/lint PASS5s, Android23/software37 CPU tests pass; standard ARM64 restored.
Remaining work: dark/readability limitations, trunk occlusion along the live route,
Web verification and continuous gait audit. Original AMD investigation remains open.
