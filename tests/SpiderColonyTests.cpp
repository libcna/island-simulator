#include "IslandSimulator/SpiderColony.hpp"
#include "IslandSimulator/SpiderMesh.hpp"
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
    for(float dt:{0.0f,-1.0f,INFINITY,NAN})invalid.Update(dt,{0,0,0});
    check(std::equal(before.begin(),before.end(),invalid.Spiders().begin()),"invalid dt changed spiders");
    std::cout<<"stance pairs "<<pairs<<", swing samples "<<swings<<", min mesh clearance "<<minimumClearance<<'\n';
}
