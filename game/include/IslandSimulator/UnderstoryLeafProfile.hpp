#pragma once

#include "IslandSimulator/LeafBladeProfile.hpp"
#include <array>

namespace island
{
    enum class UnderstoryBladeShape { Narrow, Paddle, Lance };
    inline constexpr std::array<float,5> BroadleafColumns{-1,-0.5f,0,0.5f,1};

    struct UnderstoryLeafProfile
    {
        int source;
        float angle, length, width, lift, phase;
        float bow, tipDrop, edgeCurl, petiole, pigment;
        UnderstoryBladeShape shape = UnderstoryBladeShape::Narrow;
        bool operator==(const UnderstoryLeafProfile&) const = default;
    };

    [[nodiscard]] int UnderstoryLeafCount(int variant, bool far);
    /** @brief Far leaves retain actual near identities, including posture, pigment and wind. */
    [[nodiscard]] UnderstoryLeafProfile UnderstoryLeaf(int variant, int index, bool far);
    [[nodiscard]] LeafBladePoint SampleUnderstoryLeaf(const UnderstoryLeafProfile& leaf, float t, float side);
    [[nodiscard]] LeafBladePoint UnderstoryLeafNormal(const UnderstoryLeafProfile& leaf, float t, float side);

    inline constexpr int FernPairCount = 7;
    // Identical knots in both tiers keep leaflet roots on the rendered rachis, not merely
    // on an analytic curve that coarse stem triangles fail to follow.
    inline constexpr std::array<float,9> FernRachisKnots{0,0.2f,0.3f,0.4f,0.5f,0.6f,0.7f,0.8f,1};
    struct FernLeafletProfile
    {
        float t, angle, length, width, pitch, roll, droop;
        bool operator==(const FernLeafletProfile&) const = default;
    };
    [[nodiscard]] FernLeafletProfile FernLeaflet(const UnderstoryLeafProfile& frond, int pair, int side);
    [[nodiscard]] bool RetainFernPair(int pair, bool far);
    [[nodiscard]] float FernWindWeight(float attachmentT, float along);
}
