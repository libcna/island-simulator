#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace island::leaf_surface
{
    inline constexpr int TextureSize=256;
    inline constexpr std::size_t FullMipBytes=349524;
    inline constexpr float Strength=0.68f, MinimumResponse=0.60f, MaximumResponse=1.70f;
    using Rgb=std::array<float,3>;

    inline void ValidateMean(const Rgb& mean)
    {
        for (const float channel:mean)
            if (!std::isfinite(channel) || channel<8.0f/255.0f || channel>1)
                throw std::invalid_argument("Leaf image mean must have finite non-dark RGB channels");
    }
    [[nodiscard]] inline Rgb Modulation(const Rgb& sample,const Rgb& mean)
    {
        ValidateMean(mean);
        Rgb result{};
        for (std::size_t c=0;c<result.size();++c) {
            if (!std::isfinite(sample[c]) || sample[c]<0 || sample[c]>1)
                throw std::invalid_argument("Leaf image sample must be normalized finite RGB");
            result[c]=std::lerp(1.0f,std::clamp(sample[c]/mean[c],MinimumResponse,MaximumResponse),Strength);
        }
        return result;
    }
    [[nodiscard]] inline std::array<float,2> Uv(float along,float across,float phase)
    {
        if (!std::isfinite(along) || !std::isfinite(across) || !std::isfinite(phase)
            || along<0 || along>1 || std::abs(across)>1 || phase<0 || phase>1)
            throw std::invalid_argument("Leaf image coordinates require normalized blade data");
        return {0.5f+0.5f*across*(phase<0.5f ? -1.0f:1.0f),1.0f-along};
    }
}
