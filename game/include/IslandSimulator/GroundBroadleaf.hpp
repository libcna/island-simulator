#pragma once

#include "IslandSimulator/LeafBladeProfile.hpp"

namespace island
{
    struct GroundBroadleaf
    {
        int source;
        float angle, height, reach, width, phase, pigment;
        bool operator==(const GroundBroadleaf&) const = default;
    };

    [[nodiscard]] GroundBroadleaf GroundBroadleafForTier(int index, bool far);
    // Positions include the petiole endpoint; across/height/along are plant-local metres.
    [[nodiscard]] LeafBladePoint SampleGroundBroadleaf(const GroundBroadleaf& leaf, float t, float side);
    [[nodiscard]] LeafBladePoint GroundBroadleafNormal(const GroundBroadleaf& leaf, float t, float side);
}
