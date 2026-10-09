#pragma once
#include <array>

namespace island
{
    inline constexpr int CanopyFormCount = 8;

    struct CanopyTrunkProfile
    {
        float height, leanAngle, lean;
    };

    /** @brief Shared local-space crown lobe, used by geometry and fixed-sun light baking. */
    struct CanopyCrownProfile
    {
        float x, y, z, width, height, angle, phase;
    };

    [[nodiscard]] CanopyTrunkProfile MakeCanopyTrunkProfile(int variant);
    // t is normalized height. The bottom 3 local metres stay on the collision-axis centreline.
    [[nodiscard]] std::array<float, 3> SampleCanopyTrunk(int variant, float t);
    // Both mesh tiers and the light bake share all eight lobe anchors and dimensions.
    [[nodiscard]] CanopyCrownProfile MakeCanopyCrownProfile(int variant, int cluster);
}
