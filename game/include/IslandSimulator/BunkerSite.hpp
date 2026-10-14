#pragma once

#include <algorithm>
#include <cmath>

namespace island {

/** Authored inland footprint for the concealed underground research bunker. */
struct BunkerSite final {
    static constexpr float X=92.0f;
    static constexpr float Z=24.0f;
    // The entrance faces north; the descending spine runs south into the second ridge.
    static constexpr float Angle=1.57079632679f;
    static constexpr float MaximumAcross=12.0f;
    static constexpr float MinimumDepth=-7.0f;
    static constexpr float MaximumDepth=39.0f;

    [[nodiscard]] static bool Reserves(float x,float z,float margin=0.0f) noexcept {
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(margin))return false;
        margin=std::max(0.0f,margin);
        const float dx=x-X,dz=z-Z;
        const float across=-std::sin(Angle)*dx+std::cos(Angle)*dz;
        const float depth=-std::cos(Angle)*dx-std::sin(Angle)*dz;
        return std::abs(across)<=MaximumAcross+margin
            && depth>=MinimumDepth-margin&&depth<=MaximumDepth+margin;
    }
};

}
