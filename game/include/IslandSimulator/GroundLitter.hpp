#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace island
{
    using LitterPoint = std::array<float, 3>;
    inline constexpr int GroundLitterCount = 5;
    inline constexpr std::size_t GroundLitterVertexCount = 7;
    inline constexpr float GroundLitterRadius = 0.74f;
    inline constexpr std::array<float, 7> GroundLitterShade{
        0.72f,0.82f,0.76f,0.88f,0.69f,0.78f,1.08f};
    inline constexpr std::array<std::uint16_t,18> GroundLitterIndices{
        0,1,6,1,2,6,2,3,6,3,4,6,4,5,6,5,0,6};

    // Original folded dry-leaf profile shared by rendering and terrain-contact audits.
    [[nodiscard]] inline std::array<LitterPoint,GroundLitterVertexCount>
    MakeGroundLitterLeaf(const int leaf)
    {
        if (leaf < 0 || leaf >= GroundLitterCount) throw std::invalid_argument("Invalid ground litter leaf");
        const float angle=0.38f+leaf*2.17f+(leaf>=3 ? 0.73f : 0.0f);
        const float length=0.31f+0.045f*((leaf*2+1)%3);
        const float width=0.082f+0.012f*((leaf+2)%3);
        // Retain the three established leaves exactly, then use two separate inner/outer
        // placements rather than extending the old arithmetic progression beyond the
        // qualified 0.74 m terrain-height footprint.
        const float radius=leaf<3 ? 0.18f+leaf*0.075f : (leaf==3 ? 0.26f : 0.34f);
        const float fold=leaf%2==0 ? -1.0f : 1.0f;
        const float dx=std::cos(angle), dz=std::sin(angle);
        const auto point=[&](float along,float across,float height) -> LitterPoint {
            return {dx*(radius+along)-dz*across,height,dz*(radius+along)+dx*across};
        };
        return {point(0,0,0.021f),
                point(length*.34f,-width*.78f,0.023f),
                point(length*.67f,-width,0.021f),
                point(length,0,0.028f),
                point(length*.67f,width,0.021f),
                point(length*.34f,width*.78f,0.023f),
                point(length*.52f,width*fold*.08f,0.052f)};
    }
}
