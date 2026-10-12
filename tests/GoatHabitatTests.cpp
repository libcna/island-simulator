#include "IslandSimulator/GoatHabitat.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

int main() {
    using namespace island;
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const IslandTerrain terrain;
    const RenderedTerrainSurface surface(terrain);
    const VegetationLayout palms(terrain);
    const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);
    const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> occupied;
    for(const auto obstacles:std::array<std::span<const CircularObstacle>,4>{
            palms.Obstacles(),jungle.Obstacles(),props.Obstacles(),wreck.Obstacles()})
        occupied.insert(occupied.end(),obstacles.begin(),obstacles.end());
    const auto hut=HutLayout::FindInWorld(terrain,jungle,occupied);
    check(hut.has_value(),"reference world hut missing");
    occupied.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    const WorldCollisionGrid grid(occupied);
    const auto homes=GoatHabitat::Find(terrain,surface,jungle,grid);
    check(homes.has_value(),"six open-slope goat homes unavailable");
    check(homes==GoatHabitat::Find(terrain,surface,jungle,grid),"goat placement nondeterministic");
    for(std::size_t i=0;i<homes->size();++i) {
        const auto p=(*homes)[i];
        check(p.y==surface.Sample(p.x,p.z).height,"home not on rendered ground");
        check(GoatHabitat::Accepts(terrain,surface,jungle,grid,p.x,p.z),"invalid selected home");
        for(const auto& tree:jungle.Canopy())
            check(std::hypot(p.x-tree.x,p.z-tree.z)>=GoatHabitat::TreeClearance*tree.scale,
                "home under nearby canopy tree");
        for(const auto& plant:jungle.Understory())
            check(std::hypot(p.x-plant.x,p.z-plant.z)>=GoatHabitat::BodyClearance+1.8f*plant.scale,
                "home inside understory envelope");
        for(const auto& bush:jungle.Bushes())
            check(std::hypot(p.x-bush.x,p.z-bush.z)>=GoatHabitat::BodyClearance+1.2f*bush.scale,
                "home inside bush envelope");
        for(const auto obstacle:occupied)
            check(std::hypot(p.x-obstacle.x,p.z-obstacle.z)>=GoatHabitat::BodyClearance+obstacle.radius,
                "independent obstacle clearance failed");
        for(std::size_t j=0;j<i;++j)
            check(std::hypot(p.x-(*homes)[j].x,p.z-(*homes)[j].z)>=GoatHabitat::MinimumSeparation,
                "goat homes overlap");
        check(std::hypot(p.x-homes->front().x,p.z-homes->front().z)<=GoatHabitat::HerdRadius,
            "herd dispersed beyond home region");
        const std::array<CircularObstacle,1> blocker{{{p.x,p.z,1}}};
        check(!GoatHabitat::Accepts(terrain,surface,jungle,WorldCollisionGrid(blocker),p.x,p.z),
            "occupied home accepted");
        std::cout<<"goat candidate home "<<p.x<<','<<p.y<<','<<p.z<<'\n';
    }
    const std::array<CircularObstacle,1> blocked{{{0,0,400}}};
    check(!GoatHabitat::Find(terrain,surface,jungle,WorldCollisionGrid(blocked)),"unsafe fallback on blocked island");
    check(!GoatHabitat::Accepts(terrain,surface,jungle,grid,std::numeric_limits<float>::quiet_NaN(),0),
        "nonfinite home accepted");
    check(!GoatHabitat::Accepts(terrain,surface,jungle,grid,300,300),"offshore home accepted");
    std::cout<<"Goat habitat foundation passed; no live goats are installed by this test\n";
}
