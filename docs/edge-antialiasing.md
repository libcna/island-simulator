# Portable geometric-edge antialiasing — 2026-10-09

> Update 2026-10-12: Web still requests 4x and Android still requests 2x. Native Linux now defaults
> to 0x through the performance-first profile after a severe interactive flight regression. The
> original evidence below remains historical validation of the 4x path, not the current Linux
> default.

## Finding and scope

The grass/canopy geometry has thin diagonal silhouettes, but the game left CNA's
`GraphicsDeviceManager.PreferMultiSampling` at its false default. The earlier LOD identity
work does not provide subpixel coverage. This pass enables bounded hardware-style multisample
rasterization through CNA's existing API; all qualification here still uses **software** GPUs.
It neither explains nor resolves the AMDGPU GPUVM incident. The hardware stop latch stays set.

## Implementation

`RuntimeQualityProfile` requests four samples on Linux/Web and two on Android. Its validation
admits only 0, 2 or 4, not arbitrary large sample counts. Set `PreferMultiSampling` and override
the requested count through `PreparingDeviceSettings`, using the public
`getGraphicsDeviceInformationEXT()` accessor. Current CNA would otherwise request eight.
The callback captures an immutable integer, not a game pointer; CNA owns its event lifetime.

Read-only investigation of CNA `next` established that EasyGL already owns the backbuffer,
clamps requested samples to capabilities, allocates its color/depth renderbuffers and resolves
at presentation/readback. No custom FBO, raw GL setup, post-processing effect, per-frame buffer,
framework modification or shader/material/geometry change is introduced by the game.
`RasterizerState` defaults to multisample-enabled rasterization on the existing draw paths.

At content load the game logs both requested and **renderer-reported applied** samples. A
device-limited downgrade is explicit in the diagnostic; normal play may still run. Qualification
on the known software stacks requires the expected count, so a request alone cannot pass.
This is CNA's applied count, not a measurement of a driver's physical sample storage layout.

## Cost and limits

MSAA smooths polygon coverage, not shader-detail aliasing, flickering specular highlights,
texture/shadow shimmer, coarse silhouettes or mesh-LOD replacement. It is not temporal AA.
Existing analytic normals, simplified far meshes and sparse grass remain visible limitations.

At 1280x720, nominal RGBA8 plus a four-byte depth allocation occupies about 7.03 MiB per
sample: approximately 28.13 MiB at 4x or 14.06 MiB at 2x, **before** resolve/presentation buffers,
driver padding and any sample-count rounding. Native EasyGL already used a single-sample
offscreen buffer, so the nominal increase there is about 21.09 / 7.03 MiB. Web previously
drew directly to the canvas framebuffer; the new offscreen storage is additional. These are
layout estimates, not GPU memory measurements or mobile performance guarantees. Multisample
renderbuffers are not included in the 6,033,888-byte generated texture inventory. Resize and
high-DPI drawable dimensions can increase the allocation. Physical-device profiling is pending.

## Evidence

Native llvmpipe/private-pixman grass captures under `build/software-captures/`:

- Baseline `20261009T080538.287043Z-grass`, policy still disabled.
- 4x `20261009T080747.895221Z-grass`, requested/applied 4x confirmed, capture/readback succeeds.

Both frames are 1280x720 and pass the existing scene validator with empty kernel deltas.
`build-probe/island-msaa-grass-comparison.png` compares a 500x280 crop at (410,390), enlarged
1.5x with nearest-neighbor sampling, baseline left. Diagonal edges appear less stair-stepped;
the grass shape/population and terrain detail are not changed. Normal capture time/wind is
not frozen, so this is a qualitative edge review, **not** a pixel-identity or temporal metric.
Color-bin/luminance statistics are scene checks, not measures of realism.

The quality-policy CPU assertions and 24 software-supervisor fixtures include rejection of
missing/mismatched requested/applied diagnostics.

Web evidence under `build/browser-software/`:

- `20261009T081101.528475Z-chrome-grass-motion`: SwiftShader, host PID 3969527, confirmed 4x;
  walking reaches 18.9789 m outward and returns within 0.3161 m. Seven stops, 104 telemetry
  samples, 57 grass population states, ten canopy identities changing LOD, and 106 distinct
  compositor JPEG frames. No truncation, GL error or context loss. Empty live/final kernel
  deltas. Inspect samples 0/20/40/60/80/105 in `build-probe/island-msaa-walk-samples.png`:
  scene coverage remains, but sparse grass and simplified vegetation are still evident.
  Sampling these frames is not an exhaustive temporal-aliasing measurement or video review.
- `20261009T081437.151151Z-firefox-full`: llvmpipe, host PID 3971371, confirmed 4x; complete
  input/lifecycle and six fixed-scene checks pass. Add a real resized-frame capture plus
  context/error check; `island-firefox-esr-resized.png` is inspected at 1000x600, with preserved
  16:9 scene and normal letterboxing/instructions. Context is live, GL error zero, kernel
  deltas empty. This checks the MSAA resize/resolve path, not physical-browser performance.

Android evidence `build/android-software/20261009T081708.472756Z-grass/` passes the API 35
x86_64 KVM/SwiftShader strict runtime gate with requested/applied **2x**, app PID 1825, entry
TID 1941, first-draw total 10,171.731 ms. Inspect the 1280x720 PNG: world, grass, terrain and
touch overlay remain rendered. Live kernel log is empty; final delta `[]`. This is one fixed
grass view, not a renewed touch/lifecycle matrix or physical-mobile benchmark. The tested APK
SHA-256 is `74d3bb05f6e8c57ee865ecaae0b5b8bc76900a3f5101cde9d7272f43bef2aed9`; preserve it as
`build/android-software/island-x86_64.apk`. Earlier x86/ARM packages remain as `before-msaa`.

Final native build and all 32 non-display CTest groups pass in 20.07 s; the final native jungle
capture `20261009T082202.006971Z-jungle` passes the applied-4x gate and is inspected, with empty
kernel deltas. Web package validation passes at 13,880,109 raw / 6,962,842 gzip bytes. ARM64
assembly/lint is restored and its sole ABI verified, but it has not run on a physical device.
All builds reuse existing native/Web/Android directories, shared ccache and two workers.
The AMD latch hash remains unchanged; no dependency edits or hardware qualification occurred.
