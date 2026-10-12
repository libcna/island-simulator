#include "IslandSimulator/BearMotion.hpp"

#include "IslandSimulator/CaveSite.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
constexpr float Pi=std::numbers::pi_v<float>,Tau=2*Pi,ContactSeparation=.0003f;
float Wrap(float value){return std::remainder(value,Tau);}
BearPoint Add(BearPoint a,BearPoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
BearPoint Sub(BearPoint a,BearPoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
BearPoint Mul(BearPoint a,float b){return {a.x*b,a.y*b,a.z*b};}
float Dot(BearPoint a,BearPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
BearPoint Cross(BearPoint a,BearPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
BearPoint Unit(BearPoint p){const float length=std::hypot(p.x,p.y,p.z);
    if(!std::isfinite(length)||length<1e-7f)throw std::invalid_argument("Invalid bear pose direction");
    return Mul(p,1/length);}
bool Finite(BearPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
std::uint32_t Mix(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;return x^(x>>16);}
float UnitRandom(std::uint32_t value){return static_cast<float>(Mix(value)&0x00ffffffU)/16777216.0f;}
BearPoint LocalRoot(std::size_t leg) {
    const bool left=leg<2,front=leg%2==1;
    return {left?-.38f:.38f,.74f,front?.72f:-.68f};
}
BearLegPose SolveLeg(BearPoint root,BearPoint paw,std::size_t leg,bool stance,BearPoint soleNormal) {
    BearLegPose result;result.root=root;result.stance=stance;result.soleNormal=soleNormal;
    auto delta=Sub(paw,root);const float requested=std::sqrt(Dot(delta,delta));
    constexpr float upper=BearLegPose::UpperLength,lower=BearLegPose::LowerLength;
    const float distance=std::clamp(requested,std::abs(upper-lower)+.0001f,upper+lower-.001f);
    result.reachLimited=std::abs(requested-distance)>.00002f;
    const auto direction=requested>1e-7f?Mul(delta,1/requested):BearPoint{0,-1,0};
    result.paw=Add(root,Mul(direction,distance));
    const bool front=leg%2==1;
    BearPoint hint{0,0,front?-.7f:.7f};
    auto bend=Sub(hint,Mul(direction,Dot(hint,direction)));
    if(Dot(bend,bend)<1e-7f)bend={0,1,0};
    bend=Unit(bend);
    const float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
    const float offset=std::sqrt(std::max(0.0f,upper*upper-along*along));
    result.joint=Add(root,Add(Mul(direction,along),Mul(bend,offset)));
    return result;
}
}

BearPoint BearPose::ToWorld(BearPoint p)const noexcept{return {origin.x+right.x*p.x+up.x*p.y+forward.x*p.z,
    origin.y+right.y*p.x+up.y*p.y+forward.y*p.z,origin.z+right.z*p.x+up.z*p.y+forward.z*p.z};}
BearPoint BearPose::ToLocal(BearPoint p)const noexcept {const auto d=Sub(p,origin);return {
    Dot(d,right),Dot(d,up),Dot(d,forward)};}

bool BearMotion::AcceptsGround(float x,float z)const noexcept {
    if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>252||std::abs(z)>252
        ||CaveSite::Reserves(x,z,BodyClearance))return false;
    const auto sample=terrain_.Sample(x,z);
    const auto field=IslandEnvironmentField::SampleInland(x,z,sample.signedShoreDistance);
    if(sample.signedShoreDistance<62||sample.height<5||sample.slopeRadians>.36f
        ||field.saltExposure>.08f||field.drainage>.88f)return false;
    if(solids_.IntersectsCircle(x,z,BodyClearance))return false;
    const auto centre=ground_.Sample(x,z);
    for(const auto offset:std::array<BearPoint,4>{{{.85f,0,.85f},{-.85f,0,.85f},{.85f,0,-.85f},{-.85f,0,-.85f}}}) {
        const auto edge=ground_.Sample(x+offset.x,z+offset.z);
        if(edge.normal.y<.91f||std::abs(edge.height-centre.height)>.48f)return false;
    }
    return true;
}
bool BearMotion::IsValidPosition(float x,float z)const noexcept{return AcceptsGround(x,z)
    &&std::hypot(x-state_.homeX,z-state_.homeZ)<=HomeRadius;}

BearMotion::BearMotion(const IslandTerrain& terrain,std::span<const CircularObstacle> obstacles)
    :terrain_(terrain),ground_(terrain),solids_(obstacles) {
    for(const auto& obstacle:obstacles)if(!std::isfinite(obstacle.x)||!std::isfinite(obstacle.z)
        ||!std::isfinite(obstacle.radius)||obstacle.radius<0)throw std::invalid_argument("Bear obstacles must be finite");
    bool found=false;
    for(unsigned attempt=0;attempt<700&&!found;++attempt) {
        const float angle=attempt*2.39996323f,radius=std::sqrt(static_cast<float>(attempt))*.95f;
        const float x=96+std::cos(angle)*radius,z=104+std::sin(angle)*radius;
        if(AcceptsGround(x,z)) {state_.x=state_.homeX=x;state_.z=state_.homeZ=z;found=true;}
    }
    if(!found)throw std::runtime_error("No safe habitat for the single island bear");
    state_.y=ground_.Sample(state_.x,state_.z).height;state_.heading=-1.1f;
    targetHeading_=state_.heading;state_.behavior=BearBehavior::Idle;
    for(std::size_t leg=0;leg<4;++leg) {
        const auto p=NeutralFoot(state_,leg);feet_[leg].position=feet_[leg].start=feet_[leg].target=p;
    }
    state_.pose=ContactPose(state_,feet_);
    for(const auto& leg:state_.pose.legs)if(leg.reachLimited)
        throw std::runtime_error("Initial bear stance cannot reach the rendered terrain");
}

BearPose BearMotion::BodyFrame(const BearState& state)const {
    BearPose pose;pose.origin={state.x,ground_.Sample(state.x,state.z).height,state.z};
    const auto n=ground_.Sample(state.x,state.z).normal;pose.up=Unit({n.x,n.y,n.z});
    pose.right=Unit(Cross(pose.up,{std::sin(state.heading),0,std::cos(state.heading)}));
    pose.forward=Unit(Cross(pose.right,pose.up));
    pose.bodyBob=.016f*std::abs(std::sin(state.phase*2))*std::clamp(state.speed/MaximumSpeed,0.0f,1.0f);
    return pose;
}
BearPoint BearMotion::NeutralFoot(const BearState& state,std::size_t leg,float ahead)const {
    auto pose=BodyFrame(state);auto local=LocalRoot(leg);local.y=0;local.z+=ahead;
    auto world=pose.ToWorld(local);world.y=ground_.Sample(world.x,world.z).height+ContactSeparation;
    return world;
}
BearPose BearMotion::ContactPose(const BearState& state,const std::array<Foot,4>& feet)const {
    auto pose=BodyFrame(state);
    for(std::size_t leg=0;leg<4;++leg) {
        const auto root=LocalRoot(leg);auto paw=pose.ToLocal(feet[leg].position);paw.y+=BearLegPose::PawHeight;
        const auto ground=ground_.Sample(feet[leg].position.x,feet[leg].position.z).normal;
        const BearPoint localNormal{ground.x*pose.right.x+ground.y*pose.right.y+ground.z*pose.right.z,
            ground.x*pose.up.x+ground.y*pose.up.y+ground.z*pose.up.z,
            ground.x*pose.forward.x+ground.y*pose.forward.y+ground.z*pose.forward.z};
        pose.legs[leg]=SolveLeg({root.x,root.y+pose.bodyBob,root.z},paw,leg,!feet[leg].swinging,Unit(localNormal));
    }
    return pose;
}

void BearMotion::StartFoot(std::size_t leg,BearPoint target,float duration) {
    if(leg>=4||solids_.IntersectsCircle(target.x,target.z,.20f))return;
    auto trial=feet_;auto& foot=trial[leg];foot.start=foot.position;foot.target=target;
    foot.progress=0;foot.duration=duration;foot.swinging=true;
    auto landed=trial;landed[leg].position=target;landed[leg].swinging=false;
    const auto pose=ContactPose(state_,landed);
    if(std::none_of(pose.legs.begin(),pose.legs.end(),[](const auto& p){return p.reachLimited;}))feet_=trial;
}
void BearMotion::AdvanceFeet(float seconds) {
    for(std::size_t leg=0;leg<4;++leg)if(feet_[leg].swinging) {
        auto trial=feet_;auto& foot=trial[leg];foot.progress=std::min(1.0f,foot.progress+seconds/foot.duration);
        const float t=foot.progress,u=t*t*(3-2*t),arch=std::sin(Pi*t);
        foot.position={std::lerp(foot.start.x,foot.target.x,u),std::lerp(foot.start.y,foot.target.y,u),
            std::lerp(foot.start.z,foot.target.z,u)};
        foot.position.y=std::max(foot.position.y,ground_.Sample(foot.position.x,foot.position.z).height+ContactSeparation)
            +.12f*arch*arch;foot.swinging=t<1;
        const auto pose=ContactPose(state_,trial);
        if(std::none_of(pose.legs.begin(),pose.legs.end(),[](const auto& p){return p.reachLimited;}))feet_=trial;
    }
    const int active=static_cast<int>(std::count_if(feet_.begin(),feet_.end(),[](const auto& p){return p.swinging;}));
    std::size_t chosen=4;float priority=0;
    for(std::size_t leg=0;leg<4&&active<2;++leg)if(!feet_[leg].swinging) {
        const auto neutral=NeutralFoot(state_,leg);
        const float error=std::hypot(neutral.x-feet_[leg].position.x,neutral.z-feet_[leg].position.z);
        const std::array<float,4> walkOffsets{0,.50f,.25f,.75f};
        const std::array<float,4> trotOffsets{0,.50f,.50f,0};
        const auto& offsets=state_.speed>1.05f?trotOffsets:walkOffsets;
        float cycle=std::fmod(state_.phase/Tau+offsets[leg],1.0f);if(cycle<0)cycle+=1;
        const bool scheduled=state_.speed>.08f&&cycle>.72f;
        const float score=error+(scheduled?.16f:0);
        if((error>(state_.speed<.08f?.035f:.115f)||scheduled)&&score>priority){chosen=leg;priority=score;}
    }
    if(chosen<4) {
        const float ahead=state_.speed>.08f?.20f+.18f*state_.speed/MaximumSpeed:0;
        StartFoot(chosen,NeutralFoot(state_,chosen,ahead),state_.speed>1.05f?.25f:.34f);
    }
    state_.pose=ContactPose(state_,feet_);
}

bool BearMotion::TryMove(float heading,float distance,bool& waitingForStep) {
    waitingForStep=false;const float x=state_.x+std::sin(heading)*distance,z=state_.z+std::cos(heading)*distance;
    const float mx=(x+state_.x)*.5f,mz=(z+state_.z)*.5f;
    if(!IsValidPosition(x,z)||!IsValidPosition(mx,mz)
        ||solids_.IntersectsCircle(mx,mz,BodyClearance+distance*.5f))return false;
    auto candidate=state_;candidate.x=x;candidate.z=z;candidate.y=ground_.Sample(x,z).height;candidate.heading=heading;
    const auto pose=ContactPose(candidate,feet_);
    if(std::any_of(pose.legs.begin(),pose.legs.end(),[](const auto& leg){return leg.reachLimited;})) {
        waitingForStep=true;return false;
    }
    const float vertical=candidate.y-state_.y;state_=candidate;state_.pose=pose;
    const float travelled=std::hypot(distance,vertical);state_.distanceTravelled+=travelled;
    const float stride=state_.speed>1.05f?.72f:.58f;
    state_.phase=std::fmod(state_.phase+travelled/stride*Tau,Tau);return true;
}

void BearMotion::Step(float seconds,BearPoint observer,bool observerValid) {
    remaining_-=seconds;avoidance_=std::max(0.0f,avoidance_-seconds);
    const float observerDistance=observerValid?std::hypot(state_.x-observer.x,state_.y+.8f-observer.y,state_.z-observer.z)
        :std::numeric_limits<float>::infinity();
    if(observerDistance<12) {
        state_.behavior=BearBehavior::Withdraw;remaining_=1.8f;
        const float dx=state_.x-observer.x,dz=state_.z-observer.z;
        if(std::hypot(dx,dz)>.01f)targetHeading_=std::atan2(dx,dz);
    } else if(remaining_<=0) {
        const float choice=UnitRandom(++decisions_*92821U+0x4b1dU);
        state_.behavior=choice<.18f?BearBehavior::Rest:choice<.40f?BearBehavior::Forage:
            choice<.78f?BearBehavior::Walk:choice<.90f?BearBehavior::Idle:BearBehavior::Trot;
        remaining_=state_.behavior==BearBehavior::Rest?7+choice*7:3+choice*6;
        targetHeading_=Wrap(state_.heading+(choice-.5f)*3.5f);
    }
    if(std::hypot(state_.x-state_.homeX,state_.z-state_.homeZ)>HomeRadius-5)
        targetHeading_=std::atan2(state_.homeX-state_.x,state_.homeZ-state_.z);
    const float targetSpeed=state_.behavior==BearBehavior::Trot?MaximumSpeed:
        state_.behavior==BearBehavior::Withdraw?1.25f:state_.behavior==BearBehavior::Walk?.68f:0;
    state_.speed+=std::clamp(targetSpeed-state_.speed,-1.15f*seconds,.72f*seconds);
    const float targetForage=state_.behavior==BearBehavior::Forage&&state_.speed<.08f?1.0f:0.0f;
    state_.foragePose+=std::clamp(targetForage-state_.foragePose,-1.4f*seconds,.75f*seconds);
    state_.breathPhase=std::fmod(state_.breathPhase+seconds*(state_.speed>.1f?2.1f:.82f),Tau);
    const float heading=Wrap(state_.heading+std::clamp(Wrap(targetHeading_-state_.heading),-.78f*seconds,.78f*seconds));
    if(state_.speed>.001f) {
        bool waiting=false;
        if(!TryMove(heading,state_.speed*seconds,waiting)) {
            state_.speed=0;
            if(!waiting&&avoidance_==0) {targetHeading_=Wrap(state_.heading+(decisions_%2?1:-1)*1.25f);avoidance_=1.4f;}
        }
    } else {
        auto candidate=state_;candidate.heading=heading;const auto pose=ContactPose(candidate,feet_);
        if(std::none_of(pose.legs.begin(),pose.legs.end(),[](const auto& leg){return leg.reachLimited;})) {
            state_.heading=heading;state_.pose=pose;
        }
    }
}

void BearMotion::Update(float seconds,BearPoint observer) {
    if(!std::isfinite(seconds)||seconds<=0)return;
    seconds=std::min(seconds,.1f);
    const bool valid=Finite(observer);const int steps=std::min(5,static_cast<int>(std::ceil(seconds/.02f)));
    for(int i=0;i<steps;++i){const float dt=seconds/steps;Step(dt,observer,valid);AdvanceFeet(dt);}
}
}
