#include "IslandSimulator/FruitBatColony.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

int main()
{
    const auto require=[](const bool condition,const char* message) {
        if(!condition) throw std::runtime_error(message);
    };
    island::IslandTerrain terrain;
    island::JungleLayout jungle(terrain);
    island::FruitBatColony colony(terrain,jungle.Canopy());
    island::FruitBatColony repeated(terrain,jungle.Canopy());
    require(colony.Bats().size()==island::FruitBatColony::Count,"fruit-bat count changed");
    require(colony.Roosts().size()==island::FruitBatColony::Count,"fruit-bat roost count changed");
    std::size_t flying=0,roosting=0;
    for(std::size_t i=0;i<colony.Bats().size();++i) {
        const auto& bat=colony.Bats()[i]; const auto& same=repeated.Bats()[i];
        require(bat.x==same.x&&bat.y==same.y&&bat.z==same.z
            &&bat.behavior==same.behavior&&bat.scale==same.scale,
            "fruit-bat initialization is not deterministic");
        require(std::isfinite(bat.x)&&std::isfinite(bat.y)&&std::isfinite(bat.z),
            "fruit-bat position is not finite");
        require(terrain.SignedShoreDistance(colony.Roosts()[i].x,colony.Roosts()[i].z)>=45.0f,
            "fruit-bat roost escaped inland canopy");
        if(bat.behavior==island::FruitBatBehavior::DayFlight) ++flying; else ++roosting;
    }
    require(flying==island::FruitBatColony::MaximumDayFlights&&roosting==4U,
        "fixed daytime flight bound changed");
    for(int frame=0;frame<3600;++frame) {
        colony.Update(1.0f/60.0f,0.0f,20.0f,0.0f);
        repeated.Update(1.0f/60.0f,0.0f,20.0f,0.0f);
    }
    for(std::size_t i=0;i<colony.Bats().size();++i) {
        const auto& bat=colony.Bats()[i]; const auto& same=repeated.Bats()[i];
        require(bat.x==same.x&&bat.y==same.y&&bat.z==same.z&&bat.wingPose==same.wingPose,
            "fruit-bat simulation is not deterministic");
        require(std::hypot(bat.x-colony.Roosts()[i].x,bat.z-colony.Roosts()[i].z)<=12.2f,
            "fruit bat escaped its bounded crown home");
        require(bat.y>terrain.Height(bat.x,bat.z)+2.0f,"fruit bat clipped terrain");
    }
    require(colony.FullSimulationCount()+colony.ReducedSimulationCount()
        +colony.DormantSimulationCount()==static_cast<int>(island::FruitBatColony::Count),
        "fruit-bat activation tiers do not account for the colony");
    colony.Update(NAN,NAN,NAN,NAN);
    require(colony.DormantSimulationCount()==static_cast<int>(island::FruitBatColony::Count),
        "fruit bats do not fail closed for invalid updates");
    std::cout<<"Fruit bats: six deterministic inland roosts, four hanging and two bounded daytime flights\n";
}
