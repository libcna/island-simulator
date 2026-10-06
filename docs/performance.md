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

The first suite was recorded on 2026-10-06 at 1280 by 720 with the debug build, CNA `next`
`f243c63a1dc5`, and Sharp Runtime `next` `21c06910f7cf`. The private display reported direct,
accelerated AMD Radeon 780M rendering through radeonsi/Mesa 25.0.7. Audio was unavailable inside
the isolated compositor, so this baseline does not measure the active mixer.

| View | Mean ms | p95 ms | p99 ms | Tracked submissions |
| --- | ---: | ---: | ---: | ---: |
| Overview | 1.1701 | 1.2963 | 1.4914 | 79 |
| Beach | 1.6424 | 1.9168 | 2.4599 | 117 |
| Jungle | 1.3322 | 1.5457 | 1.9885 | 85 |
| Driftwood/shore detail | 0.7560 | 1.0045 | 1.2885 | 22 |
| Monkeys | 1.4536 | 1.5980 | 1.7173 | 92 |

The beach view is currently the heaviest of these fixed native scenes. This is a development
comparison baseline, not a shipping budget: it uses a debug build, one integrated GPU/driver,
one resolution, no isolated audio output, and no WebGL 2 or Android hardware measurement. It also
does not expose GPU-only time, thermal throttling, memory bandwidth, power, or loading cost.

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
