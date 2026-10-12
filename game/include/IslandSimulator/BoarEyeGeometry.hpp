#pragma once
#include "IslandSimulator/BoarPose.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island {
// Intersect the positive-X side of the exact 16-sided near body triangles.
inline float BoarHeadSurfaceX(const BoarBodyProfile& profile,float y,float z) {
    static const auto circle=[] {
        std::array<std::array<float,2>,17> points{};
        for(std::size_t i=0;i<points.size();++i) {
            const float a=static_cast<float>(i)*2*std::numbers::pi_v<float>/16;
            points[i]={std::cos(a),std::sin(a)};
        }
        return points;
    }();
    float result=-1;
    for(std::size_t row=0;row+1<profile.size();++row) {
        if(z<profile[row].z-1e-6f||z>profile[row+1].z+1e-6f) continue;
        const auto point=[&](std::size_t r,int side) {
            const auto& direction=circle[static_cast<std::size_t>(side)];
            return BoarJoint{direction[0]*profile[r].radiusX,
                profile[r].centreY+direction[1]*profile[r].radiusY,profile[r].z};
        };
        const auto triangle=[&](BoarJoint a,BoarJoint b,BoarJoint c) {
            const float dy=b.y-a.y,dz=b.z-a.z,ey=c.y-a.y,ez=c.z-a.z;
            const float determinant=dy*ez-dz*ey;
            if(std::abs(determinant)<1e-9f) return;
            const float u=((y-a.y)*ez-(z-a.z)*ey)/determinant;
            const float v=(dy*(z-a.z)-dz*(y-a.y))/determinant;
            if(u>=-1e-5f&&v>=-1e-5f&&u+v<=1.00001f)
                result=std::max(result,a.x+u*(b.x-a.x)+v*(c.x-a.x));
        };
        for(int side=0;side<16;++side) {
            const auto a=point(row,side),b=point(row+1,side),
                c=point(row+1,side+1),d=point(row,side+1);
            triangle(a,b,c);triangle(a,c,d);
        }
    }
    if(!std::isfinite(y)||!std::isfinite(z)||result<0)
        throw std::invalid_argument("Boar eye lies outside head surface");
    return result;
}

// Outer skin rim, inner lid rim, shallow dark cornea. No independent floating sphere.
inline std::array<BoarJoint,17> BuildBoarEye(int side,float dip,float bob) {
    if((side!=-1&&side!=1)||!std::isfinite(dip)||!std::isfinite(bob))
        throw std::invalid_argument("Invalid boar eye pose");
    dip=std::clamp(dip,0.0f,0.15f);bob=std::clamp(bob,0.0f,0.018f);
    const auto profile=MakeBoarBodyProfile(dip);
    const float centreY=0.61f-dip*0.65f;
    std::array<BoarJoint,17> result{};
    const auto point=[&](float y,float z,float lift) {
        z=BoarMuzzleZ(z);
        return BoarJoint{static_cast<float>(side)*(BoarHeadSurfaceX(profile,y,z)+lift),y+bob,z};
    };
    for(int ring=0;ring<2;++ring) for(int i=0;i<8;++i) {
        const float a=static_cast<float>(i)*2*std::numbers::pi_v<float>/8;
        result[static_cast<std::size_t>(ring*8+i)]=point(
            centreY+std::sin(a)*(ring==0?0.026f:0.014f),
            0.75f+std::cos(a)*(ring==0?0.040f:0.023f),ring==0?0.0015f:0.004f);
    }
    result[16]=point(centreY,0.75f,0.009f);
    return result;
}
}
