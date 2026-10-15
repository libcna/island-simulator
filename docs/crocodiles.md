# Stream-estuary crocodiles

## Current implementation

The island has three deterministic, non-hostile saltwater crocodiles local to the stream mouth
and mangrove habitat. Each animal owns a terrain-qualified dry-bank anchor, shallow-water anchor
and a bounded 22 m home range. The simulation cycles through bask, crawl and swim behaviour.
An observer within 14 m switches the animal to a seven-second retreat toward water with an
additional away-from-observer steering component.

The portable renderer submits all visible crocodiles in one dynamic triangle batch. Its near
form has a long low volumetric torso, a segmented muscular tail, broad skull and snout, separate
lower jaw, mouth, teeth, embedded lateral eyes, pupils and nostrils, paired dorsal osteoderms and four short
splayed articulated legs with toes. Land gait alternates the feet; swimming increases the tail
wave and lowers the body toward the water. Beyond 86 m a reduced long-body silhouette is used.

## Evidence and honest limits

The 2026-10-15 follow-up animal audit found that the near torso, skull, tail and scutes were
recognisable, but all four legs still met the barrel as narrow tubes. Each existing animated
shoulder/hip joint now carries a low three-ring, eight-sided pectoral or pelvic ellipsoid embedded
in the torso. Upper limb tubes increase from seven to eight sides and taper from the new mass into
the unchanged elbow, wrist and three toes. Simulation joints, gait phase, habitat, population,
palette, LOD threshold, allocation and one-draw batching are unchanged.

The current native capture `20261015T143659.327901Z-crocodiles` passes at 1280x720 with 313 colour
bins, 38.84 luminance deviation, 156.72 spatial contrast and an empty kernel delta. It was inspected
at a front-quarter angle and shows attached shoulder/hip volumes rather than floating tubular legs.
The older side-on frame and current frame are different animation/camera angles, so they are not
claimed as a pixel-matched A/B comparison. At that milestone, snout planar detail, sampled skin
texture and physical Android appearance remained outside the limb-only pass.

### Rounded jaws and lateral eyes — 2026-10-15

A later twelve-family audit selected the head itself. The former upper and lower jaws were long
closed wedges with rectangular side planes, while the raised eyes projected their irises forward
along the snout. Each jaw now passes through four shallow elliptical stations: it stays broad at the
skull, reduces width and height independently, and ends in the existing rounded nasal cap. The two
lofts remain separate, preserving gape, mouth lining and twelve visible teeth. Eye sockets are
lower and wider; thin irises and vertical pupils are embedded on their outward faces, matching a
crocodile's lateral eye orientation and removing the stalk-eyed toy silhouette.

The first real capture safely stopped at the renderer's old 5,200-vertex per-animal boundary. The
fixed capacity is now 5,600, adding 1,200 dynamic-buffer vertex slots over all three animals; no
population, draw, habitat, motion or LOD change was made. A repeated current-binary native capture
`20261015T183247.193116Z-crocodiles` passes at 1280x720 with 313 colour bins, 39.27 luminance
deviation, 154.05 spatial contrast and an empty kernel delta. It was inspected against
`20261015T143659.327901Z-crocodiles`; the named QA camera follows the moving animal, so this is a
pose-aware silhouette comparison rather than a pixel-matched A/B. `IslandCrocodileGroupTests` and
`IslandCoreTests` pass. Web/Android gates require the new diagnostic, while new runtime images on
those targets remain part of the consolidated portable qualification pass.

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
