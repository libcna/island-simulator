#pragma once
#include "IslandSimulator/MonkeyGroundGait.hpp"

namespace island::MonkeyLimbProfile
{
    using Point = MonkeyGroundGait::Point;
    inline constexpr std::array<float,10> Fractions{0,.0625f,.125f,.25f,.375f,.5f,.625f,.75f,.875f,1};
    inline constexpr std::array<float,10> Radii{
        .040f,.080f,.095f,.090f,.073f,.063f,.055f,.048f,.042f,.038f};
    struct Section { Point centre; float radius, bodyWeight, lowerWeight; };
    inline Section Sample(const MonkeyGroundGait::Limb& limb, std::size_t row, float thickness)
    {
        const float t=Fractions.at(row);
        const auto lerp=[](Point a,Point b,float w) {
            return Point{a.x+(b.x-a.x)*w,a.y+(b.y-a.y)*w,a.z+(b.z-a.z)*w};
        };
        Point centre=t<.5f ? lerp(limb.root,limb.joint,t*2) : lerp(limb.joint,limb.sole,t*2-1);
        const float collar=std::max(0.0f,1-t/.25f);
        // Bury the proximal ring inside the torso. Its body-bone attachment stays
        // there when the arm swings, instead of exposing a rotating open tube rim.
        centre.x-=limb.root.x*.5f*collar*collar;
        centre.y+=.09f*collar*collar;
        return {centre,Radii.at(row)*thickness,std::max(0.0f,1-t/.125f),
            std::clamp((t-.375f)*4,0.0f,1.0f)};
    }
}
