#include "IslandSimulator/GoatMesh.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <stdexcept>

namespace island {
namespace {
constexpr float Tau=2*std::numbers::pi_v<float>;
GoatPoint Add(GoatPoint a,GoatPoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
GoatPoint Sub(GoatPoint a,GoatPoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
GoatPoint Mul(GoatPoint p,float s){return {p.x*s,p.y*s,p.z*s};}
float Dot(GoatPoint a,GoatPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
GoatPoint Cross(GoatPoint a,GoatPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
GoatPoint Unit(GoatPoint p){const float n=std::sqrt(Dot(p,p));
    if(!std::isfinite(n)||n<1e-7f)throw std::invalid_argument("Degenerate goat mesh direction");
    return Mul(p,1/n);}
bool Finite(GoatPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
enum class Attachment { Body,Head,Leg,Hoof };
struct Ring {float z,y,rx,ry;};
struct Tube {GoatPoint centre;float radius;};
struct Builder {
    GoatMesh mesh;
    float bob,pitch,headLower;
    GoatPart part=GoatPart::Body;
    GoatPoint hoofPivot{0,0,0},hoofNormal{0,1,0};
    GoatPoint HoofDirection(GoatPoint p) const {
        // Minimal rotation from local up to this hoof's actual triangle support normal.
        const auto axis=Cross({0,1,0},hoofNormal);
        return Add(Add(p,Cross(axis,p)),Mul(Cross(axis,Cross(axis,p)),1/(1+hoofNormal.y)));
    }
    GoatPoint HeadPoint(GoatPoint p) const {
        const float c=std::cos(pitch),s=std::sin(pitch),y=p.y-.72f,z=p.z-.25f;
        return {p.x,.72f+c*y-s*z-headLower,.25f+s*y+c*z};
    }
    GoatVertex Vertex(GoatPoint p,GoatPoint n,GoatMaterial material,float u,float v,Attachment attachment) {
        if(attachment==Attachment::Head) {
            const float c=std::cos(pitch),s=std::sin(pitch);
            p=HeadPoint(p);
            n={n.x,c*n.y-s*n.z,s*n.y+c*n.z};
        }
        if(attachment==Attachment::Hoof) {
            p=Add(hoofPivot,HoofDirection(Sub(p,hoofPivot)));
            n=HoofDirection(n);
        }
        if(attachment==Attachment::Body||attachment==Attachment::Head)p.y+=bob;
        return {p,Unit(n),u,v,material,part};
    }
    void Triangle(GoatVertex a,GoatVertex b,GoatVertex c) {
        const auto normal=Cross(Sub(b.position,a.position),Sub(c.position,a.position));
        if(Dot(normal,normal)<1e-14f)return;
        if(Dot(normal,Add(Add(a.normal,b.normal),c.normal))<0)std::swap(b,c);
        if(mesh.vertices.size()+3>GoatMesh::MaximumVertices)throw std::runtime_error("Goat mesh budget exceeded");
        mesh.vertices.insert(mesh.vertices.end(),{a,b,c});
    }
    void Loft(std::span<const Ring> rings,int sides,GoatMaterial material,Attachment attachment,
        GoatPoint offset={0,0,0}) {
        const auto vertex=[&](std::size_t row,int side) {
            const auto a=rings[row],before=rings[row==0?0:row-1],after=rings[std::min(row+1,rings.size()-1)];
            const float angle=static_cast<float>(side)*Tau/sides,c=std::cos(angle),s=std::sin(angle);
            const float nx=c/a.rx,ny=s/a.ry,inv=1/(after.z-before.z);
            return Vertex(Add({c*a.rx,a.y+s*a.ry,a.z},offset),
                {nx,ny,-nx*c*(after.rx-before.rx)*inv-ny*((after.y-before.y)+s*(after.ry-before.ry))*inv},
                material,(a.z-rings.front().z)/(rings.back().z-rings.front().z),
                static_cast<float>(side)/sides,attachment);
        };
        for(std::size_t row=0;row+1<rings.size();++row)for(int side=0;side<sides;++side) {
            const auto a=vertex(row,side),b=vertex(row+1,side),c=vertex(row+1,side+1),d=vertex(row,side+1);
            Triangle(a,b,c);Triangle(a,c,d);
        }
        for(const auto row:{std::size_t(0),rings.size()-1}) {
            const float direction=row==0?-1.0f:1.0f;
            const auto centre=Vertex(Add({0,rings[row].y,rings[row].z},offset),{0,0,direction},material,.5f,.5f,attachment);
            for(int side=0;side<sides;++side) {
                auto a=vertex(row,side),b=vertex(row,side+1);
                a.normal=centre.normal;b.normal=centre.normal;
                Triangle(centre,a,b);
            }
        }
    }
    void Sweep(std::span<const Tube> path,int sides,GoatMaterial material,Attachment attachment) {
        std::vector<float> arc(path.size(),0);
        for(std::size_t row=1;row<path.size();++row) {
            const auto step=Sub(path[row].centre,path[row-1].centre);
            arc[row]=arc[row-1]+std::sqrt(Dot(step,step));
        }
        const auto vertex=[&](std::size_t row,int side) {
            const auto a=path[row],before=path[row==0?0:row-1],after=path[std::min(row+1,path.size()-1)];
            const auto tangent=Unit(Sub(after.centre,before.centre));
            const auto right=Unit(Cross(tangent,std::abs(tangent.x)<.9f?GoatPoint{1,0,0}:GoatPoint{0,0,1}));
            const auto up=Cross(tangent,right);
            const float angle=static_cast<float>(side)*Tau/sides;
            const auto radial=Add(Mul(right,std::cos(angle)),Mul(up,std::sin(angle)));
            const float length=std::sqrt(Dot(Sub(after.centre,before.centre),Sub(after.centre,before.centre)));
            return Vertex(Add(a.centre,Mul(radial,a.radius)),
                Sub(radial,Mul(tangent,(after.radius-before.radius)/length)),material,
                arc[row]/arc.back(),static_cast<float>(side)/sides,attachment);
        };
        for(std::size_t row=0;row+1<path.size();++row)for(int side=0;side<sides;++side) {
            const auto a=vertex(row,side),b=vertex(row+1,side),c=vertex(row+1,side+1),d=vertex(row,side+1);
            Triangle(a,b,c);Triangle(a,c,d);
        }
        for(const auto row:{std::size_t(0),path.size()-1}) {
            const auto neighbour=row==0?1:row-1;
            const auto centre=Vertex(path[row].centre,Sub(path[row].centre,path[neighbour].centre),material,.5f,.5f,attachment);
            for(int side=0;side<sides;++side) {
                auto a=vertex(row,side),b=vertex(row,side+1);a.normal=centre.normal;b.normal=centre.normal;
                Triangle(centre,a,b);
            }
        }
    }
};
}
GoatMesh BuildGoatMesh(const GoatGroundPose& pose,float grazing,bool detailed) {
    if(!std::isfinite(grazing)||!std::isfinite(pose.bodyBob))throw std::invalid_argument("Invalid goat mesh pose");
    for(const auto& leg:pose.legs)
        if(!Finite(leg.root)||!Finite(leg.knee)||!Finite(leg.hoof)||leg.reachLimited
            ||!Finite(leg.soleNormal)||std::abs(Dot(leg.soleNormal,leg.soleNormal)-1)>.001f||leg.soleNormal.y<.5f)
            throw std::invalid_argument("Unrenderable goat limb pose");
    grazing=std::clamp(grazing,0.0f,1.0f);
    Builder b{{},pose.bodyBob,grazing*.85f,grazing*.18f};
    b.mesh.vertices.reserve(GoatMesh::MaximumVertices);
    const int sides=detailed?12:6;
    const std::array<Ring,11> body{{
        {-.58f,.80f,.03f,.07f},{-.52f,.82f,.15f,.20f},{-.43f,.82f,.25f,.25f},
        {-.30f,.81f,.245f,.26f},{-.12f,.79f,.25f,.26f},{.06f,.80f,.23f,.25f},
        {.20f,.83f,.24f,.245f},{.30f,.83f,.245f,.25f},{.40f,.80f,.245f,.25f},
        {.48f,.80f,.215f,.22f},{.54f,.78f,.025f,.05f}}};
    b.Loft(body,sides,GoatMaterial::Coat,Attachment::Body);
    b.part=GoatPart::Neck;
    std::array<Tube,4> neck{{{{0,.86f,.22f},.14f},{{0,.98f,.35f},.115f},
        {{0,1.07f,.43f},.095f},{{0,1.09f,.47f},.08f}}};
    constexpr std::array<float,4> neckWeights{0,.45f,.85f,1};
    for(std::size_t i=0;i<neck.size();++i)
        neck[i].centre=Add(neck[i].centre,Mul(Sub(b.HeadPoint(neck[i].centre),neck[i].centre),neckWeights[i]));
    b.Sweep(neck,sides,GoatMaterial::Coat,Attachment::Body);
    const std::array<Ring,8> head{{
        {.35f,1.08f,.025f,.05f},{.41f,1.09f,.10f,.13f},{.48f,1.08f,.125f,.145f},
        {.56f,1.035f,.112f,.13f},{.64f,.97f,.09f,.105f},{.70f,.90f,.075f,.085f},
        {.755f,.855f,.071f,.06f},{.777f,.85f,.055f,.05f}}};
    b.part=GoatPart::Head;b.Loft(head,sides,GoatMaterial::Coat,Attachment::Head);
    for(const auto& leg:pose.legs) {
        b.part=GoatPart::Leg;
        const std::array<Tube,4> limb{{{Add(leg.root,{0,.10f,0}),.050f},{leg.root,.065f},{leg.knee,.039f},{leg.hoof,.024f}}};
        b.Sweep(limb,detailed?8:5,GoatMaterial::Coat,Attachment::Leg);
        b.part=GoatPart::Hoof;
        b.hoofPivot=Add(leg.hoof,{0,-GoatLegPose::HoofHeight,GoatLegPose::SoleForward});
        b.hoofNormal=leg.soleNormal;
        for(float side:{-1.0f,1.0f}) {
            const std::array<Ring,5> hoof{{{-.055f,0,.018f,.040f},{-.025f,0,.026f,.055f},
                {.030f,0,.027f,.055f},{.075f,.004f,.024f,.049f},{.085f,.008f,.007f,.03f}}};
            b.Loft(hoof,detailed?8:5,GoatMaterial::Hoof,Attachment::Hoof,
                Add(leg.hoof,{side*.030f,0,GoatLegPose::SoleForward}));
        }
    }
    for(float side:{-1.0f,1.0f}) {
        b.part=GoatPart::Horn;
        const std::array<Tube,7> horn{{{{side*.062f,1.18f,.42f},.038f},{{side*.072f,1.29f,.38f},.032f},
            {{side*.090f,1.39f,.30f},.025f},{{side*.11f,1.45f,.20f},.018f},
            {{side*.13f,1.46f,.10f},.012f},{{side*.15f,1.42f,.015f},.007f},{{side*.16f,1.37f,-.03f},.002f}}};
        b.Sweep(horn,detailed?8:5,GoatMaterial::Horn,Attachment::Head);
        const std::array<Tube,4> ear{{{{side*.105f,1.095f,.44f},.033f},{{side*.19f,1.10f,.43f},.047f},
            {{side*.27f,1.075f,.42f},.03f},{{side*.30f,1.055f,.42f},.003f}}};
        b.part=GoatPart::Ear;b.Sweep(ear,detailed?8:5,GoatMaterial::Coat,Attachment::Head);
        if(detailed) {
            b.part=GoatPart::Eye;
            const std::array<Ring,3> eye{{{-.007f,0,.006f,.005f},{0,0,.014f,.010f},{.007f,0,.006f,.005f}}};
            b.Loft(eye,8,GoatMaterial::Eye,Attachment::Head,{side*.099f,1.068f,.569f});
        }
    }
    const std::array<Tube,4> beard{{{{0,.83f,.68f},.046f},{{0,.74f,.68f},.04f},
        {{0,.65f,.65f},.025f},{{0,.59f,.62f},.003f}}};
    b.part=GoatPart::Beard;b.Sweep(beard,detailed?8:5,GoatMaterial::Coat,Attachment::Head);
    const std::array<Tube,4> tail{{{{0,.88f,-.52f},.042f},{{0,.94f,-.61f},.035f},
        {{0,1.0f,-.68f},.024f},{{0,1.015f,-.73f},.004f}}};
    b.part=GoatPart::Tail;b.Sweep(tail,detailed?8:5,GoatMaterial::Coat,Attachment::Body);
    const std::array<Ring,3> nose{{{.770f,.851f,.058f,.042f},{.782f,.851f,.063f,.044f},{.788f,.851f,.045f,.033f}}};
    b.part=GoatPart::Nose;b.Loft(nose,detailed?10:6,GoatMaterial::Nose,Attachment::Head);
    return std::move(b.mesh);
}
}
