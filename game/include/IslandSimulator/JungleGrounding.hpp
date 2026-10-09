#pragma once

#include "IslandSimulator/RenderedTerrainSurface.hpp"

namespace island
{
    // Rendering-only copy: retain deterministic layout/collision data and all plant
    // identities. The root origin follows the exact drawn triangles, not the analytic
    // collision height. This does not drape off-centre litter or broad root footprints.
    template<typename Placement>
    [[nodiscard]] Placement GroundJunglePlacement(
        Placement placement, const RenderedTerrainSurface& surface)
    {
        placement.y = surface.Sample(placement.x, placement.z).height;
        return placement;
    }
}
