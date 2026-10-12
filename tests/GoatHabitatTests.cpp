#include "IslandSimulator/GoatHabitat.hpp"
#include "IslandSimulator/GoatPose.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
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
    check(hut.has_value(),"reference world hut missing");
    occupied.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    const WorldCollisionGrid grid(occupied);
    const auto homes=GoatHabitat::Find(terrain,surface,jungle,grid);
    check(homes.has_value(),"six open-slope goat homes unavailable");
    check(homes==GoatHabitat::Find(terrain,surface,jungle,grid),"goat placement nondeterministic");
    std::size_t fittedPoses=0,stanceContacts=0;
    const auto length=[](GoatPoint a,GoatPoint b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);};
    for(std::size_t i=0;i<homes->size();++i) {
        const auto p=(*homes)[i];
        check(p.y==surface.Sample(p.x,p.z).height,"home not on rendered ground");
        check(GoatHabitat::Accepts(terrain,surface,jungle,grid,p.x,p.z),"invalid selected home");
        for(const auto& tree:jungle.Canopy())
            check(std::hypot(p.x-tree.x,p.z-tree.z)>=GoatHabitat::TreeClearance*tree.scale,
                "home under nearby canopy tree");
        for(const auto& plant:jungle.Understory())
            check(std::hypot(p.x-plant.x,p.z-plant.z)>=GoatHabitat::BodyClearance+1.8f*plant.scale,
                "home inside understory envelope");
        for(const auto& bush:jungle.Bushes())
            check(std::hypot(p.x-bush.x,p.z-bush.z)>=GoatHabitat::BodyClearance+1.2f*bush.scale,
                "home inside bush envelope");
        for(const auto obstacle:occupied)
            check(std::hypot(p.x-obstacle.x,p.z-obstacle.z)>=GoatHabitat::BodyClearance+obstacle.radius,
                "independent obstacle clearance failed");
        for(std::size_t j=0;j<i;++j)
            check(std::hypot(p.x-(*homes)[j].x,p.z-(*homes)[j].z)>=GoatHabitat::MinimumSeparation,
                "goat homes overlap");
        check(std::hypot(p.x-homes->front().x,p.z-homes->front().z)<=GoatHabitat::HerdRadius,
            "herd dispersed beyond home region");
        const std::array<CircularObstacle,1> blocker{{{p.x,p.z,1}}};
        check(!GoatHabitat::Accepts(terrain,surface,jungle,WorldCollisionGrid(blocker),p.x,p.z),
            "occupied home accepted");
        std::cout<<"goat candidate home "<<p.x<<','<<p.y<<','<<p.z<<'\n';
        for(float scale:{.75f,1.0f,1.1f}) for(int heading=0;heading<8;++heading)
            for(int phase=0;phase<32;++phase) for(float speed:{0.0f,.4f,1.2f}) {
                const auto pose=FitGoatGroundPose(terrain,surface,p.x,p.z,
                    heading*std::numbers::pi_v<float>/4,phase*std::numbers::pi_v<float>/16,speed,scale);
                ++fittedPoses;
                for(const auto& leg:pose.legs) {
                    check(!leg.reachLimited,"reference goat home has unreachable hoof pose");
                    check(std::abs(length(leg.root,leg.knee)-GoatLegPose::UpperLength)<1e-5f,
                        "goat upper leg stretched");
                    check(std::abs(length(leg.knee,leg.hoof)-GoatLegPose::LowerLength)<1e-5f,
                        "goat lower leg stretched");
                    const auto sole=pose.ToWorld({leg.hoof.x,leg.hoof.y-GoatLegPose::HoofHeight,
                        leg.hoof.z+GoatLegPose::SoleForward});
                    const float residual=sole.y-surface.Sample(sole.x,sole.z).height;
                    check(residual>=-.00015f,"goat sole penetrates rendered terrain");
                    if(leg.stance) {
                        ++stanceContacts;
                        check(std::abs(residual)<.00015f,"goat stance sole not grounded");
                    }
                }
            }
    }
    const std::array<CircularObstacle,1> blocked{{{0,0,400}}};
    check(!GoatHabitat::Find(terrain,surface,jungle,WorldCollisionGrid(blocked)),"unsafe fallback on blocked island");
    check(!GoatHabitat::Accepts(terrain,surface,jungle,grid,std::numeric_limits<float>::quiet_NaN(),0),
        "nonfinite home accepted");
    check(!GoatHabitat::Accepts(terrain,surface,jungle,grid,300,300),"offshore home accepted");
    constexpr float tau=2*std::numbers::pi_v<float>;
    for(std::size_t leg=0;leg<4;++leg) for(float speed:{0.0f,.4f,1.2f}) {
        auto previous=EvaluateGoatLeg(-tau/4096,speed,0,leg);
        for(int step=0;step<=4096;++step) {
            const auto pose=EvaluateGoatLeg(step*tau/4096,speed,0,leg);
            check(length(pose.hoof,previous.hoof)<.002f,"goat hoof cycle discontinuity");
            previous=pose;
        }
        // When both samples are in stance, straight root travel cancels local hoof travel.
        for(float scale:{.75f,1.0f,1.1f}) {
            const float delta=GoatPhaseAdvance(.001f,speed,scale);
            for(int step=0;speed>0&&step<100;++step) {
                const float phase=step*tau/100;
                const auto a=EvaluateGoatLeg(phase,speed,0,leg),b=EvaluateGoatLeg(phase+delta,speed,0,leg);
                if(a.stance&&b.stance)
                    check(std::abs(.001f+scale*(b.hoof.z-a.hoof.z))<1e-6f,"flat constant-speed stance slides");
            }
        }
    }
    check(EvaluateGoatLeg(0,1,0,0,-10).reachLimited,"unreachable ground must be flagged");
    bool rejected=false;
    try { (void)EvaluateGoatLeg(0,1,0,4); } catch(const std::invalid_argument&) { rejected=true; }
    check(rejected,"invalid leg index accepted");
    rejected=false;
    try { (void)FitGoatGroundPose(terrain,surface,0,0,0,0,1,0); }
    catch(const std::invalid_argument&) { rejected=true; }
    check(rejected,"zero-scale pose accepted");
    for(int field=0;field<7;++field) {
        float x=0,z=0,heading=0,phase=0,speed=1,scale=1;
        const float nan=std::numeric_limits<float>::quiet_NaN();
        if(field==0)x=nan;
        if(field==1)z=nan;
        if(field==2)heading=nan;
        if(field==3)phase=nan;
        if(field==4)speed=-1;
        if(field==5)scale=nan;
        if(field==6)x=1000;
        rejected=false;
        try { (void)FitGoatGroundPose(terrain,surface,x,z,heading,phase,speed,scale); }
        catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"invalid ground-pose parameter accepted");
    }
    check(GoatPhaseAdvance(0,1,1)==0&&GoatPhaseAdvance(1,0,1)==0,"idle advances gait");
    check(std::isfinite(GoatPhaseAdvance(std::numeric_limits<float>::max(),1,1)),
        "large finite travel overflowed phase");
    rejected=false;
    try { (void)GoatPhaseAdvance(-1,1,1); } catch(const std::invalid_argument&) { rejected=true; }
    check(rejected,"negative travelled distance accepted");
    std::cout<<"goat fitted poses="<<fittedPoses<<" stance contacts="<<stanceContacts<<'\n';
    std::cout<<"Goat habitat/pose foundation passed; no live goats are installed by this test\n";
}
