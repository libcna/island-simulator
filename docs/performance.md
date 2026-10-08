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
then a 676 m moving camera rail. The generated schema-3 JSON files contain resolution, renderer
identity, sampling settings, mean/median/p95/p99 frame duration, minimum/maximum duration, mean
FPS, exact sample counts above 8.333/16.667/33.333 ms, the final scene snapshot, and min/max/mean
ranges for tracked submissions, terrain cells,
coalesced terrain draw submissions,
contact shadows, palms, canopy, grass, and visible beach crabs. These generated reports belong under `build/` or
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
The historical API 35 x86_64 software-only Android AVD reported 157126.852 ms world generation,
256204.401 ms content creation, 7708.990 ms first draw, and 421449.067 ms total at 800 by 360.
Those QEMU TCG values only proved that the Android instrumentation and GLES 3 scene reached a first
draw. After `/dev/kvm` became available, the same API 35 AVD under Android Emulator 36.5.11,
KVM, and host-GPU gfxstream reported 487.895 ms world generation, 3.421 ms framework
initialization, 1107.472 ms content creation, 20.058 ms pre-draw, 47.937 ms first draw, and
1666.782 ms total at its restored 2400 by 1080 landscape surface. This is an emulator startup
qualification point, not a physical-device budget.

Two isolated current-source Linux repetitions were recorded after all other game/browser/emulator
clients were closed. The stripped relocatable Release package reached the first completed game
draw in 506.104 and 426.087 ms. Its world phase measured 202.003/161.497 ms, GPU-content phase
233.506/202.621 ms, framework phase 28.868/26.489 ms, pre-draw phase 40.071/31.486 ms, and first-
draw body 1.656/3.995 ms. Matching unoptimized Debug runs completed in 1762.530 and 1981.890 ms;
their world phase was 901.304/917.923 ms and content phase 798.467/993.909 ms. All four used the
same 1280 by 720 overview, full quality profile, OpenGL 3.3 path, AMD Radeon 780M/Mesa 25.0.7, and
private Weston/Xwayland environment, with smoke-mode audio deliberately skipped.

The two Release results put both major application-owned construction phases near 0.2 seconds and
the complete first-draw boundary below 0.51 seconds on this machine. Their observed variability
does not justify a speculative generator rewrite or a target-specific loading path. No broad
startup budget is assigned: repeated hardware-browser cold-load measurements, interactive native
audio startup, and physical-Android launch evidence remain required.

## KVM Android emulator qualification

The accelerated API 35 x86_64 pass uses KVM version 12 and gfxstream on the host AMD Radeon 780M.
The complete GLES 3 scene, AAudio startup, independent movement/look gestures, and a same-process
Home/resume cycle are functional. A memory snapshot after the scene settled reported 87.3 MiB
total PSS, 167.3 MiB RSS, and 0.5 MiB swap PSS. The hot resume took 77 ms application total /
80 ms wait and paused then restarted the existing 44.1 kHz stereo AAudio stream.

A later current-source strict-audio run completed first draw in 1681.732 ms total: 515.618 ms world
generation, 1.996 ms framework initialization, 1099.860 ms content creation, 19.109 ms pre-draw,
and 45.149 ms first draw. The gate observed bounded sequential upload, persistent-loop DC
centering, and slow ambient variation through the Android platform log, and the inspected frame
retained the complete scene and translucent touch HUD. This single run is functional evidence, not
a startup budget or a measurement of audible quality.

The terrain originally retained 64 independently culled cells but also owned 64 duplicated-edge
vertex/index buffer pairs and issued one draw per visible cell. It now stores the 241 by 241 grid
as one 58,081-vertex, 115,200-triangle 16-bit mesh, retains all 64 cell bounds and cell-ordered
index ranges, and coalesces adjacent visible cells. The overview exposes 58 visible cells as three
terrain submissions. Native OpenGL 3.3, actual WebGL 2, and Android GLES 3 overview captures show
the same continuous terrain without seams.

Two post-change SurfaceFlinger samples at the same initial Android scene averaged 34.163 ms and
32.590 ms between presentations (29.27 and 30.68 frames/s), with 6 and 1 of 126 intervals over
40 ms. The isolated pre-change sample averaged 40.903 ms (24.45 frames/s) with 71 of 126 intervals
over 40 ms. This is a strong same-AVD direction check, not isolated GPU timing: it includes the
Android compositor, emulator, host scheduling, and host graphics translation. Physical-device
frame pacing, thermal behaviour, power, and sustained memory remain open.

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

## Current shared-buffer Linux qualification

After the full scene and shared terrain-buffer work, a fresh isolated private-Xwayland suite on the
same Radeon 780M recorded the following debug results. Audio remained unavailable in the private
compositor. These numbers are not a direct terrain-only before/after comparison with the initial
baseline because many later visual systems were added; they establish the current runnable state
and validate the lower submission accounting.

| View | Mean ms | p95 ms | p99 ms | Tracked submissions | Terrain cells / draws |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.3356 | 1.8511 | 2.1589 | 22 | 58 / 3 |
| Beach | 2.2530 | 2.9296 | 3.3000 | 91 | 44 / 4 |
| Jungle | 2.4574 | 3.8252 | 5.9426 | 88 | 23 / 8 |
| Driftwood/shore detail | 1.4333 | 1.9200 | 2.5739 | 48 | 9 / 3 |
| Monkeys | 2.3064 | 3.5301 | 4.3514 | 87 | 20 / 5 |
| Moving traversal | 3.7348 | 7.7032 | 9.6555 | 7 at final sample | 7 / 2 at final sample |

The traversal ranged over 6--47 visible terrain cells but only 2--7 terrain submissions, with a
4.4617-draw mean. Its total tracked submissions ranged from 7 to 94. Fixed-scene terrain draw
counts remain constant across all 600 samples, and every report includes the new structured
`terrain_draw_submissions` field.

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

The later coherent-current pass changes only renderer-independent player movement and debug data.
It adds one `atan2`, one sine, and a few scalar operations per active player update, allocates no
memory, and adds no release draw submission, mesh, texture, shader work, or audio source. F1/F2
reuse the existing debug passes and display the exact velocity only when diagnostics are enabled;
the fixed-scene GPU timings above therefore remain the relevant rendering baseline.

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

### Shoreline-transition surface-burst follow-up

The entry/exit burst reuses the ocean submission and player-position uniforms, adding no geometry,
texture, allocation, or draw. Its additional fragment branch is inactive outside the bounded
1.6-second effect; while active it evaluates two radial bands and angular breakup only for the
existing ocean fragments. Actual WebGL 2 and KVM/gfxstream GLES3 water captures compile and show
the effect. No new native or physical-mobile timing is assigned from those functional checks, so
the existing active-water measurements remain context rather than a claimed isolated burst cost.

### Broken shoreline-foam continuity follow-up

The broad ocean foam reuses its existing spatial sine as a squared 16--100 percent continuity
mask. The exact ribbon reuses its existing nested crest mask to modulate the trailing wash between
28 and 100 percent. The change adds no trigonometric evaluation, texture, geometry, buffer,
allocation, or draw submission; it adds only bounded scalar shaping inside the two existing
fragment shaders. A complete actual-browser WebGL 2 suite and visual comparison of the fixed water
view pass. The arm64 GLES 3 package builds and lints, but no new Android runtime or physical-GPU
performance claim is inferred.

### Reused-slope shallow-ridge follow-up

The near-shore water review showed that large calm shallow regions retained the intended colour
but lacked readable local surface relief at low viewing angles. A bounded colour cue now reuses
the length of the five-wave analytic slope already calculated for the normal and foam. It fades
out with the existing 135--420 m detail range, Fresnel reflection, and deep-water interpolation.
The change adds one length, two smooth steps, and scalar colour shaping to existing fragments; it
adds no sine, texture, varying, geometry, allocation, or draw submission.

The complete actual Chrome/SwiftShader WebGL 2 suite passes all 22 stages and requires the new
diagnostic. Original-resolution calm-water and offshore-current captures were inspected: the
shallow surface gains subdued moving relief without a bright band or a material change offshore.
The OpenGL 3.3 object compiles without relinking the visible game, while arm64 GLES 3 builds and
lints. Physical-mobile appearance and isolated fragment cost remain unqualified.

### Reused-slope sunward-crest follow-up

The calm-water review still left broad near-field areas with little directional wave definition.
A narrow crest window now projects the already-computed analytic slope onto the fixed sun's planar
direction, retains only a small sun-facing signed range, and fades from 85 to 245 m. Fresnel and
shallow-water bounds suppress it at the reflective horizon and in deeper water. The accepted path
adds one planar normalization, one dot product, three smooth steps, and bounded colour shaping; it
adds no sine, texture, varying, geometry, allocation, CPU update, or draw submission.

The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages and requires the
exact diagnostic in lifecycle, calm-water, and offshore-current gates. Both original-resolution
captures were inspected and retain a quiet horizon without repeated bright bands. A current
x86_64 KVM/gfxstream GLES 3 APK passed the same exact diagnostic gate; its inspected 2400 by 1080
water frame retains the corresponding subtle relief and clean touch overlay without a fatal,
shader, or `GL_INVALID_OPERATION` diagnostic. OpenGL 3.3 compiles the revised object, and the
default arm64 package builds and lints. The current OpenGL 3.3 water capture also retains the
subdued crest treatment without repeated bright bands. Physical-mobile appearance and isolated
fragment cost remain unqualified.

### Bounded wet-sand sheen follow-up

The existing wet-edge terrain branch now derives a geometric normal from screen-space world-position
derivatives and applies one narrow fixed-sun half-vector highlight plus a much weaker grazing term.
The calculation runs only where the authoritative signed shoreline distance produces nonzero wet
coverage. It adds no texture, varying, vertex/index data, allocation, geometry, or draw submission;
the incremental fragment work is two derivatives, bounded normalization/dot operations, and two
powers in the wet band. The complete actual-browser WebGL 2 suite compiles both GLSL ES 300 stages,
requires the exact feature diagnostic, and passes all 22 stages. Fixed water, driftwood, and beach-
ground captures were compared at original resolution and retain a restrained edge without a new
uniform bright line. The arm64 GLES 3 package builds and lints, but physical-mobile fragment cost
and runtime appearance remain unqualified.

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

### Fuller near-palm leaflet silhouette follow-up

Original-resolution close-palm review showed excessive sky gaps between the segmented near
leaflets. Their two existing root vertices now extend modestly farther along the rachis and outward,
while every tip remains unchanged. This preserves the individually tapered, separated silhouette
and the 494--566-triangle near-variant range established after the root pass. It adds no vertex,
index, instance, buffer, draw, LOD, collision, shader, or far-mesh cost.

The before/after native frames and the final actual Chrome/SwiftShader WebGL 2 frame were inspected
at original resolution. The crowns read fuller without returning to a solid paddle-shaped ribbon.
The complete 22-stage browser suite requires the exact fuller-leaflet diagnostic and passes; the
OpenGL 3.3 object and arm64 GLES 3 package build through the shared compiler cache. Physical-device
appearance and the moving 125--145 m transition remain unqualified.

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

### Near-canopy procedural bark follow-up

The shared jungle fragment shader identifies the existing brown canopy material and applies
local-space longitudinal grain plus narrow winding fissures only within a 38--68 m fade. It reuses
the existing local position and fixed-sun path, adding one interpolant and bounded fragment math
without textures, vertices, indices, instances, buffers, CPU work, draws, or changes to the far
mesh and 105--129 m LOD policy.

The complete actual Chrome/SwiftShader WebGL 2 suite requires the exact bark diagnostic and passes
all 22 stages. Original-resolution full-frame and close-crop review shows readable variation on the
nearest trunks without a strong repeating stripe pattern; distant trunks remain calm. The OpenGL
3.3 object compiles and the arm64 GLES 3 package builds/lints, but isolated GPU cost, physical-
mobile aliasing, and appearance remain unqualified.

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
and sparse lichen modulation. A later same-branch follow-up adds a low fixed-sea-level dark/cool
wet band, faded by the existing near-detail distance factor. Far rocks and contact disks skip the
branch, and neither change adds texture, geometry, instance data, CPU update, or tracked
submission. Native reef and jungle captures were inspected for the first pass; the current actual
Chrome/SwiftShader WebGL 2 reef capture was inspected for the wet band and requires both explicit
diagnostics. The arm64 GLES 3 package builds and lints, but device appearance and fragment cost
remain unqualified on physical hardware; a later fixed-view KVM/gfxstream capture qualifies the
intended close appearance without establishing physical-GPU cost.

Two isolated five-view repeats produced the ranges below. Scene populations and submissions match
the preceding ground-plant baseline exactly, and the timing ranges show no integrated regression.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible rocks / reef |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.1003--1.1116 | 1.2059--1.2179 | 1.3042--1.4784 | 77 | 43 / 4 |
| Beach | 1.6633--1.7042 | 1.7704--1.8475 | 2.1972--2.4931 | 124 | 74 / 5 |
| Jungle | 1.4197--1.4318 | 1.5744--1.6127 | 1.9769--2.1264 | 95 | 50 / 2 |
| Driftwood/shore detail | 0.7637--0.8007 | 0.8520--0.9912 | 1.0729--1.3465 | 26 | 7 / 1 |
| Monkeys | 1.5232--1.5473 | 1.9624--1.9899 | 2.1455--2.1841 | 97 | 41 / 2 |

### Near-rock crown and facet-light follow-up

The existing seven near-rock rings now receive bounded upper-ring height offsets and the existing
cap is raised and shifted per variant. Vertex and index counts remain 110 and 192 triangles for
each shared near variant; far rocks, contact disks, placements, instances, buffers, LOD distances,
collision proxies, and submissions are unchanged. The near-only material branch additionally
derives a face normal from position derivatives and applies restrained two-sided light from the
shared fixed sun. This adds fragment work only to near-rock pixels and its visible contribution
uses the existing full-strength-to-58 m, faded-to-90 m detail envelope, with no normal stream,
texture, varying, or additional draw.

The exact Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages and requires the new diagnostic
in lifecycle, overview, and reef gates. Its original-resolution reef frame was inspected and shows
the broken crown line and readable planes without changing the calm distant silhouettes. A current
x86_64 KVM/gfxstream GLES 3 fixed reef run required the same platform-log diagnostic, reported
`OPENGLES3` at a 1280 by 720 logical viewport, and produced an inspected matching 2400 by 1080
frame without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. The arm64 package builds and
lints. Isolated native timing and physical-mobile fragment cost remain open.

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

### Transform-derived jungle foliage tint

The jungle vertex shader now recombines four existing scale/yaw matrix coefficients into one
stable per-instance seed, then applies a bounded green-only colour multiplier. Near/far LODs share
the same transform, so the tint needs no stored identity and cannot add an LOD colour discontinuity.
Population, placement, geometry, instance stride, buffers, textures, culling, simulation, and draw
submissions are unchanged. The retained 1280 by 720 Chrome/WebGL 2 before/after jungle, ground, and
east-clearing pairs were compared at original resolution; the change breaks up repeated greens
without tinting brown trunks or altering composition. The complete Chrome suite passes all 22
stages, Firefox ESR passes its production first-frame/input/audio subset plus dedicated jungle and
near-surface-water live-scene gates and a short hidden/visible tab-resume check. A current
KVM/gfxstream GLES 3 jungle launch also compiles the GLSL ES 300 path and requires its exact
platform diagnostic. These are functional results; isolated vertex cost on a physical mobile GPU
remains unmeasured.

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

A later original-resolution pair showed that the east room remained too close to surrounding
forest soil. Its existing blend now leans further toward warm olive and reuses both already-
computed broad and medium fields for restrained mottling. A fixed 500 by 260 review crop moved from
normalized mean RGB 0.2482/0.3774/0.1687 to 0.2794/0.3817/0.1654. The mask, noise evaluations,
geometry, textures, instances, collision, and submissions are unchanged; the expression adds only
one scalar multiply and add over the preceding material. The complete Chrome suite passes all 22
stages and the arm64 package builds/lints, but physical-mobile appearance and fragment cost remain
open.

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

### East-clearing fallen-log landmark follow-up

The east clearing now adds one 7.1 m fallen-log instance to its outer ring, using an existing log
mesh, material, cell, LOD, culling, instancing, contact-shadow, and collision path. It adds seven
sampled collision proxies but no mesh, texture, shader, allocation class, render pass, or draw
submission. The deterministic world now contains 13 fallen logs and 827 static proxies in 482
occupied 16 m cells; core invariants prove that both authored log footprints remain inside their
respective clearings and outside the central walking rooms.

An initial actual WebGL 2 frame placed the new log broadside to the camera and read as an overly
straight horizontal beam. Its yaw was changed from the tangential first pass to 2.55 radians,
revealing the cut end and depth while preserving clearance. The final Chrome/SwiftShader suite
passes all 22 stages and its original-resolution east/west frames were inspected. The arm64 GLES 3
package builds and lints; it is 18,712,004 bytes with a 15,901,720-byte stripped library and a
valid APK v2 signature. Physical-device appearance and cost remain unqualified.

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

### Authored cove driftwood landmark follow-up

Three non-solid driftwood instances now form a spaced cove arc using the existing three mesh,
material, LOD, culling, and variant-batch paths. No mesh, texture, collision proxy, shader branch,
or new submission type was added. The deterministic QA camera moved from one close variant to the
three-piece centroid, so its visible population and historical timings are not like-for-like.

Core coverage locks all three variants, their dry-beach footprints, route clearance, trunk
clearance, and 2.4 m minimum separation. Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2
captures agree on the group composition. Physical-Android appearance and representative timing
remain open; no frame-time claim is made from this composition change.

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

### Lightweight middle grass LOD follow-up

The three-tier path inserts a ten-blade, three-control-level, 40-triangle middle cluster between
the 224-triangle near and 20-triangle far meshes. Adjacent-tier population splits cover 62--78 m
and 86--102 m; no cell activates all three tiers or mixes near directly with far. The close view
uses 21 draws for 15 cells and renders 957 near, 637 middle, and 1,233 far clusters. This totals
264,508 visible grass triangles, about 19 percent below the preceding two-tier view's 325,412.

A first 72-triangle middle prototype was rejected after two complete suites repeatedly raised the
moving traversal mean by 12--18 percent. Two suites of the accepted 40-triangle mesh produced the
ranges below while the operator game shared the GPU. The broad variation, especially on the beach,
prevents an isolated speedup claim. Both traversal runs did avoid the rejected regression and
improved over the preceding 2.8983 ms mean as representative visible grass geometry fell by about
32 percent. Hardware-browser and physical-Android budgets remain open.

| View | Mean ms range | p95 ms range | p99 ms range | Submissions | Visible grass |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overview | 1.5472--1.7343 | 1.7151--2.1256 | 2.1110--2.3584 | 77 | 0 |
| Beach | 2.4676--3.0860 | 3.3206--4.3729 | 4.0564--5.6822 | 131 | 2,954 |
| Jungle | 2.3738--2.5596 | 3.1162--3.5006 | 3.8502--4.1885 | 103 | 2,932 |
| Driftwood | 1.3194--1.5304 | 1.6323--1.9888 | 2.0161--2.5426 | 54 | 99 |
| Monkeys | 2.3322--2.3326 | 3.0050--3.0351 | 3.3030--3.5892 | 102 | 2,946 |
| Traversal | 2.4887--2.6431 | 3.1023--3.2107 | 3.4072--3.5939 | 12 final | route-varying |

### Narrow irregular footpath follow-up

The visible worn half-width now varies from 1.52 to 2.16 m instead of 2.05 to 2.77 m. This does
not change the wider vegetation reservation, traversal geometry, collision, terrain mesh, texture
count, or submissions. More of the route edge therefore returns to ordinary green substrate while
the full walking corridor stays clear. The compacted centre shifts slightly with the existing
medium field, and the shaded/sun-dry soil mix reuses the already-computed medium and fine fields.
There is no new noise evaluation in the accepted shader.

A first prototype evaluated a separate four-corner noise field inside the path. It looked nearly
identical and produced noisy traversal means of 2.6301 and 2.9680 ms while the visible operator game
shared the GPU, so it was rejected as needless mobile fragment work. The accepted reused-field
version measured 2.5100 and 2.7317 ms mean, with 3.4736 and 3.6927 ms p95. Those ranges overlap the
preceding three-tier-grass traversal observations and are only a gross-regression guard, not a
speedup claim. Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 path captures were inspected
at 1280 by 720 and retain the same narrower treatment.

### Reused crossing-wave ocean colour follow-up

The ocean now applies its existing two broad crossing-wave values to a shallow-weighted
bright/dark subsurface colour term. The field fades between 135 and 420 m and is attenuated by
Fresnel so it does not add noisy detail to the reflective horizon. The accepted implementation
moves the already-required crossing-wave calculation earlier in the fragment stage; it adds no
sinusoid, texture, vertex, index, submission, CPU update, or allocation. The camera distance was
also already required for fog. Remaining work is bounded arithmetic and one smoothstep.

Six fixed native ocean/wildlife views and the actual Chrome/SwiftShader near-surface/current views
were inspected at 1280 by 720. Two weaker colour amplitudes were rejected because their static
result was almost imperceptible. The accepted restrained setting is clearest in motion and leaves
the distant horizon calm. One shared-GPU six-view safety run produced the results below. A second
ocean-dominant overview measured 1.2183 ms mean / 1.5122 ms p95, closely matching the first run;
these observations guard against a gross regression but are not an isolated before/after claim.

| View | Mean ms | p95 ms | p99 ms | Submissions |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.2114 | 1.4792 | 1.8500 | 77 |
| Beach | 2.1186 | 2.8734 | 3.2868 | 131 |
| Jungle | 1.7005 | 2.1999 | 2.6554 | 103 |
| Driftwood | 1.0604 | 1.3832 | 1.7685 | 54 |
| Monkeys | 1.8462 | 2.6229 | 3.2287 | 102 |
| Traversal | 2.3070 | 3.4943 | 4.1055 | 12 final |

### Layered near-bird flight-feather follow-up

Each near bird adds one double-sided trailing-feather triangle behind each existing wing, growing
from 42 vertices/14 triangles to 54 vertices/18 triangles. The renderer had already reserved 54
vertices per agent, so the maximum dynamic buffer, staging reservation, population, CPU update,
culling, 255 m near-LOD threshold, 12-vertex/4-triangle far silhouette, allocation pattern, and
single dynamic draw remain unchanged. In the overview's four-near-bird case this is 48 additional
streamed vertices and 16 triangles; the absolute 24-agent worst case is 288 vertices and 96
triangles.

The exact Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages, requires the trailing-feather
diagnostic in lifecycle and bird gates, and its closer original-resolution bird frame was inspected.
The matching OpenGL 3.3 frame was inspected and the complete isolated-X11 suite passes 41/41. A
current x86_64 KVM/gfxstream GLES 3 fixed-bird run required the same platform-log diagnostic,
reported `OPENGLES3` at a 1280 by 720 logical viewport, and produced an inspected matching 2400 by
1080 frame without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. The arm64 package builds
and lints. These are functional and visual checks; isolated native timing and physical-mobile
animation cost remain open.

### Single-surface and CPU-faceted fish follow-up

`FishRenderer` already sets `RasterizerState::CullNone`, so every thin fin and tail triangle is
visible from either side. The former explicit reverse-winding copy was exactly coincident and
therefore redundant. Emitting each thin surface once reduces the near fish from 66 to 51 streamed
vertices (22 to 17 triangles) and the far fish from 24 to 12 vertices (eight to four triangles).
Worst-case dynamic-buffer capacity falls from 3,168 to 2,448 vertices without changing fish count,
LOD range, simulation, culling, or the one-draw batch.

The closed body faces now receive fixed-sun colour on the CPU before the existing dynamic upload,
giving the small low-poly forms readable volume without adding a normal attribute, shader, texture,
effect, draw, or GPU fragment work. This adds bounded cross/normalize/colour work for eight near or
two far body faces per visible fish. The dedicated camera has six near fish, so it streams 306
rather than 396 vertices. Two shared-GPU fish-view checks measured 1.6764--1.7648 ms mean and
2.1861--2.3527 ms p95. These are functional cost checks, not a before/after speedup claim.

### Single-surface and CPU-faceted shark follow-up

The shark uses the same cull-none `BasicEffect` contract as the fish. Removing exact coincident
reverse-winding fin/tail triangles reduces its near geometry from 66 to 51 streamed vertices
(22 to 17 triangles) and its far geometry from 30 to 15 vertices (ten to five triangles). Fixed-sun
CPU face colours reuse the shared wildlife helper and add no attribute, texture, effect, or draw.

The dedicated view retains one visible shark and one shark draw within 85 tracked submissions.
Two shared-GPU debug-build safety checks measured 1.4463--1.4549 ms mean,
1.9150--2.0607 ms p95, and 2.2383--2.3108 ms p99. These checks reject a gross regression but do
not establish an isolated before/after speedup.

### Close sand granulation and shoreline-raked ripple follow-up

The portable terrain fragment shader evaluates one extra four-corner value-noise sample only on
close sand and uses it for restrained warm-channel granulation. The ripple term reuses the existing
broad/medium fields and authoritative signed shore distance; it fades by 92 m and stops 14 m inland,
so it cannot become a distant full-island stripe pattern. The treatment adds no texture, vertex,
index, instance, allocation, collision rule, or draw submission.

Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 driftwood and waterline captures were
inspected at 1280 by 720 and agree on the retained subtle breakup. Two isolated private-display
driftwood runs retained the same 54 tracked submissions and measured 1.0062--1.0528 ms mean,
1.2434--1.4131 ms p95, and 1.5616--1.8055 ms p99 across 600 samples. Those ranges overlap the
immediately preceding 1.0604/1.3832/1.7685 ms integrated shore-detail observation; they guard
against a gross regression rather than proving an isolated shader cost. Physical-Android and
hardware-browser fragment measurements remain open.

### Sparse dry-beach substrate follow-up

Two deterministic elliptical-fleck grids now run only for visible dry-sand fragments within 58 m.
The fragment shader uses a fixed soft edge, then blends one restrained dark-pebble and one
lower-density pale-shell population. An explicit dry-substrate branch skips both grid hashes on
wet, inland, and more distant fragments. The pass adds no derivative evaluation, texture sample,
geometry, instance, allocation, collision rule, or draw submission.

Native OpenGL 3.3 driftwood and fallen-frond captures and actual Chrome/SwiftShader WebGL 2
captures agree on the sparse scale and distribution. The complete browser harness passes all 22
stages and requires the exact feature diagnostic. Representative native, hardware-browser, and
physical-Android timing deltas remain open; no speedup or mobile cost claim is made.

### Irregular faceted near-palm root follow-up

Each near palm replaces five single-triangle contact tabs with five closed ridge wedges whose
angle, length, width, and height vary deterministically by root and palm variant. This raises a
near variant from 469--541 to 494--566 triangles: 25 additional triangles per near palm, with no
change to the far mesh, instance record, population, LOD threshold, shader, texture, allocation,
culling rule, collision proxy, or existing four-variant near draw submissions.

Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 close-palm captures agree on the retained
grounded bases. Two isolated 600-sample palm-view runs each rendered 47 near and 246 far palms in
111 tracked scene submissions and measured 1.6128--1.6437 ms mean, 2.1310--2.2256 ms p95, and
2.5902--2.8770 ms p99. These are gross-regression guards rather than an isolated geometry-cost
claim. Physical-Android appearance and vertex cost remain open.

### Near-grass silhouette variation follow-up

The near cluster grows from 28 to 34 blades while retaining five control levels. Five blades form
a shorter, narrower juvenile underlayer; the other 29 retain the mature profile. The existing
per-instance colour value also seeds bounded per-blade stature, colour, and wind-amplitude offsets
in the portable vertex shader. This adds no instance field, layout population, texture, allocation,
culling rule, LOD distance, or draw submission. Middle and far geometry remain unchanged.

The dedicated view renders 957 near, 637 middle, and 1,233 far clusters in 21 grass submissions.
The near mesh rises from 224 to 272 triangles, increasing this view's visible grass geometry from
264,508 to 310,444 triangles, or 17.4 percent. Two isolated 600-sample debug-build checks retained
98 total tracked scene submissions and measured 6.4599--7.2119 ms mean, 8.2017--8.8514 ms p95,
and 9.1263--10.4740 ms p99. Actual OpenGL 3.3 and Chrome/SwiftShader captures were inspected and
match; a KVM/gfxstream GLES 3 run reached a clean first frame and visibly retained the grass layer.
These are gross-regression and portability checks, not an isolated speed comparison or a
physical-mobile density budget.

### Sparse near-grass seed-head follow-up

Four of the 29 mature blades in the shared near cluster now carry a narrow stem and six small
single-triangle spikelets. The addition is deliberately absent from the middle and far tiers. It
adds 88 vertices and 32 triangles to the one shared near mesh, taking it from 340 vertices and 272
triangles to 428 vertices and 304 triangles. It adds no texture, transparency, instance attribute,
buffer, population, runtime allocation, culling change, or draw submission; the new vertices reuse
the existing packed blade-facing value and wind data.

The deterministic close-grass view still renders 957 near, 637 middle, and 1,233 far clusters in
21 grass submissions. Its visible grass geometry rises from 310,444 to 341,068 triangles, a 9.86
percent increase across the full visible grass population. The OpenGL 3.3 capture was inspected
and the complete isolated-X11 suite passes 41/41. The exact Chrome 152/SwiftShader WebGL 2 suite
passes all 22 stages, requires the seed-head diagnostic, and its original-resolution capture was
inspected. A current x86_64 KVM/gfxstream GLES 3 run required the same platform-log diagnostic and
its inspected 2400 by 1080 capture retained the sparse accents without a shader, fatal, or
`GL_INVALID_OPERATION` report. The arm64 package builds and lints. These are functional
portability and visual checks; physical-mobile density cost, sustained thermals, and isolated
timing remain open.

### Near-monkey fur-silhouette follow-up

Three small tetrahedral tufts in the near body mesh break up the crown and cheek outline while
remaining rigidly attached to the existing head bone. Each shared near variant gains 27 vertices
and nine triangles; the fixed monkey camera currently draws two near agents and one far agent, so
the scene adds 54 transformed vertices and 18 triangles without another mesh, material, draw,
texture, allocation, animation palette, or far-LOD change.

The complete Chrome/SwiftShader 22-stage suite passes and the inspected fixed frame shows a
restrained rather than spiky outline. The revised arm64 GLES 3 package builds and passes app lint.
No isolated benchmark or physical-mobile run was performed, so this is a bounded geometry account
and portability check rather than a measured zero-cost claim.

### Near-monkey finger and toe silhouette follow-up

Each shared near body variant now adds three short rounded fingers per hand and three toes per
foot, weighted to the existing forearm and shin bones. The addition is 216 vertices and 240
triangles in the selected near body mesh. It adds no bone, animation palette, material, texture,
allocation, draw submission, or far-LOD geometry; the fixed monkey view still renders two near
agents and one far agent through the established main/face/eye paths.

An initial WebGL 2 capture exposed fingers that read as overly long thin claws, so their reach was
reduced and cross-section widened before acceptance. The final complete Chrome/SwiftShader suite
passes all 22 stages, requires the exact diagnostic in lifecycle and monkey checks, and its
original-resolution fixed frame shows a restrained bone-following silhouette. The OpenGL 3.3
objects compile without relinking the visible game. The arm64 GLES 3 package builds/lints, while
physical-device appearance and animation cost remain unqualified.

### Transient keyboard locomotion confirmation

A keyboard Shift edge now reuses the already resident generated debug font and one-pixel texture
for a small premultiplied `RUN MODE` or `WALK MODE` panel. It exists for 1.4 seconds, fades during
its final 0.32 seconds, and submits nothing at steady state. The Android analog stick keeps its
direct hold-style threshold and therefore does not repeatedly create the message near its
hysteresis boundary.

The complete Chrome/SwiftShader suite proves the persistent 8.0 m/s selection after key release,
the second-edge 4.3 m/s restore, and writes a visibly correct warm-gold confirmation screenshot.
The arm64 GLES 3 package builds and lints. No isolated transient `SpriteBatch` timing was measured;
the screenshot's debug-panel frame time is not a benchmark.

### Logical backbuffer qualification

The renderer identity alone was insufficient for interpreting mobile cost because Android's
physical screenshot surface is 2400 by 1080 while the game requests a 1280 by 720 backbuffer. The
first rendered frame now emits the actual `GraphicsDevice` viewport to standard output and the
Android platform log, and the opt-in F1 row displays the same value. This adds no normal-scene draw
or per-frame log work; only the already enabled debug panel gains text.

Chrome/WebGL 2 and the API 35 KVM/host-GPU GLES 3 run both reported a 1280 by 720 logical
backbuffer. Android scales that image into the centred 1920 by 1080 presentation region of the
2400 by 1080 surface. The inspected fixed-monkey frame showed 31.5 FPS / 29.1 ms with the full F1
panel active, while its first draw completed in 1439.657 ms total. Those values are a diagnostic
snapshot, not a sustained benchmark or physical-device budget. They do establish that future
mobile density decisions must be based on 921,600 shaded logical pixels rather than the physical
surface dimensions. Both Web and Android smoke gates now reject a missing viewport diagnostic or
any value other than 1280 by 720.

### Sequential procedural-audio staging

The first audio implementation generated all sixteen sources, retained their complete 3,336,164
PCM bytes, converted all sixteen into another 3,336,164 bytes, and only then constructed CNA audio
objects. Those source and conversion vectors remained alive together even though the backend
copies each submitted buffer. The revised path generates, converts, uploads, and releases one
source before beginning the next. The exact source PCM, effects, four persistent instances, twelve
one-shot effects, and six-voice runtime ceiling are unchanged.

This removes 6,672,328 bytes of simultaneously retained whole-set game staging. The largest
remaining source-plus-conversion pair is the 29-second jungle bed at 2,557,800 bytes, a reduction
of at least 4,114,528 bytes (about 3.92 MiB) in that specific staging component. These figures do
not include the generator's short-lived filtering arrays or backend-owned decoded audio and are
therefore not a claim about total process RSS. The complete Chrome/SwiftShader 22-stage suite
passes and requires the new post-gesture staging diagnostic, proving that all sources still upload
and start through Web Audio. The arm64 GLES 3 package builds and lints; native playback and Android
runtime memory are not newly claimed while the operator-visible Linux build remains active.

### Persistent-loop DC centering

The four persistent procedural beds now subtract their generated floating-point mean before
deriving peak normalization and PCM16 samples. This is one additional bounded read pass per loop
during startup; it allocates no buffer, adds no PCM byte, source, voice, or per-frame work, and
leaves the twelve silent-ended one-shots unchanged. The measured normalized loop means fall from as
much as 0.00646980 to at most approximately 0.00000002, while peak, RMS, seam, and zero-crossing
checks remain valid.

Core/audio/WAV tests pass 3/3, the complete Chrome/SwiftShader suite passes all 22 stages and
requires the post-gesture DC-centering diagnostic, and the arm64 GLES 3 package builds and lints.
A current x86_64 KVM/gfxstream run additionally passes the strict GLES 3/audio gate with the same
diagnostic mirrored through Android's platform log. The change is a numerical/source-quality
improvement, not a substitute for the open subjective headphone and speaker fatigue pass.

### Distance-scheduled land footsteps

Four 0.3-second PCM16 clips add 52,920 bytes to generated source memory and no packaged binary
asset. The scheduler performs one finite check, horizontal distance calculation, and small amount
of scalar cadence logic per audio update; it searches no world population and allocates nothing.
Only one reusable footstep instance can be live. Existing priority rules keep the complete mix at
four persistent sources plus at most two transient sources, so the maximum mixer voice count stays
six. Blocked movement and teleports do not fabricate cadence, and footsteps stop while swimming.
Core/audio/WAV tests, the complete 22-stage Chrome/WebGL 2 suite, and an actual x86_64 KVM/GLES 3
touch-traversal smoke test pass. Subjective material identity and balance remain open.

### Generated texture and transparency audit

The game currently owns six generated `Color`/RGBA8 textures and no imported texture assets. The
debug font atlas is 96 by 32, the touch HUD uses two 96 by 96 circles, the conditional swimming
view uses one 160 by 90 veil, and the debug/monkey renderers each own one white pixel. CNA's
two-dimension texture constructor creates one base level, so the exact upload inventory is 35,906
pixels or 143,624 bytes. `GeneratedTextureBudget` is shared by the generators and core tests; a
compile-time assertion requires this audit to be updated if the dimensions change. This is an
RGBA8 base-level payload figure, not total driver allocation or process RSS. At roughly 140.3 KiB,
smaller Web/mobile texture variants or compression would add complexity without a meaningful
memory/download benefit.

The associated overdraw audit found no transparent billboard vegetation: grass, palms, jungle,
props, terrain, sky, and wildlife are opaque meshes. The required transparent world passes are the
depth-tested ocean; one 8 m-wide, 4,608-triangle shoreline ribbon; and a single near-field contact
shadow batch already protected by frustum and distance culling. The full-screen water veil draws
only while its smoothed intensity is nonzero. Touch UI is conditional, the debug panel is opt-in,
and the run/walk notice lasts 1.4 seconds. This bounds the known transparent layers, but it is not
a physical-mobile fill-rate measurement; that remains part of device profiling.

### Dry-wrack beach-detail follow-up

A visual review of the fixed cove frame found that the four beach-detail variants were all similar
shell/pebble compositions. Variant three now uses four low curved opaque strands plus two small
holdfast forms in muted olive/brown colours. The replacement mesh is 80 vertices and 68 triangles
instead of 64 vertices and 56 triangles; placement, the 265-instance population, 52 m culling
distance, four variant buffers, and maximum four beach-detail submissions are unchanged. It adds
no texture, transparency, collision, simulation, or per-frame procedural work.

The complete actual Chrome/SwiftShader WebGL 2 suite compiles the shared shader, requires the new
startup diagnostic, passes all 22 stages, and produces an inspected cove frame. The OpenGL 3.3
renderer object compiles without relinking the visible game. The arm64 GLES 3 package builds and
lints, but the revised detail has not been inspected on a physical mobile GPU.

### Fallen-frond silhouette follow-up

The three existing frond meshes remain exactly 132 vertices and 74 triangles each. Leaflet roots
now follow the already curved rachis, and bounded deterministic pair/side variation changes their
span, forward sweep, tip droop, and vertex-colour ageing. Population (63), three variant buffers,
76 m culling, opaque material, instance data, maximum three submissions, texture memory, collision,
and per-frame work are unchanged.

The complete isolated OpenGL 3.3 suite passes 41/41 and the before/after close frame was inspected
at its original 1280 by 720 resolution. The actual Chrome 152/SwiftShader WebGL 2 suite passes all
22 stages, requires the new diagnostic in its frond gate, and its original-resolution frame matches
the native silhouette. The arm64 GLES 3 APK builds/lints through the shared cache, contains only
`arm64-v8a`, and verifies with APK signature scheme v2; a physical-mobile frond inspection remains
open, so no Android appearance or performance improvement is claimed.

### Broad clearing-floor patch breakup follow-up

The east and west clearing floors now classify complementary dry/cool and leaf/moss patches from
the broad, medium, and fine terrain fields already evaluated for their masks, edges, and local
material variation. Four bounded colour blends add no texture lookup, noise evaluation, varying,
geometry, allocation, or draw submission. Two deliberately weaker candidates were inspected and
rejected before retaining a readable but non-decal-like blend.

The final actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages and requires the exact
patch-path diagnostic in lifecycle and both clearing gates. Original-resolution east/west frames
were inspected after each tuning pass. The OpenGL 3.3 consumer object compiles with first-party
warnings as errors without relinking the operator-visible game. A current x86_64 KVM/gfxstream
GLES 3 fixed east-clearing launch passed the same diagnostic gate; its inspected 2400 by 1080 frame
retains a softly varied floor without a hard decal. It completed the first draw in 1594.265 ms
total and then presented at roughly 22--26 ms in the sampled emulator log. Those translated-host
figures are functional evidence, not a physical-device fragment benchmark. The restored arm64
GLES 3 package builds, passes lint, contains only `arm64-v8a`, and verifies with APK signature
scheme v2. Native runtime appearance, physical-mobile appearance, and isolated fragment cost
remain open.

### Inland forest-floor material breakup

The terrain fragment path now reuses its already-evaluated broad, medium, and fine fields to warm-
desaturate the deeper grass substrate and classify restrained leaf-humus and cool-moss patches.
Slope, clearing, signed-shore-distance, and existing camera-detail masks bound the work. It adds no
noise evaluation, texture lookup, varying, geometry, allocation, or draw submission. Native
OpenGL 3.3 and actual Chrome plus Firefox WebGL 2 jungle frames were inspected at original
resolution; the Chrome 27-stage matrix and independent Firefox gate require the exact path marker.
An API 35 x86_64 KVM/gfxstream GLES 3 run passed the same marker and produced an inspected clean
2400 by 1080 fixed-jungle frame.

One isolated Release OpenGL 3.3 fixed-jungle run recorded 2.8044 ms mean, 2.0553 ms median,
7.8039 ms p95, 12.4736 ms p99, and 24.0792 ms maximum across 600 intervals. Twenty-five exceeded
8.333 ms, one exceeded 16.667 ms, and none exceeded 33.333 ms; the fixed composition retained 89
tracked submissions. This is a current integrated scene baseline, not an isolated shader-cost or
before/after attribution. Physical-mobile fragment cost remains open.

### Transform-derived palm-frond tint follow-up

The palm vertex shader now hashes four scale/yaw coefficients already present in every instance
transform into a stable signed colour offset. A green-dominance mask keeps the bounded tint on
fronds and leaves trunks, roots, and coconuts unchanged. Near and far meshes consume the same
transform, so LOD changes retain one colour identity. The change adds only vertex arithmetic: no
attribute, instance stride, buffer, texture, geometry, population, culling, collision, upload, or
draw count changes.

Original-resolution before/after close-palm Chrome/WebGL 2 frames were inspected after rejecting a
weaker first tuning. The complete Chrome suite passes 22/22 and an eleventh clean Firefox ESR run
independently compiles the GLSL ES 300 path. A fixed-palm x86_64 KVM/gfxstream GLES 3 launch passed
the same platform-log marker and audio gate; its inspected frame retains brown structural material
and varied crowns. This qualifies portable function, not sustained or physical-mobile cost. The
OpenGL 3.3 object compiles with warnings as errors, but its updated runtime appearance awaits
relinking after the operator-visible prior executable closes.

### Fallen-log broken-end contour follow-up

The three near fallen-log variants now reuse their existing nine-sided end rings and duplicated
cap-rim vertices at matching radii. Bounded variant/end/side phases move those vertices axially,
turning the formerly planar circular saw cuts into spatially closed irregular broken contours.
The far meshes remain unchanged. This adds no vertex, index, material selector, instance field,
buffer, texture, collision proxy, upload, culling rule, population, or draw submission.

Original-resolution before/after Chrome/WebGL 2 prop frames were inspected; the retained result
breaks the artificial cylinder silhouette while preserving readable growth rings and contact
shadows. The complete Chrome suite passes 22/22, and the twelfth clean Firefox ESR run requires
the marker on its independently compiled GLSL ES 300 path. A fixed-prop x86_64 KVM/gfxstream GLES
3 launch passed the same marker and strict audio gate; its inspected frame visibly matches the
irregular contour and reached first draw in 1567.410 ms total. These are portable functional and
composition checks, not physical-mobile cost evidence. The OpenGL 3.3 object compiles with
warnings as errors but awaits runtime relinking after the operator-visible prior process closes.

### Reused-slope grazing-reflection follow-up

The low-angle ocean review found that broad calm water remained visually uniform after its
directional sunward crest pass. The accepted fragment path projects the already-computed analytic
slope toward the horizontal eye direction, smoothly saturates the signed response, and applies a
bounded cyan variation only at grazing angles. It reuses the existing 135--420 m wave-detail fade
and favours shallow water. The change adds one planar normalization, one dot product, a smooth step,
and bounded scalar colour shaping. It adds no sine, wave field, texture lookup, varying, uniform,
geometry, allocation, resource, CPU update, or draw submission.

Original-resolution offshore-current, near-surface-water, and reef frames were compared before and
after the retained tuning; a weaker first amplitude was rejected. The complete Chrome/WebGL 2
suite passes 22/22, a thirteenth clean Firefox ESR run independently passes its production/input/
audio/tab-resume/jungle/water gates, and a fixed-current x86_64 KVM/gfxstream GLES 3 launch passed
the exact marker plus strict audio gate. The Android first draw completed in 1677.445 ms total, but
that one translated-host startup figure is functional evidence rather than a sustained fragment-
cost or physical-device result. The OpenGL 3.3 object compiles with warnings as errors; current
native appearance awaits relinking after the operator-visible prior process closes.

### Transform-derived rock-tint follow-up

The current clearing and reef review found that repeated copies of each shared rock variant still
used one literal vertex palette. The natural-prop vertex shader now identifies neutral stone-family
colours by their existing relative RGB channel span and hashes four scale/yaw coefficients already present
in every instance transform. A bounded signed tint shifts red/blue in opposite directions with a
smaller green change, producing restrained warm/cool variation. The same transform preserves tint
identity across near/far LOD, while red-dominant wood and green foliage remain outside the mask.
This adds only vertex arithmetic: no attribute, instance stride, buffer, texture, geometry,
population, collision proxy, upload, culling rule, or draw submission changes.

Original-resolution before/after reef and prop frames were inspected; the accepted result adds a
subtle warm/olive rock identity without contaminating the fixed log view. The complete Chrome
suite passes 22/22 with exact lifecycle/overview/prop/reef gates, and a fifteenth clean Firefox
ESR run independently compiles the path while retaining its production input/audio/tab-resume/
jungle/water checks. A fixed-reef x86_64 KVM/gfxstream GLES 3 launch passed the same marker and
strict audio gate; its inspected frame matches the WebGL result and reached first draw in
1604.367 ms total. These checks qualify portable function and composition, not sustained or
physical-mobile GPU cost. The OpenGL 3.3 object compiles with warnings as errors; current native
appearance awaits relinking after the operator-visible prior process closes.

### Staggered near-palm leaflet follow-up

The current close-palm review found that the fuller nine-station near fronds still formed visibly
regular opposed combs. Their existing vertices now use deterministic frond/station/side phases to
vary leaflet length, longitudinal stagger, forward sweep, and droop. Increasing the maximum
leaflet span from 0.74 to 0.84 local metres brings the near crown envelope into closer agreement
with the unchanged widened far ribbon. The four near meshes remain 494--566 triangles and the far
meshes remain 102 triangles; no vertex, index, instance, buffer, texture, shader operation, wind
update, LOD threshold, collision proxy, upload, or draw submission was added.

The OpenGL 3.3 object compiles with first-party warnings as errors without relinking the
operator-visible prior executable. The complete Chrome 152/SwiftShader WebGL 2 matrix passes
23/23, requires the exact asymmetry marker in lifecycle and the palm gate, and its inspected
original-resolution frame retains separate tapered leaflets with a fuller, less mechanical crown.
The arm64 GLES 3 APK builds/lints with zero issues, contains only `arm64-v8a`, and verifies with APK
signature scheme v2. The later batched fixed-palm GLES 3 capture now qualifies the close
appearance; moving LOD-transition review and physical-mobile cost remain open.

### Near-palm fixed-sun facet-lighting follow-up

The same close-palm review found that the revised silhouette remained flatter than the grass,
jungle, and natural props around it. The existing near-only material-detail branch now reconstructs
a two-sided face normal from world-position derivatives and applies a bounded 0.88--1.08 fixed-sun
diffuse/upward-fill multiplier before its bark-scar term. Far palms skip the derivative path. This
adds one uniform and near-fragment arithmetic but no normal attribute, geometry, texture, instance
data, buffer, upload, CPU update, LOD threshold, collision proxy, or draw submission.

The OpenGL 3.3 object compiles with warnings as errors without relinking the operator-visible prior
executable. The complete Chrome 152/SwiftShader WebGL 2 matrix passes 23/23 and requires the exact
facet-shading marker in lifecycle and the dedicated palm gate; its inspected original-resolution
frame retains restrained crown colour and more legible trunk/frond planes without blown highlights.
The arm64 GLES 3 package builds/lints, contains only `arm64-v8a`, and verifies with signature scheme
v2. A later batched GLES view and physical-device fragment measurement remain required.

### Folded fallen-frond leaflet follow-up

The current dedicated frond view showed that the curved/asymmetric outline still left every
leaflet reading as one flat card. Each existing four-vertex/two-triangle leaflet already shared a
root-to-tip diagonal, so raising that edge by only 4--5 local millimetres creates a shallow
geometric fold without adding geometry. A private alpha material selector enables a bounded
0.86--1.06 two-sided fixed-sun facet multiplier for the 63 fronds; beach details remain on their
plain path. Every variant stays exactly 132 vertices/74 triangles, with unchanged population,
instance buffers, placement, 76 m culling, collision policy, and maximum three submissions.

The OpenGL 3.3 object compiles with first-party warnings as errors without relinking the visible
prior executable. The complete Chrome 152/SwiftShader WebGL 2 matrix passes 23/23, requires the
fold marker in its frond gate, and the inspected original-resolution frame retains a subtle organic
crease rather than a rigid ridge. The arm64 GLES 3 package builds/lints, contains only `arm64-v8a`,
and verifies with signature scheme v2. The later batched fixed-frond GLES 3 capture now qualifies
appearance; physical-mobile fragment cost remains open.

### Restrained fallen-log end-grain crack follow-up

The latest close-prop audit found that the irregular near-log contour and softened growth rings
still left the exposed end unusually pristine. Three thin directional masks now form slightly
curved radial drying cracks in the existing cut-end fragment branch. Their response fades near the
centre, strengthens gradually toward the perimeter, and darkens by at most 8.5 percent. This avoids
the painted-star appearance rejected during the earlier ring pass.

The change adds fragment arithmetic only: geometry, vertex/index counts, material selectors,
instance data, buffers, textures, uploads, culling, collision, population, and submissions remain
unchanged. The complete Chrome 152/SwiftShader WebGL 2 matrix passes 23/23 and requires the exact
marker in lifecycle and prop gates. Its original-resolution close-log frame and an unfiltered
detail crop were inspected; the cracks remain subordinate to the rings and have no centre seam.
Firefox ESR 140.0 independently passes its first-frame, lifecycle, input, audio, jungle, and water
gate while requiring the same marker. The OpenGL 3.3 object compiles with warnings as errors,
while the default arm64 GLES 3 package
builds/lints, contains only `arm64-v8a`, and verifies with signature scheme v2. Android runtime
appearance is qualified by the later batched fixed-prop capture; physical-mobile cost remains open.

### Sparse near-canopy liana follow-up

The latest interior critique found an underused vertical layer between broadleaf crowns and the
otherwise dense ground vegetation. Near canopy variants one and three now each append one hanging
liana: a six-section tapered three-sided strand plus two small folded pointed leaves. A shallow
outward curve and a larger lower wind weight prevent a rigid plumb-line silhouette. Each selected
shared mesh adds 31 vertices and 48 triangles; the other two near variants and every 208-triangle
far mesh remain unchanged.

The geometry is shared by existing instances, stays opaque, is non-solid, and uses the existing
jungle wind/material buffer. It adds no population, placement, texture, instance field, buffer,
upload cadence, culling rule, LOD threshold, collision proxy, shader operation, or draw submission.
The complete Chrome 152/SwiftShader matrix passes 23/23; its original-resolution jungle frame and
an unfiltered detail crop show sparse readable strands without dominating the trail composition.
Firefox ESR 140.0 independently passes its first-frame, lifecycle, input/audio, jungle, and water
gate while requiring the same liana marker. The OpenGL 3.3 object compiles with warnings as errors,
and the default arm64 GLES 3 package builds/lints, contains only `arm64-v8a`, and verifies with
signature scheme v2.

### Batched Android natural-detail qualification

One temporary x86_64 build qualified the accumulated palm leaflet asymmetry, palm near-facet
lighting, folded fallen-frond leaflets, radial log-end cracks, and hanging canopy lianas rather
than repeatedly recreating approximately 1.1 GiB of emulator intermediates. Fixed palm, frond,
prop, and jungle runs used the API 35 `Medium_Phone` AVD with KVM 12 and host AMD Radeon 780M
gfxstream. They completed first draw in 1581.837, 1589.120, 1548.131, and 1571.279 ms total.

Every strict gate required all five exact markers, reported `OPENGLES3`, opened a running 44.1 kHz
stereo AAudio stream, and retained the 1280 by 720 logical backbuffer. The four original 2400 by
1080 captures were inspected and visibly match the corresponding WebGL 2 compositions without a
fatal, shader, or `GL_INVALID_OPERATION` diagnostic. This qualifies translated-host function and
composition, not sustained frame time, thermals, audible quality, or physical-mobile GPU cost.
Afterward the emulator was stopped, the 18,750,124-byte arm64-only APK and its 15,939,840-byte
stripped `libmain.so` were restored, linted, ABI/signature-checked, and the reproducible x86_64
intermediates were removed. The compact logs and screenshots remain as qualification evidence.

### Android mobile quality default

`RuntimeQualityProfile` now makes the Android default explicit and testable instead of relying on
the full desktop distances by accident. It preserves all populations, simulations, shaders,
42-blade near-grass geometry, the 143,624-byte generated texture inventory, and the complete
six-source audio ceiling. It reduces grass maximum range from 135 to 112 m, contact-shadow ranges
to 72%, wildlife maximum draw ranges to 82%, and the ocean grid from 160 to 128 quads per axis.
The latter changes the one ocean mesh from 25,921 vertices/51,200 triangles to 16,641/32,768, a
36% triangle reduction with the same world extent, shader, shoreline ribbon, and interactions.

The full-profile fixed grass view reports 2,827 visible clusters and 27 contact shadows. The
mobile-profile version on the API 35 `Medium_Phone` KVM/gfxstream AVD reports 2,222 clusters and
16 shadows while retaining 957 near clusters, the exact same 42-blade mesh, and clean translucent
touch controls. The overlay-free 2400 by 1080 capture was visually inspected. Its logical
backbuffer remained 1280 by 720, `OPENGLES3` reached first draw in 2703.072 ms, and no fatal,
shader, or `GL_INVALID_OPERATION` diagnostic appeared. These counts demonstrate bounded work;
the timing remains functional translated-host evidence rather than a physical-device performance
or thermal claim.

The runtime rejects invalid profiles and resource inventories exceeding the selected texture or
audio ceiling. Core tests lock both profiles, the 16-bit ocean boundary, and the exact reduced
mesh counts. Native smoke/capture output confirms the full profile, while the complete 27-stage
Chrome WebGL 2 matrix and Android gate require the appropriate full/mobile diagnostic. The
restored arm64-only APK is
19,079,172 bytes with a 16,268,888-byte stripped `libmain.so`; lint reports no issues and APK
signature scheme v2 verifies. The temporary x86_64 intermediates were removed, returning the
Android tree from 2.4 GiB to 1.3 GiB.

### Natural near-grass blade silhouette follow-up

The shared near mesh keeps its 42 blades, 622 vertices, and 496 triangles, but narrows the mature
blade profile and derives a small lateral bow plus unequal left/right edge widths from each stable
blade index. A minority of blades also bends sideways and drops slightly through the upper two
control levels. The middle/far meshes, vertex format, packed wind data, shader arithmetic,
instance population, buffers, culling, LOD transitions, and submissions are unchanged. This is a
silhouette correction rather than a density or runtime-work increase.

Native OpenGL 3.3, Chrome/WebGL 2, Firefox/WebGL 2, and API 35 KVM/gfxstream GLES 3 all required
the exact natural-blade diagnostic. Their fixed close-grass frames were inspected; the mobile
capture retained the complete grass layer and translucent controls without a fatal, shader, or
`GL_INVALID_OPERATION` diagnostic. Its cold first draw completed in 2943.958 ms, which is
translated-host functional evidence rather than a physical-device frame-time result. The restored
arm64-only APK is 19,072,372 bytes with a 16,262,088-byte stripped `libmain.so`; lint passed and
signature scheme v2 verifies.

One isolated Linux Release fixed-grass run retained 2,827 visible clusters, 957 near instances,
21 grass submissions, and 99 total tracked submissions. Across 600 intervals it measured 2.6461
ms mean, 2.1421 ms median, 6.3492 ms p95, 11.7514 ms p99, and 16.8790 ms maximum. Twelve intervals
exceeded 8.333 ms, one exceeded 16.667 ms, and none exceeded 33.333 ms. This is a gross-regression
guard for the complete scene, not isolated attribution to a position-only mesh change.

### Shipwreck landmark baseline

The complete 20.0 m broken ship is one static batch containing 3,948 triangles. Its texture-free
portable shader adds longitudinal grain, broad weathering, restrained cracks, wet/submerged
darkening, salt wear, and a bounded waterline sheen; prelit vertex colours provide the fallback.
The denser cross-section, localized missing boards, split broken keel, rudder, broken rails, hull
openings, and broken foremast preserve one draw, frustum rejection, and the 310 m distance limit.
Six circular collision proxies represent only the major hull sections; the twelve beachward debris
pieces add no collision-query workload and now vary substantially on both sides of the ship axis.

Two isolated Debug OpenGL 3.3 shipwreck runs at 1280 by 720 on the AMD Radeon 780M/Mesa 25.0.7
reported 2.3154 and 2.3745 ms mean, 1.9271 and 1.9153 ms median, 4.6285 and 4.5776 ms p95, and
9.4118 and 10.4471 ms p99. Their maximum intervals were 11.8383 and 10.6526 ms, so neither run had
an interval above 16.667 ms. The fixed view reported 46 tracked submissions: nine visible terrain
cells in three draws, 19 palms, eight canopy trees, 54 grass clusters, six fish, and the shipwreck
in one draw. This is a repeatable landmark-specific Debug baseline, not a Release minimum-spec or
future combined-wildlife worst case.

The refined batch and shader passed the complete Chrome/SwiftShader WebGL 2 matrix, independent
Firefox ESR WebGL 2 scene gate, and an API 35 KVM/gfxstream GLES 3 scene gate. All three retained
one visible wreck draw; the clean 2400 by 1080 Android frame was inspected. The Android result is
useful functional translated-host evidence but not a physical-mobile frame-time, startup,
thermal, colour, or power result.

One isolated stripped-Release OpenGL 3.3 run after refinement recorded 1.9559 ms mean, 1.3566 ms
median, 5.5216 ms p95, 11.6363 ms p99, and 17.4985 ms maximum across 600 intervals. Twenty frames
exceeded 8.333 ms, one exceeded 16.667 ms, and none exceeded 33.333 ms. The view retained 46
tracked submissions and exactly one wreck draw. This is an integrated scene baseline rather than
isolated shader or geometry cost attribution.

The subsequent large-ship scale pass added 556 triangles without adding a draw, buffer, material,
texture, collision proxy, or runtime update. Its current isolated Release run retained the same 46
tracked submissions and one wreck draw. Across 600 intervals it recorded 1.4082 ms mean, 1.1270 ms
median, 1.8133 ms p95, 8.1463 ms p99, and 16.2879 ms maximum; six intervals exceeded 8.333 ms
and none exceeded 16.667 or 33.333 ms. The before/after runs are not an isolated GPU geometry
experiment, but the current view remains comfortably inside the established development-machine
60 Hz p99 gate.

### Beach-crab baseline

Thirty deterministic crabs share the existing observer-relative activation policy and occupy six
bounded sand colonies. Near crabs use faceted shells, articulated bent legs, claws, and eye stalks;
the far tier retains a compact moving silhouette. All visible agents are rebuilt into one dynamic
`VertexPositionColor` batch, so the layer contributes exactly one tracked submission when present
and none when culled. No texture, custom shader, or audio source is added.

One isolated Debug OpenGL 3.3 run of the 1280 by 720 crab view on the AMD Radeon 780M/Mesa 25.0.7
reported 2.4998 ms mean, 2.0381 ms median, 4.8406 ms p95, 11.0887 ms p99, and 13.4104 ms maximum.
Twelve of 600 intervals exceeded 8.333 ms; none exceeded 16.667 or 33.333 ms. The fixed view
reported 26 tracked submissions and five visible crabs in one draw. This is a first integrated
Debug baseline, not an isolated cost attribution or a physical-mobile budget.

The dedicated composition also passed Chrome/SwiftShader and Firefox ESR WebGL 2 plus the strict
API 35 KVM/gfxstream GLES 3 gate. The Android run completed first draw in 2336.217 ms and retained
five visible crabs in one draw. These cross-renderer checks qualify function and composition only;
a combined additional-wildlife worst-case view remains open.

### Clearing-butterfly baseline

Eighteen deterministic butterflies occupy three bounded sunny-clearing habitats and share the
observer-relative activation policy. The near tier uses hinged patterned wings and a small body;
the far tier retains a compact moving silhouette. All visible agents share one dynamic
`VertexPositionColor` batch, so the complete layer adds one tracked submission when visible and
none when culled. It adds no texture, custom shader, or audio source.

One isolated Debug OpenGL 3.3 run of the 1280 by 720 butterfly view on the AMD Radeon 780M/Mesa
25.0.7 recorded 3.5798 ms mean, 2.8551 ms median, 7.6164 ms p95, 10.5271 ms p99, and 13.1143 ms
maximum. Twenty-nine of 600 intervals exceeded 8.333 ms; none exceeded 16.667 or 33.333 ms. The
view reported 85 tracked submissions and 10--11 visible butterflies, with one butterfly draw.
This is an integrated Debug scene baseline, not an isolated species-cost attribution.

The dedicated composition also passed Chrome/SwiftShader and Firefox ESR WebGL 2; Firefox reported
11 visible agents, seven near and four far, in one draw. The strict KVM/gfxstream GLES 3 view
reported the same 11/18, seven-near/four-far composition in one draw. A combined
additional-wildlife worst-case view remains open.

### Near-shore sea-turtle baseline

Five deterministic sea turtles occupy separate 7--38 m offshore routes, share the observer-relative
activation policy, and remain between authoritative seabed clearance and the water surface. The
near tier uses a 204-vertex faceted shell, scute pattern, head, eyes, tail, and four articulated
flippers; the 24-vertex far tier retains a complete silhouette. All visible animals share one
dynamic `VertexPositionColor`/`BasicEffect` draw and add no texture, custom shader, or audio source.

One isolated Release OpenGL 3.3 run of the 1280 by 720 turtle view on the AMD Radeon 780M/Mesa
25.0.7 recorded 2.1252 ms mean, 1.7856 ms median, 3.2192 ms p95, 9.5893 ms p99, and 19.3897 ms
maximum. Eleven of 600 intervals exceeded 8.333 ms, two exceeded 16.667 ms, and none exceeded
33.333 ms. The fixed view held 75 tracked submissions and one visible near turtle in one draw.
This is an integrated Release scene baseline, not isolated species cost attribution.

The same composition passed Chrome/SwiftShader and Firefox ESR WebGL 2. A strict API 35
KVM/gfxstream GLES 3 run retained one near turtle in one draw beneath the transparent ocean; its
clean 2400 by 1080 frame was inspected and first draw completed in 2527.896 ms. These checks
qualify portable function and composition, not physical-mobile frame time, power, or thermals.

### Sparse hanging near-palm dry-frond follow-up

Palm variants zero and two now add two or three incomplete dry fronds only to their stored near
meshes. Five short rachis segments, four potential leaflet stations, deterministic gaps, unequal
leaflet length, and brown/ochre shading keep them subordinate to the living crown. Across all four
near variant meshes, the pass adds 156 vertices and 82 triangles; it adds no buffer object,
instance field, material, texture, simulation, population, culling rule, far-LOD work, or draw
submission.

The complete OpenGL 3.3 private-display suite passed 46/46 with two workers after a one-off host
Xwayland/AMD context reset under an eight-worker display load; the lower-concurrency rerun is the
authoritative game result. The complete 27-stage Chrome/SwiftShader WebGL 2 matrix and independent
Firefox ESR gate passed and required the exact dry-frond marker. Native, Chrome, and Android palm
frames were inspected at their original resolutions. A strict API 35 x86_64 KVM/gfxstream GLES 3
run reached first draw in 2413.984 ms and retained the selected dry silhouettes without a fatal,
shader, or `GL_INVALID_OPERATION` diagnostic. The restored arm64-only package is 19,079,172 bytes
with a 16,268,888-byte stripped `libmain.so`; lint passed, signature scheme v2 verifies, and the
temporary x86_64 intermediates were removed.

One isolated Linux Release palm run retained 293 visible palms (47 near and 246 far) in eight palm
submissions and 78 total tracked submissions. Across 600 intervals it measured 1.0120 ms mean,
0.9467 ms median, 1.4861 ms p95, 1.6594 ms p99, and 2.1476 ms maximum; no interval exceeded 8.333,
16.667, or 33.333 ms. This is a gross-regression guard for the complete fixed scene, not an
isolated triangle-cost attribution or a mobile result.

### Web release transfer payload

`tools/package_web_release.sh build-probe` copies the three release artifacts, emits deterministic
`gzip -9 -n` variants, and records SHA-256 integrity data and MIME types in
`web-release/manifest.json`. For the current Release/WebGL 2 build, HTML is 2,058 bytes (1,144
gzip), JavaScript is 264,591 bytes (59,251 gzip), and WebAssembly is 8,809,020 bytes (2,523,521
gzip). The complete transfer payload therefore falls from 9,075,669 to 2,583,916 bytes, a measured
71.5% reduction.

This result measures static transfer bytes only. It does not measure browser decompression,
WebAssembly compilation, decoded runtime memory, cache behaviour, or cold-load latency. The game
generates its audio PCM and render textures at runtime, so they do not add separate downloads.
Production-host header validation and repeated hardware-browser cold-load measurements remain
required before assigning a startup budget.

### Current Linux Release frame-pacing baseline

Three isolated suites were run from the stripped relocatable Release package on 2026-10-07 at
1280 by 720. The renderer was CNA `OPENGL33` on the AMD Radeon 780M with Mesa 25.0.7; the private
compositor had no audio device. Each range covers three independent 600-interval runs. The final
column is the exact 120/60/30 Hz miss count from the schema-3 run only.

| View | Mean ms range | p95 ms range | p99 ms range | >8.333 / >16.667 / >33.333 ms |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.6153--1.7101 | 2.4641--2.6501 | 8.3193--9.0752 | 6 / 1 / 0 |
| Beach | 2.4599--2.6794 | 3.8566--4.9502 | 9.2330--13.4599 | 20 / 5 / 0 |
| Jungle | 2.3109--2.5070 | 3.8703--4.6773 | 8.8886--11.2815 | 17 / 0 / 0 |
| Driftwood | 1.5310--1.6385 | 2.2126--2.5154 | 5.4131--10.1251 | 8 / 0 / 0 |
| Monkeys | 2.3531--2.6451 | 3.4242--4.9259 | 10.7130--11.4121 | 19 / 1 / 0 |
| Moving traversal | 2.2811--2.5136 | 4.6850--5.7565 | 9.9770--12.8821 | 22 / 3 / 0 |

All p99 values remain below the 16.667 ms 60 Hz interval. In the instrumented suite, only 10 of
3,600 aggregate intervals exceeded it and none exceeded 33.333 ms. This is the first qualified
Linux release gate: on the current development GPU, every representative view should keep p99
below 16.667 ms and should not show repeated 33.333 ms misses. It is not a broad minimum-spec claim
and does not replace VSync-on interactive pacing, active-audio, driver-family, or lower-end-hardware
testing.

The later 42-blade close-grass Release gate deliberately uses a heavier fixed view with 2,827
visible clusters, including 957 near instances, and 99 tracked submissions. Its 600 intervals
measured 3.1653 ms mean, 6.3239 ms p95, and 13.9728 ms p99; 15 exceeded 8.333 ms, four exceeded
16.667 ms, and none exceeded 33.333 ms. This keeps the denser first-person tier inside the same
current-GPU gate without presenting the result as physical-mobile evidence. The subsequent
position-only natural-silhouette pass retained the same populations/submissions and measured
2.6461 ms mean, 6.3492 ms p95, and 11.7514 ms p99; one of 600 intervals exceeded 16.667 ms and none
exceeded 33.333 ms.

## Measurement rules

- Record the game commit, dependency revisions, build type, resolution, GPU, driver, and audio
  availability with every baseline used for a decision.
- Compare identical viewpoints and settings. A faster number from a different visible population
  is not evidence of an optimization.
- Prefer p95 and p99 duration when judging frame stability; mean FPS can hide hitches.
- Use the schema-3 120/60/30 Hz miss counts to distinguish an isolated maximum from repeated
  frame-budget failures; each report contains exactly 600 measured intervals.
- Run at least two suites after a surprising change and retain the raw JSON until the conclusion
  is documented.
- Never use the unaccelerated Android AVD for performance claims. Android budgets require a
  physical device or a stable KVM-accelerated emulator with explicit hardware details.
- WebGL 2 needs a visible hardware-accelerated browser measurement rather than SwiftShader.

## Budgets still to establish

A first current-GPU Linux Release frame-time gate now exists above. Cross-hardware Linux draw-call,
triangle, memory, startup, and active-audio budgets remain open, as do browser cold-load/frame-time
instrumentation and physical-Android frame-time, memory, power, and thermal limits.
