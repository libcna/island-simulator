#include "IslandSimulator/GoatMesh.hpp"
#include "IslandSimulator/GoatHabitat.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

int main() {
    using namespace island;
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto sub=[](GoatPoint a,GoatPoint b){return GoatPoint{a.x-b.x,a.y-b.y,a.z-b.z};};
    const auto cross=[](GoatPoint a,GoatPoint b){return GoatPoint{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    const auto dot=[](GoatPoint a,GoatPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    float maximumRadius=0;
    std::size_t nearCount=0,farCount=0;
    for(bool detailed:{false,true})for(int step=0;step<16;++step)for(int graze=0;graze<=10;++graze) {
        GoatGroundPose pose{};
        pose.scale=1;pose.bodyBob=.008f;
        for(std::size_t leg=0;leg<4;++leg)
            pose.legs[leg]=EvaluateGoatLeg(step*std::numbers::pi_v<float>/8,1.2f,pose.bodyBob,leg);
        const auto mesh=BuildGoatMesh(pose,graze/10.0f,detailed);
        check(!mesh.vertices.empty()&&mesh.vertices.size()%3==0&&mesh.vertices.size()<=GoatMesh::MaximumVertices,
            "goat mesh triangle budget invalid");
        if(detailed)nearCount=mesh.vertices.size();else farCount=mesh.vertices.size();
        std::array<std::size_t,6> materials{};
        for(const auto& vertex:mesh.vertices) {
            const auto p=vertex.position,n=vertex.normal;
            check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z),"nonfinite goat mesh");
            check(std::abs(std::sqrt(dot(n,n))-1)<1e-5f,"goat normal not unit length");
            check(std::isfinite(vertex.u)&&std::isfinite(vertex.v)&&vertex.u>=0&&vertex.u<=1
                &&vertex.v>=0&&vertex.v<=1,"goat UV outside source window");
            check(p.y>=-.00001f,"flat goat geometry penetrates ground");
            // Worst horizontal projection for any tilt azimuth up to acos(.9).
            const float radial=std::hypot(p.x,p.z);
            const float angle=std::min(std::acos(.9f),std::atan2(std::abs(p.y),radial));
            const float bound=radial*std::cos(angle)+std::abs(p.y)*std::sin(angle);
            maximumRadius=std::max(maximumRadius,std::max(radial,bound));
            check(std::max(radial,bound)<=GoatHabitat::BodyClearance,"goat geometry exceeds motion clearance");
            ++materials[static_cast<std::size_t>(vertex.material)];
        }
        check(materials[static_cast<std::size_t>(GoatMaterial::Horn)]>0
            &&materials[static_cast<std::size_t>(GoatMaterial::Hoof)]>0
            &&materials[static_cast<std::size_t>(GoatMaterial::Nose)]>0,"goat anatomy component missing");
        check((materials[static_cast<std::size_t>(GoatMaterial::Eye)]>0)==detailed,"eye LOD mismatch");
        for(const auto& leg:pose.legs) {
            float surface=-1;
            for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
                if(mesh.vertices[i].part!=GoatPart::Body)continue;
                const auto a=mesh.vertices[i].position,b=mesh.vertices[i+1].position,c=mesh.vertices[i+2].position;
                const float dy=b.y-a.y,dz=b.z-a.z,ey=c.y-a.y,ez=c.z-a.z;
                const float determinant=dy*ez-dz*ey;
                if(std::abs(determinant)<1e-9f)continue;
                const float u=((leg.root.y-a.y)*ez-(leg.root.z-a.z)*ey)/determinant;
                const float v=(dy*(leg.root.z-a.z)-dz*(leg.root.y-a.y))/determinant;
                if(u>=-1e-5f&&v>=-1e-5f&&u+v<=1.00001f)
                    surface=std::max(surface,a.x+u*(b.x-a.x)+v*(c.x-a.x));
            }
            check(surface>=std::abs(leg.root.x)+.01f,"goat leg root detached from torso");
        }
        for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
            const auto a=mesh.vertices[i],b=mesh.vertices[i+1],c=mesh.vertices[i+2];
            const auto normal=cross(sub(b.position,a.position),sub(c.position,a.position));
            check(dot(normal,normal)>1e-14f,"degenerate goat triangle");
            const GoatPoint average{a.normal.x+b.normal.x+c.normal.x,a.normal.y+b.normal.y+c.normal.y,
                a.normal.z+b.normal.z+c.normal.z};
            check(dot(normal,average)>0,"goat winding disagrees with normals");
        }
    }
    check(nearCount>farCount,"goat far LOD did not reduce geometry");
    GoatGroundPose bad{};bad.bodyBob=std::numeric_limits<float>::quiet_NaN();
    bool rejected=false;
    try {(void)BuildGoatMesh(bad,0,true);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"invalid mesh pose accepted");
    std::cout<<"goat mesh near/far vertices="<<nearCount<<'/'<<farCount
        <<" maximum conservative horizontal radius="<<maximumRadius<<'\n';
}
