# Live-leaf material study

Started 2026-10-09. Hardware incident qualification remains stopped. This material
is a separate visual change, not a GPU fault fix.

Packaging follow-up: both materials now ship pixel-equivalent, host-reduced PNGs;
see `prepared-materials.md`. Original-source package sizes below describe this
material's initial integration, not the current rolling packages.

## Source and provenance

- Built-in image_gen mode, original generation without reference images.
- Preserved source: `assets/art-studies/leaf-lamina-v1.png` (1254-square opaque PNG).
- Exact prompt: `assets/art-studies/leaf-lamina-v1-prompt.txt`.
- Generation identifier: `01a10c93-00dd-7150-b100-cc25f7f7b4dd/exec-2d6be4d8-c8f5-4092-b26e-2baf3428d1fd.png`.
- SHA-256: `3162dc7dcfe492a0479cdc5f72474e61154d06f811388f3942d8ad90561b8e8c`.
- Project artwork, not a scan or photograph. See ASSET_LICENSES.md; no repository
  license has been declared and no CC0/exclusive-copyright claim is made.

The source fills the square with green tissue, a centred midrib, asymmetric side
veins and finer capillaries. It has some baked midrib contrast; this is an artistic
colour source, not measured albedo or a normal map. The original is unchanged.

## Integration

`OpaqueImageLoader` shares the existing bounded floor-image decoder, public CNA
byte transfers and exact area reduction. PNG streams are limited to 8 MiB and
square dimensions 2..2048; decoded alpha must be opaque. The transient decoder
texture dies before the final material is allocated. Historical `forest_floor`
pixel utility names remain to avoid a broad unrelated rename.

`LeafSurfaceMaterial` owns one 256-square RGBA8 texture, nine complete mip levels,
349,524 bytes. Texture payload total increases from 6,033,888 to 6,383,412 bytes;
decoder temporaries, CPU shadows and framebuffers are excluded. The unchanged
source is packaged as `Content/vegetation/leaf-lamina.png` on all three targets.

Jungle live-leaf vertices reuse along/across/phase attributes. The midrib stays at
U=0.5, root is V=1 and tip V=0; phase mirrors the image horizontally. The shader
uses explicit gradients outside divergent material branches. Sample RGB divided
by the last mip's RGB is clamped to 0.60..1.70, then mixed at strength 0.68 with
neutral modulation. The last mip therefore converges to neutral species colour.
This follows the existing renderer's RGB code-value convention, not a new
linear-light/PBR pipeline. Nine rounded mip reductions introduce small bias in
the global mean; exact statistical energy preservation is not claimed.

Age tint, leaf geometry/lighting, blade-edge response and wax highlights remain.
Procedural veins/mottling are used only when the diagnosed image fallback is
active, avoiding doubled vein patterns. Wood, litter, palms and grass do not use
this material. Slot 2 follows canopy slot 0 and terrain height slot 1; fallback
uses an existing complete sampler but disables its contribution. No raw GL,
framework modification, per-frame upload or extra draw call is introduced.

## Validation and remaining work

### Follow-up: isolated missing-asset qualification

On 2026-10-09, `run_software_capture.py build ground-broadleaf --missing-leaf`
binds an empty directory over only the prepared leaf directory inside the private
device/PID namespace. Host assets are never renamed or removed. Run
`20261009T124801.964528Z-ground-broadleaf` passes with the exact diagnosed leaf
fallback, completed 1280x720 image (inspected), and empty kernel delta. The source
asset's SHA-256, size, inode, device and modification time match before/after.
An ordinary subsequent grass capture `20261009T124913.245139Z-grass` passes with
the loaded marker and empty kernel delta. Four focused CPU test groups pass.

The injected gate accepts only this specific missing-leaf diagnostic; unrelated
fallbacks, a contradictory loaded marker and GL errors still fail. Normal
qualification continues to reject all fallback diagnostics. CPU tests also run
an actual temporary namespace fixture, verify no `/dev/dri`, and check host-file
preservation. This qualifies native software fallback, not Web/Android missing-
asset behaviour or hardware fault recovery. Historical integration results follow.

- Baseline native software capture: `20261009T114815.218442Z-ground-broadleaf`.
- CPU tests cover UV bounds/orientation, mirroring, neutral/bounded modulation,
  invalid inputs and exact complete mip payload. Native build and these tests pass.
  Initial asset validation caught the unlisted prompt; adding its explicit manifest
  record fixed it. No validator was weakened.
- Native software broadleaf `20261009T120122.533676Z-ground-broadleaf` and shrub
  `20261009T120211.824931Z-bush` captures pass at 1280x720. Loaded markers, llvmpipe
  identity, private-device launch metadata and empty kernel deltas are retained.
  Before/after broadleaf and current shrub images were inspected: more irregular
  venation is visible without a scene-wide palette shift; silhouettes remain
  stylized and grass/trunks remain visual weaknesses. No missing-file runtime
  injection was performed; the diagnosed fallback remains unqualified this pass.
- WebGL 2 build passes. Device-isolated Chrome/SwiftShader run
  `20261009T120431.860640Z-chrome-vegetation` passes all eight existing vegetation
  views, 60 frame intervals each, GL error zero and no context loss; live/final
  kernel deltas are empty. Jungle and broadleaf post-simulation images inspected.
  This is not a renewed full input/lifecycle suite or AMD performance qualification.
  Web package: 16,922,796 raw / 9,919,804 gzip bytes, including unchanged source PNGs.
- Android x86_64 build/lint passes (9 s). Owned API 35 KVM/SwiftShader run
  `20261009T121438.315511Z-ground-broadleaf` passes, app PID 1860 / entry TID 1915,
  first-draw total 12,245.743 ms. Its 1280x720 image was inspected; required material
  marker and new memory budget are present. Live/final kernel deltas are empty,
  and owned emulator exits zero. This was not touch, traversal or lifecycle testing.
  The existing CNA GLES platform warning remains; no framework files were edited.
- Default ARM64 build/lint restored (6 s). Three ARM64 libraries and leaf asset
  verified in the APK; ARM64 runtime/physical-device performance remains untested.

Rolling packages (only previous generated APKs replaced, not incident evidence):

| Package | Bytes | SHA-256 |
|---|---:|---|
| `build/android-software/island-x86_64.apk` | 27,011,027 | `cc0e756a84ddc9f48d7b660a8dcb2ff23f71d22a3e3e850aae7720c36809a046` |
| `android/app/build/outputs/apk/debug/app-debug.apk` (ARM64) | 27,102,844 | `46811bbd52e40b844de8d0460045179243df7bef0cf995ff4c2d0cec22b5b9af` |

Remaining limitations:

- Final 32/32 non-display CTest groups pass in 22.87 s. No owned graphics process
  remains; hardware stop-latch hash is unchanged. No hardware repair is claimed.
- One shared vein pattern is a deliberate first pass. Check stretching on narrow
  leaflets, close contrast, repetition and distant shimmer before further artwork.
- Packaging currently keeps the unchanged 3,003,277-byte PNG, although runtime
  needs only 256-square pixels. A reproducible build-time reduction using the same
  filter is a later transfer/startup optimization; keep the original and its hash.
  Do not confuse the 349,524-byte GPU payload with the download cost.
