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

## Shallow-scene completion — 2026-10-15

The generated near-surface overlay now includes sparse deterministic suspended motes without
changing its 160 by 90 allocation or single conditional draw. Close shallow sandy channels also
receive two footprint-filtered shell-fragment colours from the existing terrain material; meadow,
reef, depth and distance masks prevent a uniform scatter. Together with the spatial habitat below,
the existing sand ripples, fish and shared aquatic lighting, this closes macro-geography item 16.
Native inspected evidence is
`build-consumer/software-captures/20261014T233503.630466Z-reef-habitat`; the current browser coast
suite and ARM64 assemble/lint also pass. Full volumetric underwater fog and refractive water remain
explicitly outside this bounded scene treatment.

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

## Macro-zone follow-up — 2026-10-13

The fixed `reef` camera no longer places the largest emergent outcrop across almost the entire
frame. It now uses an elevated, wider beach-side composition that exposes the near lagoon, the
mid-shelf habitat field and the outer shelf in one view. This makes it possible to inspect the
reef as a coastal system rather than merely qualifying one rock.

The existing one-draw terrain shader now gives that composition four bounded macro cues. A pale
6–48m inner-lagoon envelope retains the existing radial sand channels; the original noise patches
are modulated by the same five/eleven-lobe angular habitat field used by CPU coral and fauna
placement; a broken 62–86m ochre crest separates the inner shelf; and deep 78m+ seabed receives a
restrained blue-green drop-off darkening. These remain substrate colours below the independently
transparent ocean, not a second water layer. No mesh, texture, uniform or draw call was added.

`ReefMacroAppearance` mirrors the broad lagoon/crest/drop-off envelopes on the CPU. Core tests pin
their separated peak regions and conservative non-finite handling. Inspected native llvmpipe
evidence `20261013T074352.699582Z-reef` passes at 1280x720 with 209 colour bins, 23.55 luminance
deviation and 123.17 spatial contrast. The companion underwater habitat view
`20261013T074428.276690Z-reef-habitat` passes with 96 bins, 26.05 deviation and 128.36 contrast;
the stronger macro field does not introduce an underwater seam or colour discontinuity.

Focused Chrome/SwiftShader evidence `20261013T074740.113865Z-chrome-coast` passes shipwreck,
water, west and reef with 120 finite intervals per view, GL error zero, intact contexts and an
empty new-kernel-fault delta. Both time-separated elevated reef images were inspected. Android,
physical performance, crest foam coupling and a higher-detail coral material remain open.

## Staghorn form follow-up — 2026-10-13

The near branching form no longer uses one heavy central trunk with an evenly radial tier of
branches and bulbous caps, which read as a pruned brown tree underwater. Eleven thinner upright
stems now rise directly from a low colony base with golden-angle spacing, unequal reach and
height. Each near stem gains one offset side fork, while both main and secondary branches end in
small lighter growing tips. The far LOD keeps five stems and the same overall character. This
changes only existing CPU-generated geometry: population, placement, culling, instancing and
draw count are unchanged.

Inspected native llvmpipe evidence `20261013T080430.507685Z-reef-habitat` passes at 1280x720 with
101 colour bins, 26.27 luminance deviation and 131.27 spatial contrast. The isolated target reads
as an upright terracotta staghorn colony rather than a trunk and crown. Focused Chrome/SwiftShader
evidence `20261013T080800.279649Z-chrome-coast` then passes all four coast views with 120 finite
intervals each, the new form diagnostic present, GL error zero, intact contexts and no new kernel
fault. Android qualification and a coral-specific surface material remain open.

## Crest-breaker coupling — 2026-10-13

The existing ocean draw now evaluates the same 62–86m offshore crest envelope and five/eleven-
lobe angular habitat field as the seabed. A derivative-filtered, slowly moving breaker carrier,
the existing analytic surface slope and coastal wind exposure modulate restrained foam inside
that intersection. This creates separated transient patches rather than a static white ring;
shore foam, player ripples and transition splashes retain their existing paths. No vertex,
texture, uniform or draw call was added.

The reef QA camera now scans the actual terrain for the strongest valid crest sample instead of
inheriting an arbitrary offshore-rock direction. The first revised camera attempt exposed a real
zero-weight sector and was rejected. The accepted camera targets weight 1 at 69m offshore from an
elevated outer-shelf position, looking inward across habitat, lagoon and coast. Inspected native
llvmpipe evidence `20261013T082913.130348Z-reef` passes at 1280x720 with 282 colour bins, 39.72
luminance deviation and 173.63 spatial contrast; 33/314 habitat instances and two reef outcrops
remain in view. The image shows a broken pale breaker zone without obscuring the seabed.

Focused Chrome/SwiftShader evidence `20261013T084825.683384Z-chrome-coast` passes shipwreck,
water, west and the new outer-shelf reef composition with 120 finite intervals each, GL error
zero, intact contexts and an empty kernel-fault delta. Both time-separated reef frames were
inspected: the breaker carrier and fish positions move while the substrate field remains fixed.
Android qualification and physical performance remain open.

## Living staghorn base and coral surface — 2026-10-16

The latest water/reef comparison accepted the surface optics but selected the foreground staghorn
as the weakest underwater object: its silhouette was established, yet the uniform rusty pigment
and single basal mound still read as a dead shrub. The shared coral material now applies
footprint-filtered broad living pigment and small near-only polyp flecks before depth tint and
diffuse underwater light. Staghorn growing tips use a deliberately pale live edge. Its near mesh
also carries five low, differently pigmented encrusting coral and sponge mounds around the base.

The colonies add 125 vertices and 200 triangles only to near variant zero. The 314-placement
population, far LOD, 46/138 m LOD and culling limits, shader inputs, buffers and draws are unchanged.
Native before evidence `20261016T042943.312246Z-reef-habitat` passed with 120 colour bins; accepted
current evidence `20261016T043419.469049Z-reef-habitat` passes at 1280x720 with 161 bins, 25.76
luminance deviation, 137.98 spatial contrast and an empty kernel delta. The view submits 51/314
habitat objects and retains 15 total natural-prop draws.

Core and capture gates pass. Focused Chrome/SwiftShader reef-adjacent evidence
`20261016T044815.059085Z-chrome-fish` contains both new material diagnostics and passes 60 finite
positive intervals, GL error zero, a live context and kernel `[]`. The broader four-scene
`20261016T043640.536564Z-chrome-coast` attempt exceeded its software-capture time limit and is
rejected rather than counted as evidence.
