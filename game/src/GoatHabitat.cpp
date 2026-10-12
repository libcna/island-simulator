#include "IslandSimulator/GoatHabitat.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/CaveSite.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace island {
bool GoatHabitat::AcceptsGround(const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
    float x,float z)
{
    if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>220||std::abs(z)>220
        ||CaveSite::Reserves(x,z,BodyClearance)) return false;
    const auto centre=surface.Sample(x,z);
    const auto environment=IslandEnvironmentField::SampleInland(
        x,z,terrain.SignedShoreDistance(x,z));
    // Sloped inland ground, not a beach, cliff or flat valley floor.
    if(centre.height<6 || centre.normal.y<.90f || centre.normal.y>.997f
        ||terrain.Sample(x,z).normal.y<.90f
        ||environment.moisture>MaximumHabitatMoisture
        ||environment.drainage>MaximumHabitatDrainage
        ||environment.saltExposure>MaximumHabitatSaltExposure) return false;
    for(int i=0;i<9;++i) {
        const float angle=static_cast<float>(i)*std::numbers::pi_v<float>/4;
        const float radius=i==8?0:BodyClearance;
        const float px=x+radius*std::cos(angle),pz=z+radius*std::sin(angle);
        const auto sample=surface.Sample(px,pz);
        if(!std::isfinite(sample.height)||sample.height<6 || sample.normal.y<.90f
            ||std::abs(sample.height-centre.height)>.55f
            ||terrain.SignedShoreDistance(px,pz)<35) return false;
    }
    return true;
}
bool GoatHabitat::Accepts(const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
    const JungleLayout& jungle,const WorldCollisionGrid& obstacles,float x,float z)
{
    if(!AcceptsGround(terrain,surface,x,z)||obstacles.IntersectsCircle(x,z,BodyClearance)) return false;
    for(const auto& tree:jungle.Canopy())
        if(std::hypot(x-tree.x,z-tree.z)<TreeClearance*tree.scale) return false;
    for(const auto& plant:jungle.Understory())
        if(std::hypot(x-plant.x,z-plant.z)<BodyClearance+1.8f*plant.scale) return false;
    for(const auto& bush:jungle.Bushes())
        if(std::hypot(x-bush.x,z-bush.z)<BodyClearance+1.2f*bush.scale) return false;
    return true;
}

std::optional<std::array<GoatHome,GoatHabitat::Count>> GoatHabitat::Find(
    const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
    const JungleLayout& jungle,const WorldCollisionGrid& obstacles)
{
    std::vector<GoatHome> candidates;
    candidates.reserve(CandidateCount);
    for(int iz=-27;iz<=27;++iz) for(int ix=-27;ix<=27;++ix) {
        auto bits=Seed+static_cast<std::uint32_t>((iz+27)*55+ix+27);
        bits^=bits>>16;bits*=0x7feb352dU;bits^=bits>>15;bits*=0x846ca68bU;bits^=bits>>16;
        const float x=static_cast<float>(ix)*8+(static_cast<float>(bits&1023U)/1023-.5f)*4;
        const float z=static_cast<float>(iz)*8+(static_cast<float>((bits>>10)&1023U)/1023-.5f)*4;
        if(Accepts(terrain,surface,jungle,obstacles,x,z))
            candidates.push_back({x,surface.Sample(x,z).height,z});
    }
    // Prefer upland clearings. Stable order gives deterministic ties without random retries.
    std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.y>b.y;});
    for(const auto& anchor:candidates) {
        std::array<GoatHome,Count> homes{};
        homes[0]=anchor;
        std::size_t count=1;
        for(const auto& candidate:candidates) {
            if(std::hypot(candidate.x-anchor.x,candidate.z-anchor.z)>HerdRadius) continue;
            bool separated=true;
            for(std::size_t i=0;i<count;++i)
                separated=separated&&std::hypot(candidate.x-homes[i].x,candidate.z-homes[i].z)>=MinimumSeparation;
            if(!separated) continue;
            homes[count++]=candidate;
            if(count==Count) return homes;
        }
    }
    return std::nullopt; // Do not invent an unsafe fallback or remove vegetation.
}
}
