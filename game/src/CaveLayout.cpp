#include "IslandSimulator/CaveLayout.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
struct V {double x,y,z;};
V Vec(CavePoint p) {return {p.x,p.y,p.z};}
V operator+(V a,V b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
V operator-(V a,V b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
V operator*(V a,double b) {return {a.x*b,a.y*b,a.z*b};}
double Dot(V a,V b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
V Cross(V a,V b) {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double Square(V a) {return Dot(a,a);}
bool Finite(CavePoint p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
float Smooth(float x) {x=std::clamp(x,0.0f,1.0f);return x*x*(3-2*x);}
float Centre(float depth) {return .22f*std::sin(depth*.31f);}
float Width(float depth) {return 2.65f+.18f*std::sin(depth*.42f+.3f);}

double PointSegment(V p,V a,V b) {
    const auto edge=b-a;
    const auto t=std::clamp(Dot(p-a,edge)/std::max(Square(edge),1e-20),0.0,1.0);
    return Square(p-(a+edge*t));
}
// Squared distance to the triangle's plane when projected inside, otherwise its edges.
double PointTriangle(V p,V a,V b,V c) {
    const auto ab=b-a,ac=c-a,n=Cross(ab,ac);
    const double nn=Square(n);
    const auto projected=p-n*(Dot(p-a,n)/nn);
    const double ab2=Square(ab),ac2=Square(ac),abac=Dot(ab,ac);
    const double pAB=Dot(projected-a,ab),pAC=Dot(projected-a,ac);
    const double denominator=ab2*ac2-abac*abac;
    const double u=(ac2*pAB-abac*pAC)/denominator;
    const double v=(ab2*pAC-abac*pAB)/denominator;
    if(u>=0&&v>=0&&u+v<=1) return Square(p-projected);
    return std::min({PointSegment(p,a,b),PointSegment(p,b,c),PointSegment(p,c,a)});
}
double SegmentSegment(V a,V b,V c,V d) {
    const auto u=b-a,v=d-c,w=a-c;
    const double aa=Square(u),bb=Dot(u,v),cc=Square(v),dd=Dot(u,w),ee=Dot(v,w);
    const double denominator=aa*cc-bb*bb;
    double result=std::min({PointSegment(a,c,d),PointSegment(b,c,d),
                            PointSegment(c,a,b),PointSegment(d,a,b)});
    if(denominator>1e-18) {
        const double s=(bb*ee-cc*dd)/denominator,t=(aa*ee-bb*dd)/denominator;
        if(s>=0&&s<=1&&t>=0&&t<=1) result=std::min(result,Square(w+u*s-v*t));
    }
    return result;
}
bool RayTriangle(V origin,V direction,const CaveTriangle& triangle,double& distance) {
    const auto a=Vec(triangle.points[0]),b=Vec(triangle.points[1]),c=Vec(triangle.points[2]);
    const auto edge=b-a,other=c-a,h=Cross(direction,other);
    const double det=Dot(edge,h);
    if(std::abs(det)<1e-12) return false;
    const auto s=origin-a;
    const double u=Dot(s,h)/det;
    const auto q=Cross(s,edge);
    const double v=Dot(direction,q)/det;
    distance=Dot(other,q)/det;
    return u>=-1e-9&&v>=-1e-9&&u+v<=1+1e-9&&distance>1e-8;
}
double SegmentTriangle(V p,V q,const CaveTriangle& triangle) {
    double t=0;
    if(RayTriangle(p,q-p,triangle,t)&&t<=1) return 0;
    const auto a=Vec(triangle.points[0]),b=Vec(triangle.points[1]),c=Vec(triangle.points[2]);
    return std::min({PointTriangle(p,a,b,c),PointTriangle(q,a,b,c),
        SegmentSegment(p,q,a,b),SegmentSegment(p,q,b,c),SegmentSegment(p,q,c,a)});
}
}

float CaveLayout::Floor(float depth) noexcept {return .025f*depth;}
CavePoint CaveLayout::World(float across,float height,float depth) const noexcept {
    return {entrance_.x-sine_*across-cosine_*depth,entrance_.y+height,
            entrance_.z+cosine_*across-sine_*depth};
}
CavePoint CaveLayout::Local(CavePoint point) const noexcept {
    const float x=point.x-entrance_.x,z=point.z-entrance_.z;
    return {-sine_*x+cosine_*z,point.y-entrance_.y,-cosine_*x-sine_*z};
}

CaveLayout::CaveLayout(const IslandTerrain& terrain) {
    // South of the eastern tide pools, on the shoulder of the rocky headland.
    constexpr float angle=CaveSite::Angle;
    cosine_=std::cos(angle);sine_=std::sin(angle);
    const float radial=terrain.ShoreRadius(angle)-CaveSite::Inset;
    entrance_={cosine_*radial,0,sine_*radial};
    entrance_.y=terrain.Height(entrance_.x,entrance_.z);
    using Ring=std::array<CavePoint,ArchSegments+1>;
    std::array<Ring,DepthSegments+1> inner{},outer{};
    for(int row=0;row<=DepthSegments;++row) {
        const float depth=Length*static_cast<float>(row)/DepthSegments;
        const float outerDepth=OuterLength*static_cast<float>(row)/DepthSegments;
        for(int slice=0;slice<=ArchSegments;++slice) {
            const float theta=std::numbers::pi_v<float>*static_cast<float>(slice)/ArchSegments;
            // Exact basal endpoints keep floor/back/entrance seams welded.
            const float arc=(slice==0||slice==ArchSegments)?0.0f:std::sin(theta);
            const float w=std::cos(theta);
            const float height=3.45f+.25f*std::sin(depth*.37f+theta*3)
                +.12f*std::sin(depth*1.3f+theta*7);
            const float innerWidth=Width(depth)*(1+.07f*std::sin(depth*.9f+theta*5+.4f)*arc);
            inner[row][slice]=World(Centre(depth)+innerWidth*w,Floor(depth)+height*arc,depth);
            const float outerWidth=6.0f+.8f*std::sin(outerDepth*.38f+theta*2)
                +.6f*std::sin(theta*5+outerDepth*.83f);
            const float outerHeight=7.4f+.6f*std::sin(outerDepth*.41f+theta*3+.6f)
                +.4f*std::sin(theta*7+outerDepth*.7f);
            // Broken projecting lip instead of a planar, manufactured arch face.
            const float rimDepth=outerDepth-(1.1f+.8f*std::sin(theta*3+.4f))
                *std::exp(-outerDepth*.4f);
            outer[row][slice]=World(Centre(outerDepth)+outerWidth*w,
                Floor(outerDepth)-1.3f+outerHeight*arc,rimDepth);
        }
    }
    const auto add=[&](CavePoint a,CavePoint b,CavePoint c,V desired,CaveSurface surface) {
        auto normal=Cross(Vec(b)-Vec(a),Vec(c)-Vec(a));
        if(Dot(normal,desired)<0) {std::swap(b,c);normal=normal*-1;}
        const double length=std::sqrt(Square(normal));
        if(length<1e-8) throw std::runtime_error("Degenerate cave shell triangle");
        triangles_.push_back({{a,b,c},{static_cast<float>(normal.x/length),
            static_cast<float>(normal.y/length),static_cast<float>(normal.z/length)},surface});
    };
    const auto quad=[&](CavePoint a,CavePoint b,CavePoint c,CavePoint d,V n,CaveSurface part) {
        add(a,b,c,n,part);add(a,c,d,n,part);
    };
    const V inward={-cosine_,0,-sine_};
    for(int row=0;row<DepthSegments;++row) {
        for(int slice=0;slice<ArchSegments;++slice) {
            const float theta=std::numbers::pi_v<float>*(static_cast<float>(slice)+.5f)/ArchSegments;
            const V radialNormal={-sine_*std::cos(theta),std::sin(theta),cosine_*std::cos(theta)};
            quad(inner[row][slice],inner[row+1][slice],inner[row+1][slice+1],inner[row][slice+1],
                 radialNormal*-1,CaveSurface::Interior);
            quad(outer[row][slice],outer[row+1][slice],outer[row+1][slice+1],outer[row][slice+1],
                 radialNormal,CaveSurface::Exterior);
        }
        quad(inner[row][0],inner[row+1][0],inner[row+1][ArchSegments],inner[row][ArchSegments],
             {0,1,0},CaveSurface::Floor);
        quad(outer[row][0],outer[row+1][0],outer[row+1][ArchSegments],outer[row][ArchSegments],
             {0,-1,0},CaveSurface::Exterior);
    }
    for(int slice=0;slice<ArchSegments;++slice)
        quad(outer[0][slice],outer[0][slice+1],inner[0][slice+1],inner[0][slice],
             inward*-1,CaveSurface::Entrance);
    quad(outer[0][ArchSegments],outer[0][0],inner[0][0],inner[0][ArchSegments],
         inward*-1,CaveSurface::Entrance);
    for(int slice=1;slice<ArchSegments;++slice) {
        add(inner.back()[0],inner.back()[slice],inner.back()[slice+1],inward*-1,CaveSurface::Back);
        add(outer.back()[0],outer.back()[slice],outer.back()[slice+1],inward,CaveSurface::Exterior);
    }
    bounds_={CavePoint{INFINITY,INFINITY,INFINITY},CavePoint{-INFINITY,-INFINITY,-INFINITY}};
    for(const auto& triangle:triangles_) for(const auto& p:triangle.points) {
        bounds_[0].x=std::min(bounds_[0].x,p.x);bounds_[1].x=std::max(bounds_[1].x,p.x);
        bounds_[0].y=std::min(bounds_[0].y,p.y);bounds_[1].y=std::max(bounds_[1].y,p.y);
        bounds_[0].z=std::min(bounds_[0].z,p.z);bounds_[1].z=std::max(bounds_[1].z,p.z);
    }
}

float CaveLayout::GroundTarget(float x,float z,float original) const noexcept {
    if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(original)) return original;
    const auto p=Local({x,entrance_.y,z});
    const float across=std::abs(p.x-Centre(p.z));
    const float influence=(1-Smooth((across-Width(p.z))/2.5f))*Smooth((p.z+6)/6)
        *(1-Smooth((p.z-Length)/3));
    // A modest buried shoulder ties the broad outcrop into the hill. The cavity stays
    // excavated, while all patch-border samples remain exactly the original terrain.
    const float shoulder=Smooth((p.z-1)/4)*(1-Smooth((p.z-Length)/5))
        *(1-Smooth((across-3.5f)/3))*Smooth((across-Width(p.z))/1.5f);
    return std::lerp(original+3.0f*shoulder,entrance_.y+Floor(p.z),influence);
}
bool CaveLayout::ReservesHabitat(float x,float z,float margin) const noexcept {
    return CaveSite::Reserves(x,z,margin);
}
bool CaveLayout::InsideRock(CavePoint point) const noexcept {
    // Oblique ray avoids aligning the common centreline queries with mesh rings.
    // Deduplicate shared-edge hits; parity tests the closed solid rock, not its empty cavity.
    std::array<double,32> hits{};
    std::size_t count=0;
    for(const auto& triangle:triangles_) {
        double t=0;
        if(!RayTriangle(Vec(point),{1,.271,.137},triangle,t)) continue;
        bool unique=true;
        for(std::size_t i=0;i<count;++i) if(std::abs(t-hits[i])<1e-6) unique=false;
        if(unique) {
            if(count==hits.size()) return true; // Impossible for this shell; fail closed.
            hits[count++]=t;
        }
    }
    return count%2!=0;
}
bool CaveLayout::IntersectsCapsule(CavePoint feet,float radius,float height) const noexcept {
    if(!Finite(feet)||!std::isfinite(radius)||!std::isfinite(height)
        ||radius<=0||height<2*radius) return true;
    if(feet.x+radius<bounds_[0].x||feet.x-radius>bounds_[1].x
        ||feet.z+radius<bounds_[0].z||feet.z-radius>bounds_[1].z
        ||feet.y+height<bounds_[0].y||feet.y>bounds_[1].y) return false;
    CavePoint centre=feet;centre.y+=height*.5f;
    if(InsideRock(centre)) return true;
    const V a={feet.x,feet.y+radius,feet.z},b={feet.x,feet.y+height-radius,feet.z};
    // Contact with the floor is permitted; genuine penetration is not.
    const double threshold=std::max(0.0,static_cast<double>(radius)-.00002);
    for(const auto& triangle:triangles_) {
        const auto p=triangle.points[0],q=triangle.points[1],r=triangle.points[2];
        if(std::max({p.x,q.x,r.x})<feet.x-radius||std::min({p.x,q.x,r.x})>feet.x+radius
            ||std::max({p.z,q.z,r.z})<feet.z-radius||std::min({p.z,q.z,r.z})>feet.z+radius
            ||std::max({p.y,q.y,r.y})<feet.y||std::min({p.y,q.y,r.y})>feet.y+height)continue;
        if(SegmentTriangle(a,b,triangle)<threshold*threshold) return true;
    }
    return false;
}
float CaveLayout::SafeTravelFraction(CavePoint from,CavePoint to,float radius,float height) const noexcept {
    if(!Finite(from)||!Finite(to)||IntersectsCapsule(from,radius,height)) return 0;
    const double distance=std::sqrt(Square(Vec(to)-Vec(from)));
    if(distance>1000) return 0;
    // No per-step work away from this one small landmark, including long landing probes.
    if(std::max(from.x,to.x)+radius<bounds_[0].x||std::min(from.x,to.x)-radius>bounds_[1].x
        ||std::max(from.z,to.z)+radius<bounds_[0].z||std::min(from.z,to.z)-radius>bounds_[1].z
        ||std::max(from.y,to.y)+height<bounds_[0].y||std::min(from.y,to.y)>bounds_[1].y)return 1;
    const int steps=std::max(1,static_cast<int>(std::ceil(distance/.1)));
    const auto at=[&](float t){return CavePoint{std::lerp(from.x,to.x,t),
        std::lerp(from.y,to.y,t),std::lerp(from.z,to.z,t)};};
    float previous=0;
    for(int i=1;i<=steps;++i) {
        const float current=static_cast<float>(i)/steps;
        if(IntersectsCapsule(at(current),radius,height)) {
            float low=previous,high=current;
            for(int pass=0;pass<12;++pass) {
                const float middle=(low+high)*.5f;
                if(IntersectsCapsule(at(middle),radius,height)) high=middle;else low=middle;
            }
            return low;
        }
        previous=current;
    }
    return 1;
}
}
