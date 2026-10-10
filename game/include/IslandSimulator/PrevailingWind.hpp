#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island::wind
{
    // One fixed trade wind blowing from the east (+x) toward the west, shared by every
    // system that reacts to it. The vector is the direction the wind blows TOWARD.
    inline constexpr float DirectionX = -1.0f;
    inline constexpr float DirectionZ = 0.0f;
    // Exposure fades inland from the coast and is strongest on the windward (east) shore.
    inline constexpr float CoastFadeStart = 12.0f, CoastFadeEnd = 75.0f;
    inline constexpr float LeewardFloor = 0.25f;
    inline constexpr float PalmMaximumLeanRadians = 0.17f; // about ten degrees

    inline float Smooth(const float a, const float b, const float x)
    {
        const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    /** @brief 0 (sheltered interior) to 1 (exposed windward coast); deterministic. */
    inline float Exposure(const float x, const float z, const float shoreDistance)
    {
        if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(shoreDistance))
            throw std::invalid_argument("Wind exposure requires finite coordinates");
        const float radius = std::hypot(x, z);
        // Windward means facing INTO the wind: the island's east side for a westward wind.
        const float windward = radius > 0.001f
            ? std::clamp((-(x * DirectionX + z * DirectionZ)) / radius, -1.0f, 1.0f) : 0.0f;
        const float side = LeewardFloor + (1.0f - LeewardFloor) * Smooth(-0.6f, 0.8f, windward);
        const float coastal = 1.0f - Smooth(CoastFadeStart, CoastFadeEnd, std::max(shoreDistance, 0.0f));
        return side * coastal;
    }

    inline float PalmLeanRadians(const float exposure)
    {
        if (!std::isfinite(exposure) || exposure < 0.0f || exposure > 1.0f)
            throw std::invalid_argument("Wind exposure must be normalized");
        return PalmMaximumLeanRadians * exposure;
    }
}
