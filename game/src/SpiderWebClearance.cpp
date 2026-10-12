#include "IslandSimulator/SpiderWebClearance.hpp"
#include "IslandSimulator/SpiderWebGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island {
namespace {
using DPoint=std::array<double,3>;
DPoint Sub(BranchPoint a,BranchPoint b){return {double(a[0])-b[0],double(a[1])-b[1],double(a[2])-b[2]};}
double Dot(DPoint a,DPoint b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
std::array<BranchPoint,3> Anchors(const SpiderWebSite& site,int variant,float time) {
    std::array<BranchPoint,3> result{};
    for(std::size_t i=0;i<3;++i) {
        const auto& a=site.anchors[i];
        result[i]=SampleCanopyBranchAnchor(MakeCanopyBranchProfile(variant,a.branch),a.triangle,a.u,a.v,time);
    }
    return result;
}
}
bool WebSegmentHitsTriangle(BranchPoint from,BranchPoint to,const std::array<BranchPoint,3>& triangle) {
    for(auto p:{from,to,triangle[0],triangle[1],triangle[2]})for(float x:p)
        if(!std::isfinite(x))throw std::invalid_argument("Nonfinite web clearance coordinate");
    const auto a=Sub(triangle[1],triangle[0]),b=Sub(triangle[2],triangle[0]);
    const DPoint normal{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    const double aa=Dot(a,a),ab=Dot(a,b),bb=Dot(b,b),det=Dot(normal,normal);
    if(det<1e-20)throw std::invalid_argument("Degenerate web clearance triangle");
    const double d0=Dot(Sub(from,triangle[0]),normal),d1=Dot(Sub(to,triangle[0]),normal);
    const double epsilon=1e-8*std::sqrt(det);
    if((d0>epsilon&&d1>epsilon)||(d0<-epsilon&&d1<-epsilon))return false;
    const auto bary=[&](BranchPoint p) {
        const auto delta=Sub(p,triangle[0]);
        const double u=(bb*Dot(delta,a)-ab*Dot(delta,b))/det;
        const double v=(aa*Dot(delta,b)-ab*Dot(delta,a))/det;
        return DPoint{1-u-v,u,v};
    };
    const auto w0=bary(from),w1=bary(to);
    if(std::abs(d0)<=epsilon&&std::abs(d1)<=epsilon) {
        double lower=0,upper=1;
        for(std::size_t k=0;k<3;++k) {
            const double slope=w1[k]-w0[k];
            if(std::abs(slope)<1e-12) {if(w0[k]<-1e-9)return false;}
            else if(slope>0)lower=std::max(lower,(-1e-9-w0[k])/slope);
            else upper=std::min(upper,(-1e-9-w0[k])/slope);
        }
        return lower<=upper;
    }
    const double t=d0/(d0-d1);
    if(t<0||t>1)return false;
    for(std::size_t k=0;k<3;++k)if(w0[k]+t*(w1[k]-w0[k])<-1e-9)return false;
    return true;
}
bool WebIntersectsCanopyBranches(const SpiderWebSite& site,int variant,float time) {
    const SpiderWebGeometry web(Anchors(site,variant,0));
    const auto nodes=web.Sample(Anchors(site,variant,time));
    for(int branch=1;branch<=7;++branch) {
        const auto profile=MakeCanopyBranchProfile(variant,branch);
        std::array<BranchPoint,18> vertices{};
        for(std::size_t i=0;i<vertices.size();++i)vertices[i]=CanopyBranchPosition(profile.vertices[i],time);
        for(const auto& strand:web.Strands()) {
            auto from=nodes[strand.from],to=nodes[strand.to];
            const auto delta=Sub(to,from);const double length=std::sqrt(Dot(delta,delta));
            const double trim=std::min(.0001/length,.1);
            if(strand.from<3&&site.anchors[strand.from].branch==branch)
                for(std::size_t k=0;k<3;++k)from[k]+=static_cast<float>(delta[k]*trim);
            if(strand.to<3&&site.anchors[strand.to].branch==branch)
                for(std::size_t k=0;k<3;++k)to[k]-=static_cast<float>(delta[k]*trim);
            for(const auto ids:profile.triangles) {
                const std::array<BranchPoint,3> tri{vertices[ids[0]],vertices[ids[1]],vertices[ids[2]]};
                bool separated=false;
                for(std::size_t k=0;k<3;++k)
                    separated=separated||std::max(from[k],to[k])<std::min({tri[0][k],tri[1][k],tri[2][k]})
                        ||std::min(from[k],to[k])>std::max({tri[0][k],tri[1][k],tri[2][k]});
                if(!separated&&WebSegmentHitsTriangle(from,to,tri))return true;
            }
        }
    }
    return false;
}
}
