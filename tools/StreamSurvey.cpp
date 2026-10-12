#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/StreamCourse.hpp"
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
