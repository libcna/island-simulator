#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/GoatMesh.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

int main() {
    using namespace island;
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const IslandTerrain terrain;
    const RenderedTerrainSurface surface(terrain);
    const VegetationLayout palms(terrain);
    const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);
    const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> occupied;
    for(const auto obstacles:std::array<std::span<const CircularObstacle>,4>{
        palms.Obstacles(),jungle.Obstacles(),props.Obstacles(),wreck.Obstacles()})
        occupied.insert(occupied.end(),obstacles.begin(),obstacles.end());
    const auto hut=HutLayout::FindInWorld(terrain,jungle,occupied);
    check(hut.has_value(),"reference hut missing");
    occupied.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    const WorldCollisionGrid grid(occupied);
    GoatHerd herd(terrain,jungle,occupied),repeat(terrain,jungle,occupied);
    const auto copy=[](const GoatHerd& h){return std::vector<GoatState>(h.Goats().begin(),h.Goats().end());};
    const auto equal=[&](const GoatHerd& a,const GoatHerd& b){return copy(a)==copy(b);};
    const GoatPoint distant{1000,1000,1000};
    std::array<float,GoatHabitat::Count> travelled{};
    std::array<bool,4> behaviors{};
    float minimumHoofClearance=1;
    for(int frame=0;frame<900;++frame) {
        const auto before=copy(herd);
        herd.Update(.1f,distant);repeat.Update(.1f,distant);
        check(equal(herd,repeat),"identical goat inputs diverged");
        for(std::size_t i=0;i<herd.Goats().size();++i) {
            const auto s=herd.Goats()[i],p=before[i];
            const float d=std::hypot(s.x-p.x,s.y-p.y,s.z-p.z);
            check(d<=.13f,"goat teleported or exceeded movement bound");
            travelled[i]+=d;behaviors[static_cast<std::size_t>(s.behavior)]=true;
            check(herd.IsValidPosition(i,s.x,s.z),"goat left allowed habitat/home");
            check(GoatHabitat::Accepts(terrain,surface,jungle,grid,s.x,s.z),
                "indexed movement clearance differs from original placement checks");
            check(s.y==surface.Sample(s.x,s.z).height,"goat lost rendered ground height");
            check(std::abs(std::remainder(s.heading-p.heading,2*std::numbers::pi_v<float>))<=.1801f,
                "goat heading snapped beyond bounded turn rate");
            check(std::isfinite(s.phase)&&s.speed>=0&&s.speed<=1.20001f
                &&s.grazingPose>=0&&s.grazingPose<=1,"invalid goat motion state");
            if(d==0) check(s.phase==p.phase,"blocked/stationary goat advanced stride");
            int supports=0;
            for(std::size_t leg=0;leg<4;++leg) {
                const auto& current=s.pose.legs[leg];const auto& previous=p.pose.legs[leg];
                supports+=current.stance?1:0;
                check(!current.reachLimited,"authoritative goat contact pose is unreachable");
                if(current.stance&&previous.stance) {
                    const auto a=p.pose.ToWorld({previous.hoof.x,previous.hoof.y-GoatLegPose::HoofHeight,
                        previous.hoof.z+GoatLegPose::SoleForward});
                    const auto b=s.pose.ToWorld({current.hoof.x,current.hoof.y-GoatLegPose::HoofHeight,
                        current.hoof.z+GoatLegPose::SoleForward});
                    check(std::hypot(b.x-a.x,b.y-a.y,b.z-a.z)<.0001f,"planted goat hoof moved with the body");
                    const auto local=s.pose.ToLocal(b);
                    check(std::abs(local.x-current.hoof.x)<.0001f,"goat body inverse transform drifted");
                }
            }
            check(supports>=2,"goat lost two-foot support during stepping");
            if(frame%10==0) {
                const auto& pose=s.pose;
                const auto mesh=BuildGoatMesh(pose,s.grazingPose,true);
                for(const auto& vertex:mesh.vertices) {
                    const auto point=pose.ToWorld(vertex.position);
                    check(std::hypot(point.x-s.x,point.z-s.z)<=GoatHabitat::BodyClearance,
                        "anchored limb mesh exceeds collision clearance envelope");
                    if(vertex.part==GoatPart::Hoof)minimumHoofClearance=std::min(minimumHoofClearance,
                        point.y-surface.Sample(point.x,point.z).height);
                }
                for(const auto& leg:pose.legs) {
                    check(!leg.reachLimited,"moving goat lost reachable hoof support");
                    const auto sole=pose.ToWorld({leg.hoof.x,leg.hoof.y-GoatLegPose::HoofHeight,
                        leg.hoof.z+GoatLegPose::SoleForward});
                    const float error=sole.y-surface.Sample(sole.x,sole.z).height;
                    check(error>=-.00015f&&(!leg.stance||std::abs(error)<.00015f),
                        "moving goat sole contact differs from rendered terrain");
                }
            }
            for(std::size_t j=0;j<i;++j)
                check(std::hypot(s.x-herd.Goats()[j].x,s.z-herd.Goats()[j].z)>=2*GoatHabitat::BodyClearance,
                    "goats overlap");
        }
    }
    std::cout<<"goat minimum actual hoof vertex clearance="<<minimumHoofClearance<<std::endl;
    check(minimumHoofClearance>=-.001f,"goat hoof mesh penetrates rendered terrain");
    for(float distance:travelled) {
        std::cout<<"goat 90s travelled="<<distance<<'\n';
        check(distance>1,"goat stayed stuck throughout 90s");
    }
    check(behaviors[0]&&behaviors[1]&&behaviors[2],"grazing/walking/rest cycle missing");
    const auto unchanged=copy(herd);
    herd.Update(0,distant);herd.Update(-1,distant);
    herd.Update(std::numeric_limits<float>::quiet_NaN(),distant);
    check(copy(herd)==unchanged,"invalid delta mutated goats");
    herd.Update(100,distant);repeat.Update(.1f,distant);
    check(equal(herd,repeat),"large delta not bounded to .1s");
    GoatHerd close(terrain,jungle,occupied),high(terrain,jungle,occupied),far(terrain,jungle,occupied);
    const auto initial=close.Goats().front();
    const GoatPoint observer{initial.x-2.5f,initial.y+.8f,initial.z};
    bool retreated=false;
    for(int frame=0;frame<200;++frame) {
        close.Update(.1f,observer);
        high.Update(.1f,{initial.x,initial.y+100,initial.z});far.Update(.1f,distant);
        check(equal(high,far),"high fly-mode observer triggers ground retreat");
        retreated|=close.Goats().front().behavior==GoatBehavior::Retreat;
    }
    const auto escaped=close.Goats().front();
    const float clearance=std::hypot(escaped.x-observer.x,escaped.z-observer.z);
    std::cout<<"goat observer clearance="<<clearance<<'\n';
    check(retreated&&clearance>3.5f,"near observer did not cause a bounded retreat");
    high.Update(.1f,{std::numeric_limits<float>::quiet_NaN(),0,0});far.Update(.1f,distant);
    check(equal(high,far),"invalid observer affects goat decisions");
    auto obstacleRoute=occupied;
    const CircularObstacle headOn{initial.x+2.7f,initial.z,.6f};
    obstacleRoute.push_back(headOn);
    GoatHerd deflected(terrain,jungle,obstacleRoute);
    check(deflected.Goats().front().x==initial.x&&deflected.Goats().front().z==initial.z,
        "head-on regression unexpectedly changed the initial home");
    for(int frame=0;frame<200;++frame) {
        deflected.Update(.1f,observer);
        for(const auto& s:deflected.Goats())
            check(std::hypot(s.x-headOn.x,s.z-headOn.z)>=GoatHabitat::BodyClearance+headOn.radius,
                "retreat tunneled through head-on obstacle");
    }
    const auto around=deflected.Goats().front();
    const float aroundClearance=std::hypot(around.x-observer.x,around.z-observer.z);
    std::cout<<"goat head-on obstacle observer clearance="<<aroundClearance<<'\n';
    check(aroundClearance>3.5f,"head-on obstacle permanently trapped retreating goat");
    const std::array<CircularObstacle,1> blocked{{{0,0,400}}};
    bool rejected=false;
    try { GoatHerd impossible(terrain,jungle,blocked); } catch(const std::runtime_error&) { rejected=true; }
    check(rejected,"impossible goat habitat silently accepted");
    std::cout<<"Goat herd CPU tests passed; this is not a live rendering audit\n";
}
