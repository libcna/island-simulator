# Forest-floor material review — 2026-10-09

## Decision: v1 interior-window runtime material; v2 remains a rejected study

Two original AI-generated candidates are hash-gated under `assets/art-studies/`.
Only v1 is staged as `Content/terrain/forest-floor.png` for native installation,
Web preloading and Android assets. Its runtime reduction uses a complete 512-square mip chain.
The procedural terrain remains the diagnosed missing/invalid-file fallback.

Full source and 2x2 repetition-sheet inspection shows richer granular detail, but recognizable
repeated leaves/twigs and clipped boundary features. Some leaf silhouettes would exceed the
requested 5–12 cm scale at a two-metre tile size. Dark crevices also need a lighting review:
requesting albedo does not prove absence of baked shading. Neither candidate is accepted for
direct repeat sampling. v2 requested edge correction only, but changed interior detail and
worsened measured discontinuity. Do not adopt it merely because it is newer.

Further blind generation is not the next step. The edge-safe source-window reference below is
now integrated for in-game comparison, not certified final release quality. Platform evidence
is recorded below; direct repeat sampling is still rejected.

## Reproducible offline review

`tools/review_texture_tile.py` reads RGB8 P6 using the existing capture reader, without a GPU or
third-party Python package. It compares wrap edges with up to 64 uniformly selected ordinary
adjacent row/column boundaries. Low edge error alone cannot certify seamlessness or albedo quality.

```sh
magick -limit thread 2 assets/art-studies/forest-floor-albedo-v1.png -depth 8 build-probe/island-forest-floor-v1.ppm
python3 tools/review_texture_tile.py build-probe/island-forest-floor-v1.ppm --output build-probe/island-forest-floor-v1-review.json
magick montage -limit thread 2 assets/art-studies/forest-floor-albedo-v1.png assets/art-studies/forest-floor-albedo-v1.png assets/art-studies/forest-floor-albedo-v1.png assets/art-studies/forest-floor-albedo-v1.png -geometry 627x627+0+0 -tile 2x2 build-probe/island-forest-floor-v1-repeat-review.png
python3 tests/TextureTileReviewTests.py
```

Repeat with `v2` for the edited candidate. PPM conversion does not resample; the reduced
repetition sheet is a review artifact, not a runtime texture.

| Candidate | X wrap/interior mean difference ratio | Y ratio | Interior cuts below wrap difference |
|---|---:|---:|---|
| v1 | 1.447480 | 1.384975 | 64/64 on both axes |
| v2 | 2.337722 | 1.731881 | 64/64 on both axes |

Both outputs are **1254x1254**, despite v1 requesting 1024x1024. Each would occupy 6,290,064
RGBA8 base bytes or 8,384,072 bytes across 11 floor-halved mip levels, excluding CPU shadows,
decoder temporaries, driver overhead and encoded bytes. A prepared 1024x1024 full-mip RGBA8
texture would use 5,592,404 bytes; 512x512 uses 1,398,100. The latter chain is now prepared in
memory by the reference below and uploaded once by the runtime material.

## Edge-safe source-window reference

`ForestFloorSampling` uses a 0.32 m world lattice and four overlapping source windows. Each
anchor has a stable unsigned integer identity, one of four quarter-turn rotations and a source
centre in [0.34, 0.66]. Sampling at 0.8 UV/metre limits all source positions to approximately
[0.084, 0.916], avoiding the mismatched borders. The complete source therefore represents
1.25 m, instead of the original two-metre generation request. Larger broadleaf fragments are
still recognizable, but no longer dominate the four-metre swatch.

Quintic interpolation followed by normalized cubed weights narrows ghosted transition regions.
The four weights form a partition of unity. At a lattice boundary the disappearing anchors have
zero weight, and surviving anchors retain their identity/coordinates on both sides. This is
continuous sampling of a **nonperiodic** source, not a magically seamless replacement image.
The first XOR combination of coordinate products failed a signed-coordinate identity test;
addition before avalanche mixing removes that evidenced mirror-pair collision. Fixed unsigned
golden hashes protect the corrected mapping. This was project reference code, not a CNA bug.

`ForestFloorPixels::PrepareTexture` accepts bounded opaque square RGBA8 input and makes a
512-square base with exact source-texel area coverage, including fractional border rows/columns.
It then builds all ten levels through 1x1. The 1,398,100-byte payload includes every level but
excludes source pixels, decoder temporaries and GPU/CPU resource shadows. RGB **code values**
are averaged to match existing `SurfaceFormat::Color` game shading; this is not linear-light,
color-managed PBR. Nonopaque input is rejected, not incorrectly averaged without premultiplication.

Native unit tests cover weights, source margins, rotation variation, signed-coordinate hashes,
cell and corner continuity (maximum 0.000376135 sample difference across a 0.2 mm displacement
in a nonperiodic analytic source), invalid inputs, constant-colour preservation, fractional source
edges, opacity rejection and the exact mip payload. That numerical source is a continuity oracle,
not a visual-quality score for the artwork.

### Actual CPU material preview

```sh
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build build --target island_forest_floor_tests island_forest_floor_preview --parallel 2
build/island_forest_floor_tests
magick -limit thread 2 assets/art-studies/forest-floor-albedo-v1.png -depth 8 build-probe/island-forest-floor-v1.ppm
build/island-forest-floor-preview build-probe/island-forest-floor-v1.ppm build-probe/island-forest-floor-overlap-mipped-v1.ppm
magick -limit thread 2 build-probe/island-forest-floor-overlap-mipped-v1.ppm build-probe/island-forest-floor-overlap-mipped-v1.png
```

This host-only tool uses the exact C++ sampling/pixel reference to render a 4x4 m, 1024-square
unlit material swatch. It performs bilinear sampling within levels and trilinear blending at the
known orthographic footprint (LOD 0.678072). Source dimensions/encoding/payload and output errors
are checked; overwriting the input is refused. CLI tests cover valid constant-colour reconstruction,
malformed/oversized/nonsquare/truncated/trailing input and source-preservation failure behavior.

Both the initial unfiltered preview (`island-forest-floor-overlap-v1.png`) and the final mipped
preview were inspected. The latter reduces the dense grain aliasing of the former. No hard source
wrap join is apparent in this swatch, but softly interrupted leaves and local crossfade ghosting
remain. Accept this approach for a **runtime comparison**, not as finished photorealistic terrain.
The PNG/PPM outputs are disposable reproducible review artifacts, not new distributed source assets.

## Runtime integration and evidence

`ForestFloorMaterial` ports the mapping to GLSL 330 / ES 300 using high-precision integer hashing
and explicitly rotated texture gradients. World derivatives are computed before material branches.
One owned texture binds to slot 1 with capability-gated 4x AnisotropicClamp or LinearClamp;
the disabled path uses the already complete
canopy slot 0 and a zero-strength uniform, never an unbound sampler. Window detail fades between
18 and 48 m and with the world pixel footprint, separately from texel mip filtering. Authored
paths/clearings, broad material variation, slope daylight and canopy transmission remain in control.
Procedural close detail is attenuated where image detail is present instead of doubling the litter.

The loader bounds encoded input to 8 MiB and validates square PNG IHDR dimensions (2..2048)
before decoding through public `TitleContainer` / `Texture2D::FromStream`. Public byte GetData
reads the default retained CPU shadow. The exact area reducer rejects nonopaque pixels; all mip
uploads finish before the texture is published to drawing. Temporary source texture/stream/pixels
are destroyed before final texture allocation. For this 1254-square source, each decoded RGBA8
copy is 6,290,064 bytes: temporary GPU texture, CNA CPU shadow and extraction vector must not be
confused with the final 1,398,100-byte texture. Encoded bytes, decoder allocations, final CPU
shadows, driver padding and framebuffer memory are additional; no measured total-memory claim.
The combined game texture payload is 6,033,888 bytes against a 6,054,104-byte profile budget.

- Native incremental build and all 31 non-display CTest groups pass (20.21 s). Header tests
  reject signature/chunk errors, truncated headers, nonsquare/oversized/one-pixel dimensions.
  Web package tests now require and round-trip the fourth `.data` artifact.
- Native llvmpipe jungle `build/software-captures/20261009T053635.696121Z-jungle/` passes:
  1280x720, 214 colour bins, 44.23 luminance deviation, 210.85 spatial contrast. Actual PNG
  inspected: materially richer close soil than the procedural baseline, still stylized vegetation
  and grazing-angle blur. Decode/reduce/upload is 175.552 ms on this software run, not GPU timing.
- Missing-file test temporarily moved only the generated native PNG and restored it with a shell
  trap. `20261009T053722.333672Z-jungle/` renders a valid procedural fallback frame, logs the exact
  missing path and exits the workload successfully. The normal qualification helper correctly
  returns failure for the diagnosed fallback; this is an intentional negative test, not a normal
  material PASS. Its inspected image validates at 191 bins, 43.13 deviation, 212.51 contrast.
  Both runs have empty live/final kernel deltas. Source and restored staged PNG hashes match.
- Web build and Chrome/SwiftShader PID 3910489 pass overview/beach/east/jungle, 60 frame intervals
  each, no GL error/context loss, nine empty kernel checks. Actual jungle PNG inspected and
  consistent with native. Previous reports/images retained with `.before-floor-runtime`.
  Four-file package: 13,854,483 bytes uncompressed, 6,954,937 gzip. Its 4,428,065-byte `.data`
  hash equals the source PNG because it currently contains exactly that one file.
- Android staging initially exposed a game-side Gradle dependency error: lint consumed generated
  files without depending on the producer. A subsequent unannotated-provider attempt passed
  compilation/lint but omitted the APK asset. Direct ZIP inspection caught that before launch.
  The final `StageIslandAssets` Sync task has an annotated output DirectoryProperty and is
  registered through [AGP's generated-source API](https://developer.android.com/reference/tools/gradle-api/8.7/com/android/build/api/variant/SourceDirectories#addGeneratedSourceDirectory(org.gradle.api.tasks.TaskProvider,kotlin.Function1)).
  Packaging/lint now pass; moving its generated PNG away triggers correct regeneration, and
  the actual APK asset matches the source hash. Temporary duplicate staging files were removed.
  This was project build integration, not a CNA/Sharp defect.
- First isolated Android run `20261009T054439.701531Z-jungle` loaded the floor successfully
  (PID 1741) and reached first draw in 9,808.752 ms, but the strict shell gate still expected the
  pre-floor texture budget and timed out. Preserve it as a failed qualification, not a game
  crash. Kernel delta is empty and the owned emulator exited zero. Correct the assertion to the
  independently tested 6,033,888/6,054,104-byte payload/budget on all affected gates. APK preflight
  now also rejects missing, wrong-size or wrong-hash floor data before installation; 15 CPU-only
  Android gate tests pass.
- Corrected isolated Android GLES jungle `20261009T054739.380317Z-jungle` passes, PID 1829,
  main TID 1939, first draw 10,065.572 ms. The inspected 1280x720 PNG shows the same material and
  mobile touch overlays. Live/final kernel deltas are empty and the owned emulator exits zero.
  Retained test APK: `build/android-software/island-x86_64.apk`, 23,903,544 bytes, SHA-256
  `910a4ffc4db3f3b2b6b890381cb2bf86d5b7426e3d9330b22c20473c1dcad1df`.
  Previous test APK is preserved as `island-x86_64.before-floor-runtime.apk`.
- Default arm64 assembly/lint restored (5 s), only arm64 libraries and the exact PNG present.
  A final native llvmpipe boar capture `20261009T055021.552666Z-boars` runs from its evidence
  directory instead of the repository, resolves title content correctly, and passes (198 bins,
  43.83 deviation, 201.63 contrast). Actual image inspected, two visible boars/one draw,
  empty live/final kernel delta. Release archive regeneration is still pending: historical
  October 8 `build-consumer` binaries were not overwritten during this pass.

No new hardware workload is authorized. Remaining visual work includes reducing grazing blur
without shimmer, judging moving views, limiting blend ghosting and bringing surrounding foliage
up to a comparable detail level. The source's baked-looking crevices are not a physical normal map.

## Grazing-angle filtering refinement

The initial trilinear image visibly blurred litter detail along the ground. The current material
queries `GraphicsDevice::SupportsCapability(CNA::GraphicsCapability::AnisotropicFiltering)` once
and selects the immutable `SamplerState::AnisotropicClamp` preset when supported. Its requested
maximum is four, not sixteen. Otherwise it selects the original `LinearClamp`. Neither path
changes image resolution, the ten-level mip chain, source windows, shader gradients, material
fade, albedo, geometry, draw count or texture payload. No negative mip bias is used.

Read-only inspection of the pinned CNA establishes the actual contract:

- `modules/graphics/src/Xna/SamplerState.cpp` and its public header define the 4x preset,
  clamp addressing, zero mip bias and normal mip range.
- `modules/renderers/easygl/src/EasyGLRenderer.cpp::SupportsCapability` queries extension
  support. `CurrentContextAnisotropyLimits`, `ClampedMaxAnisotropy` and `ApplySamplerState`
  clamp the request to the device maximum and reset anisotropy for later non-anisotropic
  samplers, rather than leaving it enabled on an unrelated draw.
- The game's non-display terrain-material test protects the preset assumptions against a
  future dependency update. Runtime logs explicitly identify the selected filter. Unsupported
  anisotropy is an ordinary portable path, not a missing-material failure.

Native software comparison (same jungle camera, same source/shader):

```sh
sg systemd-journal -c 'python3 tools/run_software_capture.py build jungle'
sg systemd-journal -c 'MESA_EXTENSION_OVERRIDE=-GL_EXT_texture_filter_anisotropic python3 tools/run_software_capture.py build jungle'
```

The extension override applies only to that software test process and is now included in the
startup environment log. It does not change machine settings or expose the AMD workload.

- `build/software-captures/20261009T062131.733404Z-jungle/`: actual llvmpipe, anisotropic
  selection logged, capture PASS (224 colour bins, 44.43 luminance deviation, 210.94 spatial
  contrast). Image inspected: clearer ground litter, particularly several metres ahead.
- `build/software-captures/20261009T062207.228248Z-jungle/`: extension hidden, trilinear
  selection logged, capture PASS (207 bins, 44.22 deviation, 210.88 contrast). The full scene
  remains rendered, matching the previous soft appearance. Both kernel deltas are empty.
- A fixed bare-soil ROI, x=[360,490), y=[525,625), has mean byte-space Rec.709 luma
  54.5535/54.3946 for trilinear/aniso and mean absolute horizontal adjacent-luma difference
  1.9987/5.9459. This quantifies the local detail change with little mean-brightness change;
  it is not a realism, aliasing or temporal-stability score.

Native and Web incremental builds pass; all 32 non-display CTest groups pass in 20.29 s,
including the new sampler preset contract. Retained log:
`build/gpu-incident-evidence/20261009-floor-filter-ctest.log`. An initial build caught the fixed
size of the diagnostic environment-name array after adding a name; switching to deduced array
size corrected this game-side compile error. No framework change was necessary.

Web runtime `build/browser-software/20261009T062644.751882Z-chrome-terrain/` passes all four
views under Chrome 152 SwiftShader, host PID 3927812. Each view reports anisotropic selection,
60 presented intervals, zero GL errors and no context loss. Jungle PNG inspected: clearer soil
as in the native comparison. Live/final kernel deltas are empty. The regenerated four-file Web
package passes gzip/hash round trips: 13,855,342 raw bytes, 6,955,371 gzip bytes. No hardware
or moving-camera performance claim is derived from these intervals.

Android x86_64 assembly/lint passes (3 s). The existing isolated KVM/SwiftShader emulator run
`build/android-software/20261009T062509.021834Z-jungle/` passes: app PID 1803, native entry
thread 1882, total first-draw startup 8893.653 ms, complete 1280x720 image inspected. This guest
reports anisotropy unavailable and correctly uses trilinear; it is not evidence of an Android
aniso path or physical-device speed. Live/final kernel deltas are empty, owned-emulator exit is
zero, and host process inspection finds no owned test processes remaining.

The tested x86_64 APK is `build/android-software/island-x86_64.apk`, 23,907,400 bytes,
SHA-256 `8c6541db1d1c9e3946a464ce9edb81d40e4934707b25d0c1b0a9a44a1f4e3f5c`.
Default ARM64 assembly/lint is restored and passes (3 s); its current debug APK is 23,998,641
bytes, SHA-256 `1ed1ff9ea343b9614d98e7b7a74342c7705851d69bd0ad85d053443830c16595`.
Prior x86_64 and ARM64 APKs are preserved as `island-{x86_64,arm64}.before-floor-filter.apk`.

Still pending: continuous moving-camera review of window crossfades/material fade, actual
mobile/hardware cost, and remaining baked lighting/ghosted leaf fragments in the source.
Anisotropy alone does not make the stylized foliage photorealistic.

## Initial read-only CNA and packaging audit (before integration)

Inspected CNA `next` revision `97783d1e090c6d84f4b6a6d7746516e65184569e`, including the consumed
`build/dependencies/cna-next` export, and Sharp Runtime `next`
`21c06910f7cf887cceebb45200e07b43dd45fef4`. No dependency edits.

- `modules/runtime/src/TitleContainer.cpp`: public `TitleContainer::OpenStream` resolves
  title-relative paths on desktop and uses the Android platform loader for AssetManager entries.
  No separate SDL/Java asset loader is needed.
- `modules/runtime/src/TitleLocation.cpp`: desktop root comes from the platform base path,
  falling back to cwd. Stage and install the same relative asset path, and test another cwd.
- `modules/graphics/src/Xna/Texture2D.cpp`: public `Texture2D::FromStream` accepts PNG/JPEG/GIF
  via the shared decoder. Ordinary PNG produces single-level RGBA8, not an automatic full mip
  chain. See `MakeTextureFromMipPixels`, `FromStream`, and CNA's
  `docs/texture-stream-formats.md`.
- Public `Texture2D(device, width, height, true, SurfaceFormat::Color)` and level-specific
  `SetData` can upload an explicitly complete mip chain. Verify every payload, color-space
  treatment and lifetime before choosing a conversion route; avoid implicit GPU readback.
- Sibling `house-simulator` uses `TitleContainer::OpenStream`; it and `cna-examples`
  demonstrate Emscripten `--preload-file source@/logical-root`. These are references only.
- At the initial audit, native installation copied presentation artwork only, Web link options
  had no asset preload, and Android had no project asset producer. Those gaps are now addressed
  by the staged native title file, Web preload/link dependencies and annotated AGP generated
  assets described above. Only the approved source is packaged, not every art study.

The original gates required source/world-scale review, explicit color/mipmap conversion,
manifest provenance, bounded loading, owned texture lifetime, explicit sampler binding,
visible failure diagnostics and serial software runtime checks. Their implementation/evidence
is above. The audit itself did not modify CNA, Sharp Runtime or the game rendering path.

## Generation provenance

Generated 2026-10-09 using the **built-in image_gen tool**, not CLI/API fallback. No model
revision or deterministic seed was returned; none is invented. Prompts do not guarantee byte
reproducibility: preserved PNGs and manifest hashes are authoritative. v1 had no reference image;
v2 used only v1. Both are unmodified output copies; originals remain in the tool output store.

These are AI-generated project art studies, not photographs, scans, CC0 downloads or stock
redistribution. Repository licensing remains undeclared; no exclusive-copyright claim is made.

- v1: `exec-b3df14da-1e2f-4f4d-aff6-ca01cf69905a`.
- v2: `exec-ef20f1d4-d555-4a18-a010-927c520c20a9`.

### Exact v1 prompt

```text
Use case: photorealistic-natural
Asset type: candidate tileable game terrain base-color texture, not a landscape picture.
Primary request: Original realistic tropical forest floor: granular warm umber humus, small decomposing tan and brown broadleaf fragments, fine root fibres, occasional slender twigs, tiny muted olive moss flecks. Soil remains visible over most of the area.
Composition/framing: square 1024 by 1024, straight orthographic top-down view of a 2 metre square patch; uniform texel density, edge-to-edge surface, seamless wrapping on both axes, no prominent focal object. Whole fallen leaves roughly 5–12 centimetres, most litter broken into smaller irregular fragments, not an autumn carpet.
Lighting/mood: neutral diffuse flat illumination appropriate to an albedo texture; no sunlight patches, cast shadows, ambient occlusion gradients, baked specular highlights, vignette, perspective or depth-of-field blur.
Color palette: restrained natural mid-dark soil browns, faded tobacco leaf browns, sparse desaturated olive; no orange saturation or black crevices.
Constraints: photorealistic material detail, no existing artwork references, no rocks larger than pebbles, no living plants, animals, text, logos, frames or watermarks; no normal-map or multi-panel material sheet. A single opaque base-color image.
```

### Exact v2 prompt

```text
Use case: precise-object-edit
Asset type: repeatable game ground albedo texture, seam-correction candidate.
Input image: edit target, the original generated forest-floor texture.
Primary request: Correct only the periodic edge continuity of this forest-floor texture. Left and right edges must join continuously, and top and bottom must join continuously: match soil granules, leaf fragments, colour, and twig contours across the boundaries, including all four corners. Do not create a border strip, blur band or mirrored motifs.
Constraints: keep the interior material, overall palette, straight top-down view, leaf/soil proportions and neutral diffuse lighting unchanged. Single opaque square texture, no labels, text, watermark, border or panels. No added objects or directional lighting. Keep the detailed granular appearance; this is an albedo asset, not a landscape photo.
```
