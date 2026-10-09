#pragma once

#include <string>

namespace island::GrassMotion
{
    inline constexpr float MinimumStature = 0.84f;
    inline constexpr float MaximumStature = 1.16f;
    inline constexpr float HeightExponent = 1.55f;
    inline constexpr float MinimumPush = 0.12f;
    inline constexpr float MaximumPush = 0.42f;
    inline constexpr float MinimumLowering = 0.08f;
    inline constexpr float MaximumLowering = 0.28f;
    inline constexpr float BroadWind = 0.085f;
    inline constexpr float DetailWind = 0.026f;
    inline constexpr float MinimumWindGain = 0.92f;
    inline constexpr float MaximumWindGain = 1.08f;
    inline constexpr float WindZ = 0.46f;
    inline constexpr float WindLowering = 0.055f;
    inline constexpr float Padding = 0.01f;
    inline constexpr float NormalFloor = 0.001f;

    struct Bounds
    {
        float horizontalRadius;
        float minimumY;
        float maximumY;
    };

    // Bounds relative to the anchor. Mesh radius includes all LOD vertices and seed stems;
    // mesh vertices must be above the local ground plane. Also covers the unanimated fallback.
    [[nodiscard]] Bounds CalculateBounds(float meshRadius, float meshHeight, float heightScale,
                                         float spreadScale, float normalX, float normalZ);
    [[nodiscard]] std::string ShaderConstants();
}
