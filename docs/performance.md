# Performance Measurement

## Purpose

Performance decisions must use repeatable scene evidence rather than the short-lived F1 FPS
display. The native benchmark mode renders the normal scene without debug overlays, disables
VSync and the fixed simulation timestep, discards 120 warm-up frame intervals, records 600
end-to-end frame intervals and matching scene-population samples, and exits. Timing is measured
between consecutive `Draw` entries, so it includes the preceding update, draw submission,
presentation, and game-loop work. It does not separate CPU and GPU cost.

One fixed view can be measured with:

```bash
build/island-simulator --benchmark /tmp/island-jungle.json --viewpoint jungle
```

Run the six-report suite without placing a window on the owner's desktop:

```bash
../cna/tools/platform/run_gpu_tests_private.sh \
  --exec ./tools/run_native_benchmarks.sh /tmp/island-benchmarks
```

The suite covers overview, beach, dense jungle, shoreline detail, and close monkey fixed views,
then a 676 m moving camera rail. The generated schema-2 JSON files contain resolution, renderer
identity, sampling settings, mean/median/p95/p99 frame duration, minimum/maximum duration, mean
FPS, the final scene snapshot, and min/max/mean ranges for tracked submissions, terrain cells,
contact shadows, palms, canopy, and grass. These generated reports belong under `build/` or
`/tmp`; they are evidence, not source artifacts.

The benchmark intentionally rejects smoke, capture, overlay, world-debug, and input-diagnostic
modes. Ordinary play retains VSync and the fixed 60 Hz simulation. Benchmarking must not change
release presentation or movement semantics.

## Startup measurement

The game can write a schema-1 JSON report immediately after its first completed `Draw` body:

```bash
build/island-simulator --smoke-test --overview \
  --startup-report /tmp/island-startup.json
```

The report separates deterministic world generation in the `IslandGame` constructor, CNA
framework/device initialization before `LoadContent`, GPU resource and procedural content creation,
the interval from completed content to the first draw, and the first draw submission. It also
records renderer, fixed viewpoint, resolution, and whether audio was attempted, available,
gesture-deferred, or deliberately skipped by smoke mode. The total spans construction through the
end of the first game-owned draw.

This is wall-clock application instrumentation. CNA presents after `IslandGame::Draw` returns, so
`first_draw` does not include swap/present or guarantee GPU completion. `gpu_content_creation`
names the phase in which GPU-backed resources are created; it can include driver synchronization
and procedural CPU work and is not GPU-only time. Native smoke mode skips audio for determinism;
use an interactive report when measuring real Linux mixer startup, and retain its audio state with
the result.

`IslandGameStartupReport` creates the report after an actual private-display OpenGL 3.3 draw and
`IslandGameStartupReportValidation` rejects missing, negative, non-finite, inconsistent, or
implausibly long measurements. The browser harness independently requires the same finite timing
line from a real WebGL 2 context. A 2026-10-06 debug smoke while the visible game shared the GPU
measured 494.8805 ms world generation, 526.8526 ms content creation, 2.6629 ms first draw, and
1075.9222 ms total. That run proves instrumentation only; concurrent load and debug compilation
make it unsuitable as a regression baseline. The actual Chrome 152/SwiftShader WebGL 2 gate
measured 134.8 ms world generation, 246.8 ms content creation, 15.7 ms first draw, and 438.0 ms
total; SwiftShader is likewise functional evidence rather than hardware-browser performance.
The API 35 x86_64 software-only Android AVD reported 157126.852 ms world generation,
256204.401 ms content creation, 7708.990 ms first draw, and 421449.067 ms total at 800 by 360.
Those QEMU TCG values prove that the Android instrumentation and current GLES 3 scene reached a
first draw; they must never be used as a device startup budget or performance comparison.
Two isolated release and debug repetitions per target remain required before setting startup
budgets.

## Moving traversal

`--viewpoint traversal` is accepted only with `--benchmark`. It advances the authoritative player
and camera through 11 points on a smooth 675.9635 m rail. The route begins in the southern
shallows, follows the beach and primary trail, crosses the east and west clearings and interior
jungle, and finishes in the northern shallows. One rail position is rendered for each of the 600
samples, deliberately exercising culling, vegetation LOD, dynamic wildlife tiers, and the
quarter-metre contact-shadow refresh threshold instead of repeatedly hitting fixed-view caches.

The first functional OpenGL 3.3 run covered signed shore distances from -18.5544 m offshore to
208.6531 m inland. Across the route it observed 12--118 tracked submissions, 6--47 terrain cells,
0--43 contact shadows, 0--325 palms, 0--281 canopy trees, and 0--3,466 grass clusters. Its timing
was intentionally not adopted as a baseline because the visible interactive game was using the
same GPU concurrently. Record two isolated repeats before using traversal timing for regression
decisions. WebGL 2 and physical GLES 3 traversal timing remain unqualified.

## Initial Linux development baseline

The baseline below was recorded on 2026-10-06 at 1280 by 720 with the debug build, CNA `next`
`f243c63a1dc5`, and Sharp Runtime `next` `21c06910f7cf`. The private display reported direct,
accelerated AMD Radeon 780M rendering through radeonsi/Mesa 25.0.7. Audio was unavailable inside
the isolated compositor, so this baseline does not measure the active mixer.

| View | Mean ms | p95 ms | p99 ms | Tracked submissions |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.2042 | 1.4473 | 1.9363 | 80 |
| Beach | 1.6497 | 1.8729 | 2.2889 | 118 |
| Jungle | 1.3333 | 1.5824 | 1.9248 | 86 |
| Driftwood/shore detail | 0.7948 | 1.1231 | 1.3856 | 23 |
| Monkeys | 1.4125 | 1.5616 | 1.6715 | 93 |

This baseline includes the exact shoreline foam ribbon and its one additional tracked submission.
The beach view is currently the heaviest of these fixed native scenes. This is a development
comparison baseline, not a shipping budget: it uses a debug build, one integrated GPU/driver,
one resolution, no isolated audio output, and no WebGL 2 or Android hardware measurement. It also
does not expose GPU-only time, thermal throttling, memory bandwidth, power, or loading cost.

### Fallen-frond follow-up

Two immediate repeats after adding 63 near-field fallen palm fronds produced the ranges below.
Only one to four fronds are visible in the affected fixed views; variant instancing adds one to
four submissions there and zero to overview. The heaviest beach view remains below 2.0 ms p95 in
both repeats, while the ranges demonstrate enough run-to-run variation that individual p99 changes
must not be attributed to the layer alone.

| View | Mean ms range | p95 ms range | Tracked submissions | Visible fronds |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.2138--1.2419 | 1.4671--1.5990 | 80 | 0 |
| Beach | 1.7127--1.7321 | 1.8899--1.9729 | 122 | 4 |
| Jungle | 1.3537--1.3655 | 1.5223--1.5727 | 89 | 2 |
| Driftwood/shore detail | 0.7677--0.7932 | 0.9293--1.0406 | 24 | 1 |
| Monkeys | 1.4361--1.4869 | 1.5600--1.6807 | 95 | 2 |

### Emergent-reef and rock-silhouette follow-up

Two repeats after adding 14 solid emergent outcrops and refining the shared near/far rock meshes
produced the ranges below. The outcrops join the existing four rock-variant instance batches, so
they add no tracked submissions in these views. Near rocks now use seven shaped rings, twelve
sides, a broad embedded base, and a small irregular cap instead of the earlier pointed five-ring
silhouette; far rocks use a reduced four-ring/seven-side form. The heaviest beach view remains
below 2.0 ms p95 in both repeats.

| View | Mean ms range | p95 ms range | Tracked submissions | Visible reef outcrops |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.1612--1.1792 | 1.2832--1.3979 | 80 | 4 |
| Beach | 1.6988--1.7015 | 1.8836--1.9754 | 122 | 5 |
| Jungle | 1.3434--1.3749 | 1.4827--1.7016 | 89 | 2 |
| Driftwood/shore detail | 0.7914--0.7975 | 1.0853--1.0934 | 24 | 1 |
| Monkeys | 1.4593--1.4856 | 1.6682--1.6773 | 95 | 2 |

### Near-surface water-view follow-up

Two repeats of the dedicated water viewpoint measure the complete swimming composition, including
the one conditional full-screen `SpriteBatch` veil submission. Both runs tracked 110 scene
submissions and held a 0.637 veil intensity. The result is 1.5405--1.5895 ms mean,
1.6881--1.8650 ms p95, and 1.7982--2.3059 ms p99. This is a viewpoint-specific cost record, not a
before/after attribution: the camera sees a different population than the five baseline views.
Dry views submit no water-view draw.

The subsequent current-feedback pass preserves that single draw and adds no texture or geometry.
Two repeats at the 62 m offshore current camera tracked 91 submissions with intensity 0.963 and
normalized current 0.741. They measured 1.4501--1.4756 ms mean, 1.6750--1.8775 ms p95, and
1.8992--2.2975 ms p99. As above, these numbers characterize that fixed composition rather than an
isolated tint cost.

### Distant-scenery follow-up

Two complete five-view repeats after adding the five-islet horizon batch produced the ranges
below. The static 787-triangle batch is always submitted once and therefore adds exactly one
tracked submission to each earlier composition. The batch also contains 11 simplified palm
silhouettes and uses stock `BasicEffect` fog; these integrated measurements do not isolate its
GPU cost. The heaviest beach view remains below 1.9 ms p95 in both repeats.

| View | Mean ms range | p95 ms range | p99 ms range | Tracked submissions |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.2134--1.2276 | 1.3573--1.4903 | 1.5140--1.7895 | 81 |
| Beach | 1.6756--1.6996 | 1.8311--1.8803 | 2.0788--2.1324 | 123 |
| Jungle | 1.3706--1.3816 | 1.5448--1.6426 | 2.1831--2.1944 | 90 |
| Driftwood/shore detail | 0.8084--0.8159 | 1.0957--1.1484 | 1.2494--1.3649 | 25 |
| Monkeys | 1.4467--1.5199 | 1.5762--1.8300 | 1.6290--2.2619 | 96 |

### Terrain contact-shadow follow-up

The first implementation sampled the procedural terrain under every visible ellipse on every
frame. Although the beach camera submitted only 348 shadow triangles, its mean time rose to
2.9014 ms. That implementation was rejected before commit. The retained path precomputes all
static terrain-conforming geometry and refreshes the culled upload only after 0.25 m of camera
motion, a small view/projection change, or 0.08 m of monkey motion. It preserves one draw and a
soft distance fade without paying the terrain generator cost every frame.

Two final five-view repeats produced the ranges below. A nearby shadow population adds exactly one
tracked submission; the far overview correctly submits none. These historical fixed cameras
exercised the cache more gently than movement; the moving traversal described above now covers
continuous refresh behaviour.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Shadows / triangles |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.2019--1.2710 | 1.4421--1.7637 | 1.9101--2.1372 | 81 | 0 / 0 |
| Beach | 1.7500--1.7589 | 2.1141--2.1572 | 2.5180--2.6049 | 124 | 29 / 348 |
| Jungle | 1.3729--1.3913 | 1.5558--1.5663 | 1.9584--1.9830 | 91 | 28 / 336 |
| Driftwood/shore detail | 0.7841--0.8205 | 0.9525--1.0426 | 1.3320--1.3448 | 26 | 3 / 36 |
| Monkeys | 1.4917--1.5610 | 1.6883--2.1026 | 2.1477--2.3905 | 97 | 26 / 312 |

### Movement-driven player-ripple follow-up

The player ripple is integrated into the existing ocean fragment shader and therefore adds no
geometry, texture, or draw submission. A uniform branch skips the distance, spatial-variation, and
band work when its smoothed intensity is zero. Two accelerated repeats of the dedicated water view
kept the ripple active at 0.984 intensity, tracked the same 112 scene submissions, and measured
1.6218--1.7052 ms mean, 2.0327--2.4040 ms p95, and 2.3293--2.7735 ms p99. This characterizes the
complete active-water composition rather than isolating fragment cost; moving-route profiling and
mobile hardware qualification remain open.

### Speed-responsive player-grass follow-up

The player bend is integrated into the existing instanced grass vertex shader and adds no mesh,
texture, draw submission, CPU spatial search, or per-patch update. A uniform controls a compact
distance branch around the authoritative player position; the affected radius grows from 0.55 m
while stationary to 1.35 m at full walking speed. Two complete five-view repeats produced the
ranges below. Tracked submissions are unchanged from the preceding contact-shadow baseline, and
all values remain within its observed debug-build variation.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.2165--1.2169 | 1.3981--1.5291 | 1.7187--2.0135 | 81 | 0 |
| Beach | 1.7383--1.7462 | 1.9396--2.1243 | 2.3705--2.4842 | 124 | 2,954 |
| Jungle | 1.3831--1.4192 | 1.6045--1.8176 | 2.0118--2.0770 | 91 | 2,932 |
| Driftwood/shore detail | 0.8063--0.8162 | 1.1357--1.1382 | 1.3767--1.4487 | 26 | 155 |
| Monkeys | 1.5090--1.5768 | 1.7521--2.1100 | 2.1391--2.3391 | 97 | 2,946 |

Two repeats of the dedicated full-interaction grass view held strength at 1.0, rendered 2,827
clusters in 126 tracked submissions, and measured 1.8014--1.8117 ms mean, 2.2166--2.3616 ms p95,
and 2.5548--2.8423 ms p99. These measurements establish the fixed-camera cost envelope; sustained
movement, transparent-overdraw inspection, release builds, and physical mobile hardware remain
open.

### Curved and daylight-shaded grass follow-up

The close mesh now uses five rather than four control levels, increasing a near cluster from 168
to 224 triangles. The far mesh drops from four to three levels and from 30 to 20 triangles because
its screen-space silhouette does not benefit from the intermediate curve. Blade orientation is
packed into existing vertex alpha, so two-sided fixed-sun diffuse, fold, root, view-edge, and
restrained transmission cues require no new vertex stream, texture, instance data, CPU update, or
draw submission.

Two complete five-view repeats produced the ranges below. The composition and submission counts
remain identical to the preceding grass-interaction measurements. The small timing differences are
inside normal debug-build run variation and must not be interpreted as an isolated shader cost.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.2185--1.2292 | 1.5142--1.6064 | 1.9785--2.0901 | 81 | 0 |
| Beach | 1.7259--1.7574 | 2.1388--2.1531 | 2.4479--2.5391 | 124 | 2,954 |
| Jungle | 1.4096--1.4220 | 1.6226--1.7988 | 2.0396--2.1484 | 91 | 2,932 |
| Driftwood/shore detail | 0.8239--0.8291 | 1.1045--1.1909 | 1.5008--1.5023 | 26 | 155 |
| Monkeys | 1.5598--1.5601 | 2.0335--2.0783 | 2.2638--2.3146 | 97 | 2,946 |

Two repeats of the dedicated full-interaction view rendered 2,827 grass clusters in 126 tracked
submissions at 1.7429--1.7595 ms mean, 1.8919--1.9007 ms p95, and 2.2903--2.3129 ms p99. Native
and actual Chrome/SwiftShader captures visually agree. Physical-device fragment cost and aliasing
remain open.

### Near-field substrate and leaf-litter follow-up

The terrain fragment shader now evaluates one additional microvariation octave and two sparse
one-hash leaf-litter cells only inside the low-slope interior and within a 24--68 m camera fade.
The first visually reviewed pass was rejected for large, frequent oval marks. The retained version
uses fewer, smaller, pointed and slightly curved leaves with bounded contrast. It adds no texture,
geometry, CPU placement data, or draw submission.

Two complete five-view repeats produced the ranges below. The overview remains effectively
unchanged because the near-field branch is inactive. Grass-heavy close views show roughly
0.04--0.11 ms mean movement relative to the preceding two-run sample, while p95/p99 values retain
substantial run variation. These integrated results do not isolate fragment cost.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.2074--1.2233 | 1.3667--1.4878 | 1.5471--1.7286 | 81 | 0 |
| Beach | 1.7705--1.7916 | 2.0252--2.2926 | 2.4122--2.5943 | 124 | 2,954 |
| Jungle | 1.4494--1.4675 | 1.7056--1.8704 | 2.1117--2.1703 | 91 | 2,932 |
| Driftwood/shore detail | 0.8202--0.8285 | 1.1343--1.1769 | 1.4441--1.4461 | 26 | 155 |
| Monkeys | 1.5414--1.6410 | 1.7863--2.1944 | 2.1887--2.5199 | 97 | 2,946 |

Two dedicated close-grass repeats rendered 2,827 clusters in 126 tracked submissions at
1.8411--1.8561 ms mean, 2.0394--2.2592 ms p95, and 2.4347--2.5395 ms p99. Sustained moving-route,
Web hardware, and physical-mobile measurements remain open. Native and actual
Chrome/SwiftShader close-grass and jungle captures visually agree on the retained detail.

### Layered high-wisp sky follow-up

The first sky-cloud field used three direction-linear sine waves that appeared as conspicuous
vertical streaks in first-person views. The retained replacement uses four trigonometric samples:
one directional warp, elevation-dominant long and fine bands, and one azimuth breakup value. It
adds no texture, vertex data, state transition, draw submission, clock, or weather update. A
visually acceptable six-sample candidate was rejected before qualification because its extra
full-screen fragment work was not justified for WebGL 2 and GLES 3.

Two complete five-view repeats produced the ranges below. Submission and visible-grass counts are
unchanged. The second run showed broad scheduling/driver variation in the jungle and monkey views,
so these integrated totals do not isolate the sky cost. The low ends lie about 0.01--0.08 ms from
the preceding sample; physical-mobile and hardware-browser fragment measurements remain required.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.2251--1.2462 | 1.3926--1.5614 | 1.7392--2.0134 | 81 | 0 |
| Beach | 1.8222--1.8359 | 2.2977--2.4338 | 2.6320--2.7505 | 124 | 2,954 |
| Jungle | 1.4726--1.6432 | 1.6870--2.3335 | 2.2558--2.7410 | 91 | 2,932 |
| Driftwood/shore detail | 0.8299--0.8814 | 1.2027--1.3149 | 1.4214--1.5850 | 26 | 155 |
| Monkeys | 1.6486--1.8296 | 2.1349--2.5882 | 2.3754--3.0800 | 97 | 2,946 |

Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 compile and render the retained shader.
The arm64 GLES 3 package builds, but its runtime appearance and fragment cost remain unqualified.

### Segmented palm leaflets and per-instance LOD follow-up

The near palm mesh replaces each broad seven-segment ribbon with a thin 14-triangle rachis and
nine pairs of tapered one-triangle leaflets. Depending on its 10--12-frond variant, a complete near
palm now contains 469--533 triangles instead of 289--317. The far mesh remains 102 triangles, but
its seven ribbons are widened to approximate the feathered crown envelope.

The previous 210 m whole-cell LOD choice kept excessive detail and could switch every palm in a
62.5 m cell together. The retained path tests the already gathered 330-instance population
individually and derives a stable 125--145 m threshold from each palm's deterministic yaw. This
reduces near instances from 35 to 0 in overview, 125 to 69 at the beach, 118 to 37 in jungle, and
87 to 37 in the monkey view; the close driftwood view intentionally keeps all 18 visible palms near.
Overview palm submissions fall from eight to four, jungle increases from five to eight because it
now legitimately contains all four near and far variants, and the other views retain their prior
palm submission counts.

Two complete five-view repeats produced the ranges below. The mixed-content timings overlap the
preceding sky sample and show the usual second-run scheduling variation, so no isolated speedup is
claimed. The result buys a materially better close silhouette while bounding the added geometry to
palms where the leaflets remain readable.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Palm near / far |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1554--1.2202 | 1.3370--1.5019 | 1.6559--1.8756 | 77 | 0 / 299 |
| Beach | 1.8172--1.9053 | 2.1165--2.5438 | 2.4653--2.9766 | 124 | 69 / 251 |
| Jungle | 1.4788--1.6474 | 1.6638--2.2253 | 2.1412--2.6124 | 94 | 37 / 83 |
| Driftwood/shore detail | 0.8373--0.9157 | 1.1517--1.3824 | 1.4243--1.7186 | 26 | 18 / 0 |
| Monkeys | 1.5833--1.7823 | 1.8955--2.4438 | 2.2376--2.8668 | 97 | 37 / 68 |

The dedicated close-palm native and actual Chrome/SwiftShader WebGL 2 captures visually agree.
The arm64 GLES 3 package builds with the revised geometry; physical-device aliasing, transition,
and fragment/vertex cost remain unqualified.

### Near-palm bark and coconut material follow-up

The two coconut-bearing near variants add four or five six-vertex faceted fruits, respectively,
for 32 or 40 extra triangles. Near variants therefore span 469--541 triangles; the unchanged far
variants remain 102 triangles. Subtle texture-free leaf-scar rings are evaluated only for near
draws through a per-batch uniform, so distant trunks avoid both the extra fragment work and
high-frequency band aliasing. The additions reuse the four existing near-variant submissions and
introduce no texture, asset, instance data, CPU update, or draw call.

Two complete final five-view repeats produced the ranges below. They overlap the preceding
segmented-crown baseline and preserve every submission and near/far population. No isolated speed
change is claimed; the result adds readable close material cues without a measurable integrated
regression on this debug baseline.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Palm near / far |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1773--1.2073 | 1.4178--1.5443 | 1.9134--1.9965 | 77 | 0 / 299 |
| Beach | 1.7777--1.8420 | 1.9404--2.3915 | 2.3468--2.6916 | 124 | 69 / 251 |
| Jungle | 1.4675--1.5164 | 1.6497--1.9800 | 2.1155--2.3181 | 94 | 37 / 83 |
| Driftwood/shore detail | 0.8188--0.8599 | 1.0771--1.2883 | 1.4991--1.5395 | 26 | 18 / 0 |
| Monkeys | 1.5762--1.5991 | 1.9065--2.0653 | 2.2238--2.3010 | 97 | 37 / 68 |

Native and actual Chrome/SwiftShader close-palm captures visually agree. The arm64 GLES 3 package
builds with the final shader and geometry, but physical-device appearance and cost remain
unqualified.

### Broadleaf crown silhouette and per-tree LOD follow-up

The retained broadleaf near mesh adds seven tapered support branches, eight pointed perimeter
leaves, smaller separated crown lobes, and deterministic contour irregularity. It contains 776
triangles instead of 688. The far crown was rounded from 136 to 208 triangles so it remains
credible at the 105--129 m transition while still costing under 27 percent of the near mesh.
The prior 155 m whole-cell choice was replaced by a yaw-staggered per-tree test after cell
rejection. Benchmark JSON and F1 diagnostics now expose near/far canopy populations.

Visual review rejected an intermediate small-core/five-leaf-fan design because it made every
canopy flat and star-shaped. Native and actual Chrome/SwiftShader captures of the retained spatial
crown agree. In the deterministic jungle view, 109 visible trees split into 63 near and 46 far
instances; the mixed LOD needs 25 jungle submissions rather than the prior 24 because one
additional variant/tier group is legitimately occupied.

Two isolated five-view repeats produced the ranges below. Overview, beach, and jungle overlap the
preceding integrated baseline. The monkey view, with 75 near crowns, rises by roughly
0.04--0.07 ms mean; that small measured cost is retained for the improved close silhouette.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Canopy near / far |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1770--1.2129 | 1.3607--1.5038 | 1.6589--1.8401 | 77 | 0 / 225 |
| Beach | 1.7932--1.7932 | 1.9957--2.0057 | 2.3679--2.4637 | 124 | 49 / 231 |
| Jungle | 1.4689--1.5011 | 1.6423--1.7189 | 2.0614--2.1899 | 95 | 63 / 46 |
| Driftwood/shore detail | 0.8360--0.8621 | 1.1511--1.2726 | 1.5135--1.6196 | 26 | 0 / 0 |
| Monkeys | 1.6264--1.6429 | 2.1366--2.1403 | 2.3410--2.3680 | 97 | 75 / 53 |

An earlier benchmark overlapping the WebAssembly link was discarded as contaminated before these
two isolated repeats. The arm64 GLES 3 package builds and lints; physical-device appearance,
transition behaviour, and cost remain unqualified.

### Pointed-leaf near-bush follow-up

The three near-bush variants now use smaller volume lobes with two folded pointed leaves per lobe,
raising their geometry from 288/360/432 to 320/400/480 triangles. The 48-triangle far meshes,
placements, instance streams, culling, update work, and existing variant/tier submissions remain
unchanged. Native and actual Chrome/SwiftShader captures agree on the retained silhouette.

Two isolated five-view repeats produced the ranges below. Scene populations and tracked
submissions exactly match the preceding canopy baseline. The bush-heavy jungle and monkey ranges
overlap it closely, so no isolated speed change is claimed.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible bushes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1614--1.1628 | 1.2993--1.3172 | 1.3906--1.4960 | 77 | 0 |
| Beach | 1.7704--1.7861 | 1.9685--2.0025 | 2.3378--2.3659 | 124 | 99 |
| Jungle | 1.4870--1.5316 | 1.8256--1.9536 | 2.1059--2.3510 | 95 | 102 |
| Driftwood/shore detail | 0.8477--0.8619 | 1.2613--1.3641 | 1.5483--1.6303 | 26 | 0 |
| Monkeys | 1.6192--1.6376 | 2.1083--2.1500 | 2.2909--2.6107 | 97 | 142 |

The arm64 GLES 3 package builds and lints with this geometry; physical-device appearance and cost
remain unqualified.

### Upright broadleaf ground-plant follow-up

Near ground-plant variant 1 now uses six visible petioles and folded upright broad leaves. Its
geometry falls from 64 to 36 triangles; the 16-triangle far mesh, other variants, placements,
instance streams, culling, update work, and submissions remain unchanged. A cleared deterministic
ground camera verifies the intended first-person silhouette on native OpenGL 3.3 and actual
Chrome/SwiftShader WebGL 2.

Two preliminary suites were rejected because the visible interactive game was still rendering on
the same GPU. After closing that client, two isolated five-view repeats produced the ranges below.
Populations and submissions exactly match the preceding baseline. No regression is visible, but
the small geometry reduction is not claimed as a measured speedup.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible ground plants |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1504--1.1513 | 1.2580--1.2740 | 1.3373--1.4452 | 77 | 0 |
| Beach | 1.6825--1.7669 | 1.8066--2.1904 | 2.2505--2.4963 | 124 | 209 |
| Jungle | 1.4611--1.4616 | 1.7001--1.7433 | 2.0958--2.1076 | 95 | 129 |
| Driftwood/shore detail | 0.7627--0.7730 | 0.8487--0.8721 | 0.9874--1.0737 | 26 | 0 |
| Monkeys | 1.5347--1.5658 | 1.9839--2.0686 | 2.2044--2.6052 | 97 | 257 |

The arm64 GLES 3 package builds and lints with this geometry; physical-device appearance and cost
remain unqualified.

### Near-rock material follow-up

Near rock vertices now select a restrained texture-free fragment treatment with world-space strata
and sparse lichen modulation. Far rocks and contact disks skip the branch, and the change adds no
texture, geometry, instance data, CPU update, or tracked submission. Native reef and jungle
captures were inspected, and the actual Chrome/SwiftShader WebGL 2 reef stage requires the same
material diagnostic. The arm64 GLES 3 package builds and lints, but device appearance and fragment
cost remain unqualified.

Two isolated five-view repeats produced the ranges below. Scene populations and submissions match
the preceding ground-plant baseline exactly, and the timing ranges show no integrated regression.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible rocks / reef |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1003--1.1116 | 1.2059--1.2179 | 1.3042--1.4784 | 77 | 43 / 4 |
| Beach | 1.6633--1.7042 | 1.7704--1.8475 | 2.1972--2.4931 | 124 | 74 / 5 |
| Jungle | 1.4197--1.4318 | 1.5744--1.6127 | 1.9769--2.1264 | 95 | 50 / 2 |
| Driftwood/shore detail | 0.7637--0.8007 | 0.8520--0.9912 | 1.0729--1.3465 | 26 | 7 / 1 |
| Monkeys | 1.5232--1.5473 | 1.9624--1.9899 | 2.1455--2.1841 | 97 | 41 / 2 |

### Near-log material follow-up

Near fallen-log sides now select local-space longitudinal bark grooves and narrow irregular
fissures, while their cut ends select gently irregular growth rings. Both treatments fade before
the 72 m log LOD transition. The same change also fades the preceding rock detail before its 92 m
transition. Far meshes, moss/contact geometry, populations, instance data, and submissions remain
unchanged. A first cut-end treatment was rejected because its high-contrast rings looked like a
painted target; the retained pass uses fewer, softer rings. Native and actual Chrome/SwiftShader
close-log captures were inspected and agree.

Two isolated five-view repeats produced the ranges below. The views containing four or five logs
show a small upper shift of a few hundredths of a millisecond, with wider normal p95 variation in
the beach and first monkey samples. The presentation gain is retained, but no zero-cost claim is
made. The log-free driftwood view remains unchanged.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible logs |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1121--1.1313 | 1.2062--1.2918 | 1.2647--1.8957 | 77 | 1 |
| Beach | 1.7399--1.7865 | 2.0930--2.0980 | 2.4339--2.7728 | 124 | 2 |
| Jungle | 1.4497--1.4644 | 1.6085--1.6288 | 2.0122--2.0430 | 95 | 4 |
| Driftwood/shore detail | 0.7623--0.7831 | 0.8515--0.8769 | 0.9498--1.1295 | 26 | 0 |
| Monkeys | 1.5258--1.5775 | 1.6939--2.0028 | 2.0933--2.3130 | 97 | 5 |

### Sun-bleached driftwood material follow-up

Near driftwood sides and branch stubs now select a soft local-space longitudinal weathering field;
cut ends and every far mesh retain the plain material. Detail fades from 30--56 m before the 58 m
mesh transition and adds no geometry, texture, instance data, CPU update, or submission. Visual
review rejected a first higher-frequency treatment because its repeated short fissures looked
stamped. Native and actual Chrome/SwiftShader close-driftwood captures agree on the retained
lower-contrast, longer-grain pass.

Two isolated five-view repeats produced the ranges below. They overlap the preceding integrated
baseline, including the driftwood view that directly exercises one near instance, so no measurable
regression or speed change is claimed.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible driftwood |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.0887--1.1593 | 1.1837--1.3169 | 1.2811--1.8612 | 77 | 0 |
| Beach | 1.6999--1.7333 | 1.8244--1.8673 | 2.4286--2.4534 | 124 | 4 |
| Jungle | 1.4114--1.4611 | 1.5479--1.7658 | 2.0095--2.1207 | 95 | 1 |
| Driftwood/shore detail | 0.7763--0.8006 | 0.8580--0.9871 | 1.2184--1.4301 | 26 | 1 |
| Monkeys | 1.4925--1.5420 | 1.6373--2.0082 | 2.0427--2.2312 | 97 | 0 |

### Fixed-sun interior-dapple follow-up

The terrain shader reuses the existing broad, medium, and fine fields to add restrained warm sun
patches and cooler floor areas beyond the beach transition. The slope and distance terms reuse
the existing steep/detail fades, and a spatially coherent early branch skips the colour shaping
for shore, distant, and fully steep fragments. The pass adds no noise sample, texture, geometry,
CPU state, or draw submission.

A same-session no-dapple control was compiled and measured twice before restoring the retained
shader. Four content-heavy view ranges overlap the retained measurements. The overview control
was 1.1640--1.1706 ms mean versus the retained shader's 1.1409--1.2137 ms; the broad variation and
overlapping tails do not support an isolated cost claim. The retained two-run results are:

| View | Mean ms range | p95 ms range | p99 ms range | Submissions |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.1409--1.2137 | 1.2637--1.4466 | 1.3718--1.8739 | 77 |
| Beach | 1.8151--1.9109 | 2.2815--2.3749 | 2.6576--2.8556 | 124 |
| Jungle | 1.4837--1.6552 | 1.6501--2.2806 | 2.0622--2.7964 | 95 |
| Driftwood/shore detail | 0.8237--0.9109 | 1.1105--1.3527 | 1.3728--1.6321 | 26 |
| Monkeys | 1.6669--1.7469 | 2.2280--2.3417 | 2.4453--2.5808 | 97 |

Native and actual Chrome/SwiftShader jungle and ground captures agree visually. Hardware-browser
and physical-mobile fragment measurements remain open.

### Jungle facet-lighting functional cost check

The portable jungle fragment shader now derives one safe geometric normal from world-position
derivatives and evaluates a two-sided fixed-sun term plus small upward fill. It changes no vertex,
index, instance, texture, culling, LOD, or submission count. Native and actual
Chrome/SwiftShader jungle/ground images were visually compared and retain the same composition
with substantially clearer low-poly crown and leaf facets.

The complete parallel native regression run, while the operator's visible game still shared the
GPU, recorded 2.6643 ms mean / 3.3668 ms p95 for the 95-submission jungle view and 2.9798 ms mean /
3.7292 ms p95 for the moving traversal. These are functional safety observations, not an isolated
before/after cost claim. Two isolated native repeats plus hardware-browser and physical-mobile
fragment measurements remain required before extending the technique to additional dense layers.

### Authored primary-footpath material follow-up

The terrain fragment shader derives a narrow broken-edge dry-earth cue from the same sinusoidal
centre line and range that reserve the primary vegetation corridor. It reuses the shader's existing
broad, medium, and fine fields, retains short mesh grass over the route, and adds no texture,
geometry, CPU placement, instance, or draw submission. Endpoint, shoreline, slope, and camera-
distance fades bound the extra fragment work to relevant low-slope interior ground.

The retained native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 beach captures were inspected
at 1280 by 720 and show the same subdued route cue without reading as a road or objective marker.
The concurrent native jungle benchmark completed at 2.6113 ms mean / 3.3813 ms p95, and the moving
traversal at 3.0229 ms mean / 3.8266 ms p95. The operator's visible game shared the GPU, so these
are functional safety checks rather than an isolated measurement of the shader branch. Hardware-
browser and physical-Android fragment cost remain open.

### Distinct authored clearing-material follow-up

The east and west clearing masks share their authoritative placement centres/radii and reuse the
terrain shader's existing noise, detail fade, slope rejection, and litter fields. Only the clearing
on the relevant side of the island is evaluated. The retained implementation compares squared
distance, avoiding the first pass's per-fragment square root while keeping a soft irregular edge.
The treatment adds no texture, geometry, CPU placement, instance, or draw submission.

Dedicated 1280 by 720 native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 captures of both
clearings were inspected and agree: the larger east room reads warmer/drier, while the smaller west
room is darker and leaf-richer without either appearing as a hard circular decal. After the squared-
distance revision, two concurrent native runs measured 2.4283--2.7870 ms mean / 3.1108--3.5510 ms
p95 for the jungle view and 3.0796--3.0869 ms mean / 3.8265--3.9338 ms p95 over the 675.9635 m
traversal. The visible game shared the GPU, so these results qualify function and guard against a
gross regression; they are not an isolated fragment-cost baseline. Hardware-browser and physical-
Android cost remain open.

### Authored natural clearing-landmark follow-up

The first natural landmark pass reuses the existing rock and fallen-log meshes, material shader,
cell ownership, LOD, culling, instancing, contact-shadow, and collision paths. It adds five land-
rock instances and one fallen-log instance across the outer rings of the two clearings, plus five
rock proxies and seven sampled log proxies. It adds no shader, mesh, texture, render pass, or draw
submission. The deterministic world now contains 171 rocks in total, including 14 offshore
outcrops, 12 fallen logs, and 820 static collision proxies in 481 occupied 16 m cells.

Dedicated 1280 by 720 native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 captures of both
clearings were inspected and agree. The east composition reads as an irregular three-rock group;
the west uses two rocks and a transverse cut-ended log. Both preserve their open central walking
room. A sequential native safety check measured 2.3671 ms mean / 2.7291 ms p95 in the fixed jungle
view and 2.7827 ms mean / 3.4859 ms p95 over the 675.9635 m traversal. The operator's visible game
shared the GPU, so these guard against a gross regression rather than establishing a new isolated
baseline. Hardware-browser and physical-Android cost remain open.

### Split-leaf ground plant and instance-accurate jungle LOD follow-up

Near ground-plant variant 2 now uses five low leaves with visible petioles, solid midribs, and three
separated folded-lobe pairs per leaf. Its 140-triangle near mesh replaces the former 72-triangle
strap-leaf rosette; its 16-triangle far mesh, 2,225-placement population, instance format, and
variant submission remain unchanged. Native review rejected an initial broad undivided silhouette
and a second overly skeletal pass before retaining the fuller folded-lobe composition.

That review exposed a pre-existing game-side precision problem: understory, bush, and ground-plant
LOD was selected from a shared cell centre, so a plant 2.8 m from the QA camera could receive its
far mesh. All jungle classes now use the shared cell AABB only as a conservative rejection. Actual
instance distance then enforces the maximum range and selects near/far LOD. The existing instance
records already contained the required position and threshold fields, so the correction adds no
allocation, instance-buffer field, shader branch, or per-vertex work. It adds bounded scalar
distance tests while producing accurate populations; the ground QA view now reports 18 near and
34 far ground plants instead of showing the selected close plant through its far representation.

Two complete six-view native safety suites ran while the operator's visible game shared the GPU.
The timing spread therefore qualifies function and catches a gross regression, but is not an
isolated before/after comparison. Exact distance selection legitimately changes visible
populations and adds three occupied near/far variant submissions in the fixed jungle view.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Understory / bushes / ground |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.7681--1.7834 | 1.9791--2.0042 | 2.1594--2.2077 | 77 | 0 / 0 / 0 |
| Beach | 2.7009--2.7467 | 3.1837--3.2864 | 3.6394--3.6663 | 124 | 352 / 80 / 168 |
| Jungle | 2.3368--2.8219 | 2.6892--3.4801 | 3.2293--3.9586 | 99 | 462 / 108 / 162 |
| Driftwood | 1.3576--1.3643 | 1.7558--1.8695 | 2.1850--2.2899 | 26 | 0 / 0 / 0 |
| Monkeys | 2.5465--2.9731 | 3.3558--3.8870 | 3.6482--4.4972 | 98 | 474 / 116 / 234 |
| Traversal | 2.8276--3.2742 | 3.5034--4.2125 | 4.3222--4.7245 | 12 final | route-varying |

Hardware-browser and physical-Android performance remain open.

### Forked driftwood and instance-accurate natural-prop LOD follow-up

Near driftwood variant 2 replaces its single four-triangle branch spike with three curved tapered
six-sided limbs. The complete near mesh grows from 74 to 160 triangles; its 30-triangle far mesh,
28-placement population, instance layout, material shader, and variant submission remain
unchanged. Native review rejected the initial side-on composition because the limbs collapsed into
the camera axis. The retained oblique QA view shows the fork clearly and reports four near / zero
far driftwood pieces. Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 captures agree.

The review also extended the preceding instance-distance correction to the natural-prop renderer.
Shared cell AABBs now perform only conservative range rejection; actual rock, fallen-log, and
driftwood positions enforce their maximum distances and choose near/far LOD. Log and driftwood
instance records already had position fields that were not populated; populating and consuming
them adds no GPU instance field, allocation, shader branch, texture, or draw. It does add bounded
scalar distance tests and can legitimately change occupied near/far submissions at cell edges.

Two six-view native safety suites ran while the operator's visible game shared the GPU. The
driftwood camera itself also changed from side-on variant 0 to an oblique variant-2 view, so its
population/submission count is not a like-for-like comparison with historical rows. These timings
qualify function and guard against a gross regression, not isolated branch or culling cost.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Rocks / logs / driftwood |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.7762--1.7842 | 2.0748--2.1294 | 2.3675--2.4924 | 77 | 34 / 0 / 1 |
| Beach | 2.6986--2.7497 | 3.0540--3.4583 | 3.6351--3.9753 | 125 | 78 / 3 / 2 |
| Jungle | 2.3285--2.3619 | 2.6086--2.6343 | 3.2364--3.2916 | 99 | 56 / 4 / 1 |
| Driftwood | 1.4848--1.4964 | 1.8351--1.9153 | 2.3159--2.3499 | 54 | 8 / 0 / 4 |
| Monkeys | 2.5316--2.5605 | 3.3698--3.3737 | 3.5537--3.6877 | 100 | 44 / 5 / 0 |
| Traversal | 2.8703--2.9211 | 3.7702--3.9156 | 4.3033--4.5354 | 12 final | route-varying |

Hardware-browser and physical-Android performance remain open.

### Biome-aware grass colour follow-up

The coast/path/clearing colour pass adds one 32-bit value to each grass instance, taking the
instance record from 40 to 44 bytes. The existing instance population, near/far geometry, cell
culling, shader effect, and draw submissions are unchanged. The vertex and fragment stages use
bounded piecewise colour mixes, and the far-ground merge consumes the same biome value so grass
does not revert to one colour at distance.

One isolated six-view debug safety suite on the private accelerated display produced the values
below. This is a post-change functional guard rather than an isolated before/after GPU-cost claim;
the existing release, hardware-browser, and physical-Android measurements remain necessary.

| View | Mean ms | p95 ms | p99 ms | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.4173 | 1.9245 | 2.3328 | 77 | 0 |
| Beach | 2.0614 | 2.2890 | 2.6905 | 125 | 2,954 |
| Jungle | 1.8265 | 2.3380 | 2.6544 | 99 | 2,932 |
| Driftwood | 1.2069 | 1.6855 | 1.9505 | 54 | 99 |
| Monkeys | 1.9478 | 2.5403 | 2.7793 | 100 | 2,946 |
| Traversal | 2.3128 | 3.2074 | 3.7775 | 12 final | route-varying |

### Sparse sun-cured grass-tip follow-up

The accepted tip variation is baked once into the existing shared near/far mesh vertex colours.
It adds no shader operation, texture, instance field, geometry, population, culling work, or draw.
A shader-based prototype was deliberately discarded after review because its subtle visual result
did not justify permanent vertex-stage arithmetic.

One six-view native debug safety suite on the private accelerated display produced the values
below while the operator's visible game shared the GPU. The immediately preceding experimental
suite moved in both directions, so these values are a gross-regression and reproducibility guard,
not evidence of a speedup or isolated static-colour cost.

| View | Mean ms | p95 ms | p99 ms | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.8332 | 2.2592 | 2.6108 | 77 | 0 |
| Beach | 2.7034 | 2.9226 | 3.6312 | 125 | 2,954 |
| Jungle | 2.3808 | 2.6545 | 3.3306 | 99 | 2,932 |
| Driftwood | 1.4789 | 1.8064 | 2.4349 | 54 | 99 |
| Monkeys | 2.5456 | 3.0738 | 3.5427 | 100 | 2,946 |
| Traversal | 2.9033 | 3.7133 | 4.4274 | 12 final | route-varying |

### Progressive grass LOD follow-up

Grass cells no longer replace every patch at one 82 m threshold. Instance records in each cell are
ordered once by a deterministic rank, and a smoothstep population split moves the boundary from an
all-near prefix to an all-far suffix across 70--94 m. A pure cell still takes one submission; only a
mixed cell takes one near and one far submission. No shader operation, instance field, geometry,
texture, population, culling radius, or per-frame instance upload was added.

The dedicated close view renders 2,827 patches in 15 cells as 1,318 near and 1,509 far patches.
Four cells are mixed, so the pass uses 19 rather than 15 submissions. OpenGL 3.3 and actual
Chrome/SwiftShader WebGL 2 report those same counts and show matching frames, which also qualifies
the nonzero per-instance buffer offset on both paths.

One six-view native debug safety suite produced the values below while the operator's visible game
shared the GPU. Fixed-view total submissions increase only where a grass cell is mixed. Compared
with the immediately preceding static-colour suite, scene timings move in both directions and are
approximately within one percent; this is functional cost evidence, not a hardware budget.

| View | Mean ms | p95 ms | p99 ms | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.7975 | 2.2197 | 2.5680 | 77 | 0 |
| Beach | 2.7294 | 3.0683 | 3.6776 | 127 | 2,954 |
| Jungle | 2.4005 | 2.6208 | 3.3256 | 103 | 2,932 |
| Driftwood | 1.4942 | 1.8951 | 2.4016 | 54 | 99 |
| Monkeys | 2.5529 | 3.3032 | 3.5371 | 105 | 2,946 |
| Traversal | 2.8983 | 3.6413 | 4.1533 | 12 final | route-varying |

## Measurement rules

- Record the game commit, dependency revisions, build type, resolution, GPU, driver, and audio
  availability with every baseline used for a decision.
- Compare identical viewpoints and settings. A faster number from a different visible population
  is not evidence of an optimization.
- Prefer p95 and p99 duration when judging frame stability; mean FPS can hide hitches.
- Run at least two suites after a surprising change and retain the raw JSON until the conclusion
  is documented.
- Never use the unaccelerated Android AVD for performance claims. Android budgets require a
  physical device or a stable KVM-accelerated emulator with explicit hardware details.
- WebGL 2 needs a visible hardware-accelerated browser measurement rather than SwiftShader.

## Budgets still to establish

Final per-target frame-time, draw-call, triangle, memory, startup, audio-source, and thermal
budgets remain open. Startup phases and report validation now exist, but release-mode Linux
repetitions, browser cold-load instrumentation, Android device measurements, memory timing, and
sustained whole-island routes are required before those limits can be set responsibly.
