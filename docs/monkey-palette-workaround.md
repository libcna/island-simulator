# Monkey palette workaround — native, Android and Web close-view checks pass

## Latest: native visibility resolved

Two consumer issues confounded the initial investigation. The stationary QA camera lost its
near subject during the 24-frame software capture (subject moved about 4.6 m; only a far
monkey remained submitted). The QA camera now follows subject translation without orbiting
with heading changes. The replacement shader also omitted the XNA [0,w] to GL [-w,w] clip
depth conversion used by terrain and foliage, causing incorrect terrain occlusion. That
conversion is now present and covered by the shader contract test.

`20261011T110027.440307Z-monkeys` passes native generic gates and was visually inspected:
the close animated monkey, face, eyes and tail are visible with normal materials, fog and
depth testing. Kernel delta is empty. Android/Web qualification remains pending; this does
not establish visual realism or complete temporal animation quality.

Diagnostic evidence retained (all kernel deltas empty, diagnostic-marked runs intentionally
fail qualification): `105007.082155Z` bind pose, `105132.210692Z` screen triangle,
`105241.957770Z` capture counters, `105550.166270Z` nonindexed screen triangle,
`105710.643799Z` tracked magenta bind pose and `105824.400692Z` tracked magenta skinning,
all on 2026-10-11 with `-monkeys` suffix. Tracked uncorrected-depth capture
`20261011T105403.150762Z-monkeys` passes generic gates but is still visually empty.
These supersede the inference below: initial empty stock/depth-disabled images did not
isolate depth or geometry because the close subject had already left the camera.
All temporary shader/colour/depth/draw substitutions have been removed.

An accidental unfiltered `ctest -R Monkey` also selected the display-labelled capture test;
it failed before graphics-device creation (`No available video device`). It is not evidence
or an authorized launch method. Subsequent CTest uses `-LE display` exclusively.

Native harness now requires a positive close-model submission count at capture readback;
this prevents the known stale first-frame-count false positive, but cannot prove visible
pixels and never replaces image inspection.

Final native `20261011T110247.359170Z-monkeys` passes that stricter gate: 2 visible,
1 near, 4 draws at readback; image inspected with visible close geometry, empty kernel
delta. Native 35/35 non-display groups pass (74.01 s); 29 software-launcher and 21 Android
device-free tests pass. Hardware stop-latch hash is unchanged.

## Android qualification

x86_64 assembly/lint and contained SwiftShader `20261011T110417.653056Z-monkeys` pass
the stricter shader/GL-error gates. Screenshot inspected: close monkey, face and limbs
visible; no stock shader link failure or guest GL error; empty final kernel delta.
This is a close-view visual/startup check, not lifecycle, touch traversal or continuous-motion
acceptance. The separate Android lifecycle issue remains open.

Tested APK: `build/android-software/island-monkey-14bone-x86_64.apk`, 20,457,659 bytes,
SHA-256 `e79fce9c03deac8eb5bd03446b58e5ec1e87bfb199dcf6746a530ccc9186836d`.
Default ARM64 assembly/lint restored: 20,541,436 bytes,
SHA-256 `9025f6e03b114a502041c62bbfa91cbbbfcd6684761e81149e1a7d1dd01f0ab7`;
physical-device runtime untested. The earlier diagnostic/rolling APKs are preserved unchanged.

Focused Chrome `monkeys` scope added using the existing software supervisor: 60 intervals,
GL/context gates and before/after-simulation images. 13 browser-launcher tests and Node syntax
pass.

## Web qualification

Contained Chrome/SwiftShader `20261011T110957.242647Z-chrome-monkeys` passes 60 intervals,
`glError: 0`, context intact. Both images inspected: close monkey remains visible before and
after simulation, with changed pose/location. Empty final kernel delta. This is bounded visual
evidence, not continuous motion-quality, full-input/lifecycle or Firefox qualification.
Web package refreshed: 10,366,754 raw / 3,534,595 gzip payload bytes.

All qualification jobs are finished. The underlying stock-shader defect remains upstream;
only this consumer's rig-sized path is worked around. Model realism is unchanged and still
needs improvement. No dependency, emulator/KVM setting, hardware stop latch or remote changed.

## Earlier investigation (retained history)

The confirmed Android stock shader in `CNA-ISLAND-002` declares 72 mat4 bones and fails
the tested 256-vector vertex-uniform limit. The game rig has 14 bones. A consumer-owned
GLSL 330 / ES 300 shader is implemented in the current working tree, not qualified yet.

The draft preserves the one-weight rig, animation/crossfade/head-attention palettes, original
one-light Gouraud ambient/diffuse/specular material, canopy tint, eye highlights and view-depth
fog. The authored texture is a constant opaque white texel, folded into the shader arithmetic.
The old SkinnedEffect object currently stores authored material properties only; it is never
applied by the replacement path. Palette upload is 14 matrices; the conservative vertex
uniform budget is 76 vectors. A compile-time rig count assertion and vertex bone-index guard
prevent indexing outside that palette. No mesh/animation/population change is intended.

## Evidence and unresolved visibility

- Native build succeeds. The first 35/35 non-display run passes (118.62 s), including the
  new shader source/dialect/budget contracts. These are not GPU skinning correctness tests.
- Replacement native `20261011T102526.856803Z-monkeys` and camera-diagnostic
  `20261011T103358.196031Z-monkeys` pass the generic gates but show NO visible monkeys.
  Do not count them as visual acceptance.
- Temporary native stock comparison `20261011T103538.801300Z-monkeys` also shows no
  monkeys. Its stock-Apply substitution has been REMOVED from source. This demonstrates
  another unresolved visibility issue, not that the replacement is correct. The diagnostic
  startup marker still said 14-bone during this temporary comparison; do not use that run
  to qualify the replacement or Android.
- Final replacement probe `20261011T103724.288807Z-monkeys` reports subject
  `(55.2353,19.1493,17.6934)`, camera `(53.0018,20.5493,21.3881)`, camera terrain 18.0495.
  First CPU-skinned pelvis is `(55.2353,19.6309,17.6934)`, bone M44=1. The CPU palette
  position is plausible. The original close-view count is 2/7 visible, four submitted draws;
  submission counts do not prove pixels. All four retained native kernel deltas are empty.
- CPU initial-root audit without obstacles finds terrain triangle gaps about 0.001–0.011 m;
  it does not prove moving/obstacle-adjusted contact or exclude line-of-sight obstruction.

Next: isolate actual vertex/clip-space output and buffer attribute/palette binding, or an
occluding draw, using contained software diagnostics. A matching empty stock image rules out
blaming only the newly written GLSL. Do not change dependencies or accept a background-only
capture. Android/Web replacement builds and visible geometry/motion checks remain pending.
The executable currently contains the replacement and bounded camera/palette diagnostics;
no temporary stock bypass remains. No graphics job remains running.
