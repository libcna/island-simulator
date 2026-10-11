#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace island {
struct ThatchPoint { float x,y,z; };
// Four longitudinal stations retain broad overlapping coverage before a narrow,
// drooping leaf tip. Two quads per segment: 36 vertices/blade, 672 blades.
struct HutThatchBlade {
    std::array<std::array<ThatchPoint,3>,4> sections;
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
    const float finish=row==6?1.005f+fringe*0.032f:start+0.205f+length*0.038f;
    const float z=-2.25f+(static_cast<float>(strip)+0.5f)*4.5f/48+(shift-0.5f)*0.006f;
    HutThatchBlade result{}; result.pigment=noise();
    constexpr std::array<float,4> stations{0.0f,0.48f,0.82f,1.0f};
    for(std::size_t station=0;station<stations.size();++station) {
        const float t=stations[station],u=start+(finish-start)*t;
        const float sag=station==1?0.012f+0.012f*bend:
            (station==3?(row==6?0.035f+0.025f*bend:0.006f):0.0f);
        const float y=3.55f-1.4f*u+0.025f*static_cast<float>(7-row)-sag;
        const float centre=z+(bend-0.5f)*(station==3?0.028f:0.004f)*t;
        const float halfWidth=station==3?0.008f+fringe*0.012f:
            (station==2?0.060f:0.067f);
        result.sections[static_cast<std::size_t>(station)]={{{static_cast<float>(side)*2.65f*u,y,centre-halfWidth},
            {static_cast<float>(side)*2.65f*u,y+0.022f*(1-t)+0.002f,centre},
            {static_cast<float>(side)*2.65f*u,y+0.003f*(shift-0.5f),centre+halfWidth}}};
    }
    return result;
}
}
