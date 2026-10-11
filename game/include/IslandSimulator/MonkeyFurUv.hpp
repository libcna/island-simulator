#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>

namespace island::MonkeyFurUv
{
    // Equal angular density on each half of the circumference. The old cosine
    // projection had zero derivative on broad side-facing surfaces: a narrow
    // strip of hairs became long wood-like bands. Fold at top/bottom instead.
    // Both ends remain inside the source image; seamless tiling is not assumed.
    inline float Around(float radians) noexcept
    {
        return .5f+.8f*std::asin(std::clamp(std::sin(radians),-1.0f,1.0f))
            /std::numbers::pi_v<float>;
    }
    inline float Along(float fraction) noexcept
    {
        return .1f+.8f*std::clamp(fraction,0.0f,1.0f);
    }
}
