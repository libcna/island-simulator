#include "IslandSimulator/GoatHerd.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace island {
namespace {
constexpr float Pi=std::numbers::pi_v<float>,Tau=2*Pi;
float Wrap(float angle) { return std::remainder(angle,Tau); }
std::uint32_t Mix(std::uint32_t x) {
    x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;return x^(x>>16);
}
}
GoatHerd::GoatHerd(const IslandTerrain& terrain,const JungleLayout& jungle,
    std::span<const CircularObstacle> obstacles):terrain_(terrain),surface_(terrain)
{
    const auto homes=GoatHabitat::Find(terrain,surface_,jungle,WorldCollisionGrid(obstacles));
    if(!homes) throw std::runtime_error("No complete safe goat habitat");
    homes_=*homes;
    // Same exclusion envelopes as placement, indexed once rather than scanning plants per step.
    std::vector<CircularObstacle> occupied(obstacles.begin(),obstacles.end());
    for(const auto& tree:jungle.Canopy()) occupied.push_back({tree.x,tree.z,
        std::max(0.0f,GoatHabitat::TreeClearance*tree.scale-GoatHabitat::BodyClearance)});
    for(const auto& plant:jungle.Understory()) occupied.push_back({plant.x,plant.z,1.8f*plant.scale});
    for(const auto& bush:jungle.Bushes()) occupied.push_back({bush.x,bush.z,1.2f*bush.scale});
    clearance_.Reset(occupied);
    for(std::size_t i=0;i<states_.size();++i) {
        const auto home=homes_[i];
        states_[i].x=home.x;states_[i].y=home.y;states_[i].z=home.z;
        states_[i].heading=Wrap(static_cast<float>(i)*2.4f);
        states_[i].scale=i==states_.size()-1?.75f:1;
        intents_[i].remaining=1+static_cast<float>(i)*.7f;
        intents_[i].targetHeading=states_[i].heading;
    }
}
bool GoatHerd::IsValidPosition(std::size_t index,float x,float z) const {
    if(index>=states_.size()) return false;
    return std::hypot(x-homes_[index].x,z-homes_[index].z)<=HomeRadius
        &&std::hypot(x-homes_[0].x,z-homes_[0].z)<=GoatHabitat::HerdRadius
        &&!clearance_.IntersectsCircle(x,z,GoatHabitat::BodyClearance)
        &&GoatHabitat::AcceptsGround(terrain_,surface_,x,z);
}
bool GoatHerd::TryMove(std::size_t index,float heading,float distance) {
    auto& state=states_[index];
    const auto normal=surface_.Sample(state.x,state.z).normal;
    const float gradient=-(normal.x*std::cos(heading)+normal.z*std::sin(heading))/normal.y;
    const float horizontal=distance/std::hypot(1.0f,gradient);
    const float x=state.x+std::cos(heading)*horizontal,z=state.z+std::sin(heading)*horizontal;
    const float midX=(x+state.x)*.5f,midZ=(z+state.z)*.5f;
    if(!IsValidPosition(index,x,z)||!IsValidPosition(index,midX,midZ)
        ||clearance_.IntersectsCircle(midX,midZ,GoatHabitat::BodyClearance+horizontal*.5f)) return false;
    for(std::size_t other=0;other<states_.size();++other) if(other!=index)
        if(std::hypot(midX-states_[other].x,midZ-states_[other].z)
            <2*GoatHabitat::BodyClearance+horizontal*.5f) return false;
    const float y=surface_.Sample(x,z).height;
    const float travelled=std::hypot(x-state.x,y-state.y,z-state.z);
    const float phase=std::fmod(state.phase+GoatPhaseAdvance(travelled,state.speed,state.scale),Tau);
    const auto pose=FitGoatGroundPose(terrain_,surface_,x,z,heading,phase,state.speed,state.scale);
    for(const auto& leg:pose.legs) if(leg.reachLimited) return false;
    state.x=x;state.y=y;state.z=z;state.heading=heading;state.phase=phase;
    return true;
}
void GoatHerd::Step(std::size_t index,float dt,GoatPoint observer,bool observerValid) {
    auto& state=states_[index];auto& intent=intents_[index];
    intent.remaining-=dt;
    intent.avoidance=std::max(0.0f,intent.avoidance-dt);
    const bool retreat=observerValid&&std::hypot(state.x-observer.x,state.y+.8f-observer.y,state.z-observer.z)<7;
    if(retreat) {
        state.behavior=GoatBehavior::Retreat;intent.remaining=.75f;
        const float dx=state.x-observer.x,dz=state.z-observer.z;
        if(intent.avoidance==0&&std::hypot(dx,dz)>.01f) intent.targetHeading=std::atan2(dz,dx);
    } else if(intent.remaining<=0) {
        const auto bits=Mix(GoatHabitat::Seed+static_cast<std::uint32_t>(index)*7919U+(++intent.decisions)*104729U);
        state.behavior=bits%5<3?GoatBehavior::Walk:(bits%5==3?GoatBehavior::Graze:GoatBehavior::Rest);
        intent.remaining=3+static_cast<float>((bits>>8)%50)/10;
        intent.targetHeading=static_cast<float>((bits>>16)%4096)/4096*Tau-Pi;
        if(std::hypot(state.x-homes_[index].x,state.z-homes_[index].z)>HomeRadius-3)
            intent.targetHeading=std::atan2(homes_[index].z-state.z,homes_[index].x-state.x);
    }
    const float desired=state.behavior==GoatBehavior::Retreat?MaximumSpeed:
        (state.behavior==GoatBehavior::Walk?.65f:0);
    state.speed+=std::clamp(desired-state.speed,-1.5f*dt,.8f*dt);
    const float graze=state.behavior==GoatBehavior::Graze&&state.speed<.05f?1.0f:0.0f;
    state.grazingPose+=std::clamp(graze-state.grazingPose,-2*dt,dt);
    if(state.speed<=.0001f) { state.speed=0;return; }
    const float heading=Wrap(state.heading+std::clamp(Wrap(intent.targetHeading-state.heading),-1.8f*dt,1.8f*dt));
    if(!TryMove(index,heading,state.speed*dt)) {
        state.speed=0;
        const auto turn=FitGoatGroundPose(terrain_,surface_,state.x,state.z,heading,state.phase,0,state.scale);
        if(std::none_of(turn.legs.begin(),turn.legs.end(),[](const auto& leg){return leg.reachLimited;}))
            state.heading=heading;
        // A bounded turn request for later steps, not a teleport or forced obstacle escape.
        if(intent.avoidance==0) {
            intent.targetHeading=Wrap(state.heading+(index%2==0?1.0f:-1.0f)*Pi*.65f);
            intent.avoidance=1.5f;
        }
        if(!retreat) intent.remaining=std::min(intent.remaining,.7f);
    }
}
void GoatHerd::Update(float dt,GoatPoint observer) {
    if(!std::isfinite(dt)||dt<=0) return;
    dt=std::min(dt,.1f);
    const int steps=std::min(5,static_cast<int>(std::ceil(dt/.02f)));
    const bool observerValid=std::isfinite(observer.x)&&std::isfinite(observer.y)&&std::isfinite(observer.z);
    for(int step=0;step<steps;++step) for(std::size_t i=0;i<states_.size();++i)
        Step(i,dt/static_cast<float>(steps),observer,observerValid);
}
}
