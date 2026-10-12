#include "IslandSimulator/SpiderPose.hpp"
#include "IslandSimulator/SpiderHabitat.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void Check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
float Distance(island::SpiderPoint a,island::SpiderPoint b) {
    return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
}
void Bones(const island::SpiderLegPose& p) {
    using island::SpiderLegPose;
    Check(std::abs(Distance(p.root,p.hip)-SpiderLegPose::CoxaLength)<1e-6f,"stretched coxa");
    Check(std::abs(Distance(p.hip,p.knee)-SpiderLegPose::UpperLength)<1e-6f,"stretched upper leg");
    Check(std::abs(Distance(p.knee,p.foot)-SpiderLegPose::LowerLength)<1e-6f,"stretched lower leg");
}
}
int main() {
    using namespace island;
    const auto rest=SpiderPose::Rest();
    for(std::size_t leg=0;leg<8;++leg) {
        const auto& p=rest[leg];Bones(p);
        Check(!p.reachLimited,"neutral foot unreachable");
        Check(Distance(p.foot,SpiderPose::NeutralFoot(leg))<1e-6f,"neutral target moved");
        Check(p.knee.y>p.hip.y&&p.foot.y<1e-6f,"rest leg not arched above floor");
        for(const auto point:{p.root,p.hip,p.knee,p.foot})
            Check(std::hypot(point.x,point.z)<SpiderHabitat::BodyRadius,"rest anatomy outside habitat envelope");
        if(leg<4) {
            const auto& mirror=rest[leg+4];
            Check(Distance({-p.knee.x,p.knee.y,p.knee.z},mirror.knee)<1e-6f,"asymmetric neutral knees");
        }
        // A local landing volume, including raised feet, longitudinal travel and
        // sideways adjustments. These are kinematic tests, not a terrain audit.
        for(int x=-1;x<=1;++x)for(int z=-1;z<=1;++z)for(int y=0;y<=2;++y) {
            auto target=SpiderPose::NeutralFoot(leg);
            target.x+=static_cast<float>(x)*.004f;
            target.z+=static_cast<float>(z)*.004f;
            target.y+=static_cast<float>(y)*.008f;
            const auto posed=SpiderPose::Solve(leg,target);Bones(posed);
            Check(!posed.reachLimited&&Distance(posed.foot,target)<1e-6f,"local landing target unreachable");
        }
        for(const auto target:{p.hip,SpiderPoint{p.hip.x,p.hip.y+1,p.hip.z},
                SpiderPoint{p.hip.x,p.hip.y-1,p.hip.z},SpiderPoint{10,10,10}}) {
            const auto limited=SpiderPose::Solve(leg,target);Bones(limited);
            Check(limited.reachLimited,"unreachable target not flagged");
        }
    }
    int rejected=0;
    for(const auto target:{SpiderPoint{NAN,0,0},SpiderPoint{0,INFINITY,0},
            SpiderPoint{0,0,-INFINITY},SpiderPoint{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),std::numeric_limits<float>::max()}}) {
        try {(void)SpiderPose::Solve(0,target);}catch(const std::invalid_argument&){++rejected;}
    }
    try{(void)SpiderPose::Solve(8,{0,0,0});}catch(const std::invalid_argument&){++rejected;}
    try{(void)SpiderPose::NeutralFoot(8);}catch(const std::invalid_argument&){++rejected;}
    Check(rejected==6,"invalid pose input not rejected");
    std::cout<<"Eight articulated spider legs: 216 local targets, 32 singular/far targets; not live locomotion\n";
}
