# Broadleaf lamina refinement — 2026-10-09

## Observed problem and change

The clearing-edge view exposed broad leaves with large flat wings and a regular strap-like
outline. Three of five near-mesh columns were concentrated within 6% of the central axis.
This represented the midrib but left nearly the entire wing as one plane.

Forms 1 and 3 now use paddle/lance width distributions measured from their existing petiole,
with bounded low-frequency left/right margin variation, a rounded cross section and subtle
edge undulation. The same five columns sample -1, -0.5, 0, 0.5, 1 across the blade. Both LODs
sample the same analytic profile and retain their previous leaf identities; far tessellation
is still coarse and is not claimed to match every near edge. Root positions, wind data,
placement, broadleaf counts and leaf lengths/lifts are unchanged.

`UnderstoryBladeShape::Narrow` preserves the previous profile for both fern forms. A dedicated
CPU regression compares it with the pre-change formula at roots, body and tip. Other checks
cover distinct paddle/lance width peaks, finite next-representable endpoint samples, invalid
shape rejection, dense above-ground bounds, normals perpendicular to independently sampled
tangents and unchanged near/far source identity. Shapes are art-directed, not species claims.

The vertex/index counts, declarations, materials, textures and buffer objects are unchanged.
All jungle vertex data remains 2,606,396 bytes. Broadleaf forms 1/3 retain 405/576 and 495/704
near vertices/triangles and 30/20 far. The normal jungle still has 65 canopy trees and 462
understory plants visible in 35 jungle draws. The new trigonometry is CPU mesh generation,
not per-frame shader work. It is not evidence of unchanged physical-device performance.

## Native visual evidence

All runs use the contained serial llvmpipe/private-pixman helper:

```sh
sg systemd-journal -c 'python3 tools/run_software_capture.py build clearing-edge'
sg systemd-journal -c 'python3 tools/run_software_capture.py build jungle'
```

Evidence under `build/software-captures/`:

- Baseline: `20261009T071345.003191Z-clearing-edge/`.
- Revised clearing edge: `20261009T071615.578229Z-clearing-edge/`, 1280x720, 180 colour bins,
  luminance deviation 41.17, spatial contrast 189.15.
- Revised jungle: `20261009T071620.064700Z-jungle/`, 228 colour bins, luminance deviation 44.62,
  spatial contrast 211.67.

All three runs pass with empty live/final kernel deltas. The before/after sheet
`build-probe/island-broadleaf-lamina-comparison.png` uses a 520x260 crop at 750,260 enlarged 1.5x.
Inspection shows redistributed curvature and changed broadleaf edges, while neighboring ferns
remain. The improvement is restrained: leaves are still coarse and oversized-looking in some
compositions, with simple rosette architecture. Image histogram scores are validation checks,
not a realism metric. No AMD hardware workload was launched; the GPUVM incident is unresolved.

The incremental shared-ccache build and all 32 non-display CTest groups pass in 20.87 s;
log `build/gpu-incident-evidence/20261009-broadleaf-ctest.log`. All builds use two workers.

## Bounded Web walking review

Instead of another static matrix, the current Web build exercised the player controller:

```sh
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope grass-motion --timeout 300'
```

Two serial runs are retained under `build/browser-software/`:

1. `20261009T072216.611324Z-chrome-grass-motion/` fails before travel: 46 telemetry samples
   remain captured=0/relative=0 at the correct clearing start. No GL/GPU fault was observed;
   kernel deltas are empty. The fixed 500 ms click did not establish game capture. The original
   run did not record browser focus/lock state, so its exact event-order cause is **unproven**.
2. `20261009T072607.224996Z-chrome-grass-motion/` passes after replacing that timing assumption
   with a real held mouse gesture until fresh game telemetry acknowledges capture, bounded by
   the existing 15 s deadline and released in `finally`. Browser focus, canvas hit target,
   bounds and pointer lock are recorded. Capture is acknowledged after **379 ms**, with release
   after 393 ms: this does **not** prove that the original 500 ms duration was too short.

The second run uses Chrome host PID 3951122 and verified SwiftShader in the private GPU-free
device namespace. It records 93 telemetry samples, 44 grass population/LOD states, nine trees
switching canopy tiers in each direction, and an 18.3634 m outward walk followed by return to
within 0.1433 m of the start. Seven full-resolution stops and 100 distinct 640x360 compositor
frames (2,708,244 bytes, no truncation) are retained. Zero GL errors/context loss and empty live/
final kernel deltas. Game/controller code and expected travel tolerances were not changed.

Inspected the farthest full-resolution stop and frames 0/20/40/60/80/99 in
`build-probe/island-broadleaf-walk-samples.png`. Broadleaf plants remain rendered; sparse tufts
and stylized crowns are still conspicuous. This is a sampled sequence, not every rendered frame
or an exhaustive visual assessment of transitions. The original capture flake needs further
bounded diagnosis; successful acknowledged activation is not proof it cannot recur.
The subsequent error-path cleanup retains both the original activation failure and a secondary
release error instead of overwriting the former; that transport-failure path is not runtime-proven.

The refreshed Web package passes validation: 13,871,273 raw / 6,960,777 gzip bytes.

## Android validation

Both ABI assembly/lint runs pass through shared ccache with two workers. The x86_64 runtime:

```sh
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint clearing-edge'
```

`build/android-software/20261009T073001.635653Z-clearing-edge/` passes on isolated API 35
KVM/SwiftShader, app PID 1770/native entry TID 1855, required lamina marker and 12,489.783 ms
total first-draw startup. The 1280x720 PNG was inspected at the intended close broadleaf view.
Live kernel log is empty, final delta `[]`, owned emulator exit zero. The floor uses trilinear
fallback as before. This is software functionality, not physical-mobile performance.

Tested x86_64 APK retained at `build/android-software/island-x86_64.apk`: 23,920,040 bytes,
SHA-256 `ee0f0cbf01b1fbf4a8789085e3025f44c149777cfee9e1d0e1f7164d4e6d1577`.
Restored ARM64 APK at `android/app/build/outputs/apk/debug/app-debug.apk`: 24,009,257 bytes,
SHA-256 `8ec534cc86f1c91de81cfe2a920c193548b68d830ad9b7b682e5d6aa09e97421`.
Its native libraries are ARM64-only; that ABI is build/lint verified, not runtime qualified.
Prior packages remain `build/android-software/island-{arm64,x86_64}.before-broadleaf.apk`.
No owned render processes remain, read-only dependencies are clean on `next`, and the hardware
stop latch is unchanged. Existing build trees were reused, never rebuilt without shared ccache.

## Remaining validation

More comprehensive walking-camera review, far-LOD transitions, leaf-scale/composition and bush silhouettes remain
open. Do not infer a completed jungle or safe hardware driver from these software stills.

## Bounded understory age pigment — 2026-10-15

The renewed overview/jungle/clearing-edge audit found that the broadleaf geometry remained
coherent, but clusters still collapsed into repeated green fans. It also exposed a material bug:
the normalized per-leaf pigment phase was passed directly to the colour multiplier, so valid phase
values near zero produced almost black healthy leaves rather than restrained botanical variation.

Both fern and broadleaf forms now remap that phase to a bounded 0.82–1.04 mature-foliage range.
For paddle and lance plants, short emerging source leaves receive a pale yellow-green blend while a
small deterministic mature subset shifts gently toward olive. Far leaves retain the exact source
identity, so their age cue no longer changes arbitrarily at the existing LOD boundary. Profiles,
geometry, buffers, textures, shaders, instance transforms, population and draw count are unchanged.

Inspected native evidence `20261015T154323.130077Z-clearing-edge` passes at 1280x720 with 271
colour bins, 49.41 luminance deviation, 223.75 spatial contrast and an empty kernel delta. The
matching denser view `20261015T154401.577603Z-jungle` passes with 277 bins, 46.46 deviation,
213.01 contrast and an empty kernel delta. The change is intentionally restrained: it separates
young and mature laminae and removes implausibly black phase samples; it does not claim a new plant
species, denser vegetation, or completed moving-camera botanical review.
