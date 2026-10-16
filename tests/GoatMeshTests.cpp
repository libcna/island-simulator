#include "IslandSimulator/GoatMesh.hpp"
#include "IslandSimulator/GoatCoatUv.hpp"
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
        std::array<std::size_t,8> materials{};
        std::array<std::size_t,2> innerEars{};
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
            if(vertex.material==GoatMaterial::Skin) {
                check(vertex.part==GoatPart::Ear,"goat inner-ear skin assigned to another part");
                ++innerEars[p.x>0?1:0];
                if(graze==0)check(n.z>.3f,"goat ear opening faces into the head or backwards");
            }
            if(vertex.part==GoatPart::Ear&&graze==0) {
                check(std::abs(p.x)>=.10199f&&std::abs(p.x)<=.32201f,
                    "goat ear lost authored attachment or tapered length");
                check(p.z>=.41399f&&p.z<=.46401f,
                    "goat ear reverted to thick cylindrical silhouette");
            }
            if(vertex.part==GoatPart::Hoof) {
                const auto leg=std::min_element(pose.legs.begin(),pose.legs.end(),[&](const auto& a,const auto& b){
                    const auto da=sub(p,a.hoof),db=sub(p,b.hoof);
                    return dot(da,da)<dot(db,db);
                });
                check(p.y-leg->hoof.y+GoatLegPose::HoofHeight<=.07001f,
                    "goat hoof wall reverted to oversized boot height");
                check(std::abs(p.x-leg->hoof.x)<=.04801f,
                    "goat hoof pair exceeds authored narrow footprint");
            }
            const auto uv=GoatCoatUv(vertex);
            if(vertex.part==GoatPart::Body) {
                check(std::abs(vertex.u-(vertex.position.z+.62f)/1.22f)<1e-5f,
                    "goat body coat stretches according to ring index rather than physical length");
                check(std::abs(vertex.v-(vertex.position.y-pose.bodyBob-.50f)/.64f)<1e-5f,
                    "goat body coat squeezes around taper or swims with body bob");
                check(std::abs(uv[0]-(.06f+.88f*(1-vertex.u)))<1e-6f
                    &&std::abs(uv[1]-(.06f+.88f*vertex.v))<1e-6f,
                    "body coat lost downward physical-height projection");
            }
            check(uv[0]>=.05999f&&uv[0]<=.94001f&&uv[1]>=.05999f&&uv[1]<=.94001f,
                "goat fur UV leaves interior source window");
        }
        check(materials[static_cast<std::size_t>(GoatMaterial::Horn)]>0
            &&materials[static_cast<std::size_t>(GoatMaterial::Hoof)]>0
            &&materials[static_cast<std::size_t>(GoatMaterial::Nose)]>0,"goat anatomy component missing");
        check((materials[static_cast<std::size_t>(GoatMaterial::Eye)]>0)==detailed,"eye LOD mismatch");
        check((materials[static_cast<std::size_t>(GoatMaterial::Iris)]>0)==detailed,"iris LOD mismatch");
        check((materials[static_cast<std::size_t>(GoatMaterial::Mouth)]>0)==detailed,"muzzle-detail LOD mismatch");
        check(innerEars[0]>0&&innerEars[0]==innerEars[1],"paired goat ear interiors missing or asymmetric");
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
        if(graze==0)for(std::size_t legIndex=0;legIndex<pose.legs.size();++legIndex) {
            const auto& leg=pose.legs[legIndex];
            const auto delta=sub(leg.knee,leg.root);
            const float length=std::sqrt(dot(delta,delta));
            const GoatPoint axis{delta.x/length,delta.y/length,delta.z/length};
            const GoatPoint centre{leg.root.x+.36f*delta.x,leg.root.y+.36f*delta.y,leg.root.z+.36f*delta.z};
            float largestRadius=0;
            for(const auto& vertex:mesh.vertices)if(vertex.part==GoatPart::Leg) {
                const auto offset=sub(vertex.position,centre);
                const float radius=std::sqrt(dot(offset,offset));
                if(std::abs(dot(offset,axis))<1e-5f&&radius<.15f)
                    largestRadius=std::max(largestRadius,radius);
            }
            check(std::abs(largestRadius-(legIndex%2==1?.075f:.090f))<1e-5f,
                "goat upper limb volume missing or inconsistent between LODs");
        }
        for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
            const auto a=mesh.vertices[i],b=mesh.vertices[i+1],c=mesh.vertices[i+2];
            const auto normal=cross(sub(b.position,a.position),sub(c.position,a.position));
            check(dot(normal,normal)>1e-14f,"degenerate goat triangle");
            const GoatPoint average{a.normal.x+b.normal.x+c.normal.x,a.normal.y+b.normal.y+c.normal.y,
                a.normal.z+b.normal.z+c.normal.z};
            check(dot(normal,average)>0,"goat winding disagrees with normals");
            if(a.part==GoatPart::Eye&&step==0) {
                const GoatPoint centre{(a.position.x+b.position.x+c.position.x)/3,
                    (a.position.y+b.position.y+c.position.y)/3,(a.position.z+b.position.z+c.position.z)/3};
                for(const auto p:{a.position,b.position,c.position,centre}) {
                    float surface=-1;const float side=p.x>0?1.0f:-1.0f;
                    for(std::size_t j=0;j<mesh.vertices.size();j+=3) {
                        if(mesh.vertices[j].part!=GoatPart::Head)continue;
                        const auto h=mesh.vertices[j].position,d=sub(mesh.vertices[j+1].position,h),e=sub(mesh.vertices[j+2].position,h);
                        const float determinant=d.y*e.z-d.z*e.y;
                        if(std::abs(determinant)<1e-9f)continue;
                        const auto q=sub(p,h);
                        const float u=(q.y*e.z-q.z*e.y)/determinant,v=(d.y*q.z-d.z*q.y)/determinant;
                        if(u>=-1e-5f&&v>=-1e-5f&&u+v<=1.00001f)
                            surface=std::max(surface,side*(h.x+u*d.x+v*e.x));
                    }
                    check(surface>0,"eye triangle has no underlying head");
                    const float gap=std::abs(p.x)-surface;
                    check(gap>.0001f&&gap<.007f,"eye is buried in head or detached from it");
                }
            }
        }
    }
    check(nearCount>farCount,"goat far LOD did not reduce geometry");
    // Sloped support planes rotate the actual hoof, not the limb chain or sole pivot.
    for(bool detailed:{false,true})for(float tilt:{-.12f,.12f}) {
        GoatGroundPose pose{};pose.scale=1;
        const GoatPoint support{tilt,std::sqrt(1-2*tilt*tilt),tilt};
        for(std::size_t i=0;i<4;++i) {
            pose.legs[i]=EvaluateGoatLeg(0,0,0,i);
            pose.legs[i].soleNormal=support;
        }
        const auto mesh=BuildGoatMesh(pose,0,detailed);
        for(const auto& vertex:mesh.vertices)if(vertex.part==GoatPart::Hoof) {
            const auto leg=std::min_element(pose.legs.begin(),pose.legs.end(),[&](const auto& a,const auto& b){
                const auto da=sub(vertex.position,a.hoof),db=sub(vertex.position,b.hoof);
                return dot(da,da)<dot(db,db);
            });
            const GoatPoint pivot{leg->hoof.x,leg->hoof.y-GoatLegPose::HoofHeight,leg->hoof.z+GoatLegPose::SoleForward};
            check(dot(sub(vertex.position,pivot),support)>=-1e-6f,"rotated hoof penetrates its support plane");
            check(std::abs(dot(vertex.normal,vertex.normal)-1)<1e-5f,"rotated hoof normal not unit length");
        }
    }
    for(const auto part:{GoatPart::Body,GoatPart::Head,GoatPart::Leg,GoatPart::Neck}) {
        GoatVertex vertex{};vertex.part=part;vertex.u=.3f;vertex.v=0;
        const auto start=GoatCoatUv(vertex);vertex.v=1;
        const auto end=GoatCoatUv(vertex);
        if(part!=GoatPart::Body)
            check(std::abs(start[0]-end[0])<1e-6f&&start[1]==end[1],"goat coat circumferential seam");
        else check(std::abs(start[1]-.06f)<1e-6f&&std::abs(end[1]-.94f)<1e-6f,
            "body physical height window changed");
        vertex.u=.1f;const float low=GoatCoatUv(vertex)[part==GoatPart::Body?0:1];
        vertex.u=.9f;const float high=GoatCoatUv(vertex)[part==GoatPart::Body?0:1];
        check(part==GoatPart::Body?high<low:high>low,"goat coat strand direction reversed");
    }
    for(const auto part:{GoatPart::Head}) {
        GoatVertex vertex{};vertex.part=part;vertex.u=.5f;
        for(const auto [angle,expected]:std::array<std::array<float,2>,4>{{{0,.5f},{.25f,.94f},{.5f,.5f},{.75f,.06f}}}) {
            vertex.v=angle;
            check(std::abs(GoatCoatUv(vertex)[0]-expected)<1e-6f,"loft coat does not follow ring height");
        }
        vertex.v=.1f;const auto left=GoatCoatUv(vertex);
        vertex.v=.4f;const auto right=GoatCoatUv(vertex);
        check(std::abs(left[0]-right[0])<1e-6f&&left[1]==right[1],"coat folds across visible flank");
    }
    GoatGroundPose bad{};bad.bodyBob=std::numeric_limits<float>::quiet_NaN();
    bool rejected=false;
    try {(void)BuildGoatMesh(bad,0,true);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"invalid mesh pose accepted");
    bad.bodyBob=0;bad.legs[0].soleNormal={0,-1,0};rejected=false;
    try {(void)BuildGoatMesh(bad,0,true);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"invalid hoof support normal accepted");
    std::cout<<"goat mesh near/far vertices="<<nearCount<<'/'<<farCount
        <<" maximum conservative horizontal radius="<<maximumRadius<<'\n';
}
