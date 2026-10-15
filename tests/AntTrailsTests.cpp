#include "IslandSimulator/AntTrails.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    void Check(const bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main()
try {
    const island::IslandTerrain terrain;
    const island::JungleLayout jungle(terrain);
    island::AntTrails ants(terrain,jungle), repeated(terrain,jungle);
    Check(ants.Ants().size()==island::AntTrails::Count,"wrong ant count");
    Check(ants.Trails().size()==island::AntTrails::TrailCount,"wrong ant trail count");
    Check(ants.Trails()[3].onTrunk,"fourth ant trail is not on a trunk");
    bool walked=false,carried=false,paused=false,climbed=false;
    for (int frame=0;frame<60*180;++frame) {
        ants.Update(1.0f/60.0f,1000,1000,1000);
        repeated.Update(1.0f/60.0f,1000,1000,1000);
        for (std::size_t index=0;index<ants.Ants().size();++index) {
            const auto& ant=ants.Ants()[index];
            const auto& copy=repeated.Ants()[index];
            Check(ant.x==copy.x&&ant.y==copy.y&&ant.z==copy.z
                &&ant.behavior==copy.behavior,"ant simulation is not deterministic");
            Check(std::isfinite(ant.x)&&std::isfinite(ant.y)&&std::isfinite(ant.z)
                &&std::abs(std::hypot(ant.forwardX,std::hypot(ant.forwardY,ant.forwardZ))-1)<.001f
                &&std::abs(std::hypot(ant.normalX,std::hypot(ant.normalY,ant.normalZ))-1)<.001f,
                "ant pose is not finite and normalized");
            walked|=ant.behavior==island::AntBehavior::Walk;
            carried|=ant.behavior==island::AntBehavior::Carry;
            paused|=ant.behavior==island::AntBehavior::Pause;
            climbed|=ant.trailIndex==3U&&ant.y>ants.Trails()[3].y+1.0f;
            if(ant.trailIndex<3U) Check(std::abs(ant.y-terrain.Height(ant.x,ant.z))<.03f,
                "ground ant left the terrain surface");
        }
    }
    Check(walked&&carried&&paused&&climbed,"ant behaviors or trunk climb were not covered");
    const auto subject=ants.Ants()[0];
    ants.Update(0.0f,subject.x,subject.y,subject.z);
    Check(ants.Ants()[0].simulationTier==island::WildlifeSimulationTier::Full,
        "near ant did not enter full simulation");
    ants.Update(0.0f,subject.x+25.0f,subject.y,subject.z);
    Check(ants.Ants()[0].simulationTier==island::WildlifeSimulationTier::Reduced,
        "mid-distance ant did not enter reduced simulation");
    ants.Update(0.0f,1000.0f,1000.0f,1000.0f);
    Check(ants.Ants()[0].simulationTier==island::WildlifeSimulationTier::Dormant,
        "distant ant did not enter dormant simulation");
    ants.Update(std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),0,0);
    for(const auto& ant:ants.Ants()) Check(std::isfinite(ant.x)&&std::isfinite(ant.gaitPhase),
        "invalid ant input poisoned state");
    std::cout<<"AntTrailsTests: 64 deterministic soil/trunk ants passed\n";
    return 0;
} catch(const std::exception& error) {
    std::cerr<<"AntTrailsTests: "<<error.what()<<'\n';
    return 1;
}
