#include "IslandSimulator/CaveLayout.hpp"
#include "IslandSimulator/CaveGroundPatch.hpp"
#include "IslandSimulator/TidePoolLayout.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/FlightMotion.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/SpiderColony.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
void Check(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
using Point=std::array<float,3>;
Point Key(island::CavePoint p) {return {p.x,p.y,p.z};}
}
int main() {
    using namespace island;
    const IslandTerrain terrain;
    const CaveLayout cave(terrain);
    const RenderedTerrainSurface original(terrain);
    const CaveGroundPatch ground(cave,original);
    const auto entrance=cave.Entrance();
    const VegetationLayout palms(terrain);
    const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);
    const auto survey=[&](const char* name,const auto& placements,float margin) {
        int count=0;for(const auto& p:placements)if(cave.ReservesHabitat(p.x,p.z,margin))++count;
        std::cout<<"cave footprint survey "<<name<<' '<<count<<'\n';
        Check(count==0,"generated placement intersects reserved cave footprint");
    };
    survey("palms",palms.Palms(),5);survey("canopy",jungle.Canopy(),6);
    survey("understory",jungle.Understory(),2);survey("bushes",jungle.Bushes(),2);
    survey("plants",jungle.GroundPlants(),1);survey("rocks",props.Rocks(),3);
    survey("logs",props.FallenLogs(),4);survey("fronds",props.FallenFronds(),2);
    std::cout<<"baseline bushes/plants "<<jungle.Bushes().size()-jungle.ClearingBushCount()<<'/'
        <<jungle.GroundPlants().size()-jungle.ClearingGroundPlantCount()
            -jungle.ForestGroundPlantCount()-jungle.StreamBankGroundPlantCount()
            -jungle.SeasonalWetlandGroundPlantCount()<<'\n';
    std::cout<<"cave entrance "<<entrance.x<<','<<entrance.y<<','<<entrance.z
             <<" triangles "<<cave.Triangles().size()<<'\n';
    Check(cave.Triangles().size()<1600,"cave geometry budget exceeded");
    Check(ground.Vertices().size()<4096,"cave ground mesh budget exceeded");
    Check(ground.Indices().size()==static_cast<std::size_t>((ground.Columns()-1)*(ground.Rows()-1)*6),
          "ground patch is missing replacement triangles");
    for(std::size_t i=0;i<ground.Indices().size();i+=3) {
        const auto a=ground.Vertices()[ground.Indices()[i]];
        const auto b=ground.Vertices()[ground.Indices()[i+1]];
        const auto c=ground.Vertices()[ground.Indices()[i+2]];
        for(const auto weights:{std::array{.2f,.3f,.5f},std::array{.7f,.2f,.1f}}) {
            const float x=a.x*weights[0]+b.x*weights[1]+c.x*weights[2];
            const float z=a.z*weights[0]+b.z*weights[1]+c.z*weights[2];
            const float y=a.y*weights[0]+b.y*weights[1]+c.y*weights[2];
            Check(std::abs(ground.SampleHeight(x,z)-y)<.0002f,
                  "ground collision does not follow the emitted triangles");
        }
    }
    for(int row=0;row<ground.Rows();++row) for(int col=0;col<ground.Columns();++col) {
        if(row!=0&&col!=0&&row!=ground.Rows()-1&&col!=ground.Columns()-1) continue;
        const auto p=ground.Vertices()[row*ground.Columns()+col];
        Check(p.y==original.Sample(p.x,p.z).height,"ground patch boundary tears original terrain");
    }
    // Every emitted tracing interval must be truly affine, including rounded-grid diagonals.
    for(float across:{-5.0f,-1.0f,2.0f,5.0f}) {
        const auto a=cave.World(across,0,-5),b=cave.World(-across,0,23);
        auto knots=ground.TraceParameters(a.x,a.z,b.x,b.z);
        knots.insert(knots.begin(),0);knots.push_back(1);
        const auto sampled=[&](float t){return ground.SampleHeight(std::lerp(a.x,b.x,t),std::lerp(a.z,b.z,t));};
        for(std::size_t i=1;i<knots.size();++i) {
            const float lo=knots[i-1],hi=knots[i];
            for(float t:{.2f,.5f,.8f})
                Check(std::abs(sampled(std::lerp(lo,hi,t))-std::lerp(sampled(lo),sampled(hi),t))<.0001f,
                      "flight ground tracing missed a patch edge or triangle diagonal");
        }
    }
    const auto mountainBack=cave.World(0,0,CaveLayout::Length);
    Check(entrance.y>10&&terrain.SignedShoreDistance(entrance.x,entrance.z)>90
          &&terrain.Height(mountainBack.x,mountainBack.z)>entrance.y+5.5f,
          "cave entrance is not embedded in a rising inland mountain slope");
    for(std::size_t i=0;i<TidePoolLayout::Definitions.size();++i) {
        const auto [x,z]=TidePoolLayout::Centre(terrain,i);
        Check(!cave.ReservesHabitat(x,z,TidePoolLayout::Definitions[i].radius),
              "cave footprint overlaps an authored tide pool");
    }
    std::map<std::array<Point,2>,std::pair<int,int>> edges;
    std::array<int,5> parts{};
    double volume=0;
    float leftMouthTop=-INFINITY,rightMouthTop=-INFINITY;
    float innerMouthExtent=0,outerMouthExtent=0,nearMantleExtent=0;
    for(const auto& triangle:cave.Triangles()) {
        for(const auto p:triangle.points) {
            Check(CaveSite::Reserves(p.x,p.z),"wildlife reserve does not contain the complete rock shell");
            const auto local=cave.Local(p);
            if(triangle.surface==CaveSurface::Entrance&&std::abs(local.z)<.0001f) {
                innerMouthExtent=std::max(innerMouthExtent,std::abs(local.x));
                if(local.x<-.1f)leftMouthTop=std::max(leftMouthTop,local.y);
                if(local.x>.1f)rightMouthTop=std::max(rightMouthTop,local.y);
            }
            if(triangle.surface==CaveSurface::Entrance&&local.z<-.1f)
                outerMouthExtent=std::max(outerMouthExtent,std::abs(local.x));
            if(triangle.surface==CaveSurface::Exterior&&local.z>3&&local.z<5)
                nearMantleExtent=std::max(nearMantleExtent,std::abs(local.x));
        }
        ++parts[static_cast<int>(triangle.surface)];
        const auto a=triangle.points[0],b=triangle.points[1],c=triangle.points[2],n=triangle.normal;
        const double nx=(b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y);
        const double ny=(b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z);
        const double nz=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
        volume+=(a.x-entrance.x)*nx+(a.y-entrance.y)*ny+(a.z-entrance.z)*nz;
        Check(std::hypot(nx,ny,nz)>1e-6,"degenerate rock surface");
        Check(nx*n.x+ny*n.y+nz*n.z>0,"cave winding disagrees with surface normal");
        Check(std::abs(std::hypot(n.x,n.y,n.z)-1)<1e-5,"cave normal not unit length");
        for(int j=0;j<3;++j) {
            auto p=Key(triangle.points[j]),q=Key(triangle.points[(j+1)%3]);
            const int direction=p<q?1:-1;
            if(q<p) std::swap(p,q);
            auto& edge=edges[{p,q}];++edge.first;edge.second+=direction;
        }
    }
    for(const auto& [key,edge]:edges) {
        (void)key;
        Check(edge.first==2&&edge.second==0,"cave solid has an open or inconsistently wound seam");
    }
    Check(std::all_of(parts.begin(),parts.end(),[](int n){return n>0;}),"missing cave surface group");
    Check(volume/6>350&&volume/6<800,"compact rock mantle volume regressed");
    Check(std::isfinite(leftMouthTop)&&std::isfinite(rightMouthTop)
          &&std::abs(leftMouthTop-rightMouthTop)>.03f,
          "cave mouth lost its entrance-local asymmetry");
    Check(outerMouthExtent-innerMouthExtent>1.0f
          &&outerMouthExtent-innerMouthExtent<2.0f,
          "cave entrance no longer has a restrained eroded rock lip");
    Check(nearMantleExtent>outerMouthExtent+.25f,
          "cave lip no longer grows into its wider inland mantle");
    constexpr float radius=PlayerMotion::CapsuleRadius,height=1.9f;
    for(int step=-30;step<=195;++step) {
        const float depth=step*.1f;
        const auto p=cave.World(0,CaveLayout::Floor(depth)+.003f,depth);
        Check(!cave.IntersectsCapsule(p,radius,height),"centreline is not walkable through the mouth");
        const auto local=cave.Local(p);
        Check(std::abs(local.z-depth)<.00003f&&std::abs(local.x)<.00003f,
              "cave coordinate frames disagree");
        if(depth>=0&&depth<=CaveLayout::Length) {
            Check(std::abs(cave.GroundTarget(p.x,p.z,p.y+7)-(p.y-.003f))<.00002f,
                  "terrain excavation does not clear the cave floor");
            Check(std::abs(ground.SampleHeight(p.x,p.z)-(p.y-.003f))<.00002f,
                  "meshed replacement seals the interior with a terrain sheet");
        }
    }
    const auto start=cave.World(0,.003f,0);
    const auto end=cave.World(0,CaveLayout::Floor(19)+.003f,19);
    Check(cave.SafeTravelFraction(start,end,radius,height)==1,"continuous entrance-to-back walk blocked");
    const auto middle=cave.World(0,CaveLayout::Floor(10)+.003f,10);
    for(const auto target:{cave.World(9,CaveLayout::Floor(10)+.003f,10),
                           cave.World(-9,CaveLayout::Floor(10)+.003f,10),
                           cave.World(0,12,10),cave.World(0,-5,10),cave.World(0,1,27)}) {
        const float fraction=cave.SafeTravelFraction(middle,target,radius,height);
        Check(fraction>=0&&fraction<1,"player tunneled through wall/floor/roof/back");
        const CavePoint stopped{std::lerp(middle.x,target.x,fraction),
            std::lerp(middle.y,target.y,fraction),std::lerp(middle.z,target.z,fraction)};
        Check(!cave.IntersectsCapsule(stopped,radius,height),"sweep leaves capsule in rock");
    }
    Check(cave.IntersectsCapsule(cave.World(3.7f,.2f,10),radius,height),
          "capsule embedded in thick rock was accepted");
    Check(cave.IntersectsCapsule({NAN,0,0},radius,height),"invalid capsule did not fail closed");
    Check(cave.SafeTravelFraction(start,{INFINITY,0,0},radius,height)==0,"invalid sweep accepted");
    Check(!cave.IntersectsCapsule({0,50,0},radius,height),"distant player collides with cave");
    for(float depth:{1.0f,5.0f,10.0f,15.0f,19.0f}) {
        for(float across:{-1.4f,0.0f,1.4f}) {
            const auto inside=cave.World(across,CaveLayout::Floor(depth)+.003f,depth);
            Check(!cave.IntersectsCapsule(inside,radius,height),"usable interior is narrower than intended");
            const auto sky=cave.World(across,9,depth);
            Check(!cave.IntersectsCapsule(sky,radius,height),"open sky was classified as solid rock");
            const float fraction=cave.SafeTravelFraction(sky,inside,radius,height);
            Check(fraction>0&&fraction<.8f,"flight from outside pierced the roof");
        }
        for(float side:{-9.0f,9.0f}) {
            const auto outside=cave.World(side,CaveLayout::Floor(depth)+.003f,depth);
            const auto inside=cave.World(0,CaveLayout::Floor(depth)+.003f,depth);
            Check(!cave.IntersectsCapsule(outside,radius,height),"outside air classified as rock");
            const auto fraction=cave.SafeTravelFraction(outside,inside,radius,height);
            Check(fraction>0&&fraction<1,"outer wall permits entry away from portal");
        }
    }
    for(float depth:{-7.0f,25.0f}) for(float across:{-8.0f,0.0f,8.0f}) {
        const auto p=cave.World(across,0,depth);
        Check(cave.GroundTarget(p.x,p.z,7.0f)==7.0f,"cave modifies distant terrain");
    }
    // Actual controller integration, not just isolated shell queries.
    PlayerMotion walker;
    walker.SetCave(&cave,&ground);
    std::vector<CircularObstacle> solids=palms.Obstacles();
    solids.insert(solids.end(),jungle.Obstacles().begin(),jungle.Obstacles().end());
    solids.insert(solids.end(),props.Obstacles().begin(),props.Obstacles().end());
    const WorldCollisionGrid worldSolids(solids);
    // Keep all live populations and check complete bodies through nearby simulation,
    // not merely their spawn centres against a nominal cave axis.
    GoatHerd goats(terrain,jungle,solids);
    BoarHerd boars(terrain,solids);
    SnakePopulation snakes(terrain,jungle,solids);
    SpiderColony spiders(terrain,jungle,solids);
    Check(goats.Goats().size()==6&&boars.Boars().size()==6&&snakes.Snakes().size()==50
        &&spiders.Spiders().size()==200,"cave reservation reduced a requested animal population");
    for(int step=0;step<180;++step) {
        const auto observer=cave.World(step%2?9.0f:-9.0f,1.7f,static_cast<float>(step%24)-2);
        goats.Update(.1f,{observer.x,observer.y,observer.z});
        boars.Update(.1f,observer.x,observer.y,observer.z);
        snakes.Update(.1f,observer.x,observer.y,observer.z);
        spiders.Update(.1f,{observer.x,observer.y,observer.z});
        for(const auto& goat:goats.Goats())
            Check(!CaveSite::Reserves(goat.x,goat.z,GoatHabitat::BodyClearance),"goat entered the cave rock footprint");
        for(const auto& boar:boars.Boars())
            Check(!CaveSite::Reserves(boar.x,boar.z,BoarHerd::CollisionRadius),"boar entered the cave rock footprint");
        for(const auto& snake:snakes.Snakes())for(const auto p:snake.body)
            Check(!CaveSite::Reserves(p.x,p.z,SnakePopulation::MaximumRadius),"snake body crossed the cave rock footprint");
        for(const auto& spider:spiders.Spiders()) {
            Check(!CaveSite::Reserves(spider.x,spider.z,SpiderHabitat::BodyRadius),"spider body entered cave rock");
            for(const auto p:spider.pose.feet)
                Check(!CaveSite::Reserves(p.x,p.z),"spider foot entered cave rock");
        }
    }
    const auto approach=cave.World(0,0,-3),forward=cave.World(0,0,1);
    const float dx=forward.x-entrance.x,dz=forward.z-entrance.z;
    walker.SetPosition(terrain,approach.x,approach.z);
    for(int i=0;i<350;++i)walker.Update(terrain,dx,dz,1.0f/60,worldSolids);
    auto local=cave.Local({walker.Position().x,walker.Position().y-PlayerMotion::EyeHeight,walker.Position().z});
    std::cout<<"walked cave depth "<<local.z<<'\n';
    Check(local.z>18&&local.z<20,"walking controller cannot reach cave back or escaped through it");
    for(int i=0;i<100;++i)walker.Update(terrain,dx,dz,.1f,worldSolids);
    Check(cave.Local({walker.Position().x,0,walker.Position().z}).z<20,"walking controller crossed back wall");
    for(int i=0;i<380;++i)walker.Update(terrain,-dx,-dz,1.0f/60,worldSolids);
    Check(cave.Local({walker.Position().x,0,walker.Position().z}).z<0,"walking controller cannot exit cave");
    const auto solid=cave.World(3.0f,0,10);
    walker.SetPosition(terrain,solid.x,solid.z);
    Check(cave.Local({walker.Position().x,0,walker.Position().z}).z<0,
          "invalid placement remained inside rock");
    FlightMotion flight;flight.SetCave(&cave,&ground);
    const auto inside=cave.World(0,CaveLayout::Floor(10)+PlayerMotion::EyeHeight+.02f,10);
    flight.Activate(terrain,{inside.x,inside.y,inside.z});
    Check(std::abs(flight.Position().y-inside.y)<.0001f,"flight activation snaps through cave roof");
    for(int i=0;i<20;++i)flight.Update(0,1,0,true,.1f);
    local=cave.Local({flight.Position().x,flight.Position().y-PlayerMotion::EyeHeight,flight.Position().z});
    Check(local.y>1&&local.y<2,"upward flight crossed or failed to approach inner ceiling");
    for(int i=0;i<20;++i)flight.Update(0,-1,0,true,.1f);
    Check(flight.Position().y>=ground.SampleHeight(inside.x,inside.z)+PlayerMotion::EyeHeight,
          "downward flight penetrated floor");
    const WorldCollisionGrid empty(std::span<const CircularObstacle>{});
    Check(flight.FindLanding(terrain,empty).has_value(),"flight cannot land inside cave");
    const auto sky=cave.World(0,12,10);
    flight.Activate(terrain,{sky.x,sky.y,sky.z});
    for(int i=0;i<20;++i)flight.Update(0,-1,0,true,.1f);
    local=cave.Local({flight.Position().x,flight.Position().y-PlayerMotion::EyeHeight,flight.Position().z});
    std::cout<<"outer roof stopped feet height "<<local.y<<'\n';
    Check(local.y>4.2f&&local.y<7.0f,"downward flight pierced compact exterior roof");
    Check(!cave.IntersectsCapsule({flight.Position().x,flight.Position().y-PlayerMotion::EyeHeight,
          flight.Position().z},radius,height),"flight stopped inside exterior roof");
    std::cout<<"cave ground "<<ground.Vertices().size()<<" vertices "<<ground.Indices().size()/3
             <<" triangles; rock volume "<<volume/6<<" cubic metres\n";
    std::cout<<"Cave shell topology, whole-capsule clearance and terrain target passed\n";
}
