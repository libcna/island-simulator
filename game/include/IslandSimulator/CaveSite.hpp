#pragma once
#include "IslandSimulator/IslandTerrain.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace island {
// Lightweight authored footprint for placement queries: no mesh allocation or terrain recursion.
struct CaveSite final {
    static constexpr float Angle=.56f;
    static constexpr float Inset=38.0f;
    static constexpr float Length=20.0f;
    static constexpr float OuterLength=22.0f;
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
        return std::abs(across)<6+margin&&depth>-6-margin&&depth<OuterLength+3+margin;
    }
};
}
