#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace island
{
    using LitterPoint = std::array<float, 3>;
    inline constexpr int GroundLitterCount = 3;
    inline constexpr float GroundLitterRadius = 0.74f;
    inline constexpr std::array<float, 5> GroundLitterShade{0.72f,0.83f,1.08f,0.67f,0.78f};
    inline constexpr std::array<std::uint16_t,12> GroundLitterIndices{0,1,2,0,2,3,1,4,2,2,4,3};

    // Original folded dry-leaf profile shared by rendering and terrain-contact audits.
    [[nodiscard]] inline std::array<LitterPoint,5> MakeGroundLitterLeaf(const int leaf)
    {
        if (leaf < 0 || leaf >= GroundLitterCount) throw std::invalid_argument("Invalid ground litter leaf");
        const float angle=0.38f+leaf*2.17f;
        const float length=0.31f+0.045f*((leaf*2+1)%3);
        const float width=0.082f+0.012f*((leaf+2)%3);
        const float radius=0.18f+leaf*0.075f;
        const float fold=leaf%2==0 ? -1.0f : 1.0f;
        const float dx=std::cos(angle), dz=std::sin(angle);
        const auto point=[&](float along,float across,float height) -> LitterPoint {
            return {dx*(radius+along)-dz*across,height,dz*(radius+along)+dx*across};
        };
        return {point(0,0,0.021f),point(length*0.47f,-width,0.021f),
                point(length*0.47f,width*fold*0.10f,0.052f),
                point(length*0.47f,width,0.021f),point(length,0,0.029f)};
    }
}
