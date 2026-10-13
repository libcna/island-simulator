#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/StreamCourse.hpp"
#include "IslandSimulator/StreamBedPatch.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/StreamWaterMesh.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <map>

// Read-only survey of the authored west drainage corridor. No world mutation.
int main() {
    const island::IslandTerrain terrain;
    const auto surface=std::make_unique<island::RenderedTerrainSurface>(terrain);
    const island::StreamCourse course(terrain),repeat(terrain);
    const island::StreamBedPatch patch(course,*surface);
    const island::StreamWaterMesh water(course,patch);
    if(water.vertices.empty()||water.vertices.size()>=65536||water.indices.empty()
        ||water.indices.size()%3!=0)return 18;
    float minimumVisibleWaterDepth=std::numeric_limits<float>::max();
    for(const auto p:water.vertices) {
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)
            ||!std::isfinite(p.flowX)||!std::isfinite(p.flowZ)||!std::isfinite(p.distance)
            ||!std::isfinite(p.alpha)||p.alpha<0||p.alpha>.65f
            ||!std::isfinite(p.lateral)||p.lateral<-1||p.lateral>1
            ||std::abs(std::hypot(p.flowX,p.flowZ)-1.0f)>.0001f
            ||p.flowX>-.8f||p.distance<0||p.distance>course.Station(course.Count()-1).distance
            ||!patch.Contains(p.x,p.z)||p.y<0)return 19;
        if(p.alpha>.2f&&p.distance>6.0f
            &&p.distance<course.Station(course.Count()-1).distance-6.0f)
            minimumVisibleWaterDepth=std::min(minimumVisibleWaterDepth,
                p.y-patch.SampleHeight(p.x,p.z));
    }
    std::cerr<<"Stream water vertices="<<water.vertices.size()<<" triangles="<<water.indices.size()/3
        <<" minimum visible depth="<<minimumVisibleWaterDepth<<'\n';
    if(minimumVisibleWaterDepth<.13f||island::StreamCourse::BankFreeboard<.34f
        ||island::StreamBedPatch::BankBlendWidth>1.1f)return 22;
    constexpr int waterAcrossCount=9;
    for(std::size_t row=1;row<water.vertices.size()/waterAcrossCount;++row)
        if(water.vertices[row*waterAcrossCount].distance
            <=water.vertices[(row-1)*waterAcrossCount].distance)return 21;
    for(std::size_t i=0;i<water.indices.size();i+=3) {
        const auto a=water.vertices.at(water.indices[i]),b=water.vertices.at(water.indices[i+1]),c=water.vertices.at(water.indices[i+2]);
        if((b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z)<=0)return 16;
        for(float u:{.1f,.4f,.8f})for(float v:{.05f,.1f}) {
            const float x=a.x*(1-u-v)+b.x*u+c.x*v,z=a.z*(1-u-v)+b.z*u+c.z*v;
            const float y=a.y*(1-u-v)+b.y*u+c.y*v;
            if(patch.SampleHeight(x,z)>y-.005f)return 17;
        }
    }
    float deepestCut=0;
    for(std::size_t i=0;i<patch.Vertices().size();++i) {
        const auto p=patch.Vertices()[i];
        const float base=surface->Sample(p.x,p.z).height;
        if(!std::isfinite(p.y)||p.y>base+.00001f)return 4;
        if(std::abs(patch.SampleHeight(p.x,p.z)-p.y)>.0001f)return 9;
        deepestCut=std::max(deepestCut,base-p.y);
        if(patch.IsBoundary(p)&&p.y!=base)return 5;
    }
    std::map<std::pair<std::uint16_t,std::uint16_t>,std::pair<int,int>> edges;
    for(std::size_t i=0;i<patch.Indices().size();i+=3) {
        for(std::size_t k=0;k<3;++k) {
            const auto start=patch.Indices()[i+k],end=patch.Indices()[i+(k+1)%3];
            auto& edge=edges[std::minmax(start,end)];
            ++edge.first;edge.second+=start<end?1:-1;
        }
        const auto a=patch.Vertices().at(patch.Indices()[i]);
        const auto b=patch.Vertices().at(patch.Indices()[i+1]);
        const auto c=patch.Vertices().at(patch.Indices()[i+2]);
        if((b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z)<=0)return 6;
        const float x=(a.x+b.x+c.x)/3,z=(a.z+b.z+c.z)/3;
        if(std::abs(patch.SampleHeight(x,z)-(a.y+b.y+c.y)/3)>.0001f)return 7;
    }
    for(const auto& [ends,counts]:edges) {
        if(counts.first==1) {
            if(!patch.IsBoundary(patch.Vertices()[ends.first])||!patch.IsBoundary(patch.Vertices()[ends.second]))return 12;
        } else if(counts.first!=2||counts.second!=0)return 13;
    }
    if(patch.Indices().size()/3>=71680/2||patch.Vertices().size()>=36273/2)return 14;
    if(deepestCut<.25f||deepestCut>1)return 8;
    island::PlayerMotion walker;walker.SetGroundPatch(&patch);
    for(std::size_t i=5;i+8<course.Count();++i) {
        const auto p=course.Station(i);
        walker.SetPosition(terrain,p.x,p.z);
        if(walker.Mode()!=island::LocomotionMode::Walking)continue;
        float support=-100;
        for(const auto offset:island::PlayerMotion::GroundContactOffsets)
            support=std::max(support,patch.SampleHeight(p.x+offset.x*island::PlayerMotion::CapsuleRadius,
                p.z+offset.z*island::PlayerMotion::CapsuleRadius));
        if(std::abs(walker.Position().y-support-island::PlayerMotion::EyeHeight)>.0001f)return 15;
    }
    for(std::size_t i=3;i<course.Count();++i) {
        const auto& p=course.Station(i);
        if(patch.SampleHeight(p.x,p.z)>p.water-.1f)return 10;
    }
    for(const auto point:std::array<std::array<float,2>,3>{{{{0,0}},{{-1000,0}},{{0,1000}}}}) {
        bool rejected=false;
        try {(void)patch.SampleHeight(point[0],point[1]);}
        catch(const std::out_of_range&) {rejected=true;}
        if(!rejected)return 11;
    }
    std::cerr << "Replacement patch vertices=" << patch.Vertices().size() << " triangles="
        << patch.Indices().size()/3 << " deepest cut=" << deepestCut << '\n';
    float minimumMeander=0.0f,maximumMeander=0.0f;
    float positiveWidthVariation=0.0f,negativeWidthVariation=0.0f;
    for(std::size_t i=0;i<course.Count();++i) {
        const auto& p=course.Station(i);
        const auto& q=repeat.Station(i);
        const float meander=p.z-(-55.0f-.23f*p.x);
        minimumMeander=std::min(minimumMeander,meander);
        maximumMeander=std::max(maximumMeander,meander);
        const float t=std::clamp(island::StreamRoute::Along(p.x)
            /island::StreamRoute::NominalLength,0.0f,1.0f);
        const float baseline=std::lerp(.45f,1.5f,t*t*(3.0f-2.0f*t));
        positiveWidthVariation=std::max(positiveWidthVariation,p.halfWidth-baseline);
        negativeWidthVariation=std::min(negativeWidthVariation,p.halfWidth-baseline);
        if(p.x!=q.x||p.z!=q.z||p.water!=q.water||p.halfWidth!=q.halfWidth
            ||!std::isfinite(p.water)||p.bed>=p.water||p.halfWidth<.45f||p.halfWidth>1.56f
            ||(i>0&&(p.water>course.Station(i-1).water||p.distance<=course.Station(i-1).distance)))
            return 2;
    }
    if(std::abs(course.Station(0).z-(-55.0f-.23f*course.Station(0).x))>.0001f
        ||std::abs(course.Station(course.Count()-1).z
            -(-55.0f-.23f*course.Station(course.Count()-1).x))>.0001f
        ||minimumMeander> -1.8f||maximumMeander<1.2f
        ||positiveWidthVariation<.16f||negativeWidthVariation>-.31f)return 20;
    if(course.Station(course.Count()-1).water!=0)return 3;
    std::cerr << "Sea-connected layout stations=" << course.Count() << " length="
        << course.Station(course.Count()-1).distance << " source water=" << course.Station(0).water
        << " width variation=" << negativeWidthVariation << ".." << positiveWidthVariation
        << "; width0.9..3m; meander " << minimumMeander << ".." << maximumMeander
        << "m; replacement bed and water mesh checked\n";
    std::cout << "x,z,inland,analytic_height,rendered_height,water_candidate,bed_cut\n";
    std::cout << std::fixed << std::setprecision(5);
    float previousWater=100.0f,maximumCut=0.0f,maximumRise=0.0f,previousGround=100.0f;
    bool reachedSea=false;
    for(int step=0;step<=160;++step) {
        const float x=island::StreamRoute::SourceX-static_cast<float>(step);
        const float z=island::StreamRoute::CentreZ(x);
        const float inland=terrain.SignedShoreDistance(x,z);
        const float ground=terrain.Height(x,z),rendered=surface->Sample(x,z).height;
        // 36cm freeboard plus 28cm water; minimum 0.2% downstream fall.
        const float stepLength=step==0?0.0f:std::hypot(1.0f,
            z-island::StreamRoute::CentreZ(x+1.0f));
        const float water=std::min(ground-island::StreamCourse::BankFreeboard,
            step==0?previousWater:previousWater-.002f*stepLength);
        const float cut=ground-(water-island::StreamCourse::BedDepth);
        maximumCut=std::max(maximumCut,cut);
        maximumRise=std::max(maximumRise,ground-previousGround);
        std::cout << x << ',' << z << ',' << inland << ',' << ground << ',' << rendered
            << ',' << water << ',' << cut << '\n';
        previousGround=ground;previousWater=water;
        if(inland<0) {reachedSea=true;break;}
    }
    std::cerr << "west corridor reached sea=" << reachedSea << " maximum local rise="
        << maximumRise << " maximum candidate incision=" << maximumCut
        << "; candidate is not yet a sea-level-connected water surface\n";
    return reachedSea?0:1;
}
