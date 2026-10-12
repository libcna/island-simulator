#include "IslandSimulator/GoatPose.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
constexpr float Tau=2*std::numbers::pi_v<float>;
float Stride(float speed) { return .25f*std::clamp(speed/GoatLegPose::WalkingSpeed,0.0f,1.0f); }
GoatPoint Cross(GoatPoint a,GoatPoint b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
GoatPoint Unit(GoatPoint p) {
    const float d=std::hypot(p.x,p.y,p.z);
    if(!std::isfinite(d)||d<1e-5f) throw std::invalid_argument("Invalid goat ground basis");
    return {p.x/d,p.y/d,p.z/d};
}
}
GoatLegPose RetargetGoatLeg(const GoatLegPose& reference,GoatPoint target,std::size_t leg) {
    const auto finite=[](GoatPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
    if(leg>=4||!finite(reference.root)||!finite(target))
        throw std::invalid_argument("Invalid goat leg target");
    auto result=reference;
    GoatPoint direction{target.x-reference.root.x,target.y-reference.root.y,target.z-reference.root.z};
    const float requested=std::hypot(direction.x,direction.y,direction.z);
    if(!std::isfinite(requested))throw std::invalid_argument("Unbounded goat leg target");
    constexpr float upper=GoatLegPose::UpperLength,lower=GoatLegPose::LowerLength;
    const float distance=std::clamp(requested,upper-lower+.0001f,upper+lower-.0001f);
    if(requested>1e-7f)direction={direction.x/requested,direction.y/requested,direction.z/requested};
    else direction={0,-1,0};
    result.reachLimited=reference.reachLimited||requested!=distance;
    result.hoof={reference.root.x+direction.x*distance,reference.root.y+direction.y*distance,
        reference.root.z+direction.z*distance};
    // Preserve the existing sagittal bend for ordinary walking, but allow lateral
    // placement when the body turns above a planted world-space foot.
    const float sign=leg%2==1?-1.0f:1.0f;
    GoatPoint bend{-sign*direction.z*direction.x,-sign*direction.z*direction.y,
        sign*(1-direction.z*direction.z)};
    if(std::hypot(bend.x,bend.y,bend.z)<1e-5f)
        bend={-direction.y*direction.x,1-direction.y*direction.y,-direction.y*direction.z};
    bend=Unit(bend);
    const float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
    const float offset=std::sqrt(std::max(0.0f,upper*upper-along*along));
    result.knee={reference.root.x+direction.x*along+bend.x*offset,
        reference.root.y+direction.y*along+bend.y*offset,
        reference.root.z+direction.z*along+bend.z*offset};
    return result;
}
GoatLegPose EvaluateGoatLeg(float phase,float speed,float bob,std::size_t leg,float groundHeight) {
    if(leg>=4||!std::isfinite(phase)||!std::isfinite(speed)||speed<0
        ||!std::isfinite(bob)||!std::isfinite(groundHeight))
        throw std::invalid_argument("Invalid goat leg pose");
    bob=std::clamp(bob,0.0f,.008f);
    // Four-beat walking sequence: left rear, left front, right rear, right front.
    constexpr std::array<float,4> offsets{0,.75f,.5f,.25f};
    float cycle=std::fmod(phase,Tau)/Tau+offsets[leg];cycle-=std::floor(cycle);
    const bool stance=cycle<GoatLegPose::StanceFraction||speed==0;
    float travel,lift=0;
    if(stance) travel=1-2*cycle/GoatLegPose::StanceFraction;
    else {
        const float t=(cycle-GoatLegPose::StanceFraction)/(1-GoatLegPose::StanceFraction);
        travel=-std::cos(std::numbers::pi_v<float>*t);
        const float arch=std::sin(std::numbers::pi_v<float>*t);
        lift=.12f*arch*arch*std::min(speed/GoatLegPose::WalkingSpeed,1.0f);
    }
    const bool front=leg%2==1;
    const GoatPoint root{leg<2?-.18f:.18f,(front?.72f:.73f)+bob,front?.40f:-.43f};
    GoatPoint hoof{root.x,groundHeight+GoatLegPose::HoofHeight+lift,root.z+travel*Stride(speed)};
    constexpr float maximum=GoatLegPose::UpperLength+GoatLegPose::LowerLength-.0001f;
    const float minY=root.y-std::sqrt(maximum*maximum-(hoof.z-root.z)*(hoof.z-root.z));
    const float requestedY=hoof.y;
    hoof.y=std::clamp(hoof.y,minY,root.y-.10f);
    return RetargetGoatLeg({root,root,hoof,stance,hoof.y!=requestedY},hoof,leg);
}
float GoatPhaseAdvance(float distance,float speed,float scale) {
    if(!std::isfinite(distance)||distance<0||!std::isfinite(speed)||speed<0
        ||!std::isfinite(scale)||scale<=0||scale>2)
        throw std::invalid_argument("Invalid goat stride travel");
    if(distance==0||Stride(speed)<1e-6f) return 0;
    const double advance=static_cast<double>(distance)*Tau*GoatLegPose::StanceFraction
        /(2.0*Stride(speed)*scale);
    return static_cast<float>(std::fmod(advance,static_cast<double>(Tau)));
}
GoatPoint GoatGroundPose::ToWorld(GoatPoint p) const noexcept {
    return {origin.x+scale*(right.x*p.x+up.x*p.y+forward.x*p.z),
        origin.y+scale*(right.y*p.x+up.y*p.y+forward.y*p.z),
        origin.z+scale*(right.z*p.x+up.z*p.y+forward.z*p.z)};
}
GoatGroundPose FitGoatGroundPose(const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
    float x,float z,float heading,float phase,float speed,float scale) {
    if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>TerrainMeshLayout::Extent-4
        ||std::abs(z)>TerrainMeshLayout::Extent-4||!std::isfinite(heading)
        ||!std::isfinite(phase)||!std::isfinite(speed)||speed<0
        ||!std::isfinite(scale)||scale<=0||scale>2)
        throw std::invalid_argument("Invalid goat ground pose");
    GoatGroundPose pose{};
    pose.origin={x,surface.Sample(x,z).height,z};
    const auto normal=terrain.Sample(x,z).normal;
    pose.up=Unit({normal.x,normal.y,normal.z});
    pose.right=Unit(Cross(pose.up,{std::cos(heading),0,std::sin(heading)}));
    pose.forward=Unit(Cross(pose.right,pose.up));
    pose.scale=scale;
    pose.bodyBob=.008f*std::abs(std::sin(2*std::fmod(phase,Tau)))*std::min(speed/GoatLegPose::WalkingSpeed,1.0f);
    for(std::size_t leg=0;leg<4;++leg) {
        const auto flat=EvaluateGoatLeg(phase,speed,pose.bodyBob,leg);
        const auto residual=[&](float height) {
            const auto sole=pose.ToWorld({flat.hoof.x,height,flat.hoof.z+GoatLegPose::SoleForward});
            return sole.y-surface.Sample(sole.x,sole.z).height;
        };
        float low=-.5f,high=.5f;
        const bool bracketed=residual(low)<=0&&residual(high)>=0;
        for(int iteration=0;iteration<18;++iteration) {
            const float mid=(low+high)*.5f;
            if(residual(mid)<0) low=mid;else high=mid;
        }
        pose.legs[leg]=EvaluateGoatLeg(phase,speed,pose.bodyBob,leg,(low+high)*.5f);
        pose.legs[leg].reachLimited|=!bracketed;
        const auto sole=pose.ToWorld({flat.hoof.x,(low+high)*.5f,flat.hoof.z+GoatLegPose::SoleForward});
        const auto ground=surface.Sample(sole.x,sole.z).normal;
        const auto project=[&](GoatPoint axis){return ground.x*axis.x+ground.y*axis.y+ground.z*axis.z;};
        pose.legs[leg].soleNormal=Unit({project(pose.right),project(pose.up),project(pose.forward)});
    }
    return pose;
}
}
