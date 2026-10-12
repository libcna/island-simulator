#include "IslandSimulator/SpiderHabitat.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace island {
namespace {
std::uint32_t Mix(std::uint32_t value) {
    value^=value>>16;value*=0x7feb352dU;value^=value>>15;value*=0x846ca68bU;return value^(value>>16);
}
}
bool SpiderHabitat::AcceptsPatch(const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
    const JungleLayout& jungle,const WorldCollisionGrid& obstacles,float x,float z,std::size_t canopyIndex) {
    if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>220||std::abs(z)>220
        ||canopyIndex>=jungle.Canopy().size())return false;
    const auto& tree=jungle.Canopy()[canopyIndex];
    const float distance=std::hypot(x-tree.x,z-tree.z);
    if(distance<2*tree.scale||distance>5.5f*tree.scale)return false;
    constexpr float radius=BodyRadius+PatrolRadius;
    if(obstacles.IntersectsCircle(x,z,radius))return false;
    const float height=surface.Sample(x,z).height;
    for(int i=0;i<9;++i) {
        const float angle=i*std::numbers::pi_v<float>/4,r=i==8?0:radius;
        const float px=x+r*std::cos(angle),pz=z+r*std::sin(angle);
        const auto ground=surface.Sample(px,pz);
        if(ground.height<2||ground.normal.y<.95f||std::abs(ground.height-height)>.25f
            ||terrain.SignedShoreDistance(px,pz)<30)return false;
    }
    // Retain leaf cover, but do not reserve patrols across plant root/stem bases.
    for(const auto& plant:jungle.GroundPlants())
        if(std::hypot(x-plant.x,z-plant.z)<radius+.18f*plant.scale)return false;
    for(const auto& plant:jungle.Understory())
        if(std::hypot(x-plant.x,z-plant.z)<radius+.25f*plant.scale)return false;
    for(const auto& bush:jungle.Bushes())
        if(std::hypot(x-bush.x,z-bush.z)<radius+.30f*bush.scale)return false;
    return true;
}
std::optional<std::array<SpiderGroundHome,SpiderHabitat::GroundCount>> SpiderHabitat::FindGround(
    const IslandTerrain& terrain,const RenderedTerrainSurface& surface,const JungleLayout& jungle,
    const WorldCollisionGrid& obstacles) {
    struct Candidate {SpiderGroundHome home;std::uint32_t rank;};
    std::vector<Candidate> candidates;
    candidates.reserve(jungle.Canopy().size()*16);
    for(std::size_t treeIndex=0;treeIndex<jungle.Canopy().size();++treeIndex) {
        const auto& tree=jungle.Canopy()[treeIndex];
        for(int ring=0;ring<2;++ring)for(int direction=0;direction<8;++direction) {
            const auto bits=Mix(Seed+static_cast<std::uint32_t>(treeIndex*16+ring*8+direction));
            const float angle=direction*std::numbers::pi_v<float>/4+tree.yawRadians+.17f*ring;
            const float radius=(ring==0?2.7f:4.2f)*tree.scale;
            const float x=tree.x+radius*std::cos(angle),z=tree.z+radius*std::sin(angle);
            if(AcceptsPatch(terrain,surface,jungle,obstacles,x,z,treeIndex))
                candidates.push_back({{x,surface.Sample(x,z).height,z,treeIndex},bits});
        }
    }
    std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.rank<b.rank;});
    std::array<SpiderGroundHome,GroundCount> homes{};std::size_t count=0;
    for(const auto& candidate:candidates) {
        bool separated=true;
        for(std::size_t i=0;i<count;++i)
            separated=separated&&std::hypot(candidate.home.x-homes[i].x,candidate.home.z-homes[i].z)>=MinimumSeparation;
        if(!separated)continue;
        homes[count++]=candidate.home;
        if(count==GroundCount)return homes;
    }
    return std::nullopt; // No unsafe fallback, invisible spider collider or vegetation clearing.
}
}
