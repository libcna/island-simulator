#include "IslandSimulator/CrocodileGroup.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/StreamRoute.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}}

int main() {
    const island::IslandTerrain terrain;
    island::CrocodileGroup first(terrain),second(terrain);
    Check(first.Crocodiles().size()==3&&first.Homes().size()==3,"wrong crocodile population");
    const float mouthX=island::StreamRoute::SourceX-island::StreamRoute::NominalLength;
    const float mouthZ=island::StreamRoute::CentreZ(mouthX);
    bool basking=false,swimming=false;
    for(std::size_t i=0;i<first.Crocodiles().size();++i) {
        const auto& state=first.Crocodiles()[i];const auto& home=first.Homes()[i];
        Check(std::hypot((home.landX+home.waterX)*.5f-mouthX,
                         (home.landZ+home.waterZ)*.5f-mouthZ)<26.0f,
              "crocodile home is not at the stream estuary");
        Check(first.IsValidHabitat(state.x,state.z,home),"initial crocodile habitat invalid");
        std::cout<<"crocodile "<<i<<" position "<<state.x<<','<<state.y<<','<<state.z
                 <<" land "<<home.landX<<','<<home.landZ<<" water "
                 <<home.waterX<<','<<home.waterZ<<'\n';
        basking|=state.behavior==island::CrocodileBehavior::Bask;
        swimming|=state.behavior==island::CrocodileBehavior::Swim;
    }
    Check(basking&&swimming,"initial population lacks basking and swimming animals");
    island::CrocodileGroup obstacleAware(terrain);
    const auto blocked=obstacleAware.Crocodiles()[2U];
    const std::array obstacles={island::CircularObstacle{blocked.x,blocked.z,2.0f}};
    obstacleAware.SetObstacles(obstacles);
    for(const auto& state:obstacleAware.Crocodiles())
        Check(std::hypot(state.x-blocked.x,state.z-blocked.z)>=4.65f,
              "crocodile placement overlaps a static world obstacle");
    for(int frame=0;frame<7200;++frame) {
        first.Update(1.0f/60.0f,1000,1000,1000);
        second.Update(1.0f/60.0f,1000,1000,1000);
    }
    for(std::size_t i=0;i<first.Crocodiles().size();++i) {
        const auto& a=first.Crocodiles()[i];const auto& b=second.Crocodiles()[i];
        Check(first.IsValidHabitat(a.x,a.z,first.Homes()[i]),"crocodile left estuary habitat");
        Check(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.behavior==b.behavior,
              "crocodile simulation is not deterministic");
    }
    const auto before=first.Crocodiles()[0];
    for(int frame=0;frame<600;++frame)
        first.Update(1.0f/60.0f,before.x,before.y+1,before.z);
    const auto after=first.Crocodiles()[0];
    Check(after.behavior==island::CrocodileBehavior::Retreat,
          "nearby crocodile does not retain retreat state");
    Check(std::hypot(after.x-before.x,after.z-before.z)>2.0f,
          "nearby crocodile does not move away toward water");
    std::cout<<"Three deterministic estuary crocodiles bask, swim and retreat from the player\n";
}
