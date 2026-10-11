#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace island {
struct ThatchPoint { float x,y,z; };
// Three longitudinal stations, each with left edge, raised midrib and right edge.
// Two quads per segment: 24 vertices/blade, 672 blades, matching the old roof budget.
struct HutThatchBlade {
    std::array<std::array<ThatchPoint,3>,3> sections;
    float pigment;
};
inline HutThatchBlade BuildHutThatchBlade(int side,int row,int strip) {
    if((side!=-1&&side!=1)||row<0||row>=7||strip<0||strip>=48)
        throw std::invalid_argument("Invalid hut thatch blade");
    std::uint32_t seed=static_cast<std::uint32_t>((side+2)*7919+row*173+strip*65537);
    const auto noise=[&]() {
        seed^=seed<<13; seed^=seed>>17; seed^=seed<<5;
        return static_cast<float>(seed&65535U)/65535.0f;
    };
    const float shift=noise(),length=noise(),bend=noise(),fringe=noise();
    const float start=(static_cast<float>(row)+(row==0?0.0f:shift*0.18f))/7;
    const float finish=row==6?1.01f+fringe*0.025f:start+0.205f+length*0.022f;
    const float z=-2.25f+(static_cast<float>(strip)+0.5f)*4.5f/48+(shift-0.5f)*0.006f;
    HutThatchBlade result{}; result.pigment=noise();
    for(int station=0;station<3;++station) {
        const float t=static_cast<float>(station)/2,u=start+(finish-start)*t;
        const float sag=station==1?0.014f+0.018f*bend:0;
        const float y=3.55f-1.4f*u+0.035f*static_cast<float>(7-row)-sag;
        const float centre=z+(bend-0.5f)*0.004f*t;
        const float halfWidth=0.067f*(station==2?0.82f:1.0f);
        result.sections[static_cast<std::size_t>(station)]={{{static_cast<float>(side)*2.65f*u,y,centre-halfWidth},
            {static_cast<float>(side)*2.65f*u,y+0.008f*(1-t)+0.002f,centre},
            {static_cast<float>(side)*2.65f*u,y+0.003f*(shift-0.5f),centre+halfWidth}}};
    }
    return result;
}
}
