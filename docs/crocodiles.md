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

### Finer primary body surfaces — 2026-10-16

The next ground-animal comparison retained the crocodile's silhouette, armor, pigment and motion,
but its four largest curved masses remained visibly coarser than the boar and the newly repaired
snake. The primary torso now uses eight by eighteen sections, the shoulder mass five by sixteen,
the skull six by eighteen and the nasal cap four by sixteen. This adds exactly 660 vertices and
220 triangles per visible near animal only. The three-agent population, far silhouette, simulation,
86 m LOD, material path and one dynamic draw are unchanged. The per-animal checked staging ceiling
increases from 5,900 to 6,500 vertices, or 1,800 additional preallocated slots across all three
animals; the accepted current animal remains inside that bound.

Inspected native evidence `20261016T135800.906229Z-crocodiles` passes at 317 colour bins, 39.43
luminance deviation and 155.26 spatial contrast with an empty kernel delta. The broadside/front-
quarter frame retains the readable eyes, jaws, teeth and low armor while reducing the largest
angular steps across the flank, shoulder, skull and nose. WebAssembly rebuilt with the exact new
diagnostic. A fresh Chrome/SwiftShader launch was unavailable because the contained runner could
not create another process (`EAGAIN`), so no new Web runtime acceptance is claimed. Android ARM64
assemble/lint passes against the locked dependency copy and produces a 24,671,835-byte APK with a
19,629,264-byte `libmain.so`; no emulator or KVM was used.

### Reticulated facet pigment — 2026-10-16

The renewed twelve-species matrix retained the crocodile's broad skull, lateral eyes, low trunk
armor and articulated limbs but selected its large uniformly coloured side planes as the weakest
remaining close surface. The existing near torso, shoulder, skull, nasal cap, three upper-jaw
sections, three lower-jaw sections, twelve tail sections and four buried limb-root volumes now
receive deterministic irregular pigment modulation. Two incommensurate row/column fields keep the
variation bounded and prevent repeating stripes; existing fixed-sun facet light remains separate.

This is vertex pigment on the geometry already submitted, not a sampled texture or normal map. It
adds no vertex, triangle, texture, buffer, shader, simulation or draw work, and the far LOD remains
calm. Five focused native groups and all 41 browser/capture harness tests pass. Inspected native
baseline `20261016T004505.743320Z-crocodiles` and accepted
`20261016T070838.194430Z-crocodiles` show the former flat flank replaced by restrained scale-like
patches without obscuring the belly, teeth, eyes or armor. The accepted 1280x720 frame passes at
315 colour bins, 39.23 luminance deviation, 153.94 spatial contrast and kernel `[]`.

Inspected Chrome 155/SwiftShader
`build/browser-software/20261016T071135.535809Z-chrome-crocodiles` passes two distinct motion
frames. The front-quarter and broadside views both retain the irregular pigment through pose
changes; 60 finite positive intervals, GL error zero, a live context, one crocodile draw and
kernel `[]` pass.

### Low trunk armor and tail keel — 2026-10-16

A later full-animal comparison retained the rounded jaws and attached legs but selected the nine
repeated tall triangular back peaks as the weakest remaining silhouette. Eleven irregular rows of
low four-sided paired trunk scutes now follow the changing dorsal line. Behind the hips, the paired
rows merge into eight progressively smaller single keel plates whose centres use the same animated
lateral wave as the underlying twelve-section tail volume. This keeps the armor readable in a
broadside view without the former saw-toothed dinosaur profile.

The replacement adds 198 generated vertices to each visible near animal. The checked per-animal
dynamic staging boundary rises from 5,600 to 5,900 vertices, adding 900 slots over the complete
three-animal batch. Population, habitat, behavior, simulation, 86 m LOD and the single draw are
unchanged. The first native attempt stopped through the old checked boundary before upload and is
explicitly rejected; no overflow or graphics submission occurred.

Inspected same-camera native baseline `20261015T183247.193116Z-crocodiles` and accepted current
evidence `20261016T004505.743320Z-crocodiles` show lower, denser trunk armor and a diminishing tail
keel. The accepted 1280x720 frame passes at 313 colour bins, 39.15 luminance deviation, 153.64
spatial contrast and an empty kernel delta. `IslandCrocodileGroupTests`, `IslandCoreTests` and all
103 browser/capture harness tests pass. Focused Chrome/SwiftShader
`20261016T004809.071264Z-chrome-crocodiles` passes two distinct inspected motion frames, 60 finite
positive intervals, GL error zero, a live context, zero target-close timeouts and an empty final
kernel delta.

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
fixed capacity was raised to 5,600 at that milestone, adding 1,200 dynamic-buffer vertex slots over
all three animals; the later armor pass documents the current 5,900 bound. No
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
