#pragma once
#include "IslandSimulator/SpiderColony.hpp"
#include "IslandSimulator/ContactShadowPatch.hpp"
namespace island {
inline constexpr std::size_t SpiderShadowVertexLimit=4096;
// Compact grounding approximation, not a directional silhouette or a leg shadow map.
[[nodiscard]] std::vector<ContactPatchVertex> BuildSpiderContactShadow(
    const SpiderState& spider,const IslandTerrain& terrain,const RenderedTerrainSurface& surface);
}
