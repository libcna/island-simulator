#include "IslandSimulator/BearMotion.hpp"
#include "IslandSimulator/CampfireLayout.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/SoundscapeModel.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
float Distance(island::BearPoint a,island::BearPoint b) {
    return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
}
island::BearPoint Sole(const island::BearState& state,std::size_t leg) {
    auto local=state.pose.legs[leg].paw;local.y-=island::BearLegPose::PawHeight;
    return state.pose.ToWorld(local);
}
void CheckPose(const island::BearMotion& bear,const island::WorldCollisionGrid& solids) {
    const auto& state=bear.State();const auto& pose=state.pose;
    Check(std::isfinite(state.x)&&std::isfinite(state.y)&&std::isfinite(state.z),"nonfinite bear state");
    Check(std::abs(Distance(pose.right,{0,0,0})-1)<.00002f
        &&std::abs(Distance(pose.up,{0,0,0})-1)<.00002f
        &&std::abs(Distance(pose.forward,{0,0,0})-1)<.00002f,"bear body basis is not normalized");
    int supports=0;
    for(std::size_t leg=0;leg<4;++leg) {
        const auto& p=pose.legs[leg];supports+=p.stance?1:0;
        if(p.reachLimited) {
            std::cerr<<"reach-limited leg "<<leg<<" bear "<<state.x<<','<<state.y<<','<<state.z
                <<" speed "<<state.speed<<" behavior "<<static_cast<int>(state.behavior)
                <<" root "<<p.root.x<<','<<p.root.y<<','<<p.root.z
                <<" paw "<<p.paw.x<<','<<p.paw.y<<','<<p.paw.z<<'\n';
            throw std::runtime_error("authoritative bear leg is reach-limited");
        }
        Check(std::abs(Distance(p.root,p.joint)-island::BearLegPose::UpperLength)<.000025f
            &&std::abs(Distance(p.joint,p.paw)-island::BearLegPose::LowerLength)<.000025f,
            "bear leg changed its bone lengths");
        const auto sole=Sole(state,leg);const auto ground=bear.Ground().Sample(sole.x,sole.z);
        Check(!solids.IntersectsCircle(sole.x,sole.z,.20f),"bear paw intersects a world obstacle");
        Check(sole.y-ground.height>=-.00012f,"bear paw penetrates rendered terrain");
        if(p.stance)Check(std::abs(sole.y-ground.height)<.0008f,"planted bear paw floats above terrain");
    }
    Check(supports>=2,"bear lost two-paw support");
    Check(!solids.IntersectsCircle(state.x,state.z,island::BearMotion::BodyClearance),
        "bear body intersects an actual world obstacle");
}
}

int main() {
    using namespace island;
    const IslandTerrain terrain;
    const VegetationLayout palms(terrain);const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);const ShipwreckLayout wreck(terrain);
    const CampfireLayout fire(terrain);
    std::vector<CircularObstacle> solids=palms.Obstacles();
    solids.insert(solids.end(),jungle.Obstacles().begin(),jungle.Obstacles().end());
    solids.insert(solids.end(),props.Obstacles().begin(),props.Obstacles().end());
    solids.insert(solids.end(),wreck.Obstacles().begin(),wreck.Obstacles().end());
    solids.insert(solids.end(),fire.Obstacles().begin(),fire.Obstacles().end());
    const auto hut=HutLayout::FindInWorld(terrain,jungle,solids);
    Check(hut.has_value(),"actual bear world is missing the hut");
    solids.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    const WorldCollisionGrid collision(solids);

    BearMotion bear(terrain,solids),repeat(terrain,solids);
    Check(bear.State()==repeat.State(),"bear initial state is not deterministic");
    Check(bear.IsValidPosition(bear.State().x,bear.State().z),"bear starts outside safe habitat");
    std::array<bool,6> observed{};std::array<std::size_t,4> swings{};
    std::size_t locked=0;float maximumMove=0;
    const BearPoint distant{1000,1000,1000};
    for(int tick=0;tick<6000;++tick) {
        const auto previous=bear.State();bear.Update(.1f,distant);repeat.Update(.1f,distant);
        const auto& state=bear.State();
        Check(state==repeat.State(),"identical bear simulations diverged");
        Check(bear.IsValidPosition(state.x,state.z),"bear left bounded safe habitat");
        Check(state.speed>=0&&state.speed<=BearMotion::MaximumSpeed+.0001f
            &&state.foragePose>=0&&state.foragePose<=1,"invalid bear motion state");
        const float move=Distance({previous.x,previous.y,previous.z},{state.x,state.y,state.z});
        maximumMove=std::max(maximumMove,move);Check(move<.18f,"bear teleported");
        CheckPose(bear,collision);
        observed[static_cast<std::size_t>(state.behavior)]=true;
        for(std::size_t leg=0;leg<4;++leg) {
            if(previous.pose.legs[leg].stance&&state.pose.legs[leg].stance) {
                Check(Distance(Sole(previous,leg),Sole(state,leg))<.0001f,"planted bear paw slides in world space");
                ++locked;
            }
            if(!state.pose.legs[leg].stance)++swings[leg];
        }
    }
    std::cout<<"bear 600s distance="<<bear.State().distanceTravelled<<" max-step="<<maximumMove<<'\n';
    Check(bear.State().distanceTravelled>30&&locked>500,"bear remains stuck instead of roaming");
    Check(observed[static_cast<std::size_t>(BearBehavior::Idle)]
        &&observed[static_cast<std::size_t>(BearBehavior::Walk)]
        &&observed[static_cast<std::size_t>(BearBehavior::Trot)]
        &&observed[static_cast<std::size_t>(BearBehavior::Forage)]
        &&observed[static_cast<std::size_t>(BearBehavior::Rest)],"bear behavioral cycle is incomplete");
    Check(std::all_of(swings.begin(),swings.end(),[](std::size_t value){return value>50;}),
        "one or more bear legs never step");

    const auto unchanged=bear.State();bear.Update(0,distant);bear.Update(-1,distant);
    bear.Update(std::numeric_limits<float>::quiet_NaN(),distant);
    Check(bear.State()==unchanged,"invalid elapsed time moves bear");
    bear.Update(100,distant);repeat.Update(.1f,distant);
    Check(bear.State()==repeat.State(),"large elapsed time is not bounded to one update");

    BearMotion close(terrain,solids),high(terrain,solids),far(terrain,solids);
    const auto initial=close.State();const BearPoint observer{initial.x-3,initial.y+.8f,initial.z};
    const float startingDistance=std::hypot(initial.x-observer.x,initial.z-observer.z);bool withdrew=false;
    for(int tick=0;tick<240;++tick) {
        close.Update(.1f,observer);high.Update(.1f,{initial.x,initial.y+100,initial.z});far.Update(.1f,distant);
        withdrew|=close.State().behavior==BearBehavior::Withdraw;
        Check(high.State()==far.State(),"high fly-mode observer disturbs the bear");
        CheckPose(close,collision);
    }
    const float endingDistance=std::hypot(close.State().x-observer.x,close.State().z-observer.z);
    Check(withdrew&&endingDistance>startingDistance+3,"bear does not calmly withdraw from nearby player");
    Check(close.State().distanceTravelled<45,"peaceful withdrawal became an unbounded chase");

    for(const auto kind:{BearCallKind::Breath,BearCallKind::Snuffle,BearCallKind::Huff}) {
        const auto a=GenerateBearCall(kind,0),b=GenerateBearCall(kind,1);
        Check(a.sampleRate==22050&&a.samples.size()==35280&&a.samples.front()==0&&a.samples.back()==0
            &&b.samples.front()==0&&b.samples.back()==0&&a.samples==GenerateBearCall(kind,0).samples
            &&a.samples!=b.samples&&std::any_of(a.samples.begin(),a.samples.end(),[](auto value){return value>900;})
            &&std::all_of(a.samples.begin(),a.samples.end(),[](auto value){return std::abs(value)<21000;}),
            "bear PCM is not deterministic, distinct, bounded and silent at endpoints");
        int zeroCrossings=0;
        for(std::size_t sample=1;sample<a.samples.size();++sample)
            zeroCrossings+=(a.samples[sample-1]<0&&a.samples[sample]>=0)
                ||(a.samples[sample-1]>0&&a.samples[sample]<=0);
        Check(zeroCrossings<9600
                &&(kind!=BearCallKind::Huff||zeroCrossings<3200),
            "bear airway synthesis regressed into high-frequency hiss");
        Check(BearCallVolume(kind,0)>BearCallVolume(kind,8)&&BearCallVolume(kind,60)==0
            &&BearCallVolume(kind,-1)==0&&BearCallVolume(kind,NAN)==0,
            "bear voice attenuation is not finite and distance-bounded");
    }
    BearState audioBear;audioBear.x=0;audioBear.y=0;audioBear.z=0;
    BearCallScheduler calls,repeatedCalls;calls.Reset();repeatedCalls.Reset();
    std::array<bool,2> variants{};std::array<bool,4> kinds{};int count=0,last=-1000;
    for(int tick=0;tick<2400;++tick) {
        audioBear.behavior=tick<600?BearBehavior::Rest:tick<1200?BearBehavior::Forage:
            tick<1800?BearBehavior::Walk:BearBehavior::Withdraw;
        const auto call=calls.Update(.25f,2,1.7f,0,audioBear,true);
        const auto repeat=repeatedCalls.Update(.25f,2,1.7f,0,audioBear,true);
        Check(call.kind==repeat.kind&&call.variant==repeat.variant&&call.volume==repeat.volume
            &&call.pitch==repeat.pitch,"bear sound scheduling is not deterministic");
        if(call.kind==BearCallKind::None)continue;
        Check(tick-last>=20&&call.variant<2&&call.volume>0&&call.volume<=.46f
            &&call.pitch>=-.08f&&call.pitch<=.03f&&call.source.available
            &&call.source.x==0&&std::abs(call.source.y-1.15f)<.0001f,"bear sound request is unbounded");
        last=tick;++count;variants[call.variant]=true;kinds[static_cast<std::size_t>(call.kind)]=true;
    }
    Check(count>25&&variants[0]&&variants[1]&&kinds[1]&&kinds[2]&&kinds[3],
        "bear breathing, snuffling and huff variation is incomplete");
    BearCallScheduler silent;silent.Reset();bool emitted=false;
    for(int tick=0;tick<100;++tick)emitted|=silent.Update(.25f,100,1.7f,0,audioBear,true).kind!=BearCallKind::None;
    Check(!emitted,"inaudible distant bear created playback requests");

    bool refused=false;
    try {const std::array blocked{CircularObstacle{0,0,400}};BearMotion impossible(terrain,blocked);}
    catch(const std::runtime_error&){refused=true;}
    Check(refused,"bear silently invents a habitat in a fully blocked world");
    std::cout<<"Bear deterministic habitat, planted paws, behavior and withdrawal tests passed\n";
}
