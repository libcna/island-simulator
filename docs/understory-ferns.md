# Connected pinnate understory — 2026-10-09

## Geometry and scope

Understory forms 0/2 now use fern-like pinnate fronds rather than uninterrupted narrow strips.
Forms 1/3 retain their broad curved blades. Population, placement transforms, reserved routes,
collision, materials, shader, instance format and submission strategy are unchanged.

Each frond keeps the existing source profile, rooted curve, phase, pigment and age variation.
Seven pairs of tapered folded leaflets attach along its exposed rachis. Leaflet length tapers
toward the tip; slight left/right and pair variation avoids perfectly mirrored comb teeth.
Their axis and plane follow the sampled rachis tangent, rather than global-horizontal blades.
Nearly vertical emerging leaves limit secondary roll to 1.15 radians, within the existing
pointed-leaf contract. This is an art-directed fern-like plant, not a specific botanical species.

Both tiers use the same nine stem knots, including all seven attachment locations. A middle
vertex at every knot gives each leaflet root exactly the same position, wind phase and weight
as its supporting stem. This matters because the shader's gust also depends on local position.
Leaf tips gain bounded additional flexibility. Far plants still select five actual near fronds;
each retains four unchanged leaflet pairs (0/2/4/6). No re-spacing or replacement pose is used.
Far leaf removal still changes coverage, so seamless LOD is not claimed.

CPU regression checks cover all generated profiles: exact knot membership, rooted blade
positions, positive above-ground bounds, unit normals, frond-plane alignment, wind envelopes,
unchanged retained LOD identities, invalid pair/side/coordinate rejection, and overflow input.
Mesh upload independently validates indices, finite fields and wind weights before GPU use.
Cell culling continues to measure the actual meshes plus the shared wind envelope.

## Cost

| Understory form | Near vertices / triangles | Far vertices / triangles |
|---|---:|---:|
| 0 fern | 776 / 704 | 335 / 320 |
| 1 broadleaf, unchanged | 405 / 576 | 30 / 20 |
| 2 fern | 970 / 880 | 335 / 320 |
| 3 broadleaf, unchanged | 495 / 704 | 30 / 20 |

The two fern forms previously used 360/512 and 450/640 near, both 30/20 far. Shared buffers gain
80,392 vertex bytes and 6,192 index bytes, with no additional buffer objects or textures. All
jungle static vertex data totals 2,606,396 bytes (excluding indices, instances and driver copies).
The normal jungle view still draws 462 understory plants (102 near/360 far), and 35 jungle draws.
Far fern cost increases substantially, from 20 to 320 triangles per plant; this is not a free
quality improvement or measured mobile-performance result. Physical GPU qualification is blocked
by the separate AMDGPU incident. Future profiling must include these additional triangles.

## Native iterations and evidence

All paths below are under `build/software-captures/`, rendered serially through the contained
llvmpipe/private-pixman supervisor with kernel checks before/during/after each run.

- Baseline clearing edge: `20261009T065101.624175Z-clearing-edge/`.
- Initial horizontal leaflets: `20261009T065500.403381Z-clearing-edge/` and
  `20261009T065504.587302Z-jungle/`. Inspection showed excessively thin wire-like profiles from
  eye height. These images are retained, not presented as the accepted art result.
- Rachis-aligned correction: `20261009T065916.323845Z-jungle/` and
  `20261009T065920.564882Z-clearing-edge/`. The comparison sheet
  `build-probe/island-fern-iterations.png` shows baseline/initial/corrected crops at 2x nearest
  enlargement. Inspection shows a legible pinnate fan and open gaps; broad blades are unchanged.
- Final native validation after overflow guards and mesh reservation cleanup:
  `20261009T070138.577645Z-jungle/`, 1280x720, 226 colour bins, luminance deviation 44.61,
  spatial contrast 211.79. Kernel deltas are empty for all six runs.

The first unbounded tangent-derived roll failed the existing pointed-leaf range test. It was
not rendered or accepted. Bounded secondary roll fixes this while every actual leaflet-plane
alignment remains above 0.95. The final shared-ccache/two-worker native build passes all 32
non-display CTest groups in 20.23 s; log `build/gpu-incident-evidence/20261009-fern-final-ctest.log`.
This is first-party art/geometry work, not a CNA bug or a fix for GPUVM faults.

## Web validation

Incremental WebGL 2 shared-cache build passes. The contained software command was:

```sh
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope vegetation --timeout 300'
```

`build/browser-software/20261009T070325.041949Z-chrome-vegetation/` passes grass, jungle, fronds,
east and clearing-edge. Each runs 60 CPU-rendered intervals, with zero GL errors and no context
loss. The strict gate requires the new fern marker. Chrome host PID 3941720 uses SwiftShader
inside the GPU-free private device namespace. The post-simulation jungle PNG was inspected:
the segmented fern fan is present without replacing the neighboring broadleaf forms. This pair
of stills is not a continuous motion/attachment inspection. Live/final kernel deltas are empty.
The refreshed Web package passes validation at 13,870,094 raw / 6,960,362 gzip bytes.

## Android validation

The isolated software command was:

```sh
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint jungle'
```

Both x86_64 and default ARM64 debug assembly/lint pass with two workers/shared ccache.
`build/android-software/20261009T070727.885381Z-jungle/` passes on API 35 KVM/SwiftShader,
app PID 1779/native entry TID 1887, required fern marker and 10,684.628 ms total first-draw
startup. The 1280x720 screenshot was inspected; fern fans, broadleaf neighbors and touch HUD
are present. Live kernel log is empty, final delta `[]`, owned emulator exit zero.

Tested x86_64 APK: `build/android-software/island-x86_64.apk`, 23,918,072 bytes,
SHA-256 `ae376130a71fcd94782d8228a02585cbd80d5b3f61a63a2cf8bb6ec2dd974dff`.
Restored ARM64 APK: `android/app/build/outputs/apk/debug/app-debug.apk`, 24,007,649 bytes,
SHA-256 `5adad5d287bf527b3950241d07ad21b4f873bfbcc4fc68babb0324effe56ee5b`.
Only ARM64 native libraries are packaged in the latter; ARM64 runtime remains untested.
Prior APKs are retained as `build/android-software/island-{arm64,x86_64}.before-ferns.apk`.
CNA/Sharp Runtime remain clean on `next`; the hardware stop latch is unchanged.

## Remaining validation

Static images and CPU attachment tests do not complete moving-camera shimmer/LOD or
botanical-realism review. The plant still uses
coarse folded leaflets and few pairs. Broadleaf scale/shape and repeating bush masses need
further attention; do not declare the entire jungle finished from this one geometry pass.
