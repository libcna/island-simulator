# Whole-island material, lighting and composition pass

## Recorded pass

The second post-realism whole-island pass on 2026-10-15 reviewed six independent compositions:

- bay overview: `20261015T120935.678046Z-bay`
- west clearing: `20261015T120939.723096Z-west`
- wet shoreline: `20261015T120943.929441Z-wet-sand`
- reef and water: `20261015T120947.960586Z-reef`
- distant island/sky composition: `20261015T120951.997664Z-distant`
- jungle-floor grounding/contact: `20261015T120956.033948Z-contact`

The fixed sunny direction, sky/haze relationship, reef depth read, clearing contrast and jungle
grounding remain coherent. The wet shoreline was the weakest surface: the broad tide-darkened
region collapsed to a nearly uniform olive multiplier, so the water film and drainage direction
were difficult to read beside the stronger dry-sand and ocean materials.

## Accepted correction

The terrain shader now preserves a restrained warm mineral component in saturated sand instead
of only multiplying the dry colour. A fixed-radius coast projection feeds one filtered noise
sample whose contours remain coherent in the shore-normal direction, forming irregular runoff
threads rather than isotropic blobs. Those threads modulate the shallow film and its bounded
fixed-sun/grazing reflection. Screen-footprint fading removes the new response before it can
shimmer. No texture, varying, vertex, index, allocation, mesh or draw was added.

The accepted native wet-sand frame is `20261015T121342.069719Z-wet-sand`: 334 colour bins,
28.37 luminance deviation and 118.65 spatial contrast, versus the recorded 340 / 25.96 / 100.08
baseline. Independent `20261015T121437.055199Z-bay-ground` and
`20261015T121441.311875Z-beach` frames retain a continuous dry-to-wet transition without a hard
stripe.

Chrome 155/SwiftShader WebGL 2 evidence
`build/browser-software/20261015T122217.349544Z-chrome-materials` passes all four material views
with a live context, exact drainage diagnostic and clean kernel monitoring; its wet-sand PNG was
inspected and matches native. The first 300-second attempt is retained as rejected partial
evidence because it completed litter and grass but timed out while entering bay-ground; the
accepted run used a 600-second supervisor limit and changed no product setting.

The Web package is 11,533,937 bytes of Wasm, 2,230,701 bytes of data and 280,772 bytes of
JavaScript. Android ARM64 assemble/lint passes without emulator/KVM. The 24,533,811-byte APK has
SHA-256 `b703cbe1c67692e7a3d84e7d2fd29402009ba367f9114e691789bf6497dd7f92` and contains a
19,491,240-byte `libmain.so`.

This completes the material/lighting/composition member of the three required recorded passes.
The separate wildlife/sound/motion pass, long subjective listening, moving-edge review and
physical-device qualification remain open.

## Aerial material-classification follow-up — 2026-10-15

A later overview retained the intentional open goat/bear slopes and the qualified eight-metre
mid-scale field, but selected the darkest humus/moss and ridge-outcrop islands as too strong at
whole-island distance. Removing the field was rejected because it would restore large featureless
green/brown areas. The accepted shader instead multiplies only those three classification blend
strengths by a common distance response: exactly one through 165 m, then progressively reduced to
the bounded aerial value by 315 m. The underlying warm substrate, fixed-sun slope light and patch
shapes remain visible. Near walking material is mathematically unchanged.

No new noise evaluation, texture access, varying, uniform, vertex, index, buffer or draw is added.
`IslandTerrainMaterialTests` and `IslandCoreTests` pass. Baseline
`20261015T185036.113477Z-overview` and accepted current-binary
`20261015T190245.454996Z-overview` were inspected side by side; the accepted 1280x720 frame retains
the ridge relief and open habitat while reducing the camouflage-like replacement contrast. It
passes with 358 colour bins, 38.13 luminance deviation, 206.11 spatial contrast and an empty kernel
delta. Close guard `20261015T190410.829356Z-jungle` retains the established 275 bins, 46.22
deviation and 213.02 contrast with an empty kernel delta. Refreshed Web and physical-Android
appearance remain part of consolidated qualification.

## Aerial material-classification refinement — 2026-10-16

A fresh current-binary overview found that the previous bounded endpoints still allowed the
darkest humus/moss patches and ridge-rock classifications to read as repeated camouflage spots
across the two inland hills. The near field and the transition distances remain unchanged: both
responses are exactly full strength through 165 m and reach their reduced value at 315 m. Only
the far endpoints change, from 0.56 to 0.30 for humus/moss and from 0.52 to 0.30 for ridge
outcrops. This keeps the authored material shapes and warm substrate while letting fixed-sun
slope relief, tree masses and the broad terrain silhouette carry more of the aerial composition.
There is still no new shader input, noise sample, texture access, geometry, buffer or draw.

Native baseline `20261016T051641.187734Z-overview` and accepted
`20261016T052228.524060Z-overview` were inspected at the same 1280x720 camera. The accepted frame
retains the beach boundary, both inland ridges and the open-slope habitats while reducing the
long-range dark spotting; it passes at 358 colour bins, 37.98 luminance deviation and 205.71
spatial contrast with an empty kernel delta. The mathematically unchanged near-field guard
`20261016T052023.335869Z-jungle` passes at 286 / 46.22 / 212.96. `IslandCoreTests`,
`IslandRenderedTerrainTests`, `IslandTerrainMaterialTests`, `IslandCaptureValidatorTests` and
`IslandSoftwareCapturePreflightTests` pass. Focused Chrome 155/SwiftShader WebGL 2 evidence
`build/browser-software/20261016T052635.393542Z-chrome-distant` executes the current shader for
60 finite positive intervals with GL error zero, a live context and an empty kernel delta. Its
beach-facing camera is portable-runtime evidence, not a substitute for the native aerial visual
comparison.
