#include "IslandSimulator/ResidentMotion.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace island {
namespace {
ResidentPoint Add(ResidentPoint a,ResidentPoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
ResidentPoint Sub(ResidentPoint a,ResidentPoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
ResidentPoint Mul(ResidentPoint a,float b){return {a.x*b,a.y*b,a.z*b};}
float Dot(ResidentPoint a,ResidentPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
ResidentPoint Cross(ResidentPoint a,ResidentPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
ResidentPoint Unit(ResidentPoint p){const float n=std::hypot(p.x,p.y,p.z);
    if(!std::isfinite(n)||n<1e-7f)throw std::invalid_argument("Invalid resident pose direction");
    return Mul(p,1/n);}
bool Finite(ResidentPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
constexpr float Tau=2*std::numbers::pi_v<float>;
float Wrap(float a){return std::remainder(a,Tau);}
float Approach(float a,float b,float step){return a+std::clamp(b-a,-step,step);}
ResidentPoint Right(float h){return {std::cos(h),0,-std::sin(h)};}
ResidentPoint Forward(float h){return {std::sin(h),0,std::cos(h)};}
constexpr std::array<std::array<float,2>,9> SolePoints{{{0,0},{-.055f,-.10f},{.055f,-.10f},
    {-.055f,.16f},{.055f,.16f},{0,-.10f},{0,.16f},{-.055f,.03f},{.055f,.03f}}};
}
ResidentLegPose SolveResidentLeg(ResidentPoint hip,const ResidentFootFrame& foot,ResidentPoint bendForward,bool stance) {
    if(!Finite(hip)||!Finite(foot.centre)||!Finite(foot.up)||!Finite(bendForward))
        throw std::invalid_argument("Nonfinite resident IK input");
    ResidentLegPose result;result.hip=hip;result.foot=foot;result.stance=stance;
    auto desired=Add(foot.centre,Mul(foot.up,ResidentLegPose::AnkleHeight));
    auto delta=Sub(desired,hip);const float request=std::sqrt(Dot(delta,delta));
    constexpr float upper=ResidentLegPose::UpperLength,lower=ResidentLegPose::LowerLength;
    const float distance=std::clamp(request,std::abs(upper-lower)+.0001f,upper+lower-.001f);
    result.reachLimited=std::abs(request-distance)>.00001f;
    const auto direction=request>1e-7f?Mul(delta,1/request):ResidentPoint{0,-1,0};
    result.ankle=Add(hip,Mul(direction,distance));
    auto bend=Sub(bendForward,Mul(direction,Dot(direction,bendForward)));
    if(Dot(bend,bend)<1e-8f) {
        const ResidentPoint axis=std::abs(direction.x)<.9f?ResidentPoint{1,0,0}:ResidentPoint{0,0,1};
        bend=Sub(axis,Mul(direction,Dot(direction,axis)));
    }
    bend=Unit(bend);
    const float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
    const float sideways=std::sqrt(std::max(0.0f,upper*upper-along*along));
    result.knee=Add(hip,Add(Mul(direction,along),Mul(bend,sideways)));
    return result;
}
ResidentFootFrame ResidentMotion::GroundFoot(float x,float z,float heading)const {
    const auto ground=ground_.Sample(x,z);
    ResidentFootFrame foot;
    foot.up={ground.normal.x,ground.normal.y,ground.normal.z};
    foot.right=Unit(Cross(foot.up,Forward(heading)));foot.forward=Unit(Cross(foot.right,foot.up));
    foot.centre={x,ground.height+.002f,z};foot.heading=heading;
    for(const auto p:SolePoints) {
        const auto offset=Add(Mul(foot.right,p[0]),Mul(foot.forward,p[1]));
        const float support=ground_.Sample(x+offset.x,z+offset.z).height+.002f-offset.y;
        foot.centre.y=std::max(foot.centre.y,support);
    }
    return foot;
}
bool ResidentMotion::FitPose(float x,float z,float heading,float speed,ResidentPose& pose)const {
    const auto ground=ground_.Sample(x,z);const auto right=Right(heading),forward=Forward(heading);
    pose.origin={x,ground.height,z};pose.heading=heading;
    pose.pelvisY=ground.height+.915f-.035f*std::clamp(speed/WalkingSpeed,0.0f,1.0f);
    constexpr float reach=ResidentLegPose::UpperLength+ResidentLegPose::LowerLength-.003f;
    for(std::size_t leg=0;leg<2;++leg) {
        const auto hip=Add(ResidentPoint{x,0,z},Mul(right,leg==0?-.095f:.095f));
        const auto ankle=Add(feet_[leg].current.centre,Mul(feet_[leg].current.up,ResidentLegPose::AnkleHeight));
        const float horizontal=std::hypot(hip.x-ankle.x,hip.z-ankle.z);
        if(horizontal>=reach)return false;
        pose.pelvisY=std::min(pose.pelvisY,ankle.y+std::sqrt(reach*reach-horizontal*horizontal));
    }
    if(pose.pelvisY<ground.height+.73f)return false;
    for(std::size_t leg=0;leg<2;++leg) {
        auto hip=Add(ResidentPoint{x,pose.pelvisY,z},Mul(right,leg==0?-.095f:.095f));
        pose.legs[leg]=SolveResidentLeg(hip,feet_[leg].current,forward,!feet_[leg].swinging);
        if(pose.legs[leg].reachLimited)return false;
        const float sign=leg==0?-1.0f:1.0f;
        // Keep the anatomical shoulder joint just inside the cloth shoulder line so
        // the animated upper arm blends into the torso instead of hanging from its rim.
        pose.shoulders[leg]=Add(ResidentPoint{x,pose.pelvisY+.43f,z},Mul(right,sign*.205f));
        const float swing=std::sin(state_.phase+(leg==0?std::numbers::pi_v<float>:0))*.52f
            *std::min(1.0f,speed/WalkingSpeed);
        const auto upper=Unit(Add(Mul(right,sign*.018f),Add(ResidentPoint{0,-std::cos(swing),0},Mul(forward,std::sin(swing)))));
        pose.elbows[leg]=Add(pose.shoulders[leg],Mul(upper,.29f));
        // Keep a relaxed visible elbow bend through idle and walking instead of
        // aligning the upper and lower arm into one mannequin-straight rod.
        pose.wrists[leg]=Add(pose.elbows[leg],Add(ResidentPoint{0,-std::cos(swing+.42f)*.26f,0},
            Mul(forward,std::sin(swing+.42f)*.26f)));
    }
    return true;
}
ResidentMotion::ResidentMotion(const IslandTerrain& terrain,std::span<const CircularObstacle> solids)
    :terrain_(terrain),ground_(terrain),solids_(solids),route_(terrain,ground_,solids_) {
    const auto start=route_.Points()[0],next=route_.Points()[1];
    state_.x=start.x;state_.z=start.z;state_.y=start.y;
    state_.heading=std::atan2(next.x-start.x,next.z-start.z);
    for(std::size_t leg=0;leg<2;++leg) {
        const auto p=Add(start,Add(Mul(Right(state_.heading),leg==0?-.095f:.095f),Mul(Forward(state_.heading),.025f)));
        feet_[leg].current=feet_[leg].start=feet_[leg].target=GroundFoot(p.x,p.z,state_.heading);
    }
    if(!FitPose(state_.x,state_.z,state_.heading,0,state_.pose))throw std::runtime_error("Initial resident pose cannot fit ground");
}
void ResidentMotion::StartStep(float x,float z,float heading,std::size_t leg,float duration) {
    if(solids_.IntersectsCircle(x,z,.17f)||std::abs(x)>ResidentRoute::Extent||std::abs(z)>ResidentRoute::Extent)return;
    auto& foot=feet_[leg];foot.start=foot.current;foot.target=GroundFoot(x,z,heading);
    foot.progress=0;foot.duration=duration;foot.swinging=true;
}
void ResidentMotion::AdvanceFeet(float seconds) {
    for(auto& foot:feet_)if(foot.swinging) {
        foot.progress=std::min(1.0f,foot.progress+seconds/foot.duration);
        const float t=foot.progress,e=t*t*(3-2*t);
        const float x=std::lerp(foot.start.centre.x,foot.target.centre.x,e),z=std::lerp(foot.start.centre.z,foot.target.centre.z,e);
        const float heading=foot.start.heading+Wrap(foot.target.heading-foot.start.heading)*e;
        foot.current=GroundFoot(x,z,heading);
        foot.current.centre.y=std::max(foot.current.centre.y,
            std::lerp(foot.start.centre.y,foot.target.centre.y,e)+.09f*std::sin(std::numbers::pi_v<float>*t));
        if(t==1){foot.current=foot.target;foot.swinging=false;}
    }
}
void ResidentMotion::Step(float seconds,ResidentPoint observer) {
    const auto previousFeet=feet_;
    const auto previousMode=state_.mode;
    auto target=route_.Points()[state_.nextPoint];
    float distance=std::hypot(target.x-state_.x,target.z-state_.z);
    if(distance<.12f&&pauseRemaining_<=0) {
        if(std::find(route_.PausePoints().begin(),route_.PausePoints().end(),state_.nextPoint)!=route_.PausePoints().end()
            ||state_.nextPoint==route_.Points().size()-1){pauseRemaining_=6+static_cast<float>(state_.pauseCount%3)*2;++state_.pauseCount;}
        ++state_.nextPoint;
        if(state_.nextPoint>=route_.Points().size()){state_.nextPoint=1;++state_.completedLaps;}
        target=route_.Points()[state_.nextPoint];distance=std::hypot(target.x-state_.x,target.z-state_.z);
    }
    const bool yield=Finite(observer)&&std::abs(observer.y-state_.y-1.0f)<3.0f
        &&std::hypot(observer.x-state_.x,observer.z-state_.z)<1.7f;
    float desiredSpeed=0;
    const float desiredHeading=std::atan2(target.x-state_.x,target.z-state_.z),turn=Wrap(desiredHeading-state_.heading);
    state_.mode=yield?ResidentMode::Yielding:pauseRemaining_>0?ResidentMode::Pausing:
        std::abs(turn)>.28f?ResidentMode::Turning:ResidentMode::Walking;
    if(!yield&&pauseRemaining_>0)pauseRemaining_=std::max(0.0f,pauseRemaining_-seconds);
    const auto surface=ground_.Sample(state_.x,state_.z);
    const auto forward=Forward(desiredHeading);
    const float slope=std::abs((surface.normal.x*forward.x+surface.normal.z*forward.z)/surface.normal.y);
    const float careful=std::clamp(1-slope*1.2f,.50f,1.0f);
    if(state_.mode==ResidentMode::Walking)desiredSpeed=WalkingSpeed*careful*std::clamp(distance/.65f,.18f,1.0f);
    const float speed=Approach(state_.speed,desiredSpeed,(desiredSpeed<state_.speed?2.0f:1.2f)*seconds);
    const float heading=Wrap(state_.heading+std::clamp(turn,-1.1f*seconds,1.1f*seconds));
    if(previousMode==ResidentMode::Walking&&state_.mode!=ResidentMode::Walking) {
        for(std::size_t leg=0;leg<2;++leg)if(feet_[leg].swinging) {
            const auto neutral=Add(ResidentPoint{state_.x,0,state_.z},Add(Mul(Right(heading),leg==0?-.095f:.095f),Mul(Forward(heading),.025f)));
            StartStep(neutral.x,neutral.z,heading,leg,.22f);
        }
    }
    AdvanceFeet(seconds);
    const float travel=std::min(distance,speed*seconds);
    const float x=distance>1e-5f?state_.x+(target.x-state_.x)*travel/distance:state_.x;
    const float z=distance>1e-5f?state_.z+(target.z-state_.z)*travel/distance:state_.z;
    if(!feet_[0].swinging&&!feet_[1].swinging) {
        std::size_t chosen=state_.mode==ResidentMode::Walking&&previousMode!=ResidentMode::Walking?0:2;
        float largest=state_.mode==ResidentMode::Walking?.17f:.045f;
        for(std::size_t leg=0;leg<2;++leg) {
            const auto neutral=Add(ResidentPoint{x,0,z},Mul(Right(heading),leg==0?-.095f:.095f));
            const auto relative=Sub(feet_[leg].current.centre,neutral);
            const float lag=state_.mode==ResidentMode::Walking?-Dot(relative,Forward(heading)):std::hypot(relative.x,relative.z);
            if(lag>largest){largest=lag;chosen=leg;}
        }
        if(chosen<2) {
            const float ahead=state_.mode==ResidentMode::Walking?.22f*careful+std::min(distance,speed*SwingSeconds):.025f;
            const auto goal=Add(ResidentPoint{x,0,z},Add(Mul(Right(heading),chosen==0?-.095f:.095f),Mul(Forward(heading),ahead)));
            StartStep(goal.x,goal.z,heading,chosen,state_.mode==ResidentMode::Walking?SwingSeconds:.22f);
        }
    }
    // Arm counter-swing follows the actual stepping foot, including shorter hill
    // strides and interrupted steps, rather than drifting against a fixed metre cycle.
    for(std::size_t leg=0;leg<2;++leg)if(feet_[leg].swinging) {
        const float t=feet_[leg].progress,e=t*t*(3-2*t);
        const float desiredPhase=(leg==0?-.5f:.5f)*std::numbers::pi_v<float>+e*std::numbers::pi_v<float>;
        state_.phase=Wrap(state_.phase+std::clamp(Wrap(desiredPhase-state_.phase),-12*seconds,12*seconds));
    }
    ResidentPose pose;
    if(ResidentRoute::SafeSegment(terrain_,ground_,solids_,{state_.x,0,state_.z},{x,0,z})&&FitPose(x,z,heading,speed,pose)) {
        const float moved=std::hypot(x-state_.x,z-state_.z);
        state_.x=x;state_.z=z;state_.y=ground_.Sample(x,z).height;
        state_.heading=heading;state_.speed=speed;
        state_.distanceTravelled+=moved;state_.pose=pose;
    }else {
        ++state_.rejectedMoves;state_.speed=0;
        // A blocked body must still let its airborne foot settle. Rolling back that
        // foot on every rejected centimetre makes the stance/swing pair deadlock.
        if(!FitPose(state_.x,state_.z,state_.heading,0,pose))feet_=previousFeet;
        for(std::size_t leg=0;leg<2;++leg)if(feet_[leg].swinging) {
            const auto neutral=Add(ResidentPoint{state_.x,0,state_.z},Add(Mul(Right(state_.heading),leg==0?-.095f:.095f),
                Mul(Forward(state_.heading),.025f)));
            if(std::hypot(feet_[leg].target.centre.x-neutral.x,feet_[leg].target.centre.z-neutral.z)>.025f)
                StartStep(neutral.x,neutral.z,state_.heading,leg,.22f);
        }
        if(FitPose(state_.x,state_.z,state_.heading,0,pose))state_.pose=pose;
    }
}
void ResidentMotion::Update(float seconds,ResidentPoint observer) {
    if(!std::isfinite(seconds)||seconds<=0)return;
    seconds=std::min(seconds,.1f);
    const int steps=static_cast<int>(std::ceil(seconds/.02f));
    for(int step=0;step<steps;++step)Step(seconds/steps,observer);
}
}
