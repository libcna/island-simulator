#pragma once
#include "IslandSimulator/IslandTerrain.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace island {
// Lightweight authored footprint for placement queries: no mesh allocation or terrain recursion.
struct CaveSite final {
    // The entrance is cut into the south-eastern main-ridge slope.  Twenty metres
    // inward the unmodified mountain rises well above the shell crown, so the cave
    // reads as excavated rock rather than a freestanding coastal tunnel.
    static constexpr float Angle=-.60f;
    static constexpr float Inset=100.0f;
    static constexpr float Length=20.0f;
    static constexpr float OuterLength=22.0f;
    // The compact outer rings remain below this conservative width. Reserve the whole
    // emitted rock shell and its terrain shoulders, not just the narrower cavity.
    static constexpr float HalfWidth=8.0f;
    [[nodiscard]] static std::array<float,2> Centre() noexcept {
        static const std::array<float,2> result=[] {
            const float radius=IslandTerrain{}.ShoreRadius(Angle)-Inset;
            return std::array{std::cos(Angle)*radius,std::sin(Angle)*radius};
        }();
        return result;
    }
    [[nodiscard]] static bool Reserves(float x,float z,float margin=0) noexcept {
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(margin))return false;
        margin=std::max(0.0f,margin);
        const auto centre=Centre();const float dx=x-centre[0],dz=z-centre[1];
        const float across=-std::sin(Angle)*dx+std::cos(Angle)*dz;
        const float depth=-std::cos(Angle)*dx-std::sin(Angle)*dz;
        return std::abs(across)<HalfWidth+margin&&depth>-6-margin&&depth<OuterLength+3+margin;
    }
};
}
