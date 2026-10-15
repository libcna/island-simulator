#include "IslandSimulator/OctopusGroup.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
constexpr float TwoPi=2*std::numbers::pi_v<float>;
constexpr std::array<float,OctopusGroup::Count> HomeAngles{-2.36f,-.88f,.72f,2.22f};
float Wrap(float v)noexcept{while(v>std::numbers::pi_v<float>)v-=TwoPi;while(v<-std::numbers::pi_v<float>)v+=TwoPi;return v;}
float MoveAngle(float a,float b,float m)noexcept{return Wrap(a+std::clamp(Wrap(b-a),-m,m));}
}
OctopusGroup::OctopusGroup(const IslandTerrain& terrain):terrain_(terrain){agents_.resize(Count);states_.resize(Count);dens_.resize(Count);for(std::size_t i=0;i<Count;++i){Initialize(i);states_[i]=agents_[i].state;dens_[i]=agents_[i].den;}}
void OctopusGroup::Initialize(const std::size_t i){
 auto& a=agents_[i];const float base=HomeAngles[i];float best=-std::numeric_limits<float>::infinity();
 for(int off=-7;off<=7;++off){const float angle=base+off*.075f;for(float d:{9.f,14.f,20.f,27.f,35.f}){const float r=terrain_.ShoreRadius(angle)+d,x=std::cos(angle)*r,z=std::sin(angle)*r;const auto s=terrain_.Sample(x,z);if(s.surface!=TerrainSurface::ShallowSeabed||s.height>-.9f||s.height<-7||s.signedShoreDistance<MinimumShoreDistance||s.signedShoreDistance>MaximumShoreDistance)continue;const float reef=IslandEnvironmentField::ReefSuitability(x,z,s.signedShoreDistance,s.height);const float score=reef*1.8f+s.coastalRockiness*.65f+(1-s.beachSuitability)*.22f-std::fabs(s.height+2.8f)*.035f;if(score>best){best=score;a.den.x=x;a.den.z=z;}}}
 if(!std::isfinite(best))throw std::runtime_error("Unable to place octopus reef den");
 a.den.radius=4.8f+(i%3U)*.8f;a.routePhase=.4f+i*1.51f;a.pulsePhase=i*1.73f;a.armPhase=i*2.11f;a.state.x=a.den.x;a.state.z=a.den.z;a.state.headingRadians=Wrap(base+std::numbers::pi_v<float>*.5f);a.state.scale=.86f+(i%3U)*.1f;a.state.colorVariant=static_cast<std::uint8_t>(i%3U);a.state.behavior=i==0?OctopusBehavior::Forage:(i==1?OctopusBehavior::Crawl:(i==2?OctopusBehavior::Den:OctopusBehavior::Swim));a.behaviorSeconds=4+i*2.8f;a.state.speed=a.state.behavior==OctopusBehavior::Crawl?.11f:(a.state.behavior==OctopusBehavior::Swim?.42f:0);a.state.hideAmount=a.state.behavior==OctopusBehavior::Den?.72f:0;a.state.y=DesiredHeight(a);a.state.mantlePulse=std::sin(a.pulsePhase);a.state.armWave=std::sin(a.armPhase);
}
bool OctopusGroup::ValidHabitat(float x,float z,const OctopusDen& d)const noexcept{const auto s=terrain_.Sample(x,z);const float reef=IslandEnvironmentField::ReefSuitability(x,z,s.signedShoreDistance,s.height);return s.surface==TerrainSurface::ShallowSeabed&&s.height<=-.9f&&s.height>=-7&&s.signedShoreDistance>=MinimumShoreDistance&&s.signedShoreDistance<=MaximumShoreDistance&&(reef>=.025f||s.coastalRockiness>=.28f)&&std::hypot(x-d.x,z-d.z)<=d.radius;}
float OctopusGroup::DesiredHeight(const Agent& a)const noexcept{const float bed=terrain_.Height(a.state.x,a.state.z);const float c=a.state.behavior==OctopusBehavior::Swim?.58f:(a.state.behavior==OctopusBehavior::Den?.08f:.14f);return std::min(-.22f,bed+c*a.state.scale);}
void OctopusGroup::Update(float dt,float ox,float oy,float oz)noexcept{const bool valid=std::isfinite(dt)&&dt>=0;const float delta=valid?std::min(dt,.1f):0;const bool observer=std::isfinite(ox)&&std::isfinite(oy)&&std::isfinite(oz);elapsedSeconds_+=delta;++frameIndex_;fullCount_=reducedCount_=dormantCount_=0;for(std::size_t i=0;i<agents_.size();++i){auto& a=agents_[i];const float distance=observer?std::hypot(a.state.y-oy,std::hypot(a.state.x-ox,a.state.z-oz)):std::numeric_limits<float>::infinity();a.state.simulationTier=valid?WildlifeActivationPolicy::Classify(distance,FullSimulationDistance,ReducedSimulationDistance):WildlifeSimulationTier::Dormant;switch(a.state.simulationTier){case WildlifeSimulationTier::Full:++fullCount_;break;case WildlifeSimulationTier::Reduced:++reducedCount_;break;case WildlifeSimulationTier::Dormant:++dormantCount_;break;}a.accumulatedSeconds+=delta;const auto divisor=WildlifeActivationPolicy::UpdateDivisor(a.state.simulationTier);if(delta>0&&(frameIndex_+i)%divisor==0){UpdateAgent(a,std::min(a.accumulatedSeconds,.5f),ox,oz,observer);a.accumulatedSeconds=0;}states_[i]=a.state;}
#ifndef NDEBUG
assert(fullCount_+reducedCount_+dormantCount_==static_cast<int>(Count));for(const auto& o:states_)assert(std::isfinite(o.x)&&std::isfinite(o.y)&&std::isfinite(o.z)&&std::isfinite(o.headingRadians)&&std::fabs(o.mantlePulse)<=1.001f&&std::fabs(o.armWave)<=1.001f);
#endif
}
void OctopusGroup::UpdateAgent(Agent& a,float dt,float ox,float oz,bool observer)noexcept{if(dt<=0)return;const float od=observer?std::hypot(a.state.x-ox,a.state.z-oz):std::numeric_limits<float>::infinity();a.behaviorSeconds-=dt;if(od<2.7f){a.state.behavior=OctopusBehavior::Den;a.behaviorSeconds=7;}else if(a.behaviorSeconds<=0){++a.transitionCount;switch(a.state.behavior){case OctopusBehavior::Den:a.state.behavior=OctopusBehavior::Crawl;break;case OctopusBehavior::Crawl:a.state.behavior=OctopusBehavior::Forage;break;case OctopusBehavior::Forage:a.state.behavior=a.transitionCount%4==0?OctopusBehavior::Swim:OctopusBehavior::Den;break;case OctopusBehavior::Swim:a.state.behavior=OctopusBehavior::Den;break;}a.behaviorSeconds=a.state.behavior==OctopusBehavior::Den?8:(a.state.behavior==OctopusBehavior::Swim?3.5f:10);}
 a.routePhase=std::fmod(a.routePhase+dt*.16f,TwoPi);float desired=std::atan2(a.den.z+std::sin(a.routePhase)*2.7f-a.state.z,a.den.x+std::cos(a.routePhase)*2.7f-a.state.x);const float hd=std::hypot(a.state.x-a.den.x,a.state.z-a.den.z);if(a.state.behavior==OctopusBehavior::Den||hd>a.den.radius*.72f)desired=std::atan2(a.den.z-a.state.z,a.den.x-a.state.x);a.state.headingRadians=MoveAngle(a.state.headingRadians,desired,(a.state.behavior==OctopusBehavior::Swim?1.2f:.65f)*dt);const float target=a.state.behavior==OctopusBehavior::Crawl?.12f:(a.state.behavior==OctopusBehavior::Forage?.06f:(a.state.behavior==OctopusBehavior::Swim?.44f:0));a.state.speed+=std::clamp(target-a.state.speed,-.28f*dt,.28f*dt);const float x=a.state.x+std::cos(a.state.headingRadians)*a.state.speed*dt,z=a.state.z+std::sin(a.state.headingRadians)*a.state.speed*dt;if(ValidHabitat(x,z,a.den)){a.state.x=x;a.state.z=z;}else{a.state.headingRadians=MoveAngle(a.state.headingRadians,std::atan2(a.den.z-a.state.z,a.den.x-a.state.x),1.5f*dt);a.state.speed=0;}const float y=DesiredHeight(a);a.state.y+=std::clamp(y-a.state.y,-.35f*dt,.35f*dt);const float hide=a.state.behavior==OctopusBehavior::Den?.72f:0;a.state.hideAmount+=std::clamp(hide-a.state.hideAmount,-.7f*dt,.7f*dt);a.pulsePhase=std::fmod(a.pulsePhase+dt*(a.state.behavior==OctopusBehavior::Swim?4.2f:1.5f),TwoPi);a.armPhase=std::fmod(a.armPhase+dt*(1.2f+a.state.speed*4),TwoPi);a.state.mantlePulse=std::sin(a.pulsePhase);a.state.armWave=std::sin(a.armPhase);}
}
