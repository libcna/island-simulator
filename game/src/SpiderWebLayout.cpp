#include "IslandSimulator/SpiderWebLayout.hpp"
#include "IslandSimulator/JungleGrounding.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island {
namespace {
BranchPoint Sub(BranchPoint a,BranchPoint b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
std::uint32_t Rank(std::size_t index) {
    auto value=static_cast<std::uint32_t>(index)^0x77656273U;
    value^=value>>16;value*=0x7feb352dU;value^=value>>15;value*=0x846ca68bU;
    return value^(value>>16);
}
float Length(BranchPoint p){return std::hypot(p[0],p[1],p[2]);}
BranchPoint Centre(const std::array<BranchPoint,3>& p) {
    return {(p[0][0]+p[1][0]+p[2][0])/3,(p[0][1]+p[1][1]+p[2][1])/3,(p[0][2]+p[1][2]+p[2][2])/3};
}
bool ClearFrame(const std::array<BranchPoint,3>& p,int variant,float scale) {
    const auto a=Sub(p[1],p[0]),b=Sub(p[2],p[0]);
    const BranchPoint cross{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    const float area=.5f*Length(cross)*scale*scale;
    if(area<.08f||area>1.5f)return false;
    for(std::size_t i=0;i<3;++i) {
        const float edge=Length(Sub(p[i],p[(i+1)%3]))*scale;
        if(edge<.3f||edge>2.5f)return false;
    }
    // Interior sample grid only: conservative trunk/core exclusions, not a leaf
    // triangle intersection test or continuous wind/whole-frame proof.
    const auto trunk=MakeCanopyTrunkProfile(variant);
    for(int i=1;i<10;++i)for(int j=1;j<10-i;++j) {
        const float u=static_cast<float>(i)/10,v=static_cast<float>(j)/10;
        BranchPoint q{};
        for(std::size_t k=0;k<3;++k)q[k]=p[0][k]*(1-u-v)+p[1][k]*u+p[2][k]*v;
        const float y=std::clamp(q[1],0.0f,trunk.height);
        const auto axis=SampleCanopyTrunk(variant,y/trunk.height);
        if(q[1]<=trunk.height+.2f&&std::hypot(q[0]-axis[0],q[2]-axis[2])
            <CanopyTrunkRadius(variant,y)*1.11f+.18f)return false;
        for(int cluster=0;cluster<8;++cluster) {
            const auto crown=MakeCanopyCrownProfile(variant,cluster);
            const float x=(q[0]-crown.x)/(crown.width*.66f+.16f);
            const float cy=(q[1]-crown.y)/(crown.height*.60f+.02f);
            const float z=(q[2]-crown.z)/(crown.width*.91f*.66f+.16f);
            if(x*x+cy*cy+z*z<1)return false;
        }
    }
    return true;
}
std::array<BranchPoint,3> Local(const SpiderWebSite& site,int variant,float time) {
    std::array<BranchPoint,3> points{};
    for(std::size_t i=0;i<3;++i) {
        const auto& a=site.anchors[i];
        points[i]=SampleCanopyBranchAnchor(MakeCanopyBranchProfile(variant,a.branch),a.triangle,a.u,a.v,time);
    }
    return points;
}
}
std::array<BranchPoint,3> SpiderWebLayout::Sample(const SpiderWebSite& site,
    const JungleLayout& jungle,const RenderedTerrainSurface& surface,float time) {
    if(site.canopyIndex>=jungle.Canopy().size()||!std::isfinite(time))
        throw std::invalid_argument("Invalid spider web site/time");
    const auto tree=GroundJunglePlacement(jungle.Canopy()[site.canopyIndex],surface);
    auto points=Local(site,tree.variant,time);
    const float c=std::cos(tree.yawRadians),s=std::sin(tree.yawRadians);
    for(auto& p:points)p={tree.x+tree.scale*(p[0]*c+p[2]*s),tree.y+tree.scale*p[1],
                         tree.z+tree.scale*(-p[0]*s+p[2]*c)};
    return points;
}
std::vector<SpiderWebSite> SpiderWebLayout::Propose(const JungleLayout& jungle,const RenderedTerrainSurface& surface) {
    std::vector<SpiderWebSite> sites;
    std::vector<BranchPoint> centres;
    std::vector<std::size_t> treeOrder;
    for(std::size_t i=0;i<jungle.Canopy().size();++i)treeOrder.push_back(i);
    std::stable_sort(treeOrder.begin(),treeOrder.end(),[](auto a,auto b){return Rank(a)<Rank(b);});
    for(const std::size_t treeIndex:treeOrder) {
        const auto& tree=jungle.Canopy()[treeIndex];
        bool selected=false;
        for(int a=1;a<=5&&!selected;++a)for(int b=a+1;b<=6&&!selected;++b)for(int c=b+1;c<=7&&!selected;++c) {
            SpiderWebSite site{treeIndex,{}};
            const std::array<int,3> branches{a,b,c};
            std::array<BranchPoint,3> midpoints{};
            for(std::size_t i=0;i<3;++i) {
                const auto branch=MakeCanopyBranchProfile(tree.variant,branches[i]);
                for(std::size_t k=0;k<3;++k)midpoints[i][k]=(branch.vertices[0].position[k]+branch.vertices[3].position[k])*.175f
                    +(branch.vertices[6].position[k]+branch.vertices[9].position[k])*.325f;
            }
            const auto centre=Centre(midpoints);
            for(std::size_t i=0;i<3;++i) {
                const auto branch=MakeCanopyBranchProfile(tree.variant,branches[i]);
                float best=1e9f;
                for(std::size_t side=0;side<6;++side) {
                    const float distance=Length(Sub(SampleCanopyBranchAnchor(branch,side*2,.25f,.65f,0),centre));
                    if(distance<best) {best=distance;site.anchors[i]={branches[i],side*2,.25f,.65f};}
                }
            }
            bool valid=true;
            for(float time:{0.0f,4.0f,13.0f})valid=valid&&ClearFrame(Local(site,tree.variant,time),tree.variant,tree.scale);
            if(!valid)continue;
            const auto world=Sample(site,jungle,surface,0);
            const auto worldCentre=Centre(world);
            if(worldCentre[1]-surface.Sample(worldCentre[0],worldCentre[2]).height<2)continue;
            for(const auto old:centres)valid=valid&&std::hypot(old[0]-worldCentre[0],old[2]-worldCentre[2])>=MinimumSeparation;
            if(!valid)continue;
            sites.push_back(site);centres.push_back(worldCentre);selected=true;
            if(sites.size()==MaximumSites)return sites;
        }
    }
    return sites; // No forced count, fallback attachment, or removed vegetation.
}
}
