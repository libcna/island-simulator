#include "IslandSimulator/SnakeContactShadow.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace island {
namespace {
    struct Point { double x,z,alpha; };
    struct Polygon { std::array<Point,8> points{}; std::size_t count=0; };
    double Side(Point a,Point b,Point p) { return (b.x-a.x)*(p.z-a.z)-(b.z-a.z)*(p.x-a.x); }
    void Push(Polygon& p,Point v) {
        if(p.count==p.points.size()) throw std::length_error("Snake shadow clip polygon overflow");
        p.points[p.count++]=v;
    }
    Polygon Clip(const Polygon& p,Point a,Point b) {
        Polygon result;
        if(p.count==0) return result;
        auto prior=p.points[p.count-1]; double priorSide=Side(a,b,prior);
        for(std::size_t i=0;i<p.count;++i) {
            const auto current=p.points[i]; const double side=Side(a,b,current);
            if((side>=0)!=(priorSide>=0)) {
                const double t=priorSide/(priorSide-side);
                Push(result,{prior.x+(current.x-prior.x)*t,prior.z+(current.z-prior.z)*t,
                    prior.alpha+(current.alpha-prior.alpha)*t});
            }
            if(side>=0) Push(result,current);
            prior=current; priorSide=side;
        }
        return result;
    }
}
std::vector<ContactPatchVertex> BuildSnakeContactShadow(const SnakeState& snake,
                                                       const RenderedTerrainSurface& surface)
{
    std::array<std::array<Point,3>,SnakeState::StationCount+1> rows{};
    for(std::size_t i=0;i<snake.body.size();++i) {
        const auto p=snake.body[i];
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)
            ||std::abs(p.x)>300||std::abs(p.z)>300)
            throw std::invalid_argument("Invalid snake contact shadow point");
        if(i>0 && std::hypot(p.x-snake.body[i-1].x,p.z-snake.body[i-1].z)>SnakePopulation::Spacing*1.03f)
            throw std::invalid_argument("Stretched snake shadow segment");
        const auto a=snake.body[i==0?0:i-1], b=snake.body[std::min(i+1,snake.body.size()-1)];
        const float length=std::hypot(b.x-a.x,b.z-a.z);
        if(!std::isfinite(length)||length<0.001f||length>0.12f)
            throw std::invalid_argument("Invalid snake contact shadow span");
        const float width=SnakePopulation::RadiusAt(i)*1.55f+0.008f;
        const double dx=-(b.z-a.z)/length*width,dz=(b.x-a.x)/length*width;
        const double taper=std::min(1.0f,SnakePopulation::RadiusAt(i)/0.03f);
        rows[i+1]={Point{p.x-dx,p.z-dz,0},Point{p.x,p.z,0.23*taper},Point{p.x+dx,p.z+dz,0}};
        if(i+1==snake.body.size()) rows[i+1][1].alpha=0;
        if(i==0) {
            const Point nose{p.x-(b.x-a.x)/length*0.06,p.z-(b.z-a.z)/length*0.06,0};
            rows[0]={nose,nose,nose};
        }
    }
    std::vector<ContactPatchVertex> result; result.reserve(1600);
    const auto grid=[](double value) {
        return std::clamp(static_cast<int>(std::floor((value+TerrainMeshLayout::Extent)
            *TerrainMeshLayout::QuadsPerAxis/(2*TerrainMeshLayout::Extent))),0,TerrainMeshLayout::QuadsPerAxis-1);
    };
    const auto triangle=[&](Point a,Point b,Point c) {
        for(int row=grid(std::min({a.z,b.z,c.z}));row<=grid(std::max({a.z,b.z,c.z}));++row)
            for(int col=grid(std::min({a.x,b.x,c.x}));col<=grid(std::max({a.x,b.x,c.x}));++col) {
                const double x0=TerrainMeshLayout::Coordinate(col),x1=TerrainMeshLayout::Coordinate(col+1);
                const double z0=TerrainMeshLayout::Coordinate(row),z1=TerrainMeshLayout::Coordinate(row+1);
                const Point tl{x0,z0,0},tr{x1,z0,0},bl{x0,z1,0},br{x1,z1,0};
                for(const auto& cell:{std::array{tl,tr,bl},std::array{tr,br,bl}}) {
                    Polygon p; Push(p,a); Push(p,b); Push(p,c);
                    for(std::size_t edge=0;edge<3;++edge) p=Clip(p,cell[edge],cell[(edge+1)%3]);
                    const auto emit=[&](Point v) {
                        const float x=static_cast<float>(v.x),z=static_cast<float>(v.z);
                        if(result.size()==SnakeShadowVertexLimit) throw std::length_error("Snake shadow vertex limit exceeded");
                        result.push_back({x,surface.Sample(x,z).height+SnakeShadowSurfaceOffset,z,
                            static_cast<float>(std::clamp(v.alpha,0.0,0.23))});
                    };
                    for(std::size_t i=1;i+1<p.count;++i) {
                        if(std::abs(Side(p.points[0],p.points[i],p.points[i+1]))<1e-10) continue;
                        emit(p.points[0]); emit(p.points[i]); emit(p.points[i+1]);
                    }
                }
            }
    };
    for(std::size_t i=1;i<rows.size();++i) for(std::size_t j=0;j<2;++j) {
        triangle(rows[i-1][j],rows[i][j],rows[i][j+1]);
        triangle(rows[i-1][j],rows[i][j+1],rows[i-1][j+1]);
    }
    return result;
}
}
