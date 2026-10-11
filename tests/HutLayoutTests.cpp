#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

static void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main() {
    const island::IslandTerrain terrain;
    const island::RenderedTerrainSurface surface(terrain);
    const island::VegetationLayout vegetation(terrain);
    const island::JungleLayout jungle(terrain);
    const island::NaturalPropLayout props(terrain,vegetation,jungle);
    const island::ShipwreckLayout ship(terrain);
    std::vector<island::CircularObstacle> occupied;
    for(const auto obstacles:{std::span<const island::CircularObstacle>(vegetation.Obstacles()),
        std::span<const island::CircularObstacle>(jungle.Obstacles()),
        std::span<const island::CircularObstacle>(props.Obstacles()),
        std::span<const island::CircularObstacle>(ship.Obstacles())})
        occupied.insert(occupied.end(),obstacles.begin(),obstacles.end());
    const auto found=island::HutLayout::Find(terrain,surface,occupied);
    Check(found.has_value(),"no hut site in current world");
    const auto repeat=island::HutLayout::Find(terrain,surface,occupied);
    Check(repeat&&found->x==repeat->x&&found->z==repeat->z&&found->floorY==repeat->floorY
        &&found->yawRadians==repeat->yawRadians,"hut placement nondeterministic");
    for(const auto& o:occupied)
        Check(std::hypot(found->x-o.x,found->z-o.z)>=island::HutLayout::SiteClearance+o.radius,
            "hut site intersects existing solid");
    for(const auto& foot:found->footings) {
        Check(foot.groundY==surface.Sample(foot.x,foot.z).height,"post misses rendered ground");
        const float clearance=found->floorY-foot.groundY;
        Check(clearance>=island::HutLayout::FloorClearance-0.00001f
            &&clearance<=island::HutLayout::FloorClearance+island::HutLayout::MaximumRelief+0.00001f,
            "unsupported or buried floor");
    }
    for(std::size_t i=0;i<found->footings.size();++i)
        for(std::size_t j=i+1;j<found->footings.size();++j)
            Check(std::hypot(found->footings[i].x-found->footings[j].x,
                found->footings[i].z-found->footings[j].z)>1.6f,"duplicate or collapsed hut posts");
    const float c=std::cos(found->yawRadians),s=std::sin(found->yawRadians);
    for(int ix=-30;ix<=30;++ix) for(int iz=-25;iz<=25;++iz) {
        const float u=static_cast<float>(ix)*0.08f,v=static_cast<float>(iz)*0.08f;
        const float x=found->x+c*u+s*v,z=found->z-s*u+c*v;
        Check(surface.Sample(x,z).height<found->floorY,"terrain clips hut floor");
    }
    const std::array<island::CircularObstacle,1> full{{{0,0,1000}}};
    Check(!island::HutLayout::Find(terrain,surface,full),"blocked world created overlapping hut");
    occupied.push_back({found->x,found->z,island::HutLayout::SiteClearance});
    const auto alternate=island::HutLayout::Find(terrain,surface,occupied);
    Check(alternate&&std::hypot(found->x-alternate->x,found->z-alternate->z)
        >=2*island::HutLayout::SiteClearance,"reserved site was not avoided");
    auto invalid=full; invalid[0].radius=std::numeric_limits<float>::quiet_NaN();
    bool rejected=false;
    try { (void)island::HutLayout::Find(terrain,surface,invalid); }
    catch(const std::invalid_argument&) { rejected=true; }
    Check(rejected,"invalid occupancy accepted");
    std::cout<<"Hut candidate "<<found->x<<','<<found->z<<" floor "<<found->floorY
        <<" yaw "<<found->yawRadians<<"; six grounded supports, no runtime hut yet\n";
}
