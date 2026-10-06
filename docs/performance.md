# Performance Measurement

## Purpose

Performance decisions must use repeatable scene evidence rather than the short-lived F1 FPS
display. The native benchmark mode renders the normal scene without debug overlays, disables
VSync and the fixed simulation timestep, discards 120 warm-up frame intervals, records 600
end-to-end frame intervals, and exits. Timing is measured between consecutive `Draw` entries, so
it includes the preceding update, draw submission, presentation, and game-loop work. It does not
separate CPU and GPU cost.

One fixed view can be measured with:

```bash
build/island-simulator --benchmark /tmp/island-jungle.json --viewpoint jungle
```

Run the five-view suite without placing a window on the owner's desktop:

```bash
../cna/tools/platform/run_gpu_tests_private.sh \
  --exec ./tools/run_native_benchmarks.sh /tmp/island-benchmarks
```

The suite covers overview, beach, dense jungle, shoreline detail, and close monkey views. The
generated JSON files contain resolution, renderer identity, sampling settings, mean/median/p95/p99
frame duration, minimum/maximum duration, mean FPS, tracked scene submissions, and representative
visible populations. These generated reports belong under `build/` or `/tmp`; they are evidence,
not source artifacts.

The benchmark intentionally rejects smoke, capture, overlay, world-debug, and input-diagnostic
modes. Ordinary play retains VSync and the fixed 60 Hz simulation. Benchmarking must not change
release presentation or movement semantics.

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
tracked submission; the far overview correctly submits none. These fixed cameras exercise the
cache more gently than a moving traversal, so a sustained moving-route benchmark remains required.

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
budgets remain open. Release-mode Linux measurements, browser instrumentation, Android device
measurements, loading timing, and sustained whole-island routes are required before those limits
can be set responsibly.
