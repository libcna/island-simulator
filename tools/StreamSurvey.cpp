#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/StreamCourse.hpp"
#include "IslandSimulator/StreamBedPatch.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>

// Read-only survey of the authored west drainage corridor. No world mutation.
int main() {
    const island::IslandTerrain terrain;
    const auto surface=std::make_unique<island::RenderedTerrainSurface>(terrain);
    const island::StreamCourse course(terrain),repeat(terrain);
    const island::StreamBedPatch patch(course,*surface);
    float deepestCut=0;
    for(std::size_t i=0;i<patch.Vertices().size();++i) {
        const auto p=patch.Vertices()[i];
        const float base=surface->Sample(p.x,p.z).height;
        if(!std::isfinite(p.y)||p.y>base+.00001f)return 4;
        if(std::abs(patch.SampleHeight(p.x,p.z)-p.y)>.0001f)return 9;
        deepestCut=std::max(deepestCut,base-p.y);
        const auto col=i%static_cast<std::size_t>(patch.Columns()),row=i/static_cast<std::size_t>(patch.Columns());
        if((col==0||row==0||col==static_cast<std::size_t>(patch.Columns()-1)
            ||row==static_cast<std::size_t>(patch.Rows()-1))&&p.y!=base)return 5;
    }
    for(std::size_t i=0;i<patch.Indices().size();i+=3) {
        const auto a=patch.Vertices().at(patch.Indices()[i]);
        const auto b=patch.Vertices().at(patch.Indices()[i+1]);
        const auto c=patch.Vertices().at(patch.Indices()[i+2]);
        if((b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z)<=0)return 6;
        const float x=(a.x+b.x+c.x)/3,z=(a.z+b.z+c.z)/3;
        if(std::abs(patch.SampleHeight(x,z)-(a.y+b.y+c.y)/3)>.0001f)return 7;
    }
    if(deepestCut<.25f||deepestCut>1)return 8;
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
    for(std::size_t i=0;i<course.Count();++i) {
        const auto& p=course.Station(i);
        const auto& q=repeat.Station(i);
        if(p.x!=q.x||p.z!=q.z||p.water!=q.water||p.halfWidth!=q.halfWidth
            ||!std::isfinite(p.water)||p.bed>=p.water||p.halfWidth<.45f||p.halfWidth>1.5f
            ||(i>0&&(p.water>course.Station(i-1).water||p.distance<=course.Station(i-1).distance)))
            return 2;
    }
    if(course.Station(course.Count()-1).water!=0)return 3;
    std::cerr << "Sea-connected layout stations=" << course.Count() << " length="
        << course.Station(course.Count()-1).distance << " source water=" << course.Station(0).water
        << "; width0.9..3m; geometry and terrain carving not implemented\n";
    std::cout << "x,z,inland,analytic_height,rendered_height,water_candidate,bed_cut\n";
    std::cout << std::fixed << std::setprecision(5);
    float previousWater=100.0f,maximumCut=0.0f,maximumRise=0.0f,previousGround=100.0f;
    bool reachedSea=false;
    for(int step=0;step<=160;++step) {
        const float x=-65.0f-static_cast<float>(step);
        const float z=-55.0f-.23f*x;
        const float inland=terrain.SignedShoreDistance(x,z);
        const float ground=terrain.Height(x,z),rendered=surface->Sample(x,z).height;
        // 15cm freeboard plus 25cm water; minimum 0.2% downstream fall.
        const float water=std::min(ground-.15f,previousWater-.002f*std::hypot(1.0f,.23f));
        const float cut=ground-(water-.25f);
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
