# Grass surface material

2026-10-09. This is an incremental visual change, not an AMDGPU incident fix.
Hardware qualification remains stopped; CNA and Sharp Runtime are unchanged.

## Material and light separation

Previously tall-blade vertex RGB included an alternating 0.94/1.06 side light
and a height ramp before the custom shader applied normal-based daylight and
another root darkening. The side assignment did not follow the sun. Custom-path
vertices now retain pigment, stable individual variation and the existing cured
tips, but omit this baked light. Short turf also omits its baked height light.
The unlit BasicEffect fallback retains exactly its previous colour calculation.

Custom shading uses a separate smooth root occlusion from 0.62 to 1 over the
normalized geometric height. Both tall grass and short turf use their own full
height, independently of wind amplitude. This is an artistic ambient-occlusion
approximation, not physically simulated light transport. Directional diffuse,
transmission, canopy lighting, basal pigment and distance fog remain separate.

## Surface detail and filtering

One float per custom vertex carries signed across-blade position (-1..1), with
2 reserved for seed heads. The existing blade wind phase provides stable grain
variation; it is constant across each blade. Interpolated coordinates follow the
actual tapered/folded mesh and its motion. Nine restrained longitudinal veins,
a narrow midrib and softer margins modulate colour by at most 0.93..1.105.
Seed heads skip this modulation and retain their former shading.

Fragment derivatives are evaluated before the seed branch. Vein modulation fades
to neutral before half a cycle per pixel; broader midrib/margin detail also fades
as across-blade footprint grows. At footprint >=0.5 the entire surface modulation
is exactly one. This deliberately sacrifices unresolved detail instead of keeping
high-contrast subpixel stripes. It is a conservative fade, not an exact integral
filter or proof of zero temporal aliasing.

Desktop GLSL 330 and GLES/WebGL GLSL 300 use the same implementation. The custom
vertex stride grows from 36 to 40 bytes: 3,856 additional bytes across the three
shared meshes. Nine vertex attribute locations are used, below the required APIs'
minimum of sixteen. No extra texture, draw, index, instance or per-frame upload
is introduced. Existing mesh counts stay near 862/640, middle 60/40 and far 42/28
(vertices/triangles). Placement, wind, LOD and culling bounds are unchanged.

## Validation ledger

- CPU reference checks cover occlusion endpoints/monotonicity, finite-domain
  rejection, analytic modulation bounds, phase-independent unresolved veins and
  exact neutral unresolved detail. Mesh upload rejects missing/invalid surface
  coordinates. A dedicated startup marker distinguishes updated builds.
- Native incremental build passes; 33/33 non-display CTest groups pass (22.61 s).
- Baseline llvmpipe capture: `20261009T124913.245139Z-grass`.
- Updated llvmpipe capture: `20261009T130135.460537Z-grass`, 1280x720,
  324 colour bins, luminance deviation 25.03, spatial contrast 64.11. Both images
  inspected: restrained grain on wider foreground blades and no gross palette or
  geometry regression. These animated captures are not synchronized pixel goldens.
  Kernel delta empty; updated material and unchanged mesh inventory logged.
- WebGL 2 builds; contained Chrome/SwiftShader
  `20261009T130317.675153Z-chrome-litter` passes both views, 60 intervals each,
  GL error zero, no context loss and empty kernel deltas. Grass image inspected;
  no new full input/lifecycle or moving-camera review. Package: 10,263,469 raw /
  3,498,026 gzip bytes. No AMD hardware run.
- Android x86_64 assembly/lint passes (2 s). Isolated API 35 KVM/SwiftShader
  `20261009T130636.434000Z-grass` passes, app PID 1770 / entry TID 1868,
  startup total 12,515.258 ms (one observation, not a benchmark). The 1280x720
  image was inspected; material marker present, kernel deltas empty and owned
  emulator exits zero. Ground grazing blur remains on the trilinear-only mobile
  path. No renewed touch/lifecycle test or physical-phone performance claim.
- Default ARM64 assembly/lint restored (2 s), with exactly three arm64-v8a
  libraries and both prepared materials. ARM64 runtime remains untested.

Rolling APKs (only generated packages replaced, historical incident evidence preserved):

| Package | Bytes | SHA-256 |
|---|---:|---|
| `build/android-software/island-x86_64.apk` | 20,349,179 | `07193bdb26e95ffd06d6e67818d78540c280a2ec48c5ed9d3d63549ef4086801` |
| `android/app/build/outputs/apk/debug/app-debug.apk` (ARM64) | 20,440,564 | `7f138322f4401a2aef27d29718456e67dec52f0618737f3462240a9bd5e9f293` |

No owned game, compositor, browser (host PID 4107517) or emulator remains after
these checks. The hardware stop-latch SHA-256 is unchanged and 359 GiB is free.
Final 33/33 non-display CTest groups pass in 22.97 s. Retained-artifact validators
in that suite do not count as additional fresh renderer runs.

### Bounded walking review before operator handoff

`20261009T131053.908704Z-chrome-grass-motion` passes in contained Chrome/SwiftShader
with real W/S input: 19.0084 m outward, 0.7167 m return error, 94 telemetry samples,
47 grass LOD population states and 20 canopy transitions. All 98 recorded frames
are distinct (2,565,775 bytes, no truncation); 30 additional frame intervals pass,
GL error zero, context live and empty kernel deltas. Full-size stages 0/3/6 and
recorded JPEG samples 25/26/27/75 were inspected: no gross material/geometry break,
but sparse coverage/repeated clusters remain visible. This is bounded compressed
sampling, not proof of no fine shimmer or seamless LOD transitions. Owned browser
PID 4112991 exits; no hardware workload was run. See root `handoff.md`.

Remaining weaknesses: sparse coverage in this viewpoint, repeated grass cluster
structure, approximate wind-deformed normals, stylized tree trunks and overall
lighting. This pass does not by itself make the island realistic. The sampled
moving-view review does not prove improved high-frame-rate temporal quality;
physical-device performance is still unqualified.
