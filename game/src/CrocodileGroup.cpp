#include "IslandSimulator/CrocodileGroup.hpp"

#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/StreamRoute.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
constexpr float Tau=2.0f*std::numbers::pi_v<float>;
float Wrap(float value) noexcept {
    while(value>std::numbers::pi_v<float>)value-=Tau;
    while(value<-std::numbers::pi_v<float>)value+=Tau;
    return value;
}
float MoveAngle(float current,float target,float step) noexcept {
    return Wrap(current+std::clamp(Wrap(target-current),-step,step));
}
}

CrocodileGroup::CrocodileGroup(const IslandTerrain& terrain):terrain_(terrain) {
    InitializeAgents();
}

void CrocodileGroup::SetObstacles(const std::span<const CircularObstacle> obstacles) {
    obstacles_.assign(obstacles.begin(),obstacles.end());
    InitializeAgents();
}

void CrocodileGroup::InitializeAgents() {
    agents_.assign(CrocodileCount,Agent{});
    states_.resize(CrocodileCount);homes_.resize(CrocodileCount);
    elapsedSeconds_=0;frameIndex_=0;fullCount_=reducedCount_=dormantCount_=0;
    constexpr float lateral[CrocodileCount]={-8.0f,0.0f,8.0f};
    for(std::size_t i=0;i<CrocodileCount;++i) {
        Agent& agent=agents_[i];
        const auto land=FindAnchor(lateral[i],3.4f,true);
        const auto water=FindAnchor(lateral[i],-4.5f,false);
        agent.home={land.first,land.second,water.first,water.second,22.0f};
        agent.state.x=i==2?water.first:land.first;
        agent.state.z=i==2?water.second:land.second;
        agent.state.headingRadians=std::atan2(water.second-land.second,water.first-land.first);
        agent.state.scale=.92f+static_cast<float>(i)*.10f;
        agent.state.colorVariant=static_cast<std::uint8_t>(i);
        agent.state.behavior=i==2?CrocodileBehavior::Swim:CrocodileBehavior::Bask;
        agent.behaviorSeconds=8.0f+static_cast<float>(i)*4.0f;
        agent.phaseOffset=.8f+static_cast<float>(i)*1.71f;
        const auto sample=terrain_.Sample(agent.state.x,agent.state.z);
        agent.state.submerged=agent.state.behavior==CrocodileBehavior::Swim?1.0f:0.0f;
        agent.state.y=agent.state.submerged>0
            ?std::max(sample.height+.20f*agent.state.scale,-.20f)
            :sample.height+.10f*agent.state.scale;
        states_[i]=agent.state;homes_[i]=agent.home;
    }
}

std::pair<float,float> CrocodileGroup::FindAnchor(float lateral,float targetShore,
                                                  bool requireDry)const {
    const float mouthX=StreamRoute::SourceX-StreamRoute::NominalLength;
    const float mouthZ=StreamRoute::CentreZ(mouthX);
    float best=std::numeric_limits<float>::infinity(),bestX=0,bestZ=0;
    for(int ix=-16;ix<=16;++ix) for(int iz=-18;iz<=18;++iz) {
        const float x=mouthX+static_cast<float>(ix)*1.15f;
        const float z=mouthZ+lateral+static_cast<float>(iz)*.85f;
        const auto sample=terrain_.Sample(x,z);
        const auto experience=IslandEnvironmentField::SampleCoastalExperience(
            x,z,sample.signedShoreDistance);
        if(experience.mangroveMouth<.025f||sample.slopeRadians>.46f)continue;
        if(requireDry?(sample.height<.04f||sample.height>2.2f)
                     :(sample.height>-.18f||sample.height<-1.8f))continue;
        bool obstacleClear=true;
        for(const CircularObstacle& obstacle:obstacles_)
            if(std::hypot(x-obstacle.x,z-obstacle.z)<obstacle.radius+2.65f) {
                obstacleClear=false;break;
            }
        if(!obstacleClear)continue;
        const float score=std::abs(sample.signedShoreDistance-targetShore)*2.8f
            +std::abs((z-mouthZ)-lateral)*.20f+std::abs(x-mouthX)*.08f
            +(1.0f-experience.mangroveMouth)*3.0f+sample.slopeRadians*2.0f;
        if(score<best){best=score;bestX=x;bestZ=z;}
    }
    if(!std::isfinite(best))throw std::runtime_error("No suitable crocodile estuary anchor");
    return {bestX,bestZ};
}

bool CrocodileGroup::IsValidHabitat(float x,float z,const CrocodileHome& home)const noexcept {
    if(!std::isfinite(x)||!std::isfinite(z))return false;
    const auto sample=terrain_.Sample(x,z);
    const auto experience=IslandEnvironmentField::SampleCoastalExperience(
        x,z,sample.signedShoreDistance);
    const float centreX=(home.landX+home.waterX)*.5f,centreZ=(home.landZ+home.waterZ)*.5f;
    return std::hypot(x-centreX,z-centreZ)<=home.radius
        &&sample.signedShoreDistance>=-18.0f&&sample.signedShoreDistance<=18.0f
        &&sample.height>=-2.2f&&sample.height<=3.0f&&sample.slopeRadians<=.52f
        &&experience.mangroveMouth>=.012f;
}

void CrocodileGroup::Update(float deltaSeconds,float observerX,float observerY,
                            float observerZ) noexcept {
    const float dt=std::isfinite(deltaSeconds)?std::clamp(deltaSeconds,0.0f,.1f):0.0f;
    const bool observerValid=std::isfinite(observerX)&&std::isfinite(observerY)
        &&std::isfinite(observerZ);
    elapsedSeconds_+=dt;++frameIndex_;fullCount_=reducedCount_=dormantCount_=0;
    for(std::size_t i=0;i<agents_.size();++i) {
        Agent& agent=agents_[i];
        const float distance=observerValid?std::hypot(agent.state.y-observerY,
            std::hypot(agent.state.x-observerX,agent.state.z-observerZ))
            :std::numeric_limits<float>::infinity();
        agent.state.simulationTier=WildlifeActivationPolicy::Classify(
            distance,FullSimulationDistance,ReducedSimulationDistance);
        switch(agent.state.simulationTier) {
        case WildlifeSimulationTier::Full:++fullCount_;break;
        case WildlifeSimulationTier::Reduced:++reducedCount_;break;
        case WildlifeSimulationTier::Dormant:++dormantCount_;break;
        }
        agent.accumulatedSeconds+=dt;
        const auto divisor=WildlifeActivationPolicy::UpdateDivisor(agent.state.simulationTier);
        if((frameIndex_+i)%divisor==0) {
            UpdateAgent(agent,std::min(agent.accumulatedSeconds,.5f),observerX,observerZ,observerValid);
            agent.accumulatedSeconds=0;
        }
        states_[i]=agent.state;
    }
#ifndef NDEBUG
    assert(fullCount_+reducedCount_+dormantCount_==static_cast<int>(CrocodileCount));
    for(const auto& state:states_) {
        assert(std::isfinite(state.x)&&std::isfinite(state.y)&&std::isfinite(state.z));
        assert(std::isfinite(state.headingRadians)&&std::isfinite(state.speed));
        assert(state.submerged>=0&&state.submerged<=1);
    }
#endif
}

void CrocodileGroup::UpdateAgent(Agent& agent,float dt,float observerX,float observerZ,
                                 bool observerValid) noexcept {
    if(dt<=0)return;
    const float observerDistance=observerValid
        ?std::hypot(agent.state.x-observerX,agent.state.z-observerZ)
        :std::numeric_limits<float>::infinity();
    if(observerDistance<14.0f) {
        agent.state.behavior=CrocodileBehavior::Retreat;
        agent.behaviorSeconds=7.0f;
    } else {
        agent.behaviorSeconds-=dt;
        if(agent.behaviorSeconds<=0) {
            ++agent.transitions;
            switch(agent.state.behavior) {
            case CrocodileBehavior::Bask:
                agent.state.behavior=CrocodileBehavior::Crawl;agent.behaviorSeconds=6.0f;break;
            case CrocodileBehavior::Crawl:
                agent.state.behavior=CrocodileBehavior::Swim;agent.behaviorSeconds=15.0f;break;
            case CrocodileBehavior::Swim:
            case CrocodileBehavior::Retreat:
                agent.state.behavior=CrocodileBehavior::Bask;
                agent.behaviorSeconds=16.0f+static_cast<float>(agent.transitions%4U)*4.0f;break;
            }
        }
    }

    float targetX=agent.home.landX,targetZ=agent.home.landZ;
    if(agent.state.behavior==CrocodileBehavior::Swim
       ||agent.state.behavior==CrocodileBehavior::Retreat) {
        targetX=agent.home.waterX;targetZ=agent.home.waterZ;
        const float orbit=elapsedSeconds_*.09f+agent.phaseOffset;
        targetX+=std::cos(orbit)*3.4f;targetZ+=std::sin(orbit)*3.4f;
    }
    if(agent.state.behavior==CrocodileBehavior::Retreat&&observerValid) {
        const float dx=agent.state.x-observerX,dz=agent.state.z-observerZ;
        const float length=std::max(.01f,std::hypot(dx,dz));
        targetX+=dx/length*8.0f;targetZ+=dz/length*8.0f;
    }
    float desired=std::atan2(targetZ-agent.state.z,targetX-agent.state.x);
    desired+=std::sin(elapsedSeconds_*.17f+agent.phaseOffset)*.10f;
    agent.state.headingRadians=MoveAngle(agent.state.headingRadians,desired,
        dt*(agent.state.behavior==CrocodileBehavior::Retreat?1.15f:.42f));
    const float targetSpeed=agent.state.behavior==CrocodileBehavior::Bask?0.0f:
        agent.state.behavior==CrocodileBehavior::Crawl?.34f:
        agent.state.behavior==CrocodileBehavior::Swim?.72f:1.05f;
    agent.state.speed+=std::clamp(targetSpeed-agent.state.speed,-dt*.7f,dt*.7f);
    const float nx=agent.state.x+std::cos(agent.state.headingRadians)*agent.state.speed*dt;
    const float nz=agent.state.z+std::sin(agent.state.headingRadians)*agent.state.speed*dt;
    if(IsValidHabitat(nx,nz,agent.home)) {agent.state.x=nx;agent.state.z=nz;}
    else agent.state.headingRadians=MoveAngle(agent.state.headingRadians,
        std::atan2((agent.home.waterZ+agent.home.landZ)*.5f-agent.state.z,
                   (agent.home.waterX+agent.home.landX)*.5f-agent.state.x),dt*1.4f);

    const auto sample=terrain_.Sample(agent.state.x,agent.state.z);
    const float targetSubmerged=(agent.state.behavior==CrocodileBehavior::Swim
        ||agent.state.behavior==CrocodileBehavior::Retreat)&&sample.height<.05f?1.0f:0.0f;
    agent.state.submerged+=std::clamp(targetSubmerged-agent.state.submerged,-dt*.8f,dt*.8f);
    const float groundY=sample.height+.10f*agent.state.scale;
    const float swimY=std::max(sample.height+.20f*agent.state.scale,-.18f);
    agent.state.y+=std::clamp(std::lerp(groundY,swimY,agent.state.submerged)-agent.state.y,
                              -dt*.55f,dt*.55f);
    agent.state.gaitPhase=std::fmod(agent.state.gaitPhase+dt*(1.1f+agent.state.speed*2.2f),Tau);
    agent.state.bodyWave=std::sin(agent.state.gaitPhase)*(.10f+.18f*agent.state.submerged);
    const float jawTarget=agent.state.behavior==CrocodileBehavior::Bask
        ?(.5f+.5f*std::sin(elapsedSeconds_*.12f+agent.phaseOffset))*.14f:0.0f;
    agent.state.jawOpen+=std::clamp(jawTarget-agent.state.jawOpen,-dt*.18f,dt*.18f);
}

}
