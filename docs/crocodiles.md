# Stream-estuary crocodiles

## Current implementation

The island has three deterministic, non-hostile saltwater crocodiles local to the stream mouth
and mangrove habitat. Each animal owns a terrain-qualified dry-bank anchor, shallow-water anchor
and a bounded 22 m home range. The simulation cycles through bask, crawl and swim behaviour.
An observer within 14 m switches the animal to a seven-second retreat toward water with an
additional away-from-observer steering component.

The portable renderer submits all visible crocodiles in one dynamic triangle batch. Its near
form has a long low volumetric torso, a segmented muscular tail, broad skull and snout, separate
lower jaw, mouth, teeth, raised eyes, pupils and nostrils, paired dorsal osteoderms and four short
splayed articulated legs with toes. Land gait alternates the feet; swimming increases the tail
wave and lowers the body toward the water. Beyond 86 m a reduced long-body silhouette is used.

## Evidence and honest limits

- `island_crocodile_tests` checks a population of three, stream-mouth locality, valid initial and
  120-second habitat containment, deterministic state, initial bask/swim diversity and a moving
  retreat response.
- Native Release forced-llvmpipe capture `--viewpoint crocodiles` has passed finite-frame and
  image-distribution validation with one unobstructed near animal visible in one draw. Placement
  is rebuilt against the world's static collision circles, so the complete animal no longer
  intersects the stream-mouth rock or mangrove trunks.
- Chrome/SwiftShader WebGL passed 60 positive frame intervals, two distinct rendered images,
  zero GL error and no context loss. Evidence is under
  `build/browser-software/20261014T200855.976569Z-chrome-crocodiles/`.
- The ARM64 debug APK builds successfully with the exact locked runtime. Its SHA-256 is
  `f540b4769a7937e018f22fb483b3b4ceebf426f15b423fd146d6f145b0364026`, and its packaged
  `libmain.so` contains the crocodile camera and form diagnostics.
- The current procedural colours provide facet lighting and three subdued olive/brown palettes;
  there is no sampled skin texture or normal map.
- Native and WebGL visual acceptance are complete for this bounded implementation. Android
  runtime/device capture remains open; only the APK build and packaged-code inspection are
  claimed, because the existing emulator/KVM state is intentionally not changed.
