#include "IslandSimulator/HelicopterLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(const bool value,const char* message){if(!value)throw std::runtime_error(message);}
}

int main() {
    const island::IslandTerrain terrain;
    const island::RenderedTerrainSurface surface(terrain);
    const island::VegetationLayout vegetation(terrain);
    const island::JungleLayout jungle(terrain);
    const island::NaturalPropLayout props(terrain,vegetation,jungle);
    const island::ShipwreckLayout wreck(terrain);
    std::vector<island::CircularObstacle> solids;
    for(const auto obstacles:{std::span<const island::CircularObstacle>(vegetation.Obstacles()),
        std::span<const island::CircularObstacle>(jungle.Obstacles()),
        std::span<const island::CircularObstacle>(props.Obstacles()),
        std::span<const island::CircularObstacle>(wreck.Obstacles())})
        solids.insert(solids.end(),obstacles.begin(),obstacles.end());
    const auto terrainOnly=island::HelicopterLayout::Find(terrain,surface,{},{});
    Check(terrainOnly.has_value(),"terrain alone has no helicopter site");
    std::cout<<"Terrain-only helicopter candidate "<<terrainOnly->x<<','<<terrainOnly->z
        <<'\n'<<std::flush;
    const auto placement=island::HelicopterLayout::FindInWorld(terrain,jungle,solids);
    const auto repeated=island::HelicopterLayout::FindInWorld(terrain,jungle,solids);
    Check(placement.has_value()&&repeated.has_value(),"current world has no helicopter site");
    Check(placement->x==repeated->x&&placement->z==repeated->z
        &&placement->yawRadians==repeated->yawRadians&&placement->floorY==repeated->floorY,
        "helicopter placement is not deterministic");
    const float centreRadius=std::hypot(placement->x,placement->z);
    Check(centreRadius>=island::HelicopterLayout::MinimumCentreRadius
        &&centreRadius<=island::HelicopterLayout::MaximumCentreRadius,
        "helicopter is not near the island centre");
    Check(terrain.SignedShoreDistance(placement->x,placement->z)
        >=island::HelicopterLayout::MinimumShoreDistance,"helicopter site is coastal");
    for(const auto& o:solids)Check(std::hypot(placement->x-o.x,placement->z-o.z)
        >=island::HelicopterLayout::SiteClearance+o.radius,"main rotor overlaps a solid");
    for(const auto& contact:placement->skidContacts)
        Check(contact.groundY==surface.Sample(contact.x,contact.z).height,
              "skid contact does not sample rendered terrain");

    constexpr float playerRadius=.32f;
    const auto pilotOutside=placement->World(1.72f,0.0f,1.02f);
    const auto pilotInside=placement->World(0.0f,0.0f,1.02f);
    const auto rearOutside=placement->World(1.72f,0.0f,-.92f);
    const auto rearInside=placement->World(.45f,0.0f,-.92f);
    Check(placement->SafeTravelFraction(pilotOutside.x,pilotOutside.z,
        pilotInside.x,pilotInside.z,playerRadius)==1.0f,"pilot doorway is not enterable");
    Check(placement->SafeTravelFraction(rearOutside.x,rearOutside.z,
        rearInside.x,rearInside.z,playerRadius)==1.0f,"rear doorway is not enterable");
    const auto leftPilotOutside=placement->World(-1.72f,0.0f,1.02f);
    const auto leftRearOutside=placement->World(-1.72f,0.0f,-.92f);
    Check(placement->SafeTravelFraction(leftPilotOutside.x,leftPilotOutside.z,
        pilotInside.x,pilotInside.z,playerRadius)==1.0f,"left pilot doorway is not enterable");
    Check(placement->SafeTravelFraction(leftRearOutside.x,leftRearOutside.z,
        rearInside.x,rearInside.z,playerRadius)==1.0f,"left rear doorway is not enterable");
    Check(placement->InPilotFootwell(pilotInside.x,pilotInside.z),
          "pilot entry does not reach cockpit footwell");
    Check(placement->InRearCabin(rearInside.x,rearInside.z),
          "rear entry does not reach cabin");
    const auto closedSide=placement->World(1.40f,0.0f,-.05f);
    const auto closedInside=placement->World(.55f,0.0f,-.05f);
    Check(placement->SafeTravelFraction(closedSide.x,closedSide.z,
        closedInside.x,closedInside.z,playerRadius)<1.0f,"centre door post is non-solid");
    const auto noseOutside=placement->World(0.0f,0.0f,3.2f);
    const auto noseInside=placement->World(0.0f,0.0f,1.05f);
    Check(placement->SafeTravelFraction(noseOutside.x,noseOutside.z,
        noseInside.x,noseInside.z,playerRadius)<1.0f,"solid nose is traversable");
    Check(placement->SupportHeight(pilotInside.x,pilotInside.z,-100.0f)==placement->floorY,
          "cockpit floor does not support the player");
    Check(placement->SupportHeight(rearInside.x,rearInside.z,-100.0f)==placement->floorY,
          "rear floor does not support the player");
    const auto empty=placement->World(3.0f,0.0f,0.0f);
    Check(placement->SupportHeight(empty.x,empty.z,12.5f)==12.5f,
          "helicopter floor leaks outside its footprint");
    const auto walkThroughDoor=[&](const island::HelicopterPoint& outside,
                                   const island::HelicopterPoint& inside,
                                   const bool pilot) {
        island::PlayerMotion player;
        player.SetHelicopter(&*placement);
        player.SetPosition(terrain,outside.x,outside.z);
        const float dx=inside.x-outside.x,dz=inside.z-outside.z,length=std::hypot(dx,dz);
        for(int frame=0;frame<240;++frame) {
            player.Update(terrain,dx/length,dz/length,1.0f/60.0f,
                          std::span<const island::CircularObstacle>{});
            const auto current=player.Position();
            if(pilot?placement->InPilotFootwell(current.x,current.z)
                    :placement->InRearCabin(current.x,current.z)) break;
        }
        const auto p=player.Position();
        Check(pilot?placement->InPilotFootwell(p.x,p.z):placement->InRearCabin(p.x,p.z),
              pilot?"player controller cannot walk into cockpit":"player controller cannot walk into rear cabin");
        Check(std::abs(p.y-(placement->floorY+island::PlayerMotion::EyeHeight))<.002f,
              "player is not standing on helicopter cabin floor");
    };
    walkThroughDoor(pilotOutside,pilotInside,true);
    walkThroughDoor(rearOutside,rearInside,false);
    walkThroughDoor(leftPilotOutside,pilotInside,true);
    walkThroughDoor(leftRearOutside,rearInside,false);
    bool invalidRejected=false;
    const std::array invalid{island::CircularObstacle{NAN,0,1}};
    try { (void)island::HelicopterLayout::Find(terrain,surface,invalid,{}); }
    catch(const std::invalid_argument&){invalidRejected=true;}
    Check(invalidRejected,"invalid helicopter obstacle was accepted");
    std::cout<<"Helicopter site "<<placement->x<<','<<placement->groundY<<','<<placement->z
        <<" yaw "<<placement->yawRadians<<"; pilot/rear doors and four skid contacts valid\n";
}
