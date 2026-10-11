#pragma once
#include <array>
#include <optional>
#include <vector>

namespace island
{
    inline constexpr int CanopyFormCount = 8;
    [[nodiscard]] inline float CanopyNearLodDistance(float yaw) noexcept
    {
        return 105.0f+24.0f*yaw/6.28318530718f;
    }

    struct CanopyTrunkProfile
    {
        float height, leanAngle, lean;
    };

    /** @brief Shared local-space crown lobe, used by geometry and fixed-sun light baking. */
    struct CanopyCrownProfile
    {
        float x, y, z, width, height, angle, phase;
    };

    // Root flare stays inside the player-blocked footprint: collision radius plus capsule.
    inline constexpr float CanopyTrunkBaseRadius = 0.43f, CanopyTrunkTipRadius = 0.23f;
    inline constexpr float CanopyRootFlare = 0.34f, CanopyRootFlareHeight = 0.42f;

    [[nodiscard]] CanopyTrunkProfile MakeCanopyTrunkProfile(int variant);
    /** @brief Smooth mesh-local radius before irregularity; a function of height, not ring index. */
    [[nodiscard]] float CanopyTrunkRadius(int variant, float localHeight);
    // t is normalized height. The bottom 3 local metres stay on the collision-axis centreline.
    [[nodiscard]] std::array<float, 3> SampleCanopyTrunk(int variant, float t);
    [[nodiscard]] std::vector<float> CanopyTrunkRingHeights(int variant, bool farMesh);
    [[nodiscard]] std::array<float, 3> CanopyTrunkVertex(int variant, float height,
                                                       float angle, bool farMesh,
                                                       std::optional<float> windTime = std::nullopt);
    struct CanopyTrunkContact { std::array<float,3> position, normal; };
    // Mesh-local triangle surface, not a smooth cylinder. Omitted time samples the rest mesh;
    // supplied time deforms its vertices before interpolation, matching the wind shader.
    // The caller must still apply the tree instance transform and select its actual LOD.
    [[nodiscard]] CanopyTrunkContact SampleCanopyTrunkSurface(int variant, float height,
                                                            float angle, bool farMesh = false,
                                                            std::optional<float> windTime = std::nullopt);
    // Both mesh tiers and the light bake share all eight lobe anchors and dimensions.
    [[nodiscard]] CanopyCrownProfile MakeCanopyCrownProfile(int variant, int cluster);
}
