#include "IslandSimulator/SpiderWebLayout.hpp"
#include "IslandSimulator/SpiderWebClearance.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace { void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);} }
int main() {
    using namespace island;
    const std::array<BranchPoint,3> triangle{{{0,0,0},{1,0,0},{0,1,0}}};
    Check(WebSegmentHitsTriangle({.2f,.2f,-1},{.2f,.2f,1},triangle),"missed perpendicular crossing");
    Check(!WebSegmentHitsTriangle({.8f,.8f,-1},{.8f,.8f,1},triangle),"false triangle-plane crossing");
    Check(WebSegmentHitsTriangle({-.2f,.2f,0},{.8f,.2f,0},triangle),"missed coplanar crossing");
    Check(WebSegmentHitsTriangle({.2f,.2f,0},{.3f,.3f,0},triangle),"missed coplanar interior");
    Check(!WebSegmentHitsTriangle({.8f,.8f,0},{1,1,0},triangle),"false coplanar crossing");
    Check(!WebSegmentHitsTriangle({.2f,.2f,.01f},{.3f,.3f,.01f},triangle),"false parallel intersection");
    Check(WebSegmentHitsTriangle({-1,0,0},{0,0,0},triangle),"missed triangle vertex contact");
    const IslandTerrain terrain;const RenderedTerrainSurface surface(terrain);const JungleLayout jungle(terrain);
    const auto sites=SpiderWebLayout::Propose(jungle,surface);
    Check(!sites.empty()&&sites.size()<=SpiderWebLayout::MaximumSites,"no bounded web proposals");
    Check(sites==SpiderWebLayout::Propose(jungle,surface),"nondeterministic web proposals");
    std::vector<BranchPoint> centres;
    float motion=0;
    for(const auto& site:sites) {
        const auto tree=jungle.Canopy().at(site.canopyIndex);
        Check(site.anchors[0].branch!=site.anchors[1].branch&&site.anchors[1].branch!=site.anchors[2].branch
            &&site.anchors[0].branch!=site.anchors[2].branch,"web lacks distinct branch supports");
        const auto rest=SpiderWebLayout::Sample(site,jungle,surface,0);
        for(int i=0;i<20;++i)Check(!WebIntersectsCanopyBranches(site,tree.variant,float(i)*1.37f),"accepted branch intersection at independent wind sample");
        BranchPoint centre{};for(const auto p:rest)for(std::size_t k=0;k<3;++k)centre[k]+=p[k]/3;
        for(const auto old:centres)Check(std::hypot(centre[0]-old[0],centre[2]-old[2])>=SpiderWebLayout::MinimumSeparation,"webs overlap homes");
        centres.push_back(centre);
        Check(centre[1]>surface.Sample(centre[0],centre[2]).height+2,"web centre underground/too low");
        for(int sample=0;sample<20;++sample) {
            const float time=static_cast<float>(sample)*1.37f;
            const auto actual=SpiderWebLayout::Sample(site,jungle,surface,time);
            BranchPoint a{},b{};
            for(std::size_t k=0;k<3;++k){a[k]=actual[1][k]-actual[0][k];b[k]=actual[2][k]-actual[0][k];}
            const float area=.5f*std::hypot(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);
            Check(area>=.08f&&area<=1.5f,"web area outside bounded range in sampled wind");
            for(std::size_t i=0;i<3;++i) {
                for(float value:actual[i])Check(std::isfinite(value),"nonfinite web anchor");
                const auto& anchor=site.anchors[i];
                const auto branch=MakeCanopyBranchProfile(tree.variant,anchor.branch);
                const auto ids=branch.triangles.at(anchor.triangle);
                const std::array<float,3> weights{1-anchor.u-anchor.v,anchor.u,anchor.v};
                BranchPoint local{};
                for(std::size_t vertex=0;vertex<3;++vertex) {
                    const auto p=CanopyBranchPosition(branch.vertices.at(ids[vertex]),time);
                    for(std::size_t k=0;k<3;++k)local[k]+=p[k]*weights[vertex];
                }
                const float dx=(actual[i][0]-tree.x)/tree.scale,dz=(actual[i][2]-tree.z)/tree.scale;
                const BranchPoint inverse{dx*std::cos(tree.yawRadians)-dz*std::sin(tree.yawRadians),
                    (actual[i][1]-surface.Sample(tree.x,tree.z).height)/tree.scale,
                    dx*std::sin(tree.yawRadians)+dz*std::cos(tree.yawRadians)};
                for(std::size_t k=0;k<3;++k)Check(std::abs(local[k]-inverse[k])<.00003f,"anchor detached from rendered tree transform");
                const auto next=actual[(i+1)%3];
                const float span=std::hypot(actual[i][0]-next[0],actual[i][1]-next[1],actual[i][2]-next[2]);
                Check(span>=.3f&&span<=2.5f,"web span outside bounded range in sampled wind");
                motion+=std::hypot(actual[i][0]-rest[i][0],actual[i][1]-rest[i][1],actual[i][2]-rest[i][2]);
            }
        }
        std::cout<<"web proposal tree "<<site.canopyIndex<<" centre "<<centre[0]<<','<<centre[1]<<','<<centre[2]<<'\n';
    }
    Check(motion>.01f,"anchors ignore branch wind");
    for(int fault=0;fault<4;++fault) {
        auto bad=sites.front();
        if(fault==0)bad.canopyIndex=jungle.Canopy().size();
        if(fault==1)bad.anchors[0].triangle=24;
        if(fault==2)bad.anchors[0].u=-1;
        bool rejected=false;
        try{(void)SpiderWebLayout::Sample(bad,jungle,surface,fault==3?NAN:0.0f);}
        catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid web attachment accepted");
    }
    std::cout<<sites.size()<<" proposed canopy webs; not live or foliage-qualified\n";
}
