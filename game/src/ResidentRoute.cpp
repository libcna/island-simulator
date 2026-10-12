#include "IslandSimulator/ResidentRoute.hpp"
#include "IslandSimulator/CaveSite.hpp"
#include "IslandSimulator/CampfireSite.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/StreamSoundField.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <functional>
#include <stdexcept>
namespace island {
bool ResidentRoute::SafePoint(const IslandTerrain& terrain,const RenderedTerrainSurface& ground,
    const WorldCollisionGrid& solids,float x,float z) {
    if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>Extent||std::abs(z)>Extent
        ||terrain.SignedShoreDistance(x,z)<10||CaveSite::Reserves(x,z,Clearance)
        ||solids.IntersectsCircle(x,z,Clearance))return false;
    const auto environment=IslandEnvironmentField::SampleInland(x,z,terrain.SignedShoreDistance(x,z));
    if(environment.drainage>.65f)return false; // Do not walk on the original, excavated stream sheet.
    if(x<-50&&z<0&&z>-65&&StreamSoundField::Sample(x,z).distance<6.5f)return false;
    const auto centre=ground.Sample(x,z);
    if(centre.height<1.2f||centre.normal.y<.86f)return false;
    for(const auto offset:std::array{std::array{Clearance,0.0f},std::array{-Clearance,0.0f},
                                   std::array{0.0f,Clearance},std::array{0.0f,-Clearance}}) {
        const auto sample=ground.Sample(x+offset[0],z+offset[1]);
        if(sample.normal.y<.86f||std::abs(sample.height-centre.height)>.28f)return false;
    }
    return true;
}
bool ResidentRoute::SafeSegment(const IslandTerrain& terrain,const RenderedTerrainSurface& ground,
    const WorldCollisionGrid& solids,ResidentPoint a,ResidentPoint b) {
    if(!std::isfinite(a.x)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.z))return false;
    const float distance=std::hypot(b.x-a.x,b.z-a.z);
    if(distance>200)return false;
    const int steps=std::max(1,static_cast<int>(std::ceil(distance/.20f)));
    for(int i=0;i<=steps;++i) {
        const float t=static_cast<float>(i)/steps,x=std::lerp(a.x,b.x,t),z=std::lerp(a.z,b.z,t);
        if(!SafePoint(terrain,ground,solids,x,z))return false;
        if(i>0) {
            const float middle=(static_cast<float>(i)-.5f)/steps;
            if(solids.IntersectsCircle(std::lerp(a.x,b.x,middle),std::lerp(a.z,b.z,middle),
                Clearance+distance/(2*steps)))return false;
        }
    }
    return true;
}
namespace {
constexpr int Axis=173,Nodes=Axis*Axis;
constexpr std::array<std::array<int,2>,8> Directions{{{1,0},{0,1},{-1,0},{0,-1},{1,1},{-1,1},{-1,-1},{1,-1}}};
class Search final {
public:
    Search(const IslandTerrain& t,const RenderedTerrainSurface& g,const WorldCollisionGrid& s)
        :terrain(t),ground(g),solids(s),valid(Nodes,0),knownEdges(Nodes,0),safeEdges(Nodes,0) {}
    ResidentPoint Point(int index)const {return {index%Axis*ResidentRoute::GridSpacing-ResidentRoute::Extent,0,
        index/Axis*ResidentRoute::GridSpacing-ResidentRoute::Extent};}
    bool Valid(int index) {
        auto& v=valid[index];
        if(v==0){const auto p=Point(index);v=ResidentRoute::SafePoint(terrain,ground,solids,p.x,p.z)?2:1;}
        return v==2;
    }
    int Nearest(ResidentPoint goal) {
        int best=-1;float score=INFINITY;
        const int gx=static_cast<int>(std::lround((goal.x+ResidentRoute::Extent)/ResidentRoute::GridSpacing));
        const int gz=static_cast<int>(std::lround((goal.z+ResidentRoute::Extent)/ResidentRoute::GridSpacing));
        for(int z=std::max(0,gz-6);z<=std::min(Axis-1,gz+6);++z)
            for(int x=std::max(0,gx-6);x<=std::min(Axis-1,gx+6);++x) {
                const int index=z*Axis+x;const auto p=Point(index);
                const float d=std::hypot(p.x-goal.x,p.z-goal.z);
                if(d<score&&Valid(index)){best=index;score=d;}
            }
        if(best<0)throw std::runtime_error("No safe resident tour landmark approach");
        return best;
    }
    std::vector<ResidentPoint> Path(int from,int to) {
        std::vector<float> costs(Nodes,INFINITY);
        std::vector<int> parent(Nodes,-1);
        std::vector<bool> closed(Nodes,false);
        using Entry=std::pair<float,int>;
        std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
        costs[from]=0;pending.emplace(0,from);
        while(!pending.empty()) {
            const int node=pending.top().second;pending.pop();
            if(closed[node])continue;
            closed[node]=true;if(node==to)break;
            const auto p=Point(node),goal=Point(to);
            for(int direction=0;direction<8;++direction) {
                const int x=node%Axis+Directions[direction][0],z=node/Axis+Directions[direction][1];
                if(x<0||z<0||x>=Axis||z>=Axis)continue;
                const int next=z*Axis+x;if(closed[next]||!Valid(next))continue;
                const auto q=Point(next);
                const auto bit=static_cast<std::uint8_t>(1U<<direction);
                if(!(knownEdges[node]&bit)) {
                    knownEdges[node]|=bit;
                    if(ResidentRoute::SafeSegment(terrain,ground,solids,p,q))safeEdges[node]|=bit;
                }
                if(!(safeEdges[node]&bit))continue;
                const float nextCost=costs[node]+std::hypot(q.x-p.x,q.z-p.z)
                    +std::abs(ground.Sample(q.x,q.z).height-ground.Sample(p.x,p.z).height)*.18f;
                if(nextCost>=costs[next])continue;
                costs[next]=nextCost;parent[next]=node;
                pending.emplace(nextCost+std::hypot(goal.x-q.x,goal.z-q.z),next);
            }
        }
        if(!closed[to])throw std::runtime_error("Resident tour is disconnected by actual ground or obstacles");
        std::vector<ResidentPoint> result;
        for(int i=to;i>=0;i=parent[i]){auto p=Point(i);p.y=ground.Sample(p.x,p.z).height;result.push_back(p);}
        std::reverse(result.begin(),result.end());return result;
    }
private:
    const IslandTerrain& terrain;
    const RenderedTerrainSurface& ground;
    const WorldCollisionGrid& solids;
    std::vector<std::uint8_t> valid,knownEdges,safeEdges;
};
}
ResidentRoute::ResidentRoute(const IslandTerrain& terrain,const RenderedTerrainSurface& ground,const WorldCollisionGrid& solids) {
    Search search(terrain,ground,solids);
    const std::array authored{ResidentPoint{CampfireSite::X-5,0,CampfireSite::Z+5},
        ResidentPoint{VegetationLayout::PrimaryPathCenterX(135),0,135},
        ResidentPoint{VegetationLayout::WestClearing.x,0,VegetationLayout::WestClearing.z},
        ResidentPoint{VegetationLayout::PrimaryPathCenterX(-165),0,-165},
        ResidentPoint{VegetationLayout::PrimaryPathCenterX(-100),0,-100}};
    std::array<int,authored.size()> goals{};
    for(std::size_t i=0;i<authored.size();++i)goals[i]=search.Nearest(authored[i]);
    points_.push_back(search.Point(goals[0]));points_.back().y=ground.Sample(points_[0].x,points_[0].z).height;
    pauses_.push_back(0);
    for(std::size_t leg=0;leg<goals.size();++leg) {
        const auto path=search.Path(goals[leg],goals[(leg+1)%goals.size()]);
        for(std::size_t first=0;first+1<path.size();) {
            std::size_t last=std::min(path.size()-1,first+12);
            while(last>first+1&&!SafeSegment(terrain,ground,solids,path[first],path[last]))--last;
            points_.push_back(path[last]);first=last;
        }
        if(leg+1<goals.size())pauses_.push_back(points_.size()-1);
    }
    if(points_.size()<6||points_.size()>512||points_.front()!=points_.back())
        throw std::runtime_error("Resident tour is not a bounded closed route");
    for(std::size_t i=1;i<points_.size();++i)length_+=std::hypot(points_[i].x-points_[i-1].x,points_[i].z-points_[i-1].z);
}
}
