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
    // The monkey's complete textured surfaces can hide one discontinuity on the rear or
    // underside. A monotonic angular window avoids the mirrored fold that otherwise turns
    // directional hairs into a conspicuous ladder along the visible crown and dorsal line.
    inline float AroundHiddenSeam(float radians) noexcept
    {
        constexpr float twoPi=2.0f*std::numbers::pi_v<float>;
        float wrapped=std::fmod(radians+std::numbers::pi_v<float>*.5f,twoPi);
        if(wrapped<0.0f) wrapped+=twoPi;
        return .1f+.8f*wrapped/twoPi;
    }
    inline float Along(float fraction) noexcept
    {
        return .1f+.8f*std::clamp(fraction,0.0f,1.0f);
    }
}
