#include "IslandSimulator/BunkerLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/PlayerMotion.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}

int main() {
    const island::IslandTerrain terrain;
    const island::BunkerLayout bunker(terrain);
    constexpr float radius=.34f,height=1.9f;
    const auto feet=[&](float across,float depth) {
        return bunker.World(across,bunker.FloorRelative(depth)+.002f,depth);
    };
    Check(terrain.SignedShoreDistance(island::BunkerSite::X,island::BunkerSite::Z)>90,
          "bunker entrance is not inland");
    Check(terrain.Height(island::BunkerSite::X,island::BunkerSite::Z)>8,
          "bunker entrance is on low coastal ground");
    Check(!bunker.IntersectsCapsule(feet(0,-1.7f),radius,height),"portal route is obstructed");
    Check(!bunker.IntersectsCapsule(feet(0,5),radius,height),"ramp route is obstructed");
    Check(!bunker.IntersectsCapsule(feet(0,18),radius,height),"corridor route is obstructed");
    Check(!bunker.IntersectsCapsule(feet(0,34),radius,height),"deep corridor is obstructed");
    Check(!bunker.IntersectsCapsule(feet(4.2f,17),radius,height),"computer room is obstructed");
    Check(!bunker.IntersectsCapsule(feet(-2.1f,29.5f),radius,height),"reactor doorway is obstructed");
    Check(!bunker.IntersectsCapsule(feet(-8.8f,29.5f),radius,height),"reactor perimeter is obstructed");
    Check(!bunker.IntersectsCapsule(feet(3.0f,30),radius,height),"service room is obstructed");
    const auto walk=[&](island::BunkerPoint a,island::BunkerPoint b,const char* message) {
        Check(bunker.SafeTravelFraction(a,b,radius,height)>.999f,message);
    };
    walk(feet(0,-1.7f),feet(0,0),"player cannot pass the open bunker portal");
    walk(feet(0,0),feet(0,9.8f),"player cannot descend the access ramp");
    walk(feet(0,9.8f),feet(0,18),"player cannot enter the main corridor");
    walk(feet(0,17),feet(4.2f,17),"computer-room doorway is not traversable");
    walk(feet(0,29.5f),feet(-2.1f,29.5f),"reactor-room doorway is not traversable");
    walk(feet(-2.1f,29.5f),feet(-3.2f,26.0f),"reactor perimeter route is obstructed");
    Check(bunker.IntersectsCapsule(feet(2.0f,8),radius,height),
          "corridor wall is not solid");
    Check(bunker.IntersectsCapsule(feet(-5.55f,29.5f),radius,height),
          "generator core is not solid");
    float floor=0;
    const auto corridor=bunker.World(0,0,7);
    Check(bunker.SupportHeight(corridor.x,corridor.z,
          bunker.EntranceGroundY()+bunker.FloorRelative(7),floor),
          "bunker floor does not support an entering player");
    Check(std::abs(floor-(bunker.EntranceGroundY()+bunker.FloorRelative(7)))<.0001f,
          "bunker floor returned the wrong height");
    Check(!bunker.SupportHeight(corridor.x,corridor.z,terrain.Height(corridor.x,corridor.z)+4,floor),
          "underground floor captures a player on the ridge above");
    const auto portalSurface=bunker.World(0,0,0);
    Check(bunker.HidesTerrain(portalSurface.x,terrain.Height(portalSurface.x,portalSurface.z),
                              portalSurface.z),
          "terrain still seals the open bunker portal");
    const auto coveredRoof=bunker.World(0,bunker.FloorRelative(20)+4.5f,20);
    Check(!bunker.HidesTerrain(coveredRoof.x,coveredRoof.y,coveredRoof.z),
          "bunker excavation removes valid ridge cover above the roof");
    const auto deepListener=bunker.World(0,bunker.FloorRelative(24)
        +island::PlayerMotion::EyeHeight,24);
    Check(island::BunkerSite::SoundPresence(deepListener.x,deepListener.y,deepListener.z,
              bunker.EntranceGroundY())>.99f,
          "deep bunker room does not activate its acoustic zone");
    Check(island::BunkerSite::SoundPresence(deepListener.x,
              terrain.Height(deepListener.x,deepListener.z)+island::PlayerMotion::EyeHeight,
              deepListener.z,bunker.EntranceGroundY())==0.0f,
          "bunker acoustic zone leaks onto the ridge above");

    // Exercise the real controller, not only abstract segment collision: approach from
    // ordinary terrain, cross the threshold, descend well below grade, then return out.
    island::PlayerMotion player;
    player.SetBunker(&bunker);
    const auto outside=bunker.World(0,0,-4.0f);
    player.SetPosition(terrain,outside.x,outside.z);
    const auto inward=bunker.World(0,0,1);
    const auto origin=bunker.World(0,0,0);
    float directionX=inward.x-origin.x,directionZ=inward.z-origin.z;
    for(int i=0;i<240;++i)player.Update(terrain,directionX,directionZ,1.0f/60.0f);
    auto local=bunker.Local({player.Position().x,player.Position().y-island::PlayerMotion::EyeHeight,
                             player.Position().z});
    Check(local.z>7.0f,"real player controller cannot enter and descend through bunker portal");
    Check(std::abs(local.y-bunker.FloorRelative(local.z))<.08f,
          "player is not supported by the bunker ramp after entry");
    for(int i=0;i<300;++i)player.Update(terrain,-directionX,-directionZ,1.0f/60.0f);
    local=bunker.Local({player.Position().x,player.Position().y-island::PlayerMotion::EyeHeight,
                        player.Position().z});
    Check(local.z<-.8f,"real player controller cannot leave the bunker");
    std::cout<<"Bunker inland entrance, ramp, corridor, computer room, reactor room and exit routes valid\n";
}
