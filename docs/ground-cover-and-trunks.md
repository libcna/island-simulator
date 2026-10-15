# Nonuniform ground cover and tree-trunk presentation

## Shade-facing trunk moss completion — 2026-10-15

The existing damp lower-bark stain previously varied only by height and filtered procedural noise,
so equal-height bark received the same average moss on the fixed-sun and shaded faces. The shared
trunk material now also consumes the signed surface-to-sun term. Its bounded multiplier transitions
from 1.18 on the back/shaded side to 0.55 on the sun face; height envelope, moss colour, lichen,
contact occlusion, draw count, geometry and instance data remain unchanged. The CPU albedo reference
checks that shaded bark has the stronger green ratio and rejects non-finite or out-of-range light
directions. Native llvmpipe jungle evidence
`build-consumer/software-captures/20261014T223900.550238Z-jungle` passes and was inspected: moss
remains a restrained damp-base cue rather than a uniform green coating. Chrome/SwiftShader
`build/browser-software/20261014T224129.570260Z-chrome-canopy-light` passes the matching inspected
jungle view with 60 positive finite intervals, GL error zero, a live context and clean kernel
delta. ARM64 assemble/lint passes through the two-worker wrapper using the locked Sharp Runtime
revision (24,025,427-byte APK, SHA-256
`6f6b1fc413a2e1d6baaeee199232989ab9729ff5cf9197515fc5e77520c463ca`) without starting an emulator
or touching KVM. The complete 63-case non-display native suite passes.

Pass of 2026-10-10, following the operator's direction that the island must look noticeably
closer to reality. All runtime evidence is from contained software renderers (llvmpipe, and
SwiftShader/KVM where noted in `PLAN.md`); the AMD hardware stop latch is unchanged and nothing
here is hardware qualification.

## What the first-person images showed

- Canopy trunks were saturated orange-brown, eight-sided with a visible crease at the 1.5 m
  ring, fully sunlit even directly under their own crowns, and stood on flat triangular fins.
- The most prominent near trunks in the grass view were coconut palms: 0.72 m thick at the
  base, orange, facet-shaded from screen-space derivatives with a fixed sun side baked into vertex
  colours (wrong after instance yaw), with coarse 0.85 m ring bands and paper-thin root wedges.
- Grass was a 2.15 m jittered lattice with 48-82% acceptance almost everywhere: evenly sprinkled
  emerald tufts on uniform brown humus. A probe of the baked canopy field showed that 91.3% of
  inland ground is fully sunlit (transmission above 0.9), so the earlier canopy-only shading
  could never create the missing variation.

## Trunks

- `CanopyTrunkRadius` (core, tested): nine deterministic natural/mangrove taper families between the
  shared conservative root and crown envelopes, each with a smooth root flare of +34% decaying
  over 0.42 m. Tests prove a monotone taper and that the flare plus 9.9% near-mesh irregularity
  stays inside the player-blocked footprint (`0.43 * scale + 0.34 m` capsule) at both authored
  scale limits.
- Near trunks: 12 sides (was 8) and rings concentrated on the flare; irregularity is a smooth
  function of height and angle, not ring index, so ring boundaries no longer crease. Far trunks
  keep five sides and their ring heights and use the same radius function.
- Buttresses are rounded five-column ridges that rise out of the flare and decay into the soil
  (25 vertices / 32 triangles each) instead of six-vertex wedges. Tip reach is unchanged.
- Branches use six sides (was four). Mesh inventory: near variant 0 is 4,818 vertices / 4,640
  triangles (was 4,620 / 4,328), far 1,071 / 1,120 (was 1,029 / 1,064); no new draw or texture.
- Bark albedo is neutral grey-brown per form (pale fig-like to darker fissured) with an
  instance-stable 0.90-1.08 weathering factor. Tagged wood now always receives the existing bark
  detail; the old red/green colour key would have dropped it for grey bark.
- `TrunkSurfaceMaterial.hpp` (CPU reference + GLSL, tested): contact occlusion to 0.64 at the
  soil over 0.9 m, a damp moss/soil stain with a noise-broken edge between about 0.4 and 2.1 m,
  and pale crustose lichen patches between 1.4 and 7.5 m. All noise is footprint-filtered and the
  lichen converges to its measured 17.97% area mean when unresolved.
- Wood in every jungle draw now samples the shared fixed-sun canopy field: direct sun is
  attenuated by transmission, fill/sky light is cooled and reduced in shade. Upper trunks under
  their own crowns are therefore shaded while lower trunks may stand in sun. Crown foliage still
  does not shadow itself.
- Canopy shade ambient is darker and cooler: `CanopyAmbientTint` 0.46/0.53/0.62 (was
  0.56/0.64/0.72). This applies to every canopy-field receiver, including boars and monkeys.

## Palms

- Palms use a 36-byte custom vertex with a normal in the custom-shader path; the stock fallback
  keeps its previous format and baked colours. A unit normal tags trunk/root surfaces; fronds keep
  zero normals and their two-sided facet response.
- Stem: base radius 0.21 m tapering to 0.14 m with a +55% bole flare over 0.38 m (was 0.36 to
  0.17 m), ten sides near / six far, smooth analytic normals tilted by the flare slope.
- Lighting is the shared one-sided wood daylight, applied to near and far tiers, with contact
  occlusion and a slightly darker bole. Bark is weathered grey below and browner toward the crown.
- Leaf-scar rings: 8.5 cycles per local metre, tilted and irregular, narrow, 13% contrast and
  footprint-filtered to their 11.5% area mean (was 1.18 cycles/m, unfiltered).
- Eight low rounded root ridges (0.11-0.17 m high) replace five tall wedges.

## Ground cover

- `GroundCoverField` (core, tested): a deterministic three-octave value-noise patch field (38,
  13 and 5.5 m cells) on the canopy-light texel grid, contrast-shaped by `smoothstep(0.34, 0.66)`.
  Cached lattice corners make the bake bit-identical to the direct reference; 73 ms at -O2
  (about 350-400 ms in the Debug build).
- `GroundCover::Density` (CPU + generated GLSL): shore gate 18-34 m, slope gate 0.30-0.49 rad,
  light response 0.18 + 0.82 * smoothstep(0.12, 0.80, transmission), patch floor 0.10.
- Grass placement uses `0.05 + 0.95 * cover` per candidate site instead of the old broad-cluster
  rule, and rich ground (cover above 0.5) adds at most one companion cluster 0.85-1.2 m away. The
  route rule and all clearing tint identities are unchanged. Population 20,523 clusters (was
  18,556, +10.6%). A regression test requires rich ground to carry more than four times the tufts
  per site of poor or shaded ground.
- The patch field is uploaded in the previously unused green channel of the existing 1024x1024
  canopy-light RGBA8 texture: no new texture, sampler or GPU memory. The CPU light and cover
  fields are now built once with the world in `IslandGame` and shared with grass placement and
  `CanopyLighting`, which only uploads.
- The terrain shader evaluates the same density and re-hues the detailed substrate toward living
  turf (olive inland, warmer toward the coast and in the east clearing, cooler in the west
  clearing) while preserving its luminance detail, adds footprint-filtered turf clumps, and
  reduces the litter image and fallen leaves under dense turf. Routes stay earthy. The turf tint
  persists beyond the grass draw distance, so distant interior ground is patchy olive and brown
  instead of uniform brown.
- Grass blades are yellow-green (base 64/120/40, was emerald 40/124/38).
- The `grass` QA viewpoint now requires cover of at least 0.6 at the selected patch, so it shows
  a representative sward instead of an isolated tuft on bare humus.

## Costs and measured views

| View (llvmpipe, 1280x720) | Grass visible / near | Draws | Notes |
|---|---|---|---|
| `jungle`, same camera before/after | 3,746 / 857 -> 3,596 / 1,074 | 24 -> 24 | redistribution, +25% near |
| `grass`, new representative camera | 4,772 / 1,580 | 24 | dense sward; old camera had 957 near |

A dense-sward camera can submit roughly 65% more near-tier grass geometry than the previous
grass QA camera (about 1.0 M versus 0.61 M near triangles). Draw counts did not increase. This is
a software-renderer observation only; physical-GPU and mobile frame cost remain unmeasured.

## Portability note

Mesa's GLSL 330 compiler accepted a function parameter named `patch`; ANGLE (WebGL 2 / GLSL ES
3.00) rejects it as a reserved word, which sent the browser terrain to its stock-effect fallback.
The native llvmpipe captures could not reveal this. `CoreTests` now scans the generated
`GroundCover` and `TrunkSurface` GLSL for GLSL ES reserved identifiers; hand-written shader bodies
are still only proven by the contained browser and Android runs.

## Platform evidence (2026-10-10)

| Platform | Run | Result |
|---|---|---|
| Native llvmpipe | `build/software-captures/20261010T130435.552471Z-grass`, `130442.064431Z-jungle`, `130447.989273Z-overview`, `131112.797097Z-litter` | pass, images inspected |
| Chrome/SwiftShader | `build/browser-software/20261010T131626.070992Z-chrome-litter`, `132225.050061Z-chrome-grass-motion` | pass; first litter run `130857Z` failed on the reserved GLSL word, two motion runs failed on the F7 pulse (see `browser-input-ownership.md`) |
| Android KVM/SwiftShader x86_64 | `build/android-software/20261010T133014.813550Z-grass` | pass; `132633Z` failed on a stale smoke-script mesh pin |

Rolling packages: Web 10,299,978 raw / 3,512,848 gzip bytes; x86_64 APK 20,382,587 bytes
(`14f98e55ccb5c54b9f29bffca5ef738e0e4a9ba5387e041d2e2bf152e2b3133d`); ARM64 APK 20,475,196
bytes (`aef3a8309955a15d81b309ac4a3cca35c6bde397820290631a488b115dda2d9d`), runtime untested.

## Remaining limitations

- The forest is still sparse (281 crowns) and reads as a park from above; crown masses, emergent
  trees and terrain macro-geography are in the 2026-10-10 backlog in `PLAN.md`.
- Grass tufts are still the same three cluster meshes; the turf undercoat is a 2D material, not
  geometry, and fine shimmer/LOD stability under motion is not established by still captures.
- Canopy and palm crowns, frond silhouettes and foliage colours remain stylized.
