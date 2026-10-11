#pragma once
#include "IslandSimulator/SnakePopulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace island {
// One fixed upload for all four ground snakes. Bounds avoid segment work away from bodies.
struct SnakeGrassContact {
    static_assert(SnakePopulation::Count==4 && SnakeState::StationCount==41);
    std::array<float,4*41*3> points{}; // x,z,radius
    std::array<std::array<float,4>,4> bounds{}; // min x/z, max x/z
    std::size_t count=0;
    static constexpr float Margin=0.015f, Fade=0.14f;
    static SnakeGrassContact Build(std::span<const SnakeState> snakes) {
        if(snakes.size()>4) throw std::invalid_argument("Snake grass contact capacity exceeded");
        SnakeGrassContact result; result.count=snakes.size();
        for(std::size_t s=0;s<snakes.size();++s) {
            auto& bounds=result.bounds[s];
            bounds={300,300,-300,-300};
            for(std::size_t j=0;j<41;++j) {
                const auto p=snakes[s].body[j];
                if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)
                    ||std::abs(p.x)>300||std::abs(p.z)>300)
                    throw std::invalid_argument("Invalid snake grass contact point");
                const auto i=(s*41+j)*3;
                const float r=SnakePopulation::RadiusAt(j);
                result.points[i]=p.x; result.points[i+1]=p.z; result.points[i+2]=r;
                const float reach=SnakePopulation::MaximumRadius+Margin+Fade;
                bounds[0]=std::min(bounds[0],p.x-reach); bounds[1]=std::min(bounds[1],p.z-reach);
                bounds[2]=std::max(bounds[2],p.x+reach); bounds[3]=std::max(bounds[3],p.z+reach);
            }
        }
        return result;
    }
    // Query the full deformed grass-cell envelope, not just its placement anchors.
    // Contact changes only height, so this horizontal rejection remains conservative.
    [[nodiscard]] std::array<float,4> CellMask(float minX,float minZ,float maxX,float maxZ) const {
        if(!std::isfinite(minX)||!std::isfinite(minZ)||!std::isfinite(maxX)||!std::isfinite(maxZ)
            ||minX>maxX||minZ>maxZ)
            throw std::invalid_argument("Invalid grass cell contact bounds");
        std::array<float,4> mask{};
        for(std::size_t s=0;s<count;++s) {
            const auto& b=bounds[s];
            mask[s]=(maxX>=b[0]&&maxZ>=b[1]&&minX<=b[2]&&minZ<=b[3])?1.0f:0.0f;
        }
        return mask;
    }
    [[nodiscard]] float Pressure(float x,float z, const std::array<float,4>& mask={1,1,1,1}) const {
        return SweptPressure(x,z,x,z,mask);
    }
    [[nodiscard]] float SweptPressure(float rootX,float rootZ,float x,float z,
                                      const std::array<float,4>& mask={1,1,1,1}) const {
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(rootX)||!std::isfinite(rootZ)) return 0;
        const auto pointDistanceSquared=[](float px,float pz,float ax,float az,float bx,float bz) {
            const float dx=bx-ax,dz=bz-az;
            const float t=std::clamp(((px-ax)*dx+(pz-az)*dz)/std::max(dx*dx+dz*dz,0.000001f),0.0f,1.0f);
            return (px-ax-t*dx)*(px-ax-t*dx)+(pz-az-t*dz)*(pz-az-t*dz);
        };
        float nearest=Fade;
        for(std::size_t s=0;s<count;++s) {
            if(mask[s]==0) continue;
            const auto b=bounds[s];
            if(std::max(x,rootX)<b[0]||std::max(z,rootZ)<b[1]
                ||std::min(x,rootX)>b[2]||std::min(z,rootZ)>b[3]) continue;
            for(std::size_t j=1;j<41;++j) {
                const auto i=(s*41+j-1)*3,k=i+3;
                const float ax=points[i],az=points[i+1],bx=points[k],bz=points[k+1];
                const float dx=x-rootX,dz=z-rootZ,sx=bx-ax,sz=bz-az;
                const float denominator=dx*sz-dz*sx;
                if(std::abs(denominator)>0.00000001f) {
                    const float t=((ax-rootX)*sz-(az-rootZ)*sx)/denominator;
                    const float u=((ax-rootX)*dz-(az-rootZ)*dx)/denominator;
                    if(t>=0&&t<=1&&u>=0&&u<=1) return 1;
                }
                const float distanceSquared=std::min({pointDistanceSquared(x,z,ax,az,bx,bz),
                    pointDistanceSquared(rootX,rootZ,ax,az,bx,bz),
                    pointDistanceSquared(ax,az,rootX,rootZ,x,z),pointDistanceSquared(bx,bz,rootX,rootZ,x,z)});
                nearest=std::min(nearest,std::sqrt(distanceSquared)
                    -std::max(points[i+2],points[k+2])-Margin);
                if(nearest<=0) return 1;
            }
        }
        const float t=std::clamp(nearest/Fade,0.0f,1.0f);
        return 1-t*t*(3-2*t);
    }
    static constexpr const char* Shader=R"(
uniform vec3 uSnakeGrassPoints[164];
uniform vec4 uSnakeGrassBounds[4];
uniform float uSnakeGrassCount;
uniform vec4 uSnakeGrassCellMask;
float snakePointSegmentSquared(vec2 p,vec2 a,vec2 b) {
    vec2 d=b-a;
    vec2 offset=p-a-d*clamp(dot(p-a,d)/max(dot(d,d),0.000001),0.0,1.0);
    return dot(offset,offset);
}
float snakeCross(vec2 a,vec2 b) { return a.x*b.y-a.y*b.x; }
float snakeGrassPressure(vec2 root,vec2 position) {
    if(!any(greaterThan(uSnakeGrassCellMask,vec4(0.0)))) return 0.0;
    float nearest = 0.14;
    for(int s=0;s<4;++s) {
        if(float(s)>=uSnakeGrassCount) break;
        if(uSnakeGrassCellMask[s]<0.5) continue;
        vec4 bounds=uSnakeGrassBounds[s];
        if(any(lessThan(max(root,position),bounds.xy)) || any(greaterThan(min(root,position),bounds.zw))) continue;
        for(int j=1;j<41;++j) {
            vec3 a=uSnakeGrassPoints[s*41+j-1], b=uSnakeGrassPoints[s*41+j];
            vec2 d=position-root, direction=b.xy-a.xy;
            float denominator=snakeCross(d,direction);
            if(abs(denominator)>0.00000001) {
                float t=snakeCross(a.xy-root,direction)/denominator;
                float u=snakeCross(a.xy-root,d)/denominator;
                if(t>=0.0&&t<=1.0&&u>=0.0&&u<=1.0) return 1.0;
            }
            float distanceSquared=min(min(snakePointSegmentSquared(root,a.xy,b.xy),
                snakePointSegmentSquared(position,a.xy,b.xy)),
                min(snakePointSegmentSquared(a.xy,root,position),snakePointSegmentSquared(b.xy,root,position)));
            nearest=min(nearest,sqrt(distanceSquared)-max(a.z,b.z)-0.015);
            if(nearest<=0.0) return 1.0;
        }
    }
    return 1.0-smoothstep(0.0,0.14,nearest);
}
)";
};
}
