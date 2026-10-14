#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/JungleWind.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    CanopyTrunkProfile MakeCanopyTrunkProfile(const int variant)
    {
        if (variant < 0 || variant >= CanopyFormCount) throw std::invalid_argument("Invalid canopy variant");
        constexpr std::array<float,CanopyFormCount> heights{
            8.4f,9.2f,9.8f,10.5f,8.2f,10.6f,9.4f,10.0f,7.2f};
        constexpr std::array<float,CanopyFormCount> leans{
            0.75f,0.40f,1.35f,0.80f,0.90f,0.55f,1.40f,0.30f,0.48f};
        return {heights[variant], 0.63f + variant * 1.49f, leans[variant]};
    }

    float CanopyTrunkRadius(const int variant, const float localHeight)
    {
        const auto trunk=MakeCanopyTrunkProfile(variant);
        if(!std::isfinite(localHeight) || localHeight<0 || localHeight>trunk.height)
            throw std::invalid_argument("Invalid canopy trunk radius height");
        const float t=localHeight/trunk.height;
        const float taper=CanopyTrunkBaseRadius+(CanopyTrunkTipRadius-CanopyTrunkBaseRadius)*t;
        return taper*(1.0f+CanopyRootFlare*std::exp(-localHeight/CanopyRootFlareHeight));
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

    std::vector<float> CanopyTrunkRingHeights(int variant, bool farMesh)
    {
        const auto profile = MakeCanopyTrunkProfile(variant);
        std::vector<float> heights = farMesh ? std::vector<float>{0,1.5f,3}
            : std::vector<float>{0,.16f,.38f,.74f,1.30f,2.10f,3};
        const int upper = farMesh ? 2 : 5;
        for (int ring = 1; ring <= upper; ++ring)
            heights.push_back(3 + (profile.height-3) * static_cast<float>(ring)/upper);
        return heights;
    }

    std::array<float,3> CanopyTrunkVertex(int variant, float height, float angle, bool farMesh,
                                        std::optional<float> windTime)
    {
        if (!std::isfinite(angle)) throw std::invalid_argument("Invalid canopy trunk angle");
        const float radius = CanopyTrunkRadius(variant,height);
        const auto centre = SampleCanopyTrunk(variant,height/MakeCanopyTrunkProfile(variant).height);
        const float irregularity = farMesh ? 1.0f
            : 1.0f + .032f*std::sin(height*1.27f+variant*.83f)
                + .045f*std::sin(angle*2+variant*.71f+height*.31f)
                + .022f*std::sin(angle*3-variant*1.30f+height*.90f);
        std::array<float,3> position{centre[0]+std::cos(angle)*radius*irregularity, centre[1],
                centre[2]+std::sin(angle)*radius*irregularity};
        if (windTime) {
            if (!std::isfinite(*windTime)) throw std::invalid_argument("Invalid canopy wind time");
            const float t = height/MakeCanopyTrunkProfile(variant).height;
            const float weight = t*t*.08f;
            const float phase = variant*.17f*2*std::numbers::pi_v<float>;
            const float broad = std::sin(*windTime*.61f+phase+position[1]*.075f);
            const float detail = std::sin(*windTime*1.07f+phase*1.41f+position[0]*.18f-position[2]*.21f);
            const float sway = (broad*JungleWind::Broad+detail*JungleWind::Detail)*weight;
            position[0] += sway;
            position[2] += sway*JungleWind::DirectionZ;
            position[1] -= std::abs(sway)*JungleWind::Lowering*weight;
        }
        return position;
    }

    CanopyTrunkContact SampleCanopyTrunkSurface(int variant, float height, float angle, bool farMesh,
                                               std::optional<float> windTime)
    {
        if (variant<0 || variant>=CanopyFormCount)
            throw std::invalid_argument("Invalid canopy variant");
        // Ring topology is immutable. Rebuilding this vector for each contact/probe
        // allocated hundreds of thousands of times during one climb preflight.
        // Keep exactly the same authored float values/order as mesh construction.
        static const auto ringTables=[] {
            std::array<std::array<std::vector<float>,2>,CanopyFormCount> tables;
            for (int form=0;form<CanopyFormCount;++form)
                for (int far=0;far<2;++far)
                    tables[form][far]=CanopyTrunkRingHeights(form,far!=0);
            return tables;
        }();
        const auto& heights=ringTables[variant][farMesh ? 1 : 0];
        if (windTime && !std::isfinite(*windTime))
            throw std::invalid_argument("Invalid canopy wind time");
        if (!std::isfinite(height) || height < 0 || height > heights.back() || !std::isfinite(angle))
            throw std::invalid_argument("Invalid canopy trunk contact coordinates");
        const int sides = farMesh ? 5 : 12;
        constexpr float tau = 2*std::numbers::pi_v<float>;
        float wrapped = std::fmod(angle,tau);
        if (wrapped < 0) wrapped += tau;
        const float column = wrapped/tau*sides;
        const int side = std::min(sides-1,static_cast<int>(column));
        const float u = column-side;
        const auto found = std::upper_bound(heights.begin(),heights.end(),height);
        const std::size_t ring = std::min(static_cast<std::size_t>(found-heights.begin()-1),heights.size()-2);
        const float v = (height-heights[ring])/(heights[ring+1]-heights[ring]);
        // Many contact/clearance queries share the same few triangles in one pose.
        // Cache vertices lazily, not whole answers or quantized wind: exact shape and
        // normal computation remain unchanged. Thread-local storage avoids shared mutation.
        struct VertexCache {
            int variant=-1;
            bool far=false;
            std::optional<float> wind;
            std::array<std::array<float,3>,12*12> vertices{};
            std::array<bool,12*12> valid{};
        };
        static thread_local VertexCache cache;
        if (cache.variant!=variant || cache.far!=farMesh || cache.wind!=windTime) {
            cache.variant=variant; cache.far=farMesh; cache.wind=windTime;
            cache.valid.fill(false);
        }
        const auto vertex = [&](std::size_t r,int s) {
            const auto index=r*static_cast<std::size_t>(sides)+static_cast<std::size_t>(s%sides);
            if (!cache.valid[index]) {
                cache.vertices[index]=CanopyTrunkVertex(variant,heights[r],tau*(s%sides)/sides,farMesh,windTime);
                cache.valid[index]=true;
            }
            return cache.vertices[index];
        };
        const auto a = u+v <= 1 ? vertex(ring,side) : vertex(ring+1,side+1);
        const auto b = u+v <= 1 ? vertex(ring+1,side) : vertex(ring,side+1);
        const auto c = u+v <= 1 ? vertex(ring,side+1) : vertex(ring+1,side);
        const float wb = u+v <= 1 ? v : 1-v, wc = u+v <= 1 ? u : 1-u;
        CanopyTrunkContact result{};
        std::array<float,3> ab{},ac{};
        for (std::size_t k = 0; k < 3; ++k) {
            result.position[k] = a[k]*(1-wb-wc)+b[k]*wb+c[k]*wc;
            ab[k]=b[k]-a[k]; ac[k]=c[k]-a[k];
        }
        result.normal = {ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]};
        float length = std::sqrt(result.normal[0]*result.normal[0]+result.normal[1]*result.normal[1]
            +result.normal[2]*result.normal[2]);
        const auto centre = SampleCanopyTrunk(variant,height/MakeCanopyTrunkProfile(variant).height);
        if (result.normal[0]*(result.position[0]-centre[0])+result.normal[2]*(result.position[2]-centre[2]) < 0)
            length = -length;
        for (auto& component : result.normal) component /= length;
        return result;
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
              {-2.75f,2.45f,-0.65f,1.15f,1.3f},{0.65f,3.55f,0.5f,1.25f,1.15f}}},
            {{{-0.3f,-0.15f,0.2f,2.35f,1.15f},{-2.0f,0.05f,0.55f,2.0f,1.05f},
              {2.05f,0.25f,-0.7f,2.15f,1.15f},{0.2f,1.25f,1.75f,1.75f,1.2f}}}
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

    float CanopyNominalHeight(const int variant)
    {
        if (variant < 0 || variant >= CanopyFormCount)
            throw std::invalid_argument("Invalid canopy variant");
        float height = 0.0f;
        for (int cluster = 0; cluster < 8; ++cluster) {
            const auto crown = MakeCanopyCrownProfile(variant, cluster);
            height = std::max(height, crown.y + crown.height);
        }
        return height;
    }
}
