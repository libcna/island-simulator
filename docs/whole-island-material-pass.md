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
