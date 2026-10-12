#pragma once
#include "IslandSimulator/StreamBedPatch.hpp"
#include <cstdint>
#include <vector>
namespace island {
struct StreamWaterVertex {float x,y,z,alpha;};
// Static shallow-water ribbon inside the actual carved bed. Ripple shading
// animates without displacing geometry through the banks or changing collision.
struct StreamWaterMesh {
    std::vector<StreamWaterVertex> vertices;
    std::vector<std::uint16_t> indices;
    StreamWaterMesh(const StreamCourse& course,const StreamBedPatch& bed) {
        constexpr int acrossCount=9;
        const float normalX=.23f/std::hypot(1.0f,.23f),normalZ=1/std::hypot(1.0f,.23f);
        const int last=static_cast<int>(course.Count()-1)*4;
        for(int row=12;row<=last;++row) {
            const auto i=std::min(static_cast<std::size_t>(row/4),course.Count()-2);
            const float t=static_cast<float>(row)/4-static_cast<float>(i);
            const auto a=course.Station(i),b=course.Station(i+1);
            const float x=std::lerp(a.x,b.x,t),z=std::lerp(a.z,b.z,t),y=std::lerp(a.water,b.water,t);
            const float width=.80f*std::lerp(a.halfWidth,b.halfWidth,t);
            for(int side=0;side<acrossCount;++side) {
                const float lateral=static_cast<float>(side)/4-1;
                const float px=x+normalX*width*lateral,pz=z+normalZ*width*lateral;
                if(bed.SampleHeight(px,pz)>y-.015f)throw std::runtime_error("Stream water intersects bed");
                const float edge=1-lateral*lateral;
                const float ends=std::min(std::clamp(static_cast<float>(row-12)/8,0.0f,1.0f),
                    std::clamp(static_cast<float>(last-row)/8,0.0f,1.0f));
                vertices.push_back({px,y,pz,.65f*edge*ends});
            }
        }
        if(vertices.size()>=65536)throw std::runtime_error("Stream water index capacity");
        for(std::size_t row=0;row+1<vertices.size()/acrossCount;++row)for(int col=0;col<acrossCount-1;++col) {
            const auto a=static_cast<std::uint16_t>(row*acrossCount+static_cast<std::size_t>(col));
            const auto b=static_cast<std::uint16_t>(a+1),c=static_cast<std::uint16_t>(a+acrossCount),d=static_cast<std::uint16_t>(c+1);
            indices.insert(indices.end(),{a,c,b,b,c,d});
        }
    }
};
}
