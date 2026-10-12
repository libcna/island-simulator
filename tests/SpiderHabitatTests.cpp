#include "IslandSimulator/SpiderHabitat.hpp"
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
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const IslandTerrain terrain;const RenderedTerrainSurface surface(terrain);
    const VegetationLayout palms(terrain);const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> obstacles;
    for(const auto items:std::array<std::span<const CircularObstacle>,4>{palms.Obstacles(),
            jungle.Obstacles(),props.Obstacles(),wreck.Obstacles()})
        obstacles.insert(obstacles.end(),items.begin(),items.end());
    const auto hut=HutLayout::FindInWorld(terrain,jungle,obstacles);
    check(hut.has_value(),"reference hut missing");
    obstacles.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    const WorldCollisionGrid grid(obstacles);
    const auto homes=SpiderHabitat::FindGround(terrain,surface,jungle,grid);
    check(homes.has_value(),"no complete ground spider habitat");
    check(homes==SpiderHabitat::FindGround(terrain,surface,jungle,grid),"spider placement not deterministic");
    constexpr float radius=SpiderHabitat::BodyRadius+SpiderHabitat::PatrolRadius;
    for(std::size_t i=0;i<homes->size();++i) {
        const auto h=(*homes)[i];
        check(SpiderHabitat::AcceptsPatch(terrain,surface,jungle,grid,h.x,h.z,h.canopyIndex),"invalid home");
        check(h.y==surface.Sample(h.x,h.z).height&&h.y>=2,"spider not on rendered dry ground");
        for(const auto& obstacle:obstacles)
            check(std::hypot(h.x-obstacle.x,h.z-obstacle.z)>=radius+obstacle.radius,"patrol intersects solid obstacle");
        for(const auto& plant:jungle.GroundPlants())
            check(std::hypot(h.x-plant.x,h.z-plant.z)>=radius+.18f*plant.scale,"patrol crosses ground-plant root");
        for(const auto& plant:jungle.Understory())
            check(std::hypot(h.x-plant.x,h.z-plant.z)>=radius+.25f*plant.scale,"patrol crosses understory stem");
        for(const auto& bush:jungle.Bushes())
            check(std::hypot(h.x-bush.x,h.z-bush.z)>=radius+.30f*bush.scale,"patrol crosses bush root");
        for(std::size_t j=0;j<i;++j)
            check(std::hypot(h.x-(*homes)[j].x,h.z-(*homes)[j].z)>=8,"spider homes overlap");
        std::cout<<"spider ground candidate "<<h.x<<','<<h.y<<','<<h.z<<" canopy="<<h.canopyIndex<<'\n';
    }
    const auto h=homes->front();
    auto localBlock=obstacles;localBlock.push_back({h.x,h.z,.2f});
    check(!SpiderHabitat::AcceptsPatch(terrain,surface,jungle,WorldCollisionGrid(localBlock),h.x,h.z,h.canopyIndex),
        "new obstacle did not invalidate an existing spider home");
    for(const auto p:std::array<std::array<float,2>,4>{{{300,300},{0,1000},
            {std::numeric_limits<float>::quiet_NaN(),0},{0,std::numeric_limits<float>::infinity()}}})
        check(!SpiderHabitat::AcceptsPatch(terrain,surface,jungle,grid,p[0],p[1],h.canopyIndex),"invalid/offshore home accepted");
    check(!SpiderHabitat::AcceptsPatch(terrain,surface,jungle,grid,h.x,h.z,jungle.Canopy().size()),"invalid canopy accepted");
    const std::array<CircularObstacle,1> block{{{0,0,400}}};
    check(!SpiderHabitat::FindGround(terrain,surface,jungle,WorldCollisionGrid(block)),"blocked world invented unsafe spider home");
    std::cout<<"Ground spider placement only; no live spiders or webs installed\n";
}
