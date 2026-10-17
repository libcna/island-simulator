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
    for(float across:{-10.5f,10.5f}) for(float depth:{-7.0f,CaveLayout::OuterLength+4.0f}) {
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

    // The support height field above is necessarily the cave floor along the centre.
    // It cannot also be the soil above the roof at the same X/Z, so provide a small
    // render-only second surface for the existing terrain batch.  It follows the
    // actual top of the closed rock shell, then eases asymmetrically into the original
    // hillside.  Collision remains the authoritative shell plus support grid.
    constexpr int acrossSegments=CoverColumns-1,depthSegments=CoverRows-1;
    constexpr float acrossMin=-14.0f,acrossMax=14.0f;
    constexpr float depthMin=1.25f,depthMax=24.25f;
    const auto smooth=[](float value) {
        value=std::clamp(value,0.0f,1.0f);return value*value*(3.0f-2.0f*value);
    };
    const auto roofAt=[&](float across,float depth) {
        const auto query=cave.World(across,0,depth);
        float highest=-INFINITY;
        for(const auto& triangle:cave.Triangles()) {
            if(triangle.surface!=CaveSurface::Exterior)continue;
            const auto& a=triangle.points[0];const auto& b=triangle.points[1];
            const auto& c=triangle.points[2];
            const float bax=b.x-a.x,baz=b.z-a.z,cax=c.x-a.x,caz=c.z-a.z;
            const float qax=query.x-a.x,qaz=query.z-a.z;
            const float denominator=bax*caz-baz*cax;
            if(std::abs(denominator)<1e-7f)continue;
            const float u=(qax*caz-qaz*cax)/denominator;
            const float v=(bax*qaz-baz*qax)/denominator;
            if(u>=-.0001f&&v>=-.0001f&&u+v<=1.0001f)
                highest=std::max(highest,a.y+u*(b.y-a.y)+v*(c.y-a.y));
        }
        return highest;
    };
    for(int row=0;row<=depthSegments;++row) {
        const float depth=std::lerp(depthMin,depthMax,static_cast<float>(row)/depthSegments);
        for(int column=0;column<=acrossSegments;++column) {
            const float across=std::lerp(acrossMin,acrossMax,
                static_cast<float>(column)/acrossSegments);
            const float frontWeight=1.0f-smooth((depth-depthMin)/3.2f);
            const float shapedDepth=depth+frontWeight*(.42f*std::sin(across*.53f+.4f)
                +.20f*std::sin(across*1.17f+1.1f));
            const float centreRoof=roofAt(0,std::min(shapedDepth,21.75f));
            const auto world=cave.World(across,0,shapedDepth);
            const float originalHeight=original.Sample(world.x,world.z).height;
            const float actualRoof=roofAt(across,shapedDepth);
            const float shiftedAcross=across-.34f*std::sin(shapedDepth*.31f+.7f);
            const float outer=12.65f+.55f*std::sin(shapedDepth*.37f+1.1f)
                +.22f*std::sin(shapedDepth*.83f+.2f);
            const float shoulder=1.0f-smooth((std::abs(shiftedAcross)-3.7f)
                                              /std::max(.5f,outer-3.7f));
            const float rear=1.0f-smooth((shapedDepth-20.2f)/4.05f);
            const float broadTarget=centreRoof+.09f-.018f*across*across
                +.10f*std::sin(across*.73f+shapedDepth*.29f)
                +.055f*std::sin(across*1.41f-shapedDepth*.47f);
            float height=std::max(originalHeight+.018f,
                std::lerp(originalHeight+.018f,broadTarget,
                          std::clamp(shoulder*rear,0.0f,1.0f)));
            if(std::isfinite(actualRoof))height=std::max(height,actualRoof+.18f);
            coverVertices_.push_back({world.x,height,world.z});
        }
    }
    for(int row=0;row<depthSegments;++row)for(int column=0;column<acrossSegments;++column) {
        const auto tl=static_cast<std::uint16_t>(row*(acrossSegments+1)+column);
        const auto tr=static_cast<std::uint16_t>(tl+1);
        const auto bl=static_cast<std::uint16_t>(tl+acrossSegments+1);
        const auto br=static_cast<std::uint16_t>(bl+1);
        coverIndices_.insert(coverIndices_.end(),{tl,bl,tr,tr,bl,br});
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
