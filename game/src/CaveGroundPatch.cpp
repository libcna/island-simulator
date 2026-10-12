#include "IslandSimulator/CaveGroundPatch.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island {
CaveGroundPatch::CaveGroundPatch(const CaveLayout& cave,const RenderedTerrainSurface& original) {
    using L=TerrainMeshLayout;
    float minX=INFINITY,minZ=INFINITY,maxX=-INFINITY,maxZ=-INFINITY;
    // One metre of unchanged terrain around the complete influence rectangle joins
    // the original mesh at its actual rounded grid coordinates, including corners.
    for(float across:{-7.0f,7.0f}) for(float depth:{-7.0f,CaveLayout::OuterLength+4.0f}) {
        const auto p=cave.World(across,0,depth);
        minX=std::min(minX,p.x);maxX=std::max(maxX,p.x);
        minZ=std::min(minZ,p.z);maxZ=std::max(maxZ,p.z);
    }
    const auto coordinates=[](float low,float high) {
        const auto grid=[](float p){return (double(p)+L::Extent)*L::QuadsPerAxis/(2*L::Extent);};
        const int first=static_cast<int>(std::floor(grid(low)));
        const int last=static_cast<int>(std::ceil(grid(high)));
        if(first<0||last>L::QuadsPerAxis) throw std::runtime_error("Cave patch exceeds island mesh");
        std::vector<float> result;
        for(int cell=first;cell<last;++cell) for(int sub=0;sub<Subdivision;++sub)
            result.push_back(std::lerp(L::Coordinate(cell),L::Coordinate(cell+1),
                                      static_cast<float>(sub)/Subdivision));
        result.push_back(L::Coordinate(last));
        return result;
    };
    xs_=coordinates(minX,maxX);zs_=coordinates(minZ,maxZ);
    if(xs_.size()*zs_.size()>=65536) throw std::runtime_error("Cave patch exceeds portable indices");
    for(float z:zs_) for(float x:xs_)
        vertices_.push_back({x,cave.GroundTarget(x,z,original.Sample(x,z).height),z});
    for(int row=0;row<Rows()-1;++row) for(int col=0;col<Columns()-1;++col) {
        const auto tl=static_cast<std::uint16_t>(row*Columns()+col);
        const auto tr=static_cast<std::uint16_t>(tl+1);
        const auto bl=static_cast<std::uint16_t>(tl+Columns());
        const auto br=static_cast<std::uint16_t>(bl+1);
        indices_.insert(indices_.end(),{tl,bl,tr,tr,bl,br});
    }
}
bool CaveGroundPatch::Contains(float x,float z) const noexcept {
    return std::isfinite(x)&&std::isfinite(z)
        &&x>=xs_.front()&&x<=xs_.back()&&z>=zs_.front()&&z<=zs_.back();
}
float CaveGroundPatch::SampleHeight(float x,float z) const {
    if(!Contains(x,z)) throw std::invalid_argument("Cave ground query outside replacement");
    const auto cell=[](const auto& coordinates,float p) {
        return std::clamp(static_cast<int>(std::upper_bound(coordinates.begin(),coordinates.end(),p)
            -coordinates.begin())-1,0,static_cast<int>(coordinates.size())-2);
    };
    const int col=cell(xs_,x),row=cell(zs_,z),index=row*Columns()+col;
    const double u=(double(x)-xs_[col])/(double(xs_[col+1])-xs_[col]);
    const double v=(double(z)-zs_[row])/(double(zs_[row+1])-zs_[row]);
    const double tl=vertices_[index].y,tr=vertices_[index+1].y;
    const double bl=vertices_[index+Columns()].y,br=vertices_[index+Columns()+1].y;
    return static_cast<float>(u+v<=1?tl+(tr-tl)*u+(bl-tl)*v
        :br+(bl-br)*(1-u)+(tr-br)*(1-v));
}
std::vector<float> CaveGroundPatch::TraceParameters(float x0,float z0,float x1,float z1) const {
    std::vector<float> result;
    if(!std::isfinite(x0)||!std::isfinite(z0)||!std::isfinite(x1)||!std::isfinite(z1)) return result;
    if(std::max(x0,x1)<xs_.front()||std::min(x0,x1)>xs_.back()
        ||std::max(z0,z1)<zs_.front()||std::min(z0,z1)>zs_.back()) return result;
    const double dx=double(x1)-x0,dz=double(z1)-z0;
    const auto append=[&](double t){if(t>0&&t<1)result.push_back(static_cast<float>(t));};
    if(dx!=0)for(float x:xs_)append((x-x0)/dx);
    if(dz!=0)for(float z:zs_)append((z-z0)/dz);
    for(int row=0;row<Rows()-1;++row) {
        if(zs_[row]>std::max(z0,z1)||zs_[row+1]<std::min(z0,z1))continue;
        for(int col=0;col<Columns()-1;++col) {
            if(xs_[col]>std::max(x0,x1)||xs_[col+1]<std::min(x0,x1))continue;
            const double w=double(xs_[col+1])-xs_[col],h=double(zs_[row+1])-zs_[row];
            const double rate=dx/w+dz/h;
            if(rate==0)continue;
            const double t=(1-(double(x0)-xs_[col])/w-(double(z0)-zs_[row])/h)/rate;
            const double u=(x0+t*dx-xs_[col])/w,v=(z0+t*dz-zs_[row])/h;
            if(u>=-1e-8&&u<=1+1e-8&&v>=-1e-8&&v<=1+1e-8)append(t);
        }
    }
    std::sort(result.begin(),result.end());
    result.erase(std::unique(result.begin(),result.end()),result.end());
    return result;
}
}
