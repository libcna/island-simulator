# Reef and seagrass substrate

2026-10-11, first pass on macro-geography items 5 and 16.

The former shallow seabed was almost uniformly sand-coloured, making the water read as a
continuous turquoise apron. The terrain fragment shader now blends dark olive seagrass
meadows and ochre-brown reef patches into that sand. The ocean still supplies all water tint.

## Material layout

- Seagrass grows between approximately 0.45 and 2.6 m depth, with a broad 26 m noise field
  and finer variation from the existing footprint-filtered terrain noise.
- Reef patches occupy roughly 24–88 m offshore. A broken material crest at 66–86 m offshore
  strengthens the outer edge; this is colour variation, not raised reef geometry or breaking surf.
- A noise field on the normalized coastal direction cuts pale radial sand channels through
  both habitats. It uses Cartesian coordinates on a circle, avoiding an angular wrap seam.
- Coverage fades before the waterline and below 5–8 m depth. Patterns stay anchored to the
  world as the camera moves. No new textures, meshes, uniforms, draw calls or simulation work.
- Three additional value-noise evaluations run only on submerged fragments. There are no
  constant arrays, indexed loops or `continue` statements in the new shader block.

This is an initial material pass. Close views still need three-dimensional habitat and coral
relief, and the future asymmetric coastline/environment field should replace offshore bands.
The current shelf limits how clearly the outer reef is visible through the existing water.

## Validation

- Linux incremental build and all 33 non-display test groups pass (29.47 s).
- Native llvmpipe overview before: `20261011T065432.951110Z-overview`; after:
  `20261011T065517.280206Z-overview`. Both captures pass and were compared visually: dark
  submerged masses are distinct from pale sand gaps, and the immediate shoreline stays sandy.
  The broad shapes remain soft; this does not complete the reef realism backlog.
- Terrain inventory remains 58,081 vertices / 115,200 triangles; overview remains 58/64
  visible cells in three terrain draws. First draw was 142.884 ms before and 284.310 ms
  after in these single startup captures, which include shader warm-up and are not a
  steady-state performance comparison.
- Android x86_64 assembly/lint and contained KVM/SwiftShader captures
  `20261011T065607.960099Z-reef` and `20261011T065707.346500Z-overview` pass. Both images
  inspected: the overview matches the native material layout, and shallow sand/contact at
  the existing reef outcrop stays intact. All three post-change capture kernel deltas are empty.
- Tested x86_64 APK: 20,429,787 bytes, SHA-256
  `002d91df9263678754dd64c25a7885d008acd437f92737cf0620013164120ba3`.
  Default ARM64 assembly/lint restored: 20,515,636 bytes, SHA-256
  `d818323726a85c9559c1d207175a6fdd3101575195b3fea2040f50397e023f10`;
  physical ARM64 runtime remains untested.
- Web incremental build and Chrome/SwiftShader terrain suite
  `20261011T065909.248161Z-chrome-terrain` pass all four views (overview, beach, east,
  jungle), each with 60 measured frame intervals. Overview image inspected and agrees with
  native/Android. No GPU faults; the kernel delta contains unrelated cupsd AppArmor/audit
  messages. This focused run does not requalify the full input/lifecycle
  suite. Web release refreshed: 10,339,183 raw / 3,525,279 gzip bytes.

Hardware qualification remains stopped. Software captures do not qualify the host AMD GPU.

## Spatial habitat follow-up — 2026-10-12

The material-only limitation now has a first three-dimensional implementation in the
existing `NaturalPropLayout` / `NaturalPropRenderer` path. A shared bounded reef field
places 314 non-solid objects on the actual seabed: multi-level staghorn branches with
secondary tips, compound rounded colonies, irregular stacked plate colonies and low
underwater rocks. All four forms have separate near/far geometry, cell/frustum and
138m distance culling, existing hardware instancing and an underwater daylight/depth
response. The combined prop mesh inventory is 3,904 vertices / 109,312 bytes.

`IslandEnvironmentField` also exposes seagrass suitability and a combined aquatic
habitat score. Fish schools choose and remain on suitable reef/seagrass route centres;
sea turtles choose suitable home centres and reject route samples outside the field.
The existing populations and behaviour remain bounded at eight schools / 48 fish and
five turtles.

The first capture was rejected because even angular spacing produced a sparse row and
simple Y-shaped branches. The second was rejected because the camera selected a large
underwater rock. Randomized patch/gap placement, compound coral geometry, secondary
branching and a coral-only underwater target replaced both. Final isolated llvmpipe
evidence `20261012T101426.680059Z-reef-habitat` passes at 1280x720 with 165 colour bins,
24.98 luminance deviation, 128.65 spatial contrast and an empty kernel incident set.
The inspected frame contains branching, rounded and plate silhouettes; runtime reports
51/314 habitat instances, three fish and one turtle in view. World fingerprint:
`4c0ea30b6c5aaac1`.

This is not final reef photorealism or portable acceptance. Native performance
comparison, near/far motion review and Web/Android runtime qualification remain.
