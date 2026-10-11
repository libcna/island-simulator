#pragma once

#include <algorithm>
#include <cmath>

namespace island
{
    struct CoastalEnvironmentSample
    {
        float rockiness = 0.0f;
        float beachSuitability = 0.0f;
    };

    /** @brief Shared abiotic coastal subset; inland moisture/light fields remain separate. */
    class IslandEnvironmentField final
    {
    public:
        [[nodiscard]] static float Smooth(float low, float high, float value) noexcept
        {
            const float t = std::clamp((value - low) / (high - low), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        /** @brief Height-independent geology: safe to use while constructing terrain. */
        [[nodiscard]] static float CoastalRockPotential(float angle) noexcept
        {
            if (!std::isfinite(angle)) return 0.0f;
            const auto lobe = [angle](float centre, float width) {
                return std::exp((std::cos(angle - centre) - 1.0f) / (width * width));
            };
            return std::clamp(0.98f * lobe(0.35f, 0.23f)
                + 0.90f * lobe(2.48f, 0.20f) + 0.82f * lobe(-2.48f, 0.20f), 0.0f, 1.0f);
        }

        /** @brief Finite normalized coastal material/placement controls; no terrain recursion. */
        [[nodiscard]] static CoastalEnvironmentSample SampleCoast(
            float x, float z, float signedShoreDistance) noexcept
        {
            if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(signedShoreDistance))
                return {};
            const float regional = CoastalRockPotential(std::atan2(z, x));
            const float band = Smooth(-8.0f, -1.0f, signedShoreDistance)
                * (1.0f - Smooth(25.0f, 65.0f, signedShoreDistance));
            return {regional * band, (1.0f - regional)
                * (1.0f - Smooth(28.0f, 58.0f, signedShoreDistance))};
        }
    };
}
