# Shared coastal environment field — implementation design

Status: design only, 2026-10-11. No field or coast-type integration is implemented yet.
This is the next step after `coastal-silhouette.md`, covering macro-geography items 2–3.

## Dependency order

1. A renderer-independent coastal profile describes the authored bay/headland lobes, local
   shore orientation and east-trade-wind exposure. It must not query terrain height.
2. `IslandTerrain::Height` uses that profile for coastal relief: gentle broad leeward beach,
   thin-soil rocky windward headlands and a short cliff segment away from the primary route.
3. `IslandEnvironmentField` samples the completed terrain and profile, deriving normalized
   moisture, salt exposure, wind exposure, wave exposure, soil depth, rockiness, drainage,
   unoccluded sun exposure and reef suitability. Existing ground-cover patch noise is an input.
4. Vegetation and prop placement consume the abiotic field. Only after placement does the
   existing canopy-light field add tree shade; shade must not feed back into initial tree
   placement or create a terrain/vegetation construction cycle.
5. Rendering consumes the same sampled field, not a separately authored GLSL coast mask.

The radial signed-shore-distance approximation remains useful for broad support bounds, but
must no longer decide every biome. Estimate local shoreline orientation from neighbouring
shore points rather than assuming every shore normal is radial; this matters inside the bay.

## First integrated milestone

Implement coastal rockiness and beach suitability through the entire chain before migrating
every inland biome. Rock relief, terrain surface/material and palm/canopy/ground-cover rejection
must agree. The western bay and leeward beach retain gentle swimming access; preserve the
primary southern spawn/trail and verify independent shore transects. Do not reserve all coast
types as walkable: an authored cliff may block a transect, but its bounds and alternate access
must be explicitly tested instead of weakening all slope/traversal assertions.

Current GPU field inspection found a reuse opportunity: `CanopyLighting` uploads RGBA8 with
red/blue duplicated transmission, green ground-cover patch, alpha 255. All shader consumers
currently read only `.r` / `.rg` through `CanopyLighting.hpp`. Blue and alpha can therefore
carry coastal rockiness and beach suitability without a new sampler, texture or vertex stride,
provided tests prove red/green behaviour unchanged. CPU placement should sample the same
quantized bilinear field and texel-centre convention, with declared out-of-domain defaults.
Keep full abiotic values CPU-side; do not squeeze nine unrelated fields into these two channels.

## Acceptance evidence

- Bounded finite fields, deterministic samples, polar-seam continuity and invalid-input policy.
- Western beach/bay versus eastern headland contrasts measured at explicit sample stations;
  distinct habitats at comparable shore distances, not just renamed concentric bands.
- Field upload/channel tests and CPU/GLSL sampling agreement; generated texture bytes and
  terrain vertex count unchanged if reusing the existing texture.
- Population budgets, roots/contact, reserved-path footprints, marine habitat and swimming
  remain valid. Update exact inventories from a measured world report, not guessed counts.
- Side-by-side native `bay`, `overview`, `beach`, headland close view and reef inspection.
  Add a headland camera if existing views cannot expose the intended cliff/material change.
- Contained Android and Web views plus relevant traversal checks; report Android lifecycle
  separately while its existing qualification failures remain unresolved.

Later migrations cover moisture-driven forest/ferns, drainage and wetland pockets, shrub
wind shaping, directional foam/waves and habitat-driven reef geometry. They remain backlog,
not implied results of introducing an environment-field class.
