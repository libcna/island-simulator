#include "IslandSimulator/SpiderPose.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island {
namespace {
SpiderPoint Add(SpiderPoint a,SpiderPoint b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
SpiderPoint Scale(SpiderPoint a,float s) {return {a.x*s,a.y*s,a.z*s};}
SpiderPoint Sub(SpiderPoint a,SpiderPoint b) {return Add(a,Scale(b,-1));}
float Length(SpiderPoint a) {return std::hypot(a.x,a.y,a.z);}
void CheckLeg(std::size_t leg) {
    if(leg>=SpiderPose::LegCount)throw std::invalid_argument("Invalid spider leg index");
}
}
SpiderPoint SpiderPose::NeutralFoot(std::size_t leg) {
    CheckLeg(leg);
    constexpr std::array<float,4> x{.080f,.112f,.114f,.083f};
    constexpr std::array<float,4> z{.100f,.053f,-.021f,-.089f};
    return {x[leg%4]*(leg<4?-1.0f:1.0f),0,z[leg%4]};
}
SpiderLegPose SpiderPose::Solve(std::size_t leg,SpiderPoint target) {
    CheckLeg(leg);
    if(!std::isfinite(target.x)||!std::isfinite(target.y)||!std::isfinite(target.z))
        throw std::invalid_argument("Nonfinite spider foot target");
    const float side=leg<4?-1.0f:1.0f;
    constexpr std::array<float,4> rootX{.011f,.017f,.017f,.012f};
    constexpr std::array<float,4> rootZ{.035f,.026f,.012f,.002f};
    SpiderLegPose result{};
    result.root={side*rootX[leg%4],.036f,rootZ[leg%4]};
    // Coxa points toward the nominal foot. Keep its attachment and length stable
    // while the distal chain reaches an independently retained stance target.
    auto radial=Sub(NeutralFoot(leg),result.root);radial.y=0;
    radial=Scale(radial,1/Length(radial));
    result.hip=Add(result.root,Scale(radial,SpiderLegPose::CoxaLength));
    auto direction=Sub(target,result.hip);
    const float requested=Length(direction);
    if(!std::isfinite(requested))throw std::invalid_argument("Unbounded spider foot target");
    constexpr float upper=SpiderLegPose::UpperLength,lower=SpiderLegPose::LowerLength;
    const float distance=std::clamp(requested,lower-upper+.00001f,upper+lower-.00001f);
    result.reachLimited=distance!=requested;
    direction=requested>1e-7f?Scale(direction,1/requested):SpiderPoint{0,-1,0};
    result.foot=Add(result.hip,Scale(direction,distance));
    // Project up perpendicular to the chain so the knee arches above the floor.
    auto bend=Sub(SpiderPoint{0,1,0},Scale(direction,direction.y));
    if(Length(bend)<1e-5f) {
        const float projection=radial.x*direction.x+radial.z*direction.z;
        bend=Sub(radial,Scale(direction,projection));
    }
    bend=Scale(bend,1/Length(bend));
    const float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
    const float rise=std::sqrt(std::max(0.0f,upper*upper-along*along));
    result.knee=Add(result.hip,Add(Scale(direction,along),Scale(bend,rise)));
    return result;
}
std::array<SpiderLegPose,SpiderPose::LegCount> SpiderPose::Rest() {
    std::array<SpiderLegPose,LegCount> result{};
    for(std::size_t leg=0;leg<LegCount;++leg)result[leg]=Solve(leg,NeutralFoot(leg));
    return result;
}
}
