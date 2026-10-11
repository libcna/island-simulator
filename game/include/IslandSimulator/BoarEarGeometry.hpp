#pragma once
#include "IslandSimulator/BoarPose.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace island {
enum class BoarEarSurface { Rim, Inner, Back };
struct BoarEarTriangle {
    std::array<BoarJoint,3> points;
    BoarEarSurface surface;
    std::array<BoarJoint,3> normals{};
};
struct BoarEarMesh { std::array<BoarEarTriangle,32> triangles; std::size_t count=0; };

inline BoarEarMesh MakeBoarEarRestMesh(int side,bool detailed) {
    constexpr std::array<BoarJoint,8> outline{{
        {0,0,0},{0.06f,0.015f,-0.035f},{0.115f,0.10f,-0.055f},
        {0.13f,0.20f,-0.035f},{0.10f,0.245f,0.005f},
        {0.05f,0.205f,0.075f},{0.035f,0.11f,0.145f},{0.012f,0.02f,0.12f}}};
    constexpr BoarJoint cup{0.035f,0.105f,0.05f},back{0.010f,0.105f,0.05f};
    std::array<BoarJoint,8> inner{};
    for(std::size_t i=0;i<outline.size();++i) {
        const auto& p=outline[i];
        inner[i]={std::max(0.0f,std::lerp(cup.x,p.x,0.70f)-0.012f),
            std::lerp(cup.y,p.y,0.70f),std::lerp(cup.z,p.z,0.70f)};
    }
    BoarEarMesh result{};
    const auto point=[&](BoarJoint p) {
        return BoarJoint{static_cast<float>(side)*(0.17f+p.x),
            0.83f+p.y,0.54f+p.z};
    };
    const auto face=[&](BoarJoint a,BoarJoint b,BoarJoint c,BoarEarSurface surface) {
        if(side<0) std::swap(b,c);
        result.triangles[result.count++]={{point(a),point(b),point(c)},surface};
    };
    for(std::size_t i=0;i<outline.size();++i) {
        const auto j=(i+1)%outline.size();
        if(detailed) {
            face(outline[i],outline[j],inner[j],BoarEarSurface::Rim);
            face(outline[i],inner[j],inner[i],BoarEarSurface::Rim);
            face(cup,inner[i],inner[j],BoarEarSurface::Inner);
            face(back,outline[j],outline[i],BoarEarSurface::Back);
        } else face(cup,outline[i],outline[j],BoarEarSurface::Rim);
    }
    // Area-weighted front normals cross the rim/cup boundary; the back stays separate.
    // Calculated once per side/LOD, not for every posed animal each frame.
    for(std::size_t i=0;i<result.count;++i) for(std::size_t v=0;v<3;++v) {
        const auto p=result.triangles[i].points[v];
        BoarJoint sum{};
        for(std::size_t j=0;j<result.count;++j) {
            const auto& other=result.triangles[j];
            if((other.surface==BoarEarSurface::Back)
                !=(result.triangles[i].surface==BoarEarSurface::Back)) continue;
            bool shared=false;
            for(const auto q:other.points) shared=shared||(p.x==q.x&&p.y==q.y&&p.z==q.z);
            if(!shared) continue;
            const auto a=other.points[0],b=other.points[1],c=other.points[2];
            sum.x+=(b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y);
            sum.y+=(b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z);
            sum.z+=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
        }
        const float length=std::sqrt(sum.x*sum.x+sum.y*sum.y+sum.z*sum.z);
        if(!std::isfinite(length)||length<1e-8f) throw std::runtime_error("Degenerate boar ear normal");
        result.triangles[i].normals[v]={sum.x/length,sum.y/length,sum.z/length};
    }
    return result;
}

inline BoarEarMesh BuildBoarEar(int side,float dip,float bob,bool detailed) {
    if((side!=-1&&side!=1)||!std::isfinite(dip)||!std::isfinite(bob))
        throw std::invalid_argument("Invalid boar ear pose");
    static const std::array<BoarEarMesh,4> rest{MakeBoarEarRestMesh(-1,false),
        MakeBoarEarRestMesh(1,false),MakeBoarEarRestMesh(-1,true),MakeBoarEarRestMesh(1,true)};
    auto result=rest[static_cast<std::size_t>((detailed?2:0)+(side>0?1:0))];
    const float offset=-std::clamp(dip,0.0f,0.15f)*0.35f+std::clamp(bob,0.0f,0.018f);
    for(std::size_t i=0;i<result.count;++i) for(auto& p:result.triangles[i].points) p.y+=offset;
    return result;
}
}
