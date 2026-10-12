#pragma once

#include "IslandSimulator/PrevailingWind.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    /** @brief Bounded windward/leeward response shared by coastal visuals and ambience. */
    class CoastalWindResponse final
    {
    public:
        [[nodiscard]] static float Exposure(const float x,const float z,
                                            const float signedShoreDistance) noexcept
        {
            if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(signedShoreDistance))
                return .5f;
            const float radius=std::hypot(x,z);
            const float windward=radius>.001f?std::clamp(
                -(x*wind::DirectionX+z*wind::DirectionZ)/radius,-1.0f,1.0f):0.0f;
            const float side=wind::LeewardFloor+(1.0f-wind::LeewardFloor)
                *wind::Smooth(-.6f,.8f,windward);
            const float coastal=1.0f-wind::Smooth(wind::CoastFadeStart,105.0f,
                std::abs(signedShoreDistance));
            return std::clamp(.5f+(side-.5f)*coastal,0.0f,1.0f);
        }

        [[nodiscard]] static float WaveScale(const float exposure) noexcept
        { return .78f+.44f*Safe(exposure); }
        [[nodiscard]] static float FoamScale(const float exposure) noexcept
        { return .75f+.50f*Safe(exposure); }
        [[nodiscard]] static float AmbientScale(const float exposure) noexcept
        { return .82f+.36f*Safe(exposure); }

    private:
        [[nodiscard]] static float Safe(const float exposure) noexcept
        { return std::isfinite(exposure)?std::clamp(exposure,0.0f,1.0f):.5f; }
    };
}
