#include "IslandSimulator/FlightMotion.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

void Check(bool condition,const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    using namespace island;
    const IslandTerrain terrain;
    const RenderedTerrainSurface surface(terrain);
    FlightMotion flight;
    flight.Update(1,1,1,true,1);
    Check(!flight.IsActive(),"inactive flight must not move or activate");
    auto valid=[&](PlayerPosition p) {
        Check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z),"finite flight coordinates");
        Check(std::hypot(p.x,p.z)<=FlightMotion::Radius+.0002f,"flight remains inside radial boundary");
        const float ground=surface.Sample(p.x,p.z).height;
        Check(p.y>=std::max(0.0f,ground)+FlightMotion::Clearance-.0002f,"no terrain/sea penetration");
        Check(p.y<=ground+100.0002f,"at most 100m above rendered terrain");
    };
    flight.Activate(terrain,{1e30f,-1e30f,-1e30f}); valid(flight.Position());
    flight.Activate(terrain,{std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<float>::infinity(),0}); valid(flight.Position());
    auto before=flight.Position();
    flight.Update(1,1,1,true,std::numeric_limits<float>::quiet_NaN());
    Check(flight.Position().x==before.x&&flight.Position().y==before.y,"invalid time is neutral");
    flight.Update(0,1,0,false,.1f);
    Check(flight.Position().y>before.y+1.19f,"E-style vertical movement works");
    for(int i=0;i<200;++i) flight.Update(0,1,0,true,1);
    valid(flight.Position());
    Check(flight.Position().y>surface.Sample(0,0).height+99.9f,"ceiling is reachable");
    for(int i=0;i<200;++i) flight.Update(0,-1,0,true,.1f);
    valid(flight.Position());
    flight.Activate(terrain,{300,50,0});
    for(int i=0;i<30;++i) flight.Update(1,0,0,true,.1f);
    valid(flight.Position());
    Check(std::abs(flight.Position().x-FlightMotion::Radius)<.001f,"outward flight reaches and respects radius");
    flight.Activate(terrain,{0,50,0});
    before=flight.Position();
    flight.Update(1.3660254f,0,.3660254f,false,.1f);
    Check(std::abs((flight.Position().x-before.x)/(flight.Position().z-before.z)
                   -1.3660254f/.3660254f)<.001f,"normalization preserves rotated diagonal heading");
    // Dense independent checks along accepted segments, including triangle crossings,
    // horizontal travel near terrain and ceiling, diagonal input and clamped stalls.
    for(int site=0;site<48;++site) {
        const float angle=site*.73f;
        const float radius=20.0f+site*6.0f;
        const float x=std::cos(angle)*radius,z=std::sin(angle)*radius;
        flight.Activate(terrain,{x,surface.Sample(x,z).height+(site%3==0 ? 100.0f : 1.72f),z});
        for(int step=0;step<120;++step) {
            before=flight.Position();
            flight.Update(std::sin(angle+step*.04f),static_cast<float>((step/20)%3-1),
                          std::cos(angle+step*.04f),true,step%2 ? .1f : 10.0f);
            const auto after=flight.Position();
            Check(std::hypot(after.y-before.y,std::hypot(after.x-before.x,after.z-before.z))<3.601f,
                  "normalization and stall clamp bound 3D speed");
            for(int sample=0;sample<=32;++sample) {
                const float t=sample/32.0f;
                valid({std::lerp(before.x,after.x,t),std::lerp(before.y,after.y,t),std::lerp(before.z,after.z,t)});
            }
        }
    }
    const VegetationLayout vegetation(terrain);
    const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,vegetation,jungle);
    const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> obstacles;
    for (auto list:std::array<std::span<const CircularObstacle>,4>{
            vegetation.Obstacles(),jungle.Obstacles(),props.Obstacles(),wreck.Obstacles()})
        obstacles.insert(obstacles.end(),list.begin(),list.end());
    const WorldCollisionGrid world(obstacles);
    for(const auto p:std::array<PlayerPosition,4>{{{0,90,-330},{330,90,0},{0,100,0},{-330,80,0}}}) {
        flight.Activate(terrain,p);
        const auto landing=flight.FindLanding(terrain,world);
        Check(landing.has_value(),"actual island has a safe dry landing from offshore and interior flight");
        PlayerMotion player;
        player.SetPosition(terrain,landing->x,landing->z);
        player.Update(terrain,0,0,0,world);
        Check(player.Mode()==LocomotionMode::Walking,"flight off returns to walking, not stranded swimming");
        Check(player.GroundSlopeRadians()<=PlayerMotion::MaximumWalkSlopeRadians,"landing slope is walkable");
        Check(!world.IntersectsCircle(player.Position().x,player.Position().z,PlayerMotion::CapsuleRadius),
              "landing does not embed the player in a solid prop");
        Check(player.Position().y>surface.Sample(player.Position().x,player.Position().z).height,
              "landing camera stays above rendered terrain");
    }
    const std::array<CircularObstacle,1> blocked{{{0,0,1000}}};
    Check(!flight.FindLanding(terrain,WorldCollisionGrid(blocked)),"fully blocked world refuses unsafe landing");
    flight.Deactivate();
    Check(!flight.IsActive(),"deactivation exits flight");
    std::cout<<"Flight bounds, swept terrain/ceiling checks and actual-island dry landing passed\n";
}
