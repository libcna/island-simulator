#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace island
{
    /** @brief Bounded supplementary surface contrast and calm-water breaking region. */
    struct CoastalSurfaceAppearance final
    {
        static constexpr float ReflectionSlopeSoftness = 0.18f;
        static constexpr float FoamFullDepth = 0.35f;
        static constexpr float FoamEndDepth = 1.25f;
        static constexpr float FoamShoreDecayMetres = 4.0f;

        [[nodiscard]] static float ReflectionModulation(float slope) noexcept
        {
            if (!std::isfinite(slope)) return 0;
            return slope / (ReflectionSlopeSoftness + std::abs(slope));
        }

        [[nodiscard]] static float BreakingCoverage(float depth, float shoreDistance) noexcept
        {
            if (!std::isfinite(depth) || !std::isfinite(shoreDistance) || depth <= 0) return 0;
            const float t = std::clamp((depth-FoamFullDepth)/(FoamEndDepth-FoamFullDepth),0.0f,1.0f);
            return (1-t*t*(3-2*t)) * std::exp(-std::abs(shoreDistance)/FoamShoreDecayMetres);
        }
    };

    /** @brief Calm clear-water tuning, uploaded as one vec4 to the portable ocean shader. */
    struct CoastalWaterOptics final
    {
        static constexpr float ExtinctionPerMetre = 0.42f;
        static constexpr float LagoonDecayPerMetre = 0.28f;
        static constexpr float ContactFadeDepth = 0.12f;
        static constexpr float MaximumBodyOpacity = 0.92f;

        [[nodiscard]] static float BodyOpacity(float depth) noexcept
        {
            if (!std::isfinite(depth)) return MaximumBodyOpacity;
            return MaximumBodyOpacity * (1.0f - std::exp(-std::max(depth, 0.0f) * ExtinctionPerMetre));
        }

        [[nodiscard]] static float LagoonWeight(float depth) noexcept
        {
            if (!std::isfinite(depth)) return 0.0f;
            return std::exp(-std::max(depth, 0.0f) * LagoonDecayPerMetre);
        }

        [[nodiscard]] static float ContactCoverage(float depth) noexcept
        {
            if (!std::isfinite(depth)) return 0.0f;
            const float t = std::clamp(depth / ContactFadeDepth, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }
    };

    /** @brief Unlit RGB byte-space sand palette, continuous at the mean waterline and depth knots.
     * Water tint belongs to the ocean, not a second turquoise layer baked into the seabed.
     */
    [[nodiscard]] inline std::array<float, 3> CoastalSandColor(float height) noexcept
    {
        if (!std::isfinite(height)) height = -22.0f;
        constexpr std::array<float, 3> deep{81, 99, 80};
        constexpr std::array<float, 3> submerged{152, 151, 101};
        constexpr std::array<float, 3> wet{186, 174, 118};
        constexpr std::array<float, 3> dry{226, 201, 139};
        const auto blend = [](const auto& a, const auto& b, float t) {
            t = std::clamp(t, 0.0f, 1.0f);
            return std::array<float, 3>{a[0] + (b[0] - a[0]) * t,
                                       a[1] + (b[1] - a[1]) * t,
                                       a[2] + (b[2] - a[2]) * t};
        };
        if (height < -3.0f) return blend(deep, submerged, (height + 22.0f) / 19.0f);
        if (height < 0.0f) return blend(submerged, wet, (height + 3.0f) / 3.0f);
        return blend(wet, dry, height / 1.4f);
    }
}
