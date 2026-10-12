#pragma once
#include "IslandSimulator/StreamCourse.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <vector>
#include <cstdint>

namespace island {
struct StreamBedVertex {float x,y,z;};
// Local replacement patch, not an overlay. Integration must remove the original
// covered terrain triangles and use SampleHeight for matching collision queries.
class StreamBedPatch final {
public:
    static constexpr int Subdivision=8;
    StreamBedPatch(const StreamCourse& course,const RenderedTerrainSurface& original) {
        constexpr float spacing=2*TerrainMeshLayout::Extent/TerrainMeshLayout::QuadsPerAxis;
        const auto& a=course.Station(0);const auto& b=course.Station(course.Count()-1);
        const auto grid=[](float p){return (p+TerrainMeshLayout::Extent)/spacing;};
        const int firstX=static_cast<int>(std::floor(grid(b.x-7)));
        const int lastX=static_cast<int>(std::ceil(grid(a.x+7)));
        const int firstZ=static_cast<int>(std::floor(grid(a.z-7)));
        const int lastZ=static_cast<int>(std::ceil(grid(b.z+7)));
        minX_=TerrainMeshLayout::Coordinate(firstX);minZ_=TerrainMeshLayout::Coordinate(firstZ);
        columns_=(lastX-firstX)*Subdivision+1;rows_=(lastZ-firstZ)*Subdivision+1;
        step_=spacing/Subdivision;
        if(static_cast<std::size_t>(columns_)*rows_>=65536)
            throw std::runtime_error("Stream bed exceeds portable index capacity");
        const float length=b.distance,dx=(b.x-a.x)/length,dz=(b.z-a.z)/length;
        vertices_.reserve(static_cast<std::size_t>(columns_)*rows_);
        for(int row=0;row<rows_;++row)for(int col=0;col<columns_;++col) {
            const float x=minX_+static_cast<float>(col)*step_;
            const float z=minZ_+static_cast<float>(row)*step_;
            const float base=original.Sample(x,z).height;
            const float along=(x-a.x)*dx+(z-a.z)*dz;
            const float across=std::abs((x-a.x)*dz-(z-a.z)*dx);
            float height=base;
            if(along>0&&along<length+4) {
                const float s=std::clamp(along/length,0.0f,1.0f)*static_cast<float>(course.Count()-1);
                const auto i=std::min(static_cast<std::size_t>(s),course.Count()-2);
                const float t=std::clamp(s-static_cast<float>(i),0.0f,1.0f);
                const float width=std::lerp(course.Station(i).halfWidth,course.Station(i+1).halfWidth,t);
                const float water=std::lerp(course.Station(i).water,course.Station(i+1).water,t);
                if(across<width+2) {
                    const auto smooth=[](float v){v=std::clamp(v,0.0f,1.0f);return v*v*(3-2*v);};
                    const float bowl=water-.25f*(1-std::min(1.0f,across*across/(width*width)));
                    const float influence=(1-smooth((across-width)/2))*smooth(along/2)
                        *(1-smooth((along-length)/4));
                    height=std::lerp(base,std::min(base,bowl),influence);
                }
            }
            vertices_.push_back({x,height,z});
        }
        indices_.reserve(static_cast<std::size_t>(columns_-1)*(rows_-1)*6);
        for(int row=0;row<rows_-1;++row)for(int col=0;col<columns_-1;++col) {
            const auto tl=static_cast<std::uint16_t>(row*columns_+col);
            const auto tr=static_cast<std::uint16_t>(tl+1),bl=static_cast<std::uint16_t>(tl+columns_);
            const auto br=static_cast<std::uint16_t>(bl+1);
            indices_.insert(indices_.end(),{tl,bl,tr,tr,bl,br});
        }
    }
    [[nodiscard]] const auto& Vertices() const noexcept{return vertices_;}
    [[nodiscard]] const auto& Indices() const noexcept{return indices_;}
    [[nodiscard]] int Columns() const noexcept{return columns_;}
    [[nodiscard]] int Rows() const noexcept{return rows_;}
    [[nodiscard]] float SampleHeight(float x,float z) const {
        float gx=(x-minX_)/step_,gz=(z-minZ_)/step_;
        // Roundoff at an inclusive stored boundary must not reject its own vertex.
        if(!std::isfinite(gx)||!std::isfinite(gz)||gx<-.0001f||gz<-.0001f
            ||gx>columns_-1+.0001f||gz>rows_-1+.0001f)
            throw std::out_of_range("Outside stream bed patch");
        gx=std::clamp(gx,0.0f,static_cast<float>(columns_-1));
        gz=std::clamp(gz,0.0f,static_cast<float>(rows_-1));
        const int col=std::min(static_cast<int>(gx),columns_-2),row=std::min(static_cast<int>(gz),rows_-2);
        const float u=gx-static_cast<float>(col),v=gz-static_cast<float>(row);
        const auto i=static_cast<std::size_t>(row*columns_+col);
        const float tl=vertices_[i].y,tr=vertices_[i+1].y,bl=vertices_[i+columns_].y,br=vertices_[i+columns_+1].y;
        return u+v<=1?tl+(tr-tl)*u+(bl-tl)*v:br+(bl-br)*(1-u)+(tr-br)*(1-v);
    }
private:
    float minX_=0,minZ_=0,step_=0;
    int columns_=0,rows_=0;
    std::vector<StreamBedVertex> vertices_;
    std::vector<std::uint16_t> indices_;
};
}
