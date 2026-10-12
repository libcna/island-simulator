#include "IslandSimulator/SpiderColony.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
constexpr float Pi=std::numbers::pi_v<float>,Contact=.0001f;
SpiderPoint Add(SpiderPoint a,SpiderPoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
SpiderPoint Sub(SpiderPoint a,SpiderPoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
SpiderPoint Mul(SpiderPoint a,float s){return {a.x*s,a.y*s,a.z*s};}
float Dot(SpiderPoint a,SpiderPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
SpiderPoint Cross(SpiderPoint a,SpiderPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
SpiderPoint Unit(SpiderPoint p){return Mul(p,1/std::hypot(p.x,p.y,p.z));}
float Wrap(float a){return std::remainder(a,2*Pi);}
std::uint32_t Mix(std::uint32_t v){v^=v>>16;v*=0x7feb352dU;v^=v>>15;v*=0x846ca68bU;return v^(v>>16);}
}
SpiderPoint SpiderGroundPose::ToWorld(SpiderPoint p) const noexcept {
    return Add(origin,Add(Add(Mul(right,p.x),Mul(up,p.y)),Mul(forward,p.z)));
}
SpiderPoint SpiderGroundPose::ToLocal(SpiderPoint p) const noexcept {
    p=Sub(p,origin);return {Dot(p,right),Dot(p,up),Dot(p,forward)};
}
SpiderGroundPose SpiderColony::Frame(float x,float z,float heading) const {
    SpiderGroundPose pose{};
    const auto sample=surface_.Sample(x,z);
    pose.origin={x,sample.height,z};pose.up={sample.normal.x,sample.normal.y,sample.normal.z};
    pose.right=Unit(Cross(pose.up,{std::cos(heading),0,std::sin(heading)}));
    pose.forward=Unit(Cross(pose.right,pose.up));
    return pose;
}
SpiderGroundPose SpiderColony::Pose(const SpiderState& state,const Feet& feet) const {
    auto pose=Frame(state.x,state.z,state.heading);
    for(std::size_t leg=0;leg<8;++leg) {
        pose.feet[leg]=feet[leg].position;pose.stance[leg]=!feet[leg].swinging;
        pose.legs[leg]=SpiderPose::Solve(leg,pose.ToLocal(feet[leg].position));
    }
    return pose;
}
bool SpiderColony::Valid(const SpiderGroundPose& pose) const {
    if(pose.up.y<.95f)return false;
    for(const auto& leg:pose.legs) {
        if(leg.reachLimited)return false;
        const std::array<SpiderPoint,4> joints{leg.root,leg.hip,leg.knee,leg.foot};
        constexpr std::array<float,4> radii{.0038f,.0032f,.0023f,0};
        for(std::size_t segment=0;segment<3;++segment)for(int step=0;step<=4;++step) {
            const float t=static_cast<float>(step)/4;
            const auto p=pose.ToWorld(Add(Mul(joints[segment],1-t),Mul(joints[segment+1],t)));
            const float r=radii[segment]*(1-t)+radii[segment+1]*t;
            if(std::hypot(p.x-pose.origin.x,p.z-pose.origin.z)>SpiderHabitat::BodyRadius
                ||p.y-surface_.Sample(p.x,p.z).height<r-.00002f)return false;
        }
    }
    return true;
}
SpiderColony::SpiderColony(const IslandTerrain& terrain,const JungleLayout& jungle,
    std::span<const CircularObstacle> obstacles):surface_(terrain) {
    const auto homes=SpiderHabitat::FindGround(terrain,surface_,jungle,WorldCollisionGrid(obstacles));
    if(!homes)throw std::runtime_error("No complete ground spider habitat");
    homes_=*homes;
    for(std::size_t i=0;i<states_.size();++i) {
        auto& s=states_[i];const auto h=homes_[i];
        s.x=h.x;s.y=h.y;s.z=h.z;s.heading=Wrap(static_cast<float>(i)*2.4f);
        const auto frame=Frame(s.x,s.z,s.heading);
        for(std::size_t leg=0;leg<8;++leg) {
            auto p=frame.ToWorld(SpiderPose::NeutralFoot(leg));
            p.y=surface_.Sample(p.x,p.z).height+Contact;
            feet_[i][leg].position=p;
        }
        s.pose=Pose(s,feet_[i]);
        if(!Valid(s.pose))throw std::runtime_error("Initial spider pose does not fit ground");
        intents_[i]={.5f+.13f*static_cast<float>(i),s.heading,0};
    }
}
void SpiderColony::AdvanceFeet(std::size_t index,float dt) {
    auto& state=states_[index];auto& feet=feet_[index];
    for(std::size_t leg=0;leg<8;++leg)if(feet[leg].swinging) {
        auto trial=feet;auto& foot=trial[leg];
        foot.progress=std::min(1.0f,foot.progress+dt/.12f);
        const float t=foot.progress,u=t*t*(3-2*t),arch=std::sin(Pi*t);
        foot.position=Add(Mul(foot.start,1-u),Mul(foot.target,u));
        foot.position.y=std::max(foot.position.y,surface_.Sample(foot.position.x,foot.position.z).height+Contact)
            +.008f*arch*arch;
        foot.swinging=t<1;
        if(Valid(Pose(state,trial)))feet=trial;
    }
    const auto frame=Frame(state.x,state.z,state.heading);
    const auto active=std::count_if(feet.begin(),feet.end(),[](const auto& f){return f.swinging;});
    std::size_t selected=8;float largest=.0035f;
    if(active<2)for(std::size_t leg=0;leg<8;++leg)if(!feet[leg].swinging) {
        const auto neutral=frame.ToWorld(SpiderPose::NeutralFoot(leg));
        const float error=std::hypot(neutral.x-feet[leg].position.x,neutral.z-feet[leg].position.z);
        if(error>largest){largest=error;selected=leg;}
    }
    if(selected<8) {
        auto trial=feet;auto& foot=trial[selected];
        auto local=SpiderPose::NeutralFoot(selected);
        if(state.behavior!=SpiderBehavior::Rest)local.z+=.004f;
        foot.target=frame.ToWorld(local);
        foot.target.y=surface_.Sample(foot.target.x,foot.target.z).height+Contact;
        auto landing=trial;landing[selected].position=foot.target;
        if(Valid(Pose(state,landing))) {
            foot.start=foot.position;foot.progress=0;foot.swinging=true;feet=trial;
        }
    }
    state.pose=Pose(state,feet);
}
void SpiderColony::Step(std::size_t index,float dt,SpiderPoint observer,bool observerValid) {
    auto& state=states_[index];auto& intent=intents_[index];
    AdvanceFeet(index,dt);
    intent.remaining-=dt;
    const bool retreat=observerValid&&std::hypot(state.x-observer.x,state.y+.04f-observer.y,state.z-observer.z)<2.5f;
    if(retreat) {
        state.behavior=SpiderBehavior::Retreat;intent.remaining=.5f;
        if(std::hypot(state.x-observer.x,state.z-observer.z)>.001f)
            intent.heading=std::atan2(state.z-observer.z,state.x-observer.x);
    } else if(intent.remaining<=0) {
        const auto bits=Mix(SpiderHabitat::Seed+static_cast<std::uint32_t>(index)*7919U+(++intent.decisions)*104729U);
        state.behavior=bits%4==0?SpiderBehavior::Rest:SpiderBehavior::Crawl;
        intent.remaining=1.5f+static_cast<float>((bits>>8)%30)/10;
        intent.heading=static_cast<float>((bits>>16)%4096)/4096*2*Pi-Pi;
        if(std::hypot(state.x-homes_[index].x,state.z-homes_[index].z)>.40f)
            intent.heading=std::atan2(homes_[index].z-state.z,homes_[index].x-state.x);
    }
    if(state.behavior==SpiderBehavior::Rest){state.speed=0;return;}
    auto trial=state;
    trial.heading=Wrap(state.heading+std::clamp(Wrap(intent.heading-state.heading),-.6f*dt,.6f*dt));
    const float speed=state.behavior==SpiderBehavior::Retreat?RetreatSpeed:CrawlSpeed;
    // Turn before retreating rather than walking toward the observer while the
    // body catches up with the desired heading. Turning still replants feet.
    const float distance=retreat&&std::abs(Wrap(intent.heading-trial.heading))>Pi/3?0:speed*dt;
    trial.x+=std::cos(trial.heading)*distance;trial.z+=std::sin(trial.heading)*distance;
    if(std::hypot(trial.x-homes_[index].x,trial.z-homes_[index].z)>SpiderHabitat::PatrolRadius) {
        // Retain the turn even when translation reaches the patrol boundary.
        // Otherwise an outward-facing animal could never turn back into its disk.
        trial.x=state.x;trial.z=state.z;if(!retreat)intent.remaining=0;
    }
    trial.y=surface_.Sample(trial.x,trial.z).height;trial.pose=Pose(trial,feet_[index]);
    if(!Valid(trial.pose)){state.speed=0;return;}
    trial.speed=std::hypot(trial.x-state.x,trial.y-state.y,trial.z-state.z)/dt;
    state=trial;
}
void SpiderColony::Update(float dt,SpiderPoint observer) {
    if(!std::isfinite(dt)||dt<=0)return;
    dt=std::min(dt,.1f);
    const bool valid=std::isfinite(observer.x)&&std::isfinite(observer.y)&&std::isfinite(observer.z);
    const int steps=static_cast<int>(std::ceil(dt/.02f));
    for(int step=0;step<steps;++step)for(std::size_t i=0;i<states_.size();++i)
        Step(i,dt/static_cast<float>(steps),observer,valid);
}
}
