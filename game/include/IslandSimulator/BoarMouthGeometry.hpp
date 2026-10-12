#pragma once
#include "IslandSimulator/BoarEyeGeometry.hpp"

namespace island {
// Two edges of a closed-mouth crease. Fit to the same head triangles as the eyes.
inline std::array<BoarJoint,42> BuildBoarMouth(int side,float dip,float bob) {
    if((side!=-1&&side!=1)||!std::isfinite(dip)||!std::isfinite(bob))
        throw std::invalid_argument("Invalid boar mouth pose");
    dip=std::clamp(dip,0.0f,0.15f);bob=std::clamp(bob,0.0f,0.018f);
    const auto profile=MakeBoarBodyProfile(dip);
    constexpr std::array<float,6> z{0.80f,0.85f,0.90f,0.95f,1.00f,1.035f};
    constexpr std::array<float,6> y{0.415f,0.395f,0.385f,0.38f,0.38f,0.395f};
    constexpr std::array<float,6> dipWeight{0.85f,0.92f,0.99f,1,1,1};
    std::array<BoarJoint,42> result{};
    // Four segments per authored span follow cheek curvature instead of bridging it.
    for(std::size_t i=0;i<result.size()/2;++i) for(std::size_t edge=0;edge<2;++edge) {
        const auto span=std::min(i/4,z.size()-2);
        const float t=static_cast<float>(i-span*4)/4;
        const auto width=[](std::size_t station){return station==0||station==5?0.001f:0.003f;};
        const float halfWidth=std::lerp(width(span),width(span+1),t);
        const float height=std::lerp(y[span],y[span+1],t)
            -dip*std::lerp(dipWeight[span],dipWeight[span+1],t)+(edge==0?-halfWidth:halfWidth);
        const float depth=BoarMuzzleZ(std::lerp(z[span],z[span+1],t));
        result[i*2+edge]={static_cast<float>(side)*(BoarHeadSurfaceX(profile,height,depth)+0.002f),
            height+bob,depth};
    }
    return result;
}
}
