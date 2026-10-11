#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island {
std::optional<HutPlacement> HutLayout::Find(const IslandTerrain& terrain,
    const RenderedTerrainSurface& surface, std::span<const CircularObstacle> occupied)
{
    for(const auto& o:occupied)
        if(!std::isfinite(o.x)||!std::isfinite(o.z)||!std::isfinite(o.radius)||o.radius<0)
            throw std::invalid_argument("Invalid occupied hut site");
    std::optional<HutPlacement> best;
    float bestScore=std::numeric_limits<float>::infinity();
    for(int ring=0;ring<6;++ring) for(int sector=0;sector<96;++sector) {
        const float angle=1.1f+2*std::numbers::pi_v<float>*static_cast<float>(sector)/96;
        const float inland=18.0f+static_cast<float>(ring)*4.0f;
        const float radius=terrain.ShoreRadius(angle)-inland;
        const float x=radius*std::cos(angle),z=radius*std::sin(angle);
        bool blocked=false;
        for(const auto& o:occupied)
            if(std::hypot(x-o.x,z-o.z)<SiteClearance+o.radius) { blocked=true; break; }
        if(blocked) continue;
        // Front (+local Z) faces the coast. Test the complete roof's enclosing
        // rectangle against the exact rendered triangles, not just the post feet.
        const float yaw=std::atan2(x,z),c=std::cos(yaw),s=std::sin(yaw);
        const float halfX=std::abs(c)*(Width/2+RoofOverhang)+std::abs(s)*(Depth/2+RoofOverhang);
        const float halfZ=std::abs(s)*(Width/2+RoofOverhang)+std::abs(c)*(Depth/2+RoofOverhang);
        const auto heights=surface.HeightRange(x-halfX,z-halfZ,x+halfX,z+halfZ);
        if(heights[0]<1.2f||heights[1]-heights[0]>MaximumRelief) continue;
        // Reserve a walkable approach ring too; this is local accessibility, not
        // proof of a global route from every player spawn.
        for(int i=0;i<16&&!blocked;++i) {
            const float a=2*std::numbers::pi_v<float>*static_cast<float>(i)/16;
            const auto sample=terrain.Sample(x+SiteClearance*std::cos(a),z+SiteClearance*std::sin(a));
            blocked=sample.height<1.2f||sample.signedShoreDistance<8||sample.normal.y<0.97f;
        }
        if(blocked) continue;
        const float score=(heights[1]-heights[0])*10+std::abs(inland-26)*0.03f;
        if(score>=bestScore) continue;
        HutPlacement candidate{x,z,yaw,heights[1]+FloorClearance,{}};
        std::size_t index=0;
        for(float u:{-Width/2,Width/2}) for(float v:{-Depth/2,0.0f,Depth/2}) {
            const float px=x+c*u+s*v,pz=z-s*u+c*v;
            candidate.footings[index++]={px,surface.Sample(px,pz).height,pz};
        }
        best=candidate;
        bestScore=score;
    }
    return best;
}
}
