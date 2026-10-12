#include "IslandSimulator/SpiderWebGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
BranchPoint Sub(BranchPoint a,BranchPoint b) { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
float Dot(BranchPoint a,BranchPoint b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
void Validate(const std::array<BranchPoint,3>& anchors) {
    for(const auto& p:anchors)for(float v:p)
        if(!std::isfinite(v))throw std::invalid_argument("Nonfinite web anchor");
    const auto a=Sub(anchors[1],anchors[0]),b=Sub(anchors[2],anchors[0]);
    const float determinant=Dot(a,a)*Dot(b,b)-Dot(a,b)*Dot(a,b);
    if(!std::isfinite(determinant)||determinant<1e-8f)
        throw std::invalid_argument("Degenerate web frame");
}
}
SpiderWebGeometry::SpiderWebGeometry(const std::array<BranchPoint,3>& anchors) {
    Validate(anchors);
    weights_.reserve(MaximumNodes);strands_.reserve(MaximumStrands);
    weights_={{1,0,0},{0,1,0},{0,0,1},{1.0f/3,1.0f/3,1.0f/3}};
    const auto a=Sub(anchors[1],anchors[0]),b=Sub(anchors[2],anchors[0]);
    const float aa=Dot(a,a),ab=Dot(a,b),bb=Dot(b,b),det=aa*bb-ab*ab;
    BranchPoint x{},y{};
    for(std::size_t k=0;k<3;++k){x[k]=a[k]/std::sqrt(aa);y[k]=b[k]-a[k]*ab/aa;}
    const float yl=std::sqrt(Dot(y,y));for(float& v:y)v/=yl;
    std::array<BranchPoint,Spokes> directions{};
    std::array<std::size_t,Spokes> boundary{};
    float radius=std::numeric_limits<float>::max();
    for(std::size_t spoke=0;spoke<Spokes;++spoke) {
        const float angle=.17f+2*std::numbers::pi_v<float>*static_cast<float>(spoke)/static_cast<float>(Spokes);
        BranchPoint ray{};for(std::size_t k=0;k<3;++k)ray[k]=x[k]*std::cos(angle)+y[k]*std::sin(angle);
        const float u=(bb*Dot(ray,a)-ab*Dot(ray,b))/det;
        const float v=(aa*Dot(ray,b)-ab*Dot(ray,a))/det;
        directions[spoke]={-u-v,u,v};
        float reach=std::numeric_limits<float>::max();
        for(float d:directions[spoke])if(d<0)reach=std::min(reach,-1.0f/(3*d));
        radius=std::min(radius,reach);
        BranchPoint w{};for(std::size_t k=0;k<3;++k)w[k]=std::max(0.0f,1.0f/3+reach*directions[spoke][k]);
        const float sum=w[0]+w[1]+w[2];for(float& value:w)value/=sum;
        // Reuse exact frame vertices for rays that happen to hit a corner.
        boundary[spoke]=weights_.size();
        for(std::size_t k=0;k<3;++k)if(w[k]>1-1e-5f)boundary[spoke]=k;
        if(boundary[spoke]==weights_.size())weights_.push_back(w);
    }
    // Subdivide each frame edge at the radial attachment nodes: no visually
    // coincident but topologically disconnected filament endpoints.
    for(std::size_t edge=0;edge<3;++edge) {
        std::vector<std::size_t> nodes;
        const auto next=(edge+1)%3,opposite=(edge+2)%3;
        for(std::size_t i=0;i<weights_.size();++i)if(weights_[i][opposite]<1e-6f)nodes.push_back(i);
        std::sort(nodes.begin(),nodes.end(),[&](auto l,auto r){return weights_[l][next]<weights_[r][next];});
        for(std::size_t i=1;i<nodes.size();++i)strands_.push_back({nodes[i-1],nodes[i],WebStrandKind::Frame});
    }
    auto previous=boundary;previous.fill(3);
    std::size_t last=0;
    for(std::size_t n=0;n<Spokes*Turns;++n) {
        const auto spoke=n%Spokes;
        const float distance=radius*(.12f+.76f*static_cast<float>(n)/static_cast<float>(Spokes*Turns-1));
        BranchPoint w{};for(std::size_t k=0;k<3;++k)w[k]=1.0f/3+distance*directions[spoke][k];
        const auto node=weights_.size();weights_.push_back(w);
        strands_.push_back({previous[spoke],node,WebStrandKind::Radial});
        if(n)strands_.push_back({last,node,WebStrandKind::Spiral});
        previous[spoke]=node;last=node;
    }
    for(std::size_t i=0;i<Spokes;++i)strands_.push_back({previous[i],boundary[i],WebStrandKind::Radial});
}
std::vector<BranchPoint> SpiderWebGeometry::Sample(const std::array<BranchPoint,3>& anchors) const {
    Validate(anchors);
    std::vector<BranchPoint> points;points.reserve(weights_.size());
    for(const auto w:weights_) {
        BranchPoint p{};
        for(std::size_t k=0;k<3;++k)for(std::size_t i=0;i<3;++i)p[k]+=w[i]*anchors[i][k];
        points.push_back(p);
    }
    return points;
}
}
