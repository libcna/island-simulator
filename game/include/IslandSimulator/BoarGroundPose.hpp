#pragma once

#include "IslandSimulator/BoarPose.hpp"
#include <array>

namespace island
{
    class IslandTerrain;
    class RenderedTerrainSurface;
    struct BoarState;

    /** @brief Renderer-independent contact fit; not world-space stance locking. */
    struct BoarGroundPose final
    {
        BoarJoint origin, right, up, forward;
        float scale;
        float bodyBob;
        // Left rear, left front, right rear, right front, matching geometry emission order.
        std::array<BoarLegPose, 4> legs;
        std::array<float, 4> groundHeights;
        [[nodiscard]] BoarJoint ToWorld(BoarJoint local) const noexcept;
    };

    /** @brief Sample visible terrain under each sole, retaining smooth analytic body orientation.
     * Throws on invalid state or queries outside the rendered mesh; bounded iterative solving.
     */
    [[nodiscard]] BoarGroundPose FitBoarGroundPose(const IslandTerrain& terrain,
                                                  const RenderedTerrainSurface& surface,
                                                  const BoarState& state);
}
