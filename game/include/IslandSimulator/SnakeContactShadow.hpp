#pragma once
#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/ContactShadowPatch.hpp"
namespace island {
    inline constexpr float SnakeShadowSurfaceOffset=0.004f;
    inline constexpr std::size_t SnakeShadowVertexLimit=12000;
    // Soft continuous ribbon, clipped to the rendered mesh triangles. No texture.
    [[nodiscard]] std::vector<ContactPatchVertex> BuildSnakeContactShadow(
        const SnakeState& snake,const RenderedTerrainSurface& surface);
}
