#include "IslandSimulator/SpiderMesh.hpp"
#include "IslandSimulator/SpiderHabitat.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
island::SpiderPoint Sub(island::SpiderPoint a,island::SpiderPoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
island::SpiderPoint Cross(island::SpiderPoint a,island::SpiderPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
float Dot(island::SpiderPoint a,island::SpiderPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
}
int main() {
    using namespace island;
    std::size_t nearCount=0,farCount=0;
    float nearBodyError=0,farBodyError=0;
    for(bool detailed:{false,true})for(int step=0;step<9;++step) {
        auto legs=SpiderPose::Rest();
        for(std::size_t i=0;i<legs.size();++i) {
            auto target=SpiderPose::NeutralFoot(i);
            target.y=.002f*static_cast<float>(step);
            target.z+=.001f*static_cast<float>(step-4);
            legs[i]=SpiderPose::Solve(i,target);
            Check(!legs[i].reachLimited,"mesh fixture target unreachable");
        }
        const auto mesh=BuildSpiderMesh(legs,detailed);
        const auto restMesh=BuildSpiderMesh(SpiderPose::Rest(),detailed);
        Check(!mesh.vertices.empty()&&mesh.vertices.size()%3==0&&mesh.vertices.size()<=SpiderMesh::MaximumVertices,"mesh budget invalid");
        Check(mesh.vertices.size()==restMesh.vertices.size(),"pose changes mesh topology");
        for(std::size_t i=0;i<mesh.vertices.size();++i) {
            Check(mesh.vertices[i].u==restMesh.vertices[i].u&&mesh.vertices[i].v==restMesh.vertices[i].v,
                "setae UV swims with articulated pose");
        }
        (detailed?nearCount:farCount)=mesh.vertices.size();
        std::array<std::size_t,6> parts{};
        for(const auto& v:mesh.vertices) {
            const auto p=v.position;
            Check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z),"nonfinite vertex");
            Check(std::abs(Dot(v.normal,v.normal)-1)<1e-5f,"non-unit normal");
            Check(std::isfinite(v.u)&&std::isfinite(v.v)&&v.u>=.05999f&&v.u<=.94001f
                &&v.v>=.05999f&&v.v<=.94001f,"spider albedo UV outside interior window");
            Check(p.y>=-1e-7f,"flat mesh penetrates ground");
            Check(std::hypot(p.x,p.z)<=SpiderHabitat::BodyRadius,"mesh exceeds habitat envelope");
            if(v.part==SpiderPart::Leg) {
                bool withinEnvelope=false;
                constexpr std::array<float,4> radii{.0038f,.0032f,.0023f,0};
                for(const auto& leg:legs) {
                    const std::array<SpiderPoint,4> joints{leg.root,leg.hip,leg.knee,leg.foot};
                    for(std::size_t segment=0;segment<3;++segment) {
                        const auto axis=Sub(joints[segment+1],joints[segment]);
                        const auto offset=Sub(p,joints[segment]);
                        const float t=Dot(offset,axis)/Dot(axis,axis);
                        const SpiderPoint radial{offset.x-t*axis.x,offset.y-t*axis.y,offset.z-t*axis.z};
                        const float radius=std::lerp(radii[segment],radii[segment+1],std::clamp(t,0.0f,1.0f));
                        withinEnvelope|=t>=-.00001f&&t<=1.00001f
                            &&std::sqrt(Dot(radial,radial))<=radius+.0000001f;
                    }
                }
                Check(withinEnvelope,"shaped limb exceeds the collision taper envelope");
            }
            ++parts[static_cast<std::size_t>(v.part)];
        }
        for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
            const auto& a=mesh.vertices[i];const auto& b=mesh.vertices[i+1];const auto& c=mesh.vertices[i+2];
            const auto cross=Cross(Sub(b.position,a.position),Sub(c.position,a.position));
            Check(Dot(cross,cross)>1e-24f,"degenerate triangle");
            Check(Dot(cross,a.normal)+Dot(cross,b.normal)+Dot(cross,c.normal)>0,"inverted winding");
            if(a.part==SpiderPart::FrontBody||a.part==SpiderPart::Abdomen) {
                const auto centre=a.part==SpiderPart::FrontBody?SpiderPose::ThoraxCentre:SpiderPose::AbdomenCentre;
                const auto radii=a.part==SpiderPart::FrontBody?SpiderPose::ThoraxRadii:SpiderPose::AbdomenRadii;
                const SpiderPoint offset{(a.position.x+b.position.x+c.position.x)/3-centre.x,
                    (a.position.y+b.position.y+c.position.y)/3-centre.y,
                    (a.position.z+b.position.z+c.position.z)/3-centre.z};
                const float radial=std::sqrt(offset.x*offset.x/(radii.x*radii.x)
                    +offset.y*offset.y/(radii.y*radii.y)+offset.z*offset.z/(radii.z*radii.z));
                const float error=std::sqrt(Dot(offset,offset))*(1/radial-1);
                auto& maximum=detailed?nearBodyError:farBodyError;
                maximum=std::max(maximum,error);
            }
        }
        for(std::size_t i=0;i<4;++i)Check(parts[i]>0,"missing body/limbs/palps");
        Check((parts[4]>0)==detailed&&(parts[5]>0)==detailed,"eye/hair LOD wrong");
    }
    Check(nearCount>farCount,"far LOD not reduced");
    Check(farCount==1632,"far mesh budget changed");
    Check(nearBodyError<.0007f&&nearBodyError<farBodyError*.4f,"near body surface is too faceted");
    auto bad=SpiderPose::Rest();bad[0].knee.x=NAN;
    bool rejected=false;try{(void)BuildSpiderMesh(bad,true);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected,"nonfinite pose accepted");
    std::cout<<"Spider mesh near/far vertices "<<nearCount<<'/'<<farCount
        <<"; max face-centre radial error "<<nearBodyError<<'/'<<farBodyError
        <<"; not live or visually accepted\n";
}
