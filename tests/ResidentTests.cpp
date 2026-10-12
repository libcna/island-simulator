#include "IslandSimulator/ResidentMotion.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/CampfireLayout.hpp"
#include "IslandSimulator/StreamBedPatch.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
float Distance(island::ResidentPoint a,island::ResidentPoint b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);}
float Dot(island::ResidentPoint a,island::ResidentPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
void CheckPose(const island::ResidentMotion& actor,const island::WorldCollisionGrid& solids) {
    const auto& state=actor.State();const auto& pose=state.pose;
    Check(std::isfinite(state.x)&&std::isfinite(state.y)&&std::isfinite(state.z)&&std::isfinite(pose.pelvisY),"resident state is nonfinite");
    for(std::size_t side=0;side<2;++side) {
        const auto& leg=pose.legs[side];const auto& foot=leg.foot;
        Check(!solids.IntersectsCircle(foot.centre.x,foot.centre.z,.17f),"resident foot intersects a world obstacle");
        Check(!leg.reachLimited,"resident silently stretches or clamps a planted leg");
        Check(std::abs(Distance(leg.hip,leg.knee)-island::ResidentLegPose::UpperLength)<.000025f
            &&std::abs(Distance(leg.knee,leg.ankle)-island::ResidentLegPose::LowerLength)<.000025f,
            "resident leg changed its bone lengths");
        const island::ResidentPoint expected{foot.centre.x+foot.up.x*island::ResidentLegPose::AnkleHeight,
            foot.centre.y+foot.up.y*island::ResidentLegPose::AnkleHeight,
            foot.centre.z+foot.up.z*island::ResidentLegPose::AnkleHeight};
        Check(Distance(expected,leg.ankle)<.00003f,"resident ankle does not reach its foot");
        Check(std::abs(Dot(foot.up,foot.up)-1)<.00001f&&std::abs(Dot(foot.right,foot.forward))<.00001f,
              "resident sole basis is not orthonormal");
        float smallest=INFINITY;
        for(float x:{-.055f,0.0f,.055f})for(float z:{-.10f,.03f,.16f}) {
            const island::ResidentPoint p{foot.centre.x+foot.right.x*x+foot.forward.x*z,
                foot.centre.y+foot.right.y*x+foot.forward.y*z,foot.centre.z+foot.right.z*x+foot.forward.z*z};
            const float gap=p.y-actor.Ground().Sample(p.x,p.z).height;
            Check(gap>-.00005f,"resident sole crosses the rendered terrain");smallest=std::min(smallest,gap);
        }
        if(leg.stance)Check(smallest<.004f,"planted resident sole floats above the terrain");
        Check(std::abs(Distance(pose.shoulders[side],pose.elbows[side])-.29f)<.00003f
            &&std::abs(Distance(pose.elbows[side],pose.wrists[side])-.26f)<.00003f,"resident arm length changes");
    }
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
    Check(hut.has_value(),"actual resident world is missing the hut");
    solids.push_back({hut->x,hut->z,HutLayout::CollisionRadius});
    const auto started=std::chrono::steady_clock::now();
    ResidentMotion actor(terrain,solids);
    const float setup=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-started).count();
    const WorldCollisionGrid collisions(solids);
    const auto& route=actor.Route();
    std::cout<<"resident tour "<<route.Points().size()<<" points "<<route.Length()<<" metres; setup "<<setup<<" ms\n"<<std::flush;
    Check(route.Length()>400&&route.Length()<1800&&route.Points().size()<512,"resident does not have a bounded island tour");
    const StreamBedPatch stream(StreamCourse(terrain),actor.Ground());
    for(std::size_t i=1;i<route.Points().size();++i) {
        const auto a=route.Points()[i-1],b=route.Points()[i];
        Check(ResidentRoute::SafeSegment(terrain,actor.Ground(),collisions,a,b),"resident route crosses actual world solids");
        const int steps=std::max(1,static_cast<int>(std::ceil(Distance(a,b)/.3f)));
        for(int sample=0;sample<=steps;++sample) {
            const float t=static_cast<float>(sample)/steps,x=std::lerp(a.x,b.x,t),z=std::lerp(a.z,b.z,t);
            if(stream.Contains(x,z))Check(std::abs(stream.SampleHeight(x,z)-actor.Ground().Sample(x,z).height)<.001f,
                "resident tour walks on the removed stream terrain sheet");
        }
    }
    ResidentMotion repeated(terrain,solids);
    Check(std::equal(route.Points().begin(),route.Points().end(),repeated.Route().Points().begin()),"resident tour is not deterministic");
    std::size_t locked=0;
    std::array<std::size_t,2> swings{};
    for(int tick=0;tick<12000;++tick) {
        const auto previous=actor.State();
        actor.Update(.1f,{NAN,NAN,NAN});repeated.Update(.1f,{NAN,NAN,NAN});
        const auto& current=actor.State();
        if(current.rejectedMoves>200) {
            const auto target=route.Points()[current.nextPoint];
            std::cout<<"resident blocked position "<<current.x<<','<<current.y<<','<<current.z
                <<" heading "<<current.heading<<" mode "<<static_cast<int>(current.mode)
                <<" target "<<target.x<<','<<target.y<<','<<target.z<<" safe-direct "
                <<ResidentRoute::SafeSegment(terrain,actor.Ground(),collisions,{current.x,0,current.z},target)<<'\n'<<std::flush;
            for(const auto& leg:current.pose.legs)std::cout<<"blocked foot "<<leg.foot.centre.x<<','<<leg.foot.centre.y<<','
                <<leg.foot.centre.z<<" stance "<<leg.stance<<" pelvis "<<current.pose.pelvisY-current.y<<'\n'<<std::flush;
            throw std::runtime_error("resident is repeatedly blocked rather than continuing its safe tour");
        }
        Check(current==repeated.State(),"resident motion/pose is not deterministic");CheckPose(actor,collisions);
        Check(ResidentRoute::SafePoint(terrain,actor.Ground(),collisions,current.x,current.z),"resident left safe terrain");
        Check(!collisions.IntersectsCircle(current.x,current.z,ResidentRoute::Clearance),
            "resident body intersects an actual world solid");
        Check(Distance({previous.x,previous.y,previous.z},{current.x,current.y,current.z})<.14f,"resident teleported");
        for(std::size_t side=0;side<2;++side) {
            if(previous.pose.legs[side].stance&&current.pose.legs[side].stance) {
                Check(previous.pose.legs[side].foot.centre==current.pose.legs[side].foot.centre,
                    "resident planted foot slides in world space");++locked;
            }
            if(!current.pose.legs[side].stance)++swings[side];
        }
        if(tick%2000==1999)std::cout<<"resident simulation seconds "<<(tick+1)*.1f<<" travel "<<current.distanceTravelled
            <<" next "<<current.nextPoint<<" rejected "<<current.rejectedMoves<<" pauses "<<current.pauseCount<<'\n'<<std::flush;
    }
    Check(actor.State().completedLaps>=1&&actor.State().pauseCount>=5&&locked>500&&swings[0]>500&&swings[1]>500,
        "resident cannot complete a tour with pauses and grounded alternating steps");
    const auto beforeStall=actor.State();actor.Update(100,{NAN,NAN,NAN});
    Check(Distance({beforeStall.x,beforeStall.y,beforeStall.z},{actor.State().x,actor.State().y,actor.State().z})<.14f,
        "a render stall causes unbounded resident catch-up motion");
    for(int tick=0;tick<40;++tick)actor.Update(.1f,{actor.State().x,actor.State().y+1.72f,actor.State().z});
    Check(actor.State().mode==ResidentMode::Yielding&&actor.State().speed==0,"resident does not yield to a nearby player");
    const auto stopped=actor.State();actor.Update(NAN,{NAN,NAN,NAN});actor.Update(-1,{NAN,NAN,NAN});
    Check(actor.State()==stopped,"invalid elapsed time moved the resident");
    const float beforeResume=actor.State().distanceTravelled;
    for(int tick=0;tick<100;++tick){actor.Update(.1f,{NAN,NAN,NAN});CheckPose(actor,collisions);}
    Check(actor.State().distanceTravelled>beforeResume+1,"resident does not resume when the player leaves");
    bool refused=false;
    try {const std::array blocked{CircularObstacle{0,0,350}};ResidentMotion impossible(terrain,blocked);}
    catch(const std::runtime_error&){refused=true;}
    Check(refused,"resident silently creates a fallback in a fully blocked world");
    std::cout<<"resident route, fixed bone lengths, planted soles, pauses and player yielding passed\n";
}
