# Eight canopy forms — 2026-10-09

## Scope

`CanopyFormCount` now selects eight shared procedural forms rather than four. Forms 0–3 keep
their original geometry; forms 4–7 add broad-low, narrow-high, hanging and open-fork structures.
Their lobe dimensions/anchors, trunk heights/leans, leaf postures and base tints vary. These are
art-directed forms, not a claim of botanically accurate species. No external asset is added.

Placement still uses the same independent site hash, acceptance rules, coordinates, yaw and
scale. Only the final variant hash changes from modulo four to modulo eight. All 281 trees,
trunk collision proxies, reserved routes and other population counts are unchanged. Every form
has at least ten representatives in the default island. CPU checks cover all eight trunks,
positive/bounded crowns and support spans, distinct aspect ratios, blade posture envelopes,
determinism, route clearance and shared shadow baking. Invalid variants fail closed.

Both LOD tiers, measured culling bounds and fixed-sun bake consume the shared profiles. Materials,
vertex declarations, shader features, textures and instance format are unchanged. The existing
lianas remain exclusive to forms 1 and 3. No framework file was modified.

## Cost

New forms add eight shared meshes (near/far for each), or sixteen vertex/index buffer objects.
They add 1,175,616 vertex bytes and 129,480 index bytes. All jungle static vertex data now totals
2,526,004 bytes, not including indices, instances, driver overhead or temporary CPU copies.
Form 4/6 near meshes contain 4,620 vertices/4,328 triangles; 5/7 contain 4,626/4,334. Each far
mesh has 1,029 vertices/1,064 triangles. There is no higher per-tree triangle ceiling.

The normal jungle capture retains 65 visible canopy trees (38 near/27 far), but jungle draws
increase from 28 to 35. Eight variants can add up to eight near/far submissions relative to four.
This is a deliberate bounded diversity tradeoff; hardware/mobile performance is not measured.

## Native software validation

Incremental shared-ccache build passes with two workers. All 32 non-display CTest groups pass
in 20.53 s. `build/island-world-report --output build/island-world-report.json` passes; the report
fingerprint is `022cd193037fe080` (variant identity is part of this fingerprint).

Commands use the contained supervisor, one workload at a time:

```sh
sg systemd-journal -c 'python3 tools/run_software_capture.py build jungle'
# For each target below, run near and far serially; stop on the first failure.
sg systemd-journal -c 'python3 tools/run_software_capture.py build canopy --canopy-lod-inspection -87082 -170810 near'
sg systemd-journal -c 'python3 tools/run_software_capture.py build canopy --canopy-lod-inspection -87082 -170810 far'
```

The actual normal jungle image is `build/software-captures/20261009T064200.906596Z-jungle/`.
It passes at 1280x720 with 223 colour bins, 44.55 luminance deviation and 210.96 spatial contrast.
Inspection against the retained four-form `062131.733404Z-jungle` image shows a more open split
central crown and different background silhouettes; floor, routes and ground layers remain.
This is visible variation, not finished realism: low-poly leaves and simple forks remain obvious.

Diagnostic directories use `build/software-captures/20261009T<TIMESTAMP>Z-canopy/`:

| Form / target in millimetres | Near timestamp | Far timestamp | Changed pixels | Whole-image normalized RGB MAE |
|---|---|---|---:|---:|
| 4 / -87082:-170810 | 064231.777866 | 064247.855734 | 965 | 0.0000519585 |
| 5 / 163600:126958 | 064235.742496 | 064252.029675 | 280 | 0.0000175597 |
| 6 / -98207:-158973 | 064239.703990 | 064256.153393 | 818 | 0.0000387263 |
| 7 / -62659:-155698 | 064243.670560 | 064300.332101 | 589 | 0.0000461218 |

All four pairs visibly cover their target; palms partly obscure the narrow form. Changes are
confined to each target's image region. The retained 2x enlarged nearest-neighbor review sheet is
`build-probe/island-eight-canopy-lod-review.png` (180x180 source crop at 550,270). It was inspected.
LOD still changes leaf coverage, trunk detail and lighting, especially form 4. Small whole-image
error is not proof of seamless transitions. These are frozen elevated-camera images, not a walk.
The nine captures use llvmpipe/private pixman, have no reported failures, and retain empty live
and final kernel deltas. They do not qualify the AMD GPU or resolve its primary GPUVM fault.

## Web validation

Incremental WebGL 2 build passes with shared ccache and two workers. The contained command was:

```sh
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope terrain --timeout 240'
```

`build/browser-software/20261009T064451.929486Z-chrome-terrain/` passes overview, beach, east and
jungle, each with 60 CPU-rendered frame intervals, zero GL errors and no context loss. Chrome
host PID 3933155 uses ANGLE/SwiftShader; the private device namespace excludes the host GPU.
The jungle PNG was inspected and retains the new central fork and floor material. Live kernel
log is empty and final delta is `[]`. The refreshed four-file Web package passes its validator:
13,856,503 raw bytes / 6,957,042 gzip bytes. This is not a moving-camera LOD or hardware test.

## Android validation

Both x86_64 and default ARM64 debug assembly/lint pass through shared ccache, two workers.
The x86_64 package runs in the existing isolated API 35 KVM/SwiftShader emulator:

```sh
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint jungle'
```

Evidence `build/android-software/20261009T064727.074751Z-jungle/` passes, app PID 1846/native
entry TID 2000. The strict gate requires the eight-form marker; first-draw total is 10,171.476 ms.
The 1280x720 screenshot was inspected, including the central fork and touch HUD. The floor uses
its supported trilinear fallback. Live kernel log is empty, final delta `[]`, and the owned
emulator exits zero. Emulator shutdown still reports `stop: Not implemented`; this is retained
in its log, not suppressed or misclassified as a game/GPU fault. No physical-device claim.

Tested x86_64 APK retained at `build/android-software/island-x86_64.apk`: 23,910,056 bytes,
SHA-256 `c48c5ce92a5af42e150d000bc3224a8e4146caea49606cae222bcaeec88a7dc8`.
Default ARM64 APK restored at `android/app/build/outputs/apk/debug/app-debug.apk`: 24,001,121 bytes,
SHA-256 `ee01d9e5ba214a6b70a4bd5a4495f116b889f3f556195133a9c8987aff50bebe`.
Its native libraries contain only the ARM64 ABI; it has build/lint, not ARM64 runtime evidence.
Prior tested packages remain in `build/android-software/island-{arm64,x86_64}.before-eight-canopy.apk`.

## Remaining validation

Moving-camera LOD review, the complete browser suites and physical-device performance remain
pending. Host inspection after these tests found no surviving game, Weston, Chrome or emulator.
The hardware stop-latch hash is unchanged; no AMD workload was launched. Future work
should prioritize more natural branching and leaf scale, not simply keep increasing variants.
