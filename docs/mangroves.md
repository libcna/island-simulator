# Localized mangrove mouth habitat

Status: native first pass integrated and visually reviewed on 2026-10-12. Portable
and performance qualification remain, so the ecosystem ledger stays in progress.

## Placement

`JungleLayout` appends one deterministic colony along the final reach of the shared
`StreamRoute`. Candidates alternate banks and use the route tangent/normal rather
than a separate straight mask. Accepted trees must satisfy all of these constraints:

- 0.4–24m signed distance inland, terrain height -0.05–3.4m, slope at most 0.38
  radians and coastal rockiness at most 0.38;
- shared environment moisture at least 0.30, salt exposure at least 0.06 and
  drainage at most 0.76;
- no reserved route or existing canopy collision. A whole-world regression also
  proves that the resulting default colony does not intersect palm collisions.

The measured default world accepts exactly 15 of 44 bounded candidates. They remain
localized at the stream mouth; natural interior canopy selection cannot choose the
mangrove variant. The complete canopy count is 272.

## Geometry and rendering

Canopy variant 8 uses the existing instanced jungle draw and material pipeline. Its
profile is lower and broader than the eight interior forms. Near geometry adds eight
quadratic tubular stilt roots and three secondary stems; far geometry retains five
roots and two stems. Existing leaf blades, filtered bark detail, canopy lighting,
wind, culling and near/far selection remain shared. A lighter sun-bleached grey bark
keeps the shaded supports readable. Each placement has a collision radius matching
the visible root spread.

The first real render exposed a reversed basis cross-product in the curved support
mesh and startup aborted with `Procedural surface normal is not outward at vertex 0`.
The local frame now follows the renderer's established outward winding, and runtime
mesh validation stays enabled so this class of failure cannot be silently hidden.

## Evidence and remaining work

- `IslandCoreTests` passes deterministic population, habitat, scale, route,
  drainage, root-collision and palm-clearance assertions.
- `20261012T095538.769586Z-mangroves` is an isolated llvmpipe capture: 1280x720,
  410 colour bins, luminance deviation 49.05, spatial contrast 204.42, empty kernel
  incident set. The image was inspected for roots, stems, crowns, bark and context.
- The world report fingerprint is `32d95c2a1819702e`.

Still required: Web and Android runtime checks, a moving near/far transition review,
and performance comparison. Those are deliberately not implied by the native image.
