#pragma once
#include "IslandSimulator/StreamCourse.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <vector>
#include <cstdint>
#include <limits>

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
        float routeMinimumZ=a.z,routeMaximumZ=a.z;
        for(std::size_t i=1;i<course.Count();++i) {
            routeMinimumZ=std::min(routeMinimumZ,course.Station(i).z);
            routeMaximumZ=std::max(routeMaximumZ,course.Station(i).z);
        }
        const int firstX=static_cast<int>(std::floor(grid(b.x-7)));
        const int lastX=static_cast<int>(std::ceil(grid(a.x+7)));
        const int firstZ=static_cast<int>(std::floor(grid(routeMinimumZ-7)));
        const int lastZ=static_cast<int>(std::ceil(grid(routeMaximumZ+7)));
        minX_=TerrainMeshLayout::Coordinate(firstX);minZ_=TerrainMeshLayout::Coordinate(firstZ);
        columns_=(lastX-firstX)*Subdivision+1;rows_=(lastZ-firstZ)*Subdivision+1;
        step_=spacing/Subdivision;
        if(static_cast<std::size_t>(columns_)*rows_>=65536)
            throw std::runtime_error("Stream bed exceeds portable index capacity");
        const float length=b.distance;
        const int coarseColumns=lastX-firstX,coarseRows=lastZ-firstZ;
        std::vector<bool> refined(static_cast<std::size_t>(coarseColumns*coarseRows),false);
        vertices_.reserve(static_cast<std::size_t>(columns_)*rows_);
        for(int row=0;row<rows_;++row)for(int col=0;col<columns_;++col) {
            const float x=minX_+static_cast<float>(col)*step_;
            const float z=minZ_+static_cast<float>(row)*step_;
            const float base=original.Sample(x,z).height;
            const float rawStation=StreamRoute::Along(x);
            std::size_t i=0;
            float t=0;
            if(rawStation<0) {
                t=rawStation;
            } else if(rawStation>static_cast<float>(course.Count()-1)) {
                i=course.Count()-2;
                t=rawStation-static_cast<float>(i);
            } else {
                float nearestSquared=std::numeric_limits<float>::max();
                const int approximate=static_cast<int>(rawStation);
                const auto firstCandidate=static_cast<std::size_t>(std::max(0,approximate-6));
                const auto lastCandidate=static_cast<std::size_t>(std::min(
                    static_cast<int>(course.Count()-2),approximate+6));
                for(std::size_t candidate=firstCandidate;candidate<=lastCandidate;++candidate) {
                    const auto& candidateA=course.Station(candidate);
                    const auto& candidateB=course.Station(candidate+1);
                    const float segmentX=candidateB.x-candidateA.x;
                    const float segmentZ=candidateB.z-candidateA.z;
                    const float candidateT=std::clamp(((x-candidateA.x)*segmentX
                        +(z-candidateA.z)*segmentZ)/(segmentX*segmentX+segmentZ*segmentZ),
                        0.0f,1.0f);
                    const float deltaX=x-std::lerp(candidateA.x,candidateB.x,candidateT);
                    const float deltaZ=z-std::lerp(candidateA.z,candidateB.z,candidateT);
                    const float squared=deltaX*deltaX+deltaZ*deltaZ;
                    if(squared<nearestSquared) {
                        nearestSquared=squared;i=candidate;t=candidateT;
                    }
                }
            }
            const auto& routeA=course.Station(i);const auto& routeB=course.Station(i+1);
            const float centreX=std::lerp(routeA.x,routeB.x,t);
            const float centreZ=std::lerp(routeA.z,routeB.z,t);
            float along=std::lerp(routeA.distance,routeB.distance,t);
            const float across=std::hypot(x-centreX,z-centreZ);
            float height=base;
            if(along>0&&along<length+4) {
                const float shapeT=std::clamp(t,0.0f,1.0f);
                const float width=std::lerp(routeA.halfWidth,routeB.halfWidth,shapeT);
                const float water=std::lerp(routeA.water,routeB.water,shapeT);
                if(across<width+2) {
                    const auto smooth=[](float v){v=std::clamp(v,0.0f,1.0f);return v*v*(3-2*v);};
                    const float bowl=water-.25f*(1-std::min(1.0f,across*across/(width*width)));
                    const float influence=(1-smooth((across-width)/2))*smooth(along/2)
                        *(1-smooth((along-length)/4));
                    height=std::lerp(base,std::min(base,bowl),influence);
                }
            }
            vertices_.push_back({x,height,z});
            heights_.push_back(height);
            if(height!=base) {
                // A changed coarse-edge vertex refines both adjacent cells.
                for(int r=std::max(0,(row-1)/Subdivision);r<=std::min(coarseRows-1,row/Subdivision);++r)
                    for(int c=std::max(0,(col-1)/Subdivision);c<=std::min(coarseColumns-1,col/Subdivision);++c)
                        refined[static_cast<std::size_t>(r*coarseColumns+c)]=true;
            }
        }
        const auto dense=[&](int row,int col){return row>=0&&col>=0&&row<coarseRows&&col<coarseColumns
            &&refined[static_cast<std::size_t>(row*coarseColumns+col)];};
        const auto index=[&](int row,int col){return static_cast<std::uint16_t>(row*columns_+col);};
        for(int r=0;r<coarseRows;++r)for(int c=0;c<coarseColumns;++c) {
            const int row=r*Subdivision,col=c*Subdivision;
            if(dense(r,c)) {
                for(int y=0;y<Subdivision;++y)for(int x=0;x<Subdivision;++x) {
                    const auto tl=index(row+y,col+x),tr=index(row+y,col+x+1);
                    const auto bl=index(row+y+1,col+x),br=index(row+y+1,col+x+1);
                    indices_.insert(indices_.end(),{tl,bl,tr,tr,bl,br});
                }
            } else {
                // Fan centre lies on the original TR-BL diagonal. Each fan sector
                // stays in one original plane; only edges next to fine cells split.
                const auto centre=index(row+Subdivision/2,col+Subdivision/2);
                const auto edge=[&](int y,int x,int dy,int dx2,bool split) {
                    const int stride=split?1:Subdivision;
                    for(int k=0;k<Subdivision;k+=stride)
                        indices_.insert(indices_.end(),{centre,index(row+y+dy*k,col+x+dx2*k),
                            index(row+y+dy*(k+stride),col+x+dx2*(k+stride))});
                };
                edge(0,0,1,0,dense(r,c-1));
                edge(Subdivision,0,0,1,dense(r+1,c));
                edge(Subdivision,Subdivision,-1,0,dense(r,c+1));
                edge(0,Subdivision,0,-1,dense(r-1,c));
            }
        }
        // Compact unused dense-grid vertices; keep only scalar heights for queries.
        std::vector<int> remap(vertices_.size(),-1);
        std::vector<StreamBedVertex> compact;
        for(auto& i:indices_) {
            if(remap[i]<0) {remap[i]=static_cast<int>(compact.size());compact.push_back(vertices_[i]);}
            i=static_cast<std::uint16_t>(remap[i]);
        }
        vertices_=std::move(compact);
    }
    [[nodiscard]] const auto& Vertices() const noexcept{return vertices_;}
    [[nodiscard]] const auto& Indices() const noexcept{return indices_;}
    [[nodiscard]] int Columns() const noexcept{return columns_;}
    [[nodiscard]] int Rows() const noexcept{return rows_;}
    [[nodiscard]] bool Contains(float x,float z) const noexcept {
        return std::isfinite(x)&&std::isfinite(z)&&x>=minX_&&z>=minZ_
            &&x<=minX_+static_cast<float>(columns_-1)*step_
            &&z<=minZ_+static_cast<float>(rows_-1)*step_;
    }
    [[nodiscard]] bool IsBoundary(StreamBedVertex p) const noexcept {
        return p.x==minX_||p.z==minZ_||p.x==minX_+static_cast<float>(columns_-1)*step_
            ||p.z==minZ_+static_cast<float>(rows_-1)*step_;
    }
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
        const float tl=heights_[i],tr=heights_[i+1],bl=heights_[i+columns_],br=heights_[i+columns_+1];
        return u+v<=1?tl+(tr-tl)*u+(bl-tl)*v:br+(bl-br)*(1-u)+(tr-br)*(1-v);
    }
private:
    float minX_=0,minZ_=0,step_=0;
    int columns_=0,rows_=0;
    std::vector<StreamBedVertex> vertices_;
    std::vector<float> heights_;
    std::vector<std::uint16_t> indices_;
};
}
