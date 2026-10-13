#pragma once

#include <algorithm>
#include <cmath>

namespace island
{
    /** @brief Height-independent authored centreline shared by stream consumers. */
    class StreamRoute final
    {
    public:
        static constexpr float SourceX = -65.0f;
        static constexpr float NominalLength = 98.0f;

        [[nodiscard]] static float Along(float x) noexcept
        {
            return SourceX - x;
        }

        [[nodiscard]] static float CentreZ(float x) noexcept
        {
            if (!std::isfinite(x)) return 0.0f;
            const float along = Along(x);
            const float sourceFade = Smooth(0.0f, 9.0f, along);
            const float mouthFade = 1.0f - Smooth(86.0f, NominalLength, along);
            const float envelope = sourceFade * mouthFade;
            const float meander = 1.55f * std::sin(along * .117f)
                + .62f * std::sin(along * .253f + .7f);
            return -55.0f - .23f * x + envelope * meander;
        }

        /** @brief Shared, gently irregular channel half-width in metres. */
        [[nodiscard]] static float HalfWidth(float x) noexcept
        {
            if (!std::isfinite(x)) return 0.45f;
            const float along = Along(x);
            const float t = std::clamp(along / NominalLength, 0.0f, 1.0f);
            const float shaped = t * t * (3.0f - 2.0f * t);
            const float base = std::lerp(0.45f, 1.5f, shaped);
            // Two incommensurate low-frequency terms create pools and pinches without
            // changing either endpoint or introducing sub-mesh-scale saw teeth.
            const float envelope = Smooth(4.0f, 12.0f, along)
                * (1.0f - Smooth(88.0f, NominalLength, along));
            const float variation = envelope
                * (0.16f * std::sin(along * 0.19f + 0.5f)
                    + 0.07f * std::sin(along * 0.43f - 1.1f));
            return base * (1.0f + variation);
        }

    private:
        [[nodiscard]] static float Smooth(float low, float high, float value) noexcept
        {
            const float t = std::clamp((value - low) / (high - low), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }
    };
}
