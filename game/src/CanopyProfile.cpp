#include "IslandSimulator/CanopyProfile.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    CanopyTrunkProfile MakeCanopyTrunkProfile(const int variant)
    {
        if (variant < 0 || variant >= CanopyFormCount) throw std::invalid_argument("Invalid canopy variant");
        constexpr std::array<float,CanopyFormCount> heights{8.4f,9.2f,9.8f,10.5f,8.2f,10.6f,9.4f,10.0f};
        constexpr std::array<float,CanopyFormCount> leans{0.75f,0.40f,1.35f,0.80f,0.90f,0.55f,1.40f,0.30f};
        return {heights[variant], 0.63f + variant * 1.49f, leans[variant]};
    }

    std::array<float,3> SampleCanopyTrunk(const int variant,const float t)
    {
        const auto trunk=MakeCanopyTrunkProfile(variant);
        if(!std::isfinite(t) || t<0 || t>1) throw std::invalid_argument("Invalid canopy trunk height");
        const float y=trunk.height*t;
        const float u=std::max(0.0f,(y-3.0f)/(trunk.height-3.0f));
        const float bend=trunk.lean*u*u*(3-2*u);
        const float crossBend=std::sin(std::numbers::pi_v<float>*u)*u*(1-u)
            * (0.72f+0.24f*variant);
        const float c=std::cos(trunk.leanAngle),s=std::sin(trunk.leanAngle);
        return {c*bend-s*crossBend,y,s*bend+c*crossBend};
    }

    CanopyCrownProfile MakeCanopyCrownProfile(const int variant, const int cluster)
    {
        const auto trunk = MakeCanopyTrunkProfile(variant);
        if (cluster < 0 || cluster >= 8) {
            throw std::invalid_argument("Invalid canopy crown cluster");
        }
        struct Group { float x,y,z,width,height; };
        // Original forms plus broad low, narrow high, hanging and open-forked companions.
        // These change branch architecture, not just uniform scale or foliage colour. The same
        // eight lobe anchors drive near/far geometry and the fixed-sun occluder bake.
        constexpr std::array<std::array<Group,4>,CanopyFormCount> groups{{
            {{{0,0.3f,0,2.4f,1.6f},{-2.3f,-0.35f,0.4f,1.95f,1.3f},
              {1.9f,0.45f,1.4f,2.15f,1.5f},{0.2f,1.3f,-1.65f,1.7f,1.65f}}},
            {{{0,0.5f,0,1.85f,2.0f},{-1.35f,0.25f,0.3f,1.7f,1.9f},
              {1.45f,1.2f,0.6f,1.65f,1.7f},{-0.15f,3.8f,-0.5f,1.25f,1.6f}}},
            {{{0.45f,0.3f,0,2.1f,1.45f},{2.5f,0.8f,0.85f,2.1f,1.5f},
              {-0.7f,-0.9f,-1.2f,1.8f,1.4f},{3.5f,2.0f,-0.9f,1.9f,1.6f}}},
            {{{-0.65f,0.6f,0,1.8f,1.8f},{1.7f,0.6f,1.4f,1.85f,1.7f},
              {-2.2f,3.1f,-0.6f,1.5f,1.7f},{2.65f,2.1f,0.25f,1.55f,1.6f}}},
            {{{0,0.8f,0,2.1f,1.1f},{-2.8f,0.25f,0.45f,2.2f,0.9f},
              {2.55f,-0.35f,-0.9f,1.9f,1.2f},{0.3f,1.65f,-1.8f,1.7f,1.2f}}},
            {{{0,0.3f,0,1.2f,1.7f},{-0.75f,1.8f,-0.25f,1.05f,1.5f},
              {1.05f,2.1f,0.75f,1.15f,1.55f},{0.1f,3.6f,-0.6f,0.9f,1.3f}}},
            {{{-0.7f,0.35f,0.4f,2.1f,1.35f},{1.8f,-1.2f,1.45f,1.8f,1.65f},
              {-1.7f,-0.65f,-1.65f,1.6f,1.5f},{1.0f,1.35f,-0.6f,1.5f,1.2f}}},
            {{{-2.05f,0.35f,1.25f,1.45f,1.55f},{2.05f,1.65f,-0.95f,1.35f,1.75f},
              {-2.75f,2.45f,-0.65f,1.15f,1.3f},{0.65f,3.55f,0.5f,1.25f,1.15f}}}
        }};
        const auto& group=groups[variant][cluster/2];
        const float angle=std::atan2(group.z,group.x);
        const float splitAngle=variant*0.57f+(cluster/2)*2.39996323f+1.17f;
        const float sign=cluster%2 ? 1.0f : -1.0f;
        const auto tip=SampleCanopyTrunk(variant,1.0f);
        return {tip[0]+group.x+std::cos(splitAngle)*group.width*0.33f*sign,
            trunk.height+group.y+group.height*0.22f*sign,
            tip[2]+group.z+std::sin(splitAngle)*group.width*0.33f*sign,
            group.width*(cluster%2 ? 0.96f : 1.04f),group.height*(cluster%2 ? 0.91f : 0.96f),
            angle,std::fmod(cluster*0.139f+variant*0.23f,1.0f)};
    }
}
