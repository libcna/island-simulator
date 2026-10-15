#include "IslandSimulator/DragonflySwarm.hpp"

#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/StreamRoute.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island { namespace {
constexpr float Pi=std::numbers::pi_v<float>,TwoPi=2*Pi;
struct Habitat {float x,z,radius;};
const std::array<Habitat,DragonflySwarm::HabitatCount> Habitats={{
    {IslandEnvironmentField::SeasonalWetlandX,IslandEnvironmentField::SeasonalWetlandZ,10.5f},
    {-103.0f,StreamRoute::CentreZ(-103.0f),8.5f},
    {-137.0f,StreamRoute::CentreZ(-137.0f),9.0f}}};
float Wrap(float a)noexcept{while(a>Pi)a-=TwoPi;while(a<-Pi)a+=TwoPi;return a;}
float MoveAngle(float a,float b,float step)noexcept{return Wrap(a+std::clamp(Wrap(b-a),-step,step));}
}

DragonflySwarm::DragonflySwarm(const IslandTerrain& terrain):terrain_(terrain){
    for(std::size_t i=0;i<Count;++i){Initialize(i);states_[i]=agents_[i].state;homes_[i]=agents_[i].home;}
}
void DragonflySwarm::Initialize(std::size_t i){
    auto& a=agents_[i];const auto& h=Habitats[i/3];a.home={h.x,h.z,h.radius};
    float angle=.41f+static_cast<float>(i)*2.39996323f,radius=1.2f+static_cast<float>(i%3)*1.15f;
    a.state.x=h.x+std::cos(angle)*radius;a.state.z=h.z+std::sin(angle)*radius;
    for(int n=0;n<8&&!Valid(a.state.x,a.state.z,a.home);++n){radius*=.72f;a.state.x=h.x+std::cos(angle)*radius;a.state.z=h.z+std::sin(angle)*radius;}
    if(!Valid(a.state.x,a.state.z,a.home))throw std::runtime_error("No suitable dragonfly habitat");
    a.altitude=.72f+static_cast<float>(i%3)*.24f;
    a.state.y=terrain_.Height(a.state.x,a.state.z)+a.altitude;
    // The authored mesh is expressed in metre-sized local units.  Keep even the
    // largest tropical individuals around a ten-centimetre body length so they do
    // not read as birds when they cross a clearing close to the player.
    a.state.headingRadians=Wrap(angle+Pi*.5f);
    a.state.scale=.17f+static_cast<float>(i%3)*.010f;
    a.state.colorVariant=static_cast<std::uint8_t>(i%3);
    a.state.behavior=i%3==0?DragonflyBehavior::Hover:DragonflyBehavior::Patrol;
    a.seconds=1.7f+static_cast<float>(i%5)*.71f;a.turnA=.7f+static_cast<float>(i)*1.31f;
    a.turnB=1.2f+static_cast<float>(i)*2.07f;a.vertical=.3f+static_cast<float>(i)*1.73f;
    a.state.wingPhase=static_cast<float>(i)*.83f;
}
bool DragonflySwarm::Valid(float x,float z,const DragonflyHome& h)const noexcept{
    const auto s=terrain_.Sample(x,z);const auto inland=IslandEnvironmentField::SampleInland(x,z,s.signedShoreDistance);
    return std::hypot(x-h.x,z-h.z)<=h.radius&&s.height>0&&s.signedShoreDistance>12
        &&s.slopeRadians<.55f&&inland.moisture>.34f;
}
void DragonflySwarm::Update(float dt,float ox,float oy,float oz)noexcept{
    const float safe=std::isfinite(dt)?std::clamp(dt,0.0f,.1f):0;const bool observer=std::isfinite(ox)&&std::isfinite(oy)&&std::isfinite(oz);
    ++frame_;fullCount_=reducedCount_=dormantCount_=0;
    for(std::size_t i=0;i<Count;++i){auto& a=agents_[i];const float distance=observer?std::hypot(a.state.y-oy,std::hypot(a.state.x-ox,a.state.z-oz)):std::numeric_limits<float>::infinity();
        a.state.simulationTier=WildlifeActivationPolicy::Classify(distance,FullSimulationDistance,ReducedSimulationDistance);
        switch(a.state.simulationTier){case WildlifeSimulationTier::Full:++fullCount_;break;case WildlifeSimulationTier::Reduced:++reducedCount_;break;case WildlifeSimulationTier::Dormant:++dormantCount_;break;}
        a.accumulated+=safe;const auto divisor=WildlifeActivationPolicy::UpdateDivisor(a.state.simulationTier);
        if((frame_+i)%divisor==0){UpdateAgent(a,std::min(a.accumulated,.5f),ox,oz,observer);a.accumulated=0;}states_[i]=a.state;}
    assert(fullCount_+reducedCount_+dormantCount_==static_cast<int>(Count));
}
void DragonflySwarm::UpdateAgent(Agent& a,float dt,float ox,float oz,bool observer)noexcept{
    if(dt<=0)return;
    const float d=observer?std::hypot(a.state.x-ox,a.state.z-oz):9999;
    if(d<2.8f&&a.state.behavior!=DragonflyBehavior::Dart){a.state.behavior=DragonflyBehavior::Dart;a.seconds=.85f;a.state.headingRadians=std::atan2(a.state.z-oz,a.state.x-ox);}
    else if((a.seconds-=dt)<=0){++a.transitions;if(a.state.behavior==DragonflyBehavior::Dart){a.state.behavior=DragonflyBehavior::Hover;a.seconds=1.1f+.25f*(a.transitions%4);}else if(a.state.behavior==DragonflyBehavior::Hover){a.state.behavior=DragonflyBehavior::Patrol;a.seconds=4.2f+.6f*(a.transitions%5);}else{a.state.behavior=a.transitions%3==0?DragonflyBehavior::Hover:DragonflyBehavior::Dart;a.seconds=a.state.behavior==DragonflyBehavior::Hover?1.4f:.75f;}}
    a.turnA=std::fmod(a.turnA+dt*1.03f,TwoPi);a.turnB=std::fmod(a.turnB+dt*.37f,TwoPi);a.vertical=std::fmod(a.vertical+dt*2.3f,TwoPi);
    const float target=a.state.behavior==DragonflyBehavior::Hover?.05f:(a.state.behavior==DragonflyBehavior::Dart?2.8f:1.05f);
    a.state.speed+=std::clamp(target-a.state.speed,-dt*5.0f,dt*8.0f);
    const float home=std::hypot(a.state.x-a.home.x,a.state.z-a.home.z);
    if(home>a.home.radius*.72f)a.state.headingRadians=MoveAngle(a.state.headingRadians,std::atan2(a.home.z-a.state.z,a.home.x-a.state.x),dt*3.4f);
    else if(a.state.behavior!=DragonflyBehavior::Hover)a.state.headingRadians=Wrap(a.state.headingRadians+dt*(std::sin(a.turnA)*1.25f+std::sin(a.turnB)*.42f));
    const float nx=a.state.x+std::cos(a.state.headingRadians)*a.state.speed*dt,nz=a.state.z+std::sin(a.state.headingRadians)*a.state.speed*dt;
    if(Valid(nx,nz,a.home)){a.state.x=nx;a.state.z=nz;}else a.state.headingRadians=std::atan2(a.home.z-a.state.z,a.home.x-a.state.x);
    const float ground=terrain_.Height(a.state.x,a.state.z);const float targetY=ground+a.altitude+std::sin(a.vertical)*(a.state.behavior==DragonflyBehavior::Hover?.06f:.20f);
    a.state.y+=std::clamp(targetY-a.state.y,-dt*1.8f,dt*1.8f);a.state.wingPhase=std::fmod(a.state.wingPhase+dt*55.0f,TwoPi);
    a.state.bankRadians=std::clamp(std::sin(a.turnA)*.16f*(a.state.speed/1.05f),-.34f,.34f);
}
}
