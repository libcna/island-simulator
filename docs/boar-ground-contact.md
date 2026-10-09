# Boar rendered-terrain contact

## Defect and correction

The old renderer used the analytic terrain height and normal at the body centre, then placed all
four feet on that single tilted plane. The terrain mesh is piecewise triangular and can differ
from the analytic field, while each hoof is displaced from the centre. Connected legs alone did
not prevent visible hovering or penetration. This is game-side geometry, not an evidenced CNA bug.

`BoarGroundPose` retains smooth analytic body orientation, anchors body height to the rendered
mesh, and solves a separate ground height under each sole centre. Its body-up ray includes the
horizontal displacement induced by tilt. The 18-step bounded bisection samples a cached
`RenderedTerrainSurface`, matching the exact TL/BL/TR and TR/BL/BR terrain triangles. Body bob
and the existing swing lift then feed fixed-length two-link joints in `BoarPose`.

Target height is limited to the anatomical reach interval without stretching bones. Invalid or
unbracketed requests set `reachLimited`; the renderer emits a one-time warning and all three
software runtime gates reject that warning. Invalid state/out-of-mesh inputs throw instead of
sampling invented seabed. The normal valid-habitat audit requires no limited contacts.

Both near and far geometry consume the same four poses. The existing single stock BasicEffect
batch, 36,000-vertex capacity, coat texture and geometry counts remain unchanged. The renderer
owns an additional 232,324-byte CPU height cache, constructed once; no new GPU resource or
asynchronous upload path is introduced. At most four bounded solves run per visible animal.

## Deterministic evidence

`tests/BoarPoseTests.cpp` retains full-cycle joint-length, stance/swing and phase-continuity tests.
Additional checks cover low/high/invalid targets, adult and juvenile scales, several headings,
idle and walking poses over valid inland slopes, triangle-boundary crossings, and invalid state.

Native audit on 2026-10-09:

- 10,752 fitted poses, 29,568 stance contacts.
- Original single-plane maximum absolute error: 0.0880604 m.
- Fitted sole-centre maximum absolute error: 0.00000572205 m.
- No limited reach in the sampled valid habitat; upper/lower lengths stay within 0.00001 model m.
- Stance residual tolerance: 0.00015 world m, including float world-position roundoff.
- All 29 non-display CTest groups pass in 20.10 s. These include existing-artifact validators,
  not 29 fresh graphical executions.

## Native visual evidence

Forced llvmpipe/private pixman, one game at a time:

- Before: `build/software-captures/20261009T045947.808700Z-boars/`.
- After: `build/software-captures/20261009T050221.862010Z-boars/`, PID 3902255.
- After image inspected at 1280x720 and as a side-by-side crop with the baseline:
  `build-probe/island-boar-ground-contact-comparison.png`.
- Two visible boars, one draw; 154 colour bins, 42.59 luminance deviation, 199.40 contrast.
- Empty live/final kernel deltas, no graphics/fallback/contact-limit diagnostics.

The pictured starting pose shows only a small correction; the larger numerical defect occurs
elsewhere in the habitat audit. Do not treat that still as proof of natural-looking walking.

## Software platform evidence

Software WebGL checks also pass in Chrome PID 3902657: jungle, boars, monkeys and east,
60 presentation intervals each, nine empty kernel checkpoints, no GL/context/fallback/contact
limit failures. Initial and post-simulation boar PNGs are inspected; animals move farther into
foliage in the latter, so it does not establish close gait quality. Reports are
`build-probe/island-browser-wildlife-foliage.json` and `island-browser-foliage-kernel.json`;
previous outputs have `.before-boar-ground-contact` suffixes.

Android x86_64 assembly/lint passes (4 s). Isolated SwiftShader GLES boar evidence
`build/android-software/20261009T050603.871427Z-boars/` passes: PID 1868/main TID 1940,
startup 8,865.189 ms, current contact marker, no limited-reach/GL/fallback/fatal diagnostics,
empty live/final kernel deltas and owned emulator exit zero. Full 1280x720 image/mobile controls
are inspected. Tested APK provenance is in `build-android.md`. This is neither phone performance
nor hardware incident qualification.

## Remaining work

Only the sole centre is fitted; curved toe surfaces are not individually oriented to terrain.
World-space stance locking, scale-aware stride-speed synchronization and turn-related sliding
remain. Body shape, anatomy and close animation are still stylized. This is a grounding correction,
not finished wildlife realism and not evidence that the unresolved AMDGPU incident is fixed.
