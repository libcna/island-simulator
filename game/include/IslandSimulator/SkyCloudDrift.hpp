#pragma once

#include "IslandSimulator/PrevailingWind.hpp"

#include <cmath>
#include <numbers>

namespace island::sky_cloud
{
    // Roughly 1.2 degrees per minute: visible over a walk, still calm moment to moment.
    inline constexpr float AngularSpeedRadiansPerSecond=0.00035f;

    [[nodiscard]] inline float DriftAngle(const float seconds) noexcept
    {
        if(!std::isfinite(seconds)||seconds<=0.0f)return 0.0f;
        constexpr float turn=2.0f*std::numbers::pi_v<float>;
        // DirectionX couples the phase to the shared east-to-west trade wind. A complete
        // turn is shader-periodic, so the long-session wrap has no visible discontinuity.
        const float direction=wind::DirectionX<0.0f?1.0f:-1.0f;
        return std::fmod(seconds*AngularSpeedRadiansPerSecond*direction,turn);
    }
}
