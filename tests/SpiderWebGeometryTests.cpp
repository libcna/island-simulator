#include "IslandSimulator/SpiderWebGeometry.hpp"
#include "IslandSimulator/SpiderWebLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Verify(const island::SpiderWebGeometry& web,const std::array<island::BranchPoint,3>& anchors) {
    const auto points=web.Sample(anchors);
    Check(points.size()<=web.MaximumNodes&&web.Strands().size()<=web.MaximumStrands,"web exceeded budget");
    for(std::size_t i=0;i<3;++i)Check(points[i]==anchors[i],"frame anchor detached");
    std::vector<std::vector<std::size_t>> neighbours(points.size());
    std::set<std::pair<std::size_t,std::size_t>> edges;
    std::size_t spiral=0,radial=0;
    for(const auto& e:web.Strands()) {
        Check(e.from<points.size()&&e.to<points.size()&&e.from!=e.to,"invalid strand indices");
        Check(edges.emplace(std::min(e.from,e.to),std::max(e.from,e.to)).second,"duplicate strand");
        neighbours[e.from].push_back(e.to);neighbours[e.to].push_back(e.from);
        const auto a=points[e.from],b=points[e.to];
        Check(std::hypot(a[0]-b[0],a[1]-b[1],a[2]-b[2])>1e-6f,"zero-length strand");
        spiral+=e.kind==island::WebStrandKind::Spiral;radial+=e.kind==island::WebStrandKind::Radial;
    }
    Check(spiral==web.Spokes*web.Turns-1&&radial==web.Spokes*(web.Turns+1),"incomplete spiral/radials");
    std::vector<bool> seen(points.size());std::vector<std::size_t> pending{0};seen[0]=true;
    for(std::size_t n=0;n<pending.size();++n)for(auto next:neighbours[pending[n]])if(!seen[next]){seen[next]=true;pending.push_back(next);}
    Check(pending.size()==points.size(),"disconnected web component");
    for(std::size_t i=0;i<points.size();++i) {
        const auto w=web.Weights()[i];
        Check(std::abs(w[0]+w[1]+w[2]-1)<1e-5f,"unnormalized web node");
        for(float v:w)Check(std::isfinite(v)&&v>=0&&v<=1,"node outside supporting frame");
        for(std::size_t k=0;k<3;++k) {
            const double expected=double(w[0])*anchors[0][k]+double(w[1])*anchors[1][k]+double(w[2])*anchors[2][k];
            Check(std::isfinite(points[i][k])&&std::abs(double(points[i][k])-expected)<.00004,"web deformation mismatch");
        }
    }
}
}
int main() {
    using namespace island;
    const IslandTerrain terrain;const RenderedTerrainSurface surface(terrain);const JungleLayout jungle(terrain);
    const auto sites=SpiderWebLayout::Propose(jungle,surface);
    Check(!sites.empty(),"no web sites tested");
    for(const auto& site:sites) {
        const SpiderWebGeometry web(SpiderWebLayout::Sample(site,jungle,surface,0));
        const auto rest=web.Sample(SpiderWebLayout::Sample(site,jungle,surface,0));
        const auto first=rest.size()-web.Spokes*web.Turns;
        float startRadius=0;
        for(std::size_t n=0;n<web.Spokes*web.Turns;++n) {
            const auto p=rest[first+n],centre=rest[3];
            const float radius=std::hypot(p[0]-centre[0],p[1]-centre[1],p[2]-centre[2]);
            if(n==0)startRadius=radius;
            const float expected=startRadius*(.12f+.76f*float(n)/float(web.Spokes*web.Turns-1))/.12f;
            Check(std::abs(radius-expected)<.0001f,"spiral is not circular with increasing radius at rest");
        }
        for(int sample=0;sample<40;++sample)Verify(web,SpiderWebLayout::Sample(site,jungle,surface,float(sample)*.79f));
        std::cout<<"tree "<<site.canopyIndex<<": "<<web.Weights().size()<<" nodes, "<<web.Strands().size()<<" strands\n";
    }
    for(int fault=0;fault<3;++fault) {
        std::array<BranchPoint,3> anchors{{{0,0,0},{1,0,0},{0,1,0}}};
        if(fault==0)anchors[2]=anchors[1];
        if(fault==1)anchors[1][0]=NAN;
        if(fault==2)anchors[2]={2,0,0};
        bool rejected=false;try{const SpiderWebGeometry bad(anchors);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid frame accepted");
        const SpiderWebGeometry valid({{{0,0,0},{1,0,0},{0,1,0}}});
        rejected=false;try{(void)valid.Sample(anchors);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid moving frame accepted");
    }
    // Different orientations/aspect ratios must not rely on default tree shapes.
    for(int i=0;i<24;++i) {
        const float skew=float(i)*.07f;
        const std::array<BranchPoint,3> anchors{{{0,0,0},{.3f+skew,.1f,0},{-.2f,.2f+skew,.7f}}};
        const SpiderWebGeometry web(anchors);Verify(web,anchors);
    }
}
