#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {
using namespace island;
constexpr float Step=.02f;
struct Foot {
    bool planted=false;
    GoatPoint previous{},anchor{};
};
struct Audit {
    std::array<Foot,GoatHabitat::Count*4> feet{};
    std::size_t samples=0,sliding=0;
    float worstStep=0,worstDrift=0,worstTime=0;
    std::size_t worstGoat=0,worstLeg=0;
    float previousSpeed=0,currentSpeed=0,turn=0;
    void Sample(std::size_t goat,std::size_t leg,GoatPoint sole,bool stance,float time,
                float beforeSpeed,float afterSpeed,float headingDelta) {
        auto& foot=feet[goat*4+leg];
        if(!stance) {foot.planted=false;return;}
        if(!foot.planted) {foot.anchor=foot.previous=sole;foot.planted=true;return;}
        const float distance=std::hypot(sole.x-foot.previous.x,sole.z-foot.previous.z);
        const float drift=std::hypot(sole.x-foot.anchor.x,sole.z-foot.anchor.z);
        if(!std::isfinite(distance)||!std::isfinite(drift))throw std::runtime_error("nonfinite stance sample");
        ++samples;if(distance>.001f)++sliding;
        worstDrift=std::max(worstDrift,drift);
        if(distance>worstStep) {
            worstStep=distance;worstTime=time;worstGoat=goat;worstLeg=leg;
            previousSpeed=beforeSpeed;currentSpeed=afterSpeed;turn=headingDelta;
        }
        foot.previous=sole;
    }
    void Print(const char* scenario) const {
        std::cout<<"{\"scenario\":\""<<scenario<<"\",\"dt\":"<<Step
            <<",\"stance_pairs\":"<<samples<<",\"pairs_over_1mm\":"<<sliding
            <<",\"max_step_m\":"<<worstStep<<",\"max_anchor_drift_m\":"<<worstDrift
            <<",\"worst_time_s\":"<<worstTime<<",\"goat\":"<<worstGoat<<",\"leg\":"<<worstLeg
            <<",\"speed_before\":"<<previousSpeed<<",\"speed_after\":"<<currentSpeed
            <<",\"heading_delta\":"<<turn<<"}\n";
    }
};
}
int main() {
    using namespace island;
    Audit fixture;
    fixture.Sample(0,0,{0,0,0},true,0,0,0,0);
    fixture.Sample(0,0,{.01f,0,0},true,Step,0,0,0);
    fixture.Sample(0,0,{1,0,0},false,2*Step,0,0,0);
    fixture.Sample(0,0,{2,0,0},true,3*Step,0,0,0);
    if(fixture.samples!=1||fixture.sliding!=1||std::abs(fixture.worstStep-.01f)>1e-6f
        ||std::abs(fixture.worstDrift-.01f)>1e-6f)
        throw std::runtime_error("stance audit slip/reset fixture failed");
    // Independent flat constant-speed calibration. The body moves +z while a
    // stance foot moves -z locally; world sole position must cancel that travel.
    Audit flat;float phase=0,z=0;
    for(int step=0;step<=100;++step) {
        if(step) {z+=.65f*Step;phase+=GoatPhaseAdvance(.65f*Step,.65f,1);}
        for(std::size_t leg=0;leg<4;++leg) {
            const auto p=EvaluateGoatLeg(phase,.65f,0,leg);
            flat.Sample(0,leg,{p.hoof.x,0,z+p.hoof.z+GoatLegPose::SoleForward},p.stance,
                        step*Step,.65f,.65f,0);
        }
    }
    if(flat.samples<200||flat.worstStep>1e-5f)throw std::runtime_error("stance audit straight calibration failed");
    flat.Print("flat-straight-calibration");
    const IslandTerrain terrain;const RenderedTerrainSurface surface(terrain);
    const VegetationLayout palms(terrain);const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> occupied;
    for(const auto items:std::array<std::span<const CircularObstacle>,4>{palms.Obstacles(),
            jungle.Obstacles(),props.Obstacles(),wreck.Obstacles()})
        occupied.insert(occupied.end(),items.begin(),items.end());
    const auto hut=HutLayout::FindInWorld(terrain,jungle,occupied);
    if(!hut)throw std::runtime_error("stance audit world has no hut");
    occupied.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    for(bool retreat:{false,true}) {
        GoatHerd herd(terrain,jungle,occupied);Audit audit;
        const auto first=herd.Goats().front();
        const GoatPoint observer=retreat?GoatPoint{first.x-2.5f,first.y+.8f,first.z}:GoatPoint{1000,1000,1000};
        for(int step=0;step<=1500;++step) {
            const std::vector<GoatState> before(herd.Goats().begin(),herd.Goats().end());
            if(step)herd.Update(Step,observer);
            for(std::size_t i=0;i<herd.Goats().size();++i) {
                const auto& s=herd.Goats()[i];
                const auto pose=FitGoatGroundPose(terrain,surface,s.x,s.z,s.heading,s.phase,s.speed,s.scale);
                for(std::size_t leg=0;leg<4;++leg) {
                    const auto& p=pose.legs[leg];
                    const auto sole=pose.ToWorld({p.hoof.x,p.hoof.y-GoatLegPose::HoofHeight,p.hoof.z+GoatLegPose::SoleForward});
                    audit.Sample(i,leg,sole,p.stance,step*Step,before[i].speed,s.speed,
                        std::remainder(s.heading-before[i].heading,2*std::numbers::pi_v<float>));
                }
            }
        }
        if(audit.samples<10000)throw std::runtime_error("insufficient herd stance evidence");
        audit.Print(retreat?"observer-retreat-30s":"undisturbed-herd-30s");
    }
    // Successful execution means valid measurements, NOT accepted foot locking.
}
