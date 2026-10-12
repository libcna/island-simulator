#include "IslandSimulator/SpiderColony.hpp"
#include "IslandSimulator/SpiderMesh.hpp"
#include "IslandSimulator/SpiderContactShadow.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    using namespace island;
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const IslandTerrain terrain;const RenderedTerrainSurface surface(terrain);
    const VegetationLayout palms(terrain);const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> obstacles;
    for(const auto items:std::array<std::span<const CircularObstacle>,4>{palms.Obstacles(),
            jungle.Obstacles(),props.Obstacles(),wreck.Obstacles()})
        obstacles.insert(obstacles.end(),items.begin(),items.end());
    const auto hut=HutLayout::FindInWorld(terrain,jungle,obstacles);
    check(hut.has_value(),"reference hut absent");
    obstacles.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    SpiderColony colony(terrain,jungle,obstacles),repeat(terrain,jungle,obstacles);
    std::array<float,12> travel{};
    std::size_t pairs=0,swings=0;float minimumClearance=1;
    for(int step=0;step<1500;++step) {
        const std::vector<SpiderState> previous(colony.Spiders().begin(),colony.Spiders().end());
        colony.Update(.02f,{1000,1000,1000});repeat.Update(.02f,{1000,1000,1000});
        check(std::equal(colony.Spiders().begin(),colony.Spiders().end(),repeat.Spiders().begin()),"nondeterministic crawling");
        for(std::size_t i=0;i<12;++i) {
            const auto& state=colony.Spiders()[i];const auto& old=previous[i];const auto home=colony.Homes()[i];
            const float distance=std::hypot(state.x-old.x,state.y-old.y,state.z-old.z);
            travel[i]+=distance;
            check(distance<.0008f,"crawl teleport/speed error");
            check(std::hypot(state.x-home.x,state.z-home.z)<=SpiderHabitat::PatrolRadius,"escaped home disk");
            check(std::count(state.pose.stance.begin(),state.pose.stance.end(),true)>=6,"too few ground supports");
            for(std::size_t leg=0;leg<8;++leg) {
                const auto foot=state.pose.feet[leg];
                check(!state.pose.legs[leg].reachLimited,"accepted stretched leg");
                if(state.pose.stance[leg]) {
                    check(std::abs(foot.y-surface.Sample(foot.x,foot.z).height-.0001f)<.00002f,"foot not grounded");
                    if(old.pose.stance[leg]){++pairs;check(foot==old.pose.feet[leg],"planted world foot slides");}
                }else ++swings;
                const auto actual=state.pose.ToWorld(state.pose.legs[leg].foot);
                check(std::hypot(actual.x-foot.x,actual.y-foot.y,actual.z-foot.z)<.00003f,"rendered foot differs from anchor");
            }
            if(step%50==0) {
                const auto shadow=BuildSpiderContactShadow(state,terrain,surface);
                check(!shadow.empty()&&shadow.size()%3==0&&shadow.size()<=SpiderShadowVertexLimit,
                    "spider shadow triangle budget invalid");
                for(const auto& p:shadow) {
                    check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)
                        &&std::isfinite(p.opacity)&&p.opacity>=0&&p.opacity<=.300001f,
                        "invalid spider shadow vertex");
                    check(std::abs(p.y-surface.Sample(p.x,p.z).height-SmallContactSurfaceOffset)<.00003f,
                        "spider shadow not on rendered terrain");
                }
                for(std::size_t triangle=0;triangle<shadow.size();triangle+=3) {
                    const auto a=shadow[triangle],b=shadow[triangle+1],c=shadow[triangle+2];
                    const float x=(a.x+b.x+c.x)/3,z=(a.z+b.z+c.z)/3,y=(a.y+b.y+c.y)/3;
                    check(std::abs(y-surface.Sample(x,z).height-SmallContactSurfaceOffset)<.00003f,
                        "spider shadow triangle cuts through terrain");
                }
                auto raised=state;raised.pose.stance.fill(false);
                check(BuildSpiderContactShadow(raised,terrain,surface).size()<shadow.size(),
                    "raised feet retain grounded shadows");
                const auto mesh=BuildSpiderMesh(state.pose.legs,true);
                for(const auto& vertex:mesh.vertices) {
                    const auto p=state.pose.ToWorld(vertex.position);
                    const float clearance=p.y-surface.Sample(p.x,p.z).height;
                    minimumClearance=std::min(minimumClearance,clearance);
                    check(clearance>=-.0001f,"actual spider mesh penetrates terrain");
                    check(std::hypot(p.x-state.x,p.z-state.z)<=SpiderHabitat::BodyRadius,"actual mesh exceeds reserved disk");
                }
            }
        }
    }
    for(float d:travel){std::cout<<"spider 30s travel "<<d<<'\n';check(d>.05f,"immobile spider");}
    check(pairs>100000&&swings>1000,"insufficient stance/swing evidence");
    const auto first=colony.Spiders()[0];
    const SpiderPoint observer{first.x-.5f,first.y+.1f,first.z};
    colony.Update(.02f,observer);
    check(colony.Spiders()[0].behavior==SpiderBehavior::Retreat,"near observer did not trigger retreat");
    const float initialObserverDistance=std::hypot(first.x-observer.x,first.z-observer.z);
    float lastObserverDistance=initialObserverDistance;
    for(int step=0;step<500;++step) {
        colony.Update(.02f,observer);const auto& s=colony.Spiders()[0];
        const float distance=std::hypot(s.x-observer.x,s.z-observer.z);
        check(distance>=lastObserverDistance-.00002f,"retreat approached observer");
        lastObserverDistance=distance;
    }
    check(lastObserverDistance>initialObserverDistance+.05f,"retreat did not actually move away");
    SpiderColony high(terrain,jungle,obstacles),invalid(terrain,jungle,obstacles);
    for(int step=0;step<50;++step){high.Update(.02f,{first.x,first.y+100,first.z});invalid.Update(.02f,{NAN,0,0});}
    check(std::equal(high.Spiders().begin(),high.Spiders().end(),invalid.Spiders().begin()),"high/invalid observer affected ground colony");
    const std::vector<SpiderState> before(invalid.Spiders().begin(),invalid.Spiders().end());
    for(bool turning:{false,true}) {
        SpiderColony audit(terrain,jungle,obstacles);
        int eligible=0,paused=0,reachBlocked=0;
        float moved=0;
        for(int step=0;step<400;++step) {
            const auto old=audit.Spiders()[0];
            const SpiderPoint close{old.x+(turning?.36f:-.5f),old.y+.28f,old.z+(turning?.45f:0)};
            audit.Update(.02f,close);
            const auto& s=audit.Spiders()[0];
            moved+=std::hypot(s.x-old.x,s.z-old.z);
            const float away=std::atan2(old.z-close.z,old.x-close.x);
            const auto home=audit.Homes()[0];
            if(step>10&&std::abs(std::remainder(away-old.heading,6.283185307f))<1.03f
                    &&std::hypot(old.x-home.x,old.z-home.z)<.55f) {
                ++eligible;
                if(s.x==old.x&&s.z==old.z) {
                    ++paused;
                    // Reconstruct the blocked translation with the retained pose.
                    // This checks reach only, not other possible validity failures.
                    auto predicted=s.pose;
                    predicted.origin.x+=std::cos(s.heading)*SpiderColony::RetreatSpeed*.02f;
                    predicted.origin.z+=std::sin(s.heading)*SpiderColony::RetreatSpeed*.02f;
                    predicted.origin.y=surface.Sample(predicted.origin.x,predicted.origin.z).height;
                    bool limited=false;
                    for(std::size_t leg=0;leg<8;++leg)
                        limited|=SpiderPose::Solve(leg,predicted.ToLocal(s.pose.feet[leg])).reachLimited;
                    if(limited)++reachBlocked;
                }
            }
        }
        std::cout<<"spider pause audit turning="<<turning<<" eligible="<<eligible
                 <<" paused="<<paused<<" reachBlocked="<<reachBlocked<<" travel="<<moved<<std::endl;
        check(eligible>200,"too few eligible retreat samples");
        check(paused==0,"eligible retreat stalls while waiting for foot reach");
    }
    for(float dt:{0.0f,-1.0f,INFINITY,NAN})invalid.Update(dt,{0,0,0});
    check(std::equal(before.begin(),before.end(),invalid.Spiders().begin()),"invalid dt changed spiders");
    std::cout<<"stance pairs "<<pairs<<", swing samples "<<swings<<", min mesh clearance "<<minimumClearance<<'\n';
}
