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
        if(part==GoatPart::Body) {
            // Physical-height projection preserves hair spacing through the rump
            // taper. Angular ring coordinates squeeze a full fur window into every
            // shrinking ring. Author before body bob so the coat cannot swim.
            u=(p.z+.62f)/1.22f;
            v=std::clamp((p.y-.52f)/.56f,0.0f,1.0f);
        }
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
    void Eye(float side) {
        // Fit to the actual tessellated head, not its enclosing ellipse. The
        // head pitch leaves x unchanged, so a side ray works in every graze pose.
        const auto fitted=[&](float y,float z,float lift,GoatMaterial material) {
            auto p=HeadPoint({0,y,z});p.y+=bob;
            float surface=-1;GoatPoint normal{};
            for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
                const auto& a=mesh.vertices[i];if(a.part!=GoatPart::Head)continue;
                const auto& b=mesh.vertices[i+1];const auto& c=mesh.vertices[i+2];
                const auto d=Sub(b.position,a.position),e=Sub(c.position,a.position),q=Sub(p,a.position);
                const float determinant=d.y*e.z-d.z*e.y;
                if(std::abs(determinant)<1e-9f)continue;
                const float u=(q.y*e.z-q.z*e.y)/determinant,v=(d.y*q.z-d.z*q.y)/determinant;
                if(u< -1e-5f||v< -1e-5f||u+v>1.00001f)continue;
                const float x=side*(a.position.x+u*d.x+v*e.x);
                if(x>surface) {surface=x;normal=Add(a.normal,Add(Mul(Sub(b.normal,a.normal),u),Mul(Sub(c.normal,a.normal),v)));}
            }
            if(surface<0)throw std::runtime_error("Goat eye has no head surface support");
            p.x=side*(surface+lift);
            return GoatVertex{p,Unit(normal),.5f,.5f,material,GoatPart::Eye};
        };
        constexpr int count=12;
        std::array<GoatVertex,count> outer,iris,pupil;
        for(int i=0;i<count;++i) {
            const float a=Tau*i/count,c=std::cos(a),s=std::sin(a);
            outer[i]=fitted(1.061f+.014f*s,.568f+.026f*c,.0015f,GoatMaterial::Eye);
            iris[i]=fitted(1.061f+.011f*s,.568f+.022f*c,.0035f,GoatMaterial::Iris);
            pupil[i]=fitted(1.061f+.0025f*s,.568f+.015f*c,.0045f,GoatMaterial::Eye);
        }
        const auto centre=fitted(1.061f,.568f,.0055f,GoatMaterial::Eye);
        for(int i=0;i<count;++i) {
            const int j=(i+1)%count;
            auto a=iris[i],b=iris[j];a.material=b.material=GoatMaterial::Eye;
            Triangle(outer[i],outer[j],b);Triangle(outer[i],b,a);
            a=pupil[i];b=pupil[j];a.material=b.material=GoatMaterial::Iris;
            Triangle(iris[i],iris[j],b);Triangle(iris[i],b,a);
            Triangle(pupil[i],pupil[j],centre);
        }
    }
    void Ear(float side,bool detailed) {
        // Thin cupped pinna: a recessed forward face, furred back and closed rim.
        // Skin is part of the surface, never a coplanar overlay.
        constexpr std::array<float,7> widths{.014f,.030f,.038f,.034f,.026f,.014f,.002f};
        const int columns=detailed?5:3;
        const auto point=[&](int row,int column,bool front) {
            const float t=static_cast<float>(row)/6,q=2.0f*column/(columns-1)-1;
            return GoatPoint{side*(.102f+.22f*t),1.095f+.025f*std::sin(t*std::numbers::pi_v<float>)-.045f*t+widths[row]*q,
                .44f-.02f*t+(front?.006f:-.006f)+.018f*q*q};
        };
        const auto vertex=[&](int row,int column,bool front,GoatMaterial material) {
            const auto longitudinal=Sub(point(std::min(row+1,6),column,front),point(std::max(row-1,0),column,front));
            const auto transverse=Sub(point(row,std::min(column+1,columns-1),front),point(row,std::max(column-1,0),front));
            return Vertex(point(row,column,front),Mul(Cross(longitudinal,transverse),front?side:-side),
                material,static_cast<float>(row)/6,static_cast<float>(column)/(columns-1),Attachment::Head);
        };
        for(bool front:{false,true})for(int row=0;row<6;++row)for(int column=0;column<columns-1;++column) {
            const bool inner=front&&row>=1&&row<=4&&(columns==3||column==1||column==2);
            const auto material=inner?GoatMaterial::Skin:GoatMaterial::Coat;
            const auto a=vertex(row,column,front,material),b=vertex(row+1,column,front,material),
                c=vertex(row+1,column+1,front,material),d=vertex(row,column+1,front,material);
            Triangle(a,b,c);Triangle(a,c,d);
        }
        const auto rim=[&](int r0,int c0,int r1,int c1) {
            auto a=vertex(r0,c0,true,GoatMaterial::Coat),b=vertex(r1,c1,true,GoatMaterial::Coat),
                c=vertex(r1,c1,false,GoatMaterial::Coat),d=vertex(r0,c0,false,GoatMaterial::Coat);
            auto normal=Unit(Cross(Sub(b.position,a.position),Sub(d.position,a.position)));
            // Point away from the leaf centre, including its short root/tip edges.
            const auto centre=HeadPoint({side*.212f,1.09f,.43f});
            if(Dot(normal,Sub(a.position,Add(centre,{0,bob,0})))<0)normal=Mul(normal,-1);
            a.normal=b.normal=c.normal=d.normal=normal;
            Triangle(a,b,c);Triangle(a,c,d);
        };
        for(int row=0;row<6;++row) {rim(row,0,row+1,0);rim(row,columns-1,row+1,columns-1);}
        for(int column=0;column<columns-1;++column) {rim(0,column,0,column+1);rim(6,column,6,column+1);}
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
    // Closely spaced terminal rings retain closed ends without pulling the rump
    // and brisket into the old two sharp cones. The broad middle rings supply a
    // recognisable barrel, shoulder and hindquarter rather than one uniform tube.
    const std::array<Ring,13> body{{
        {-.62f,.82f,.025f,.06f},{-.57f,.81f,.12f,.17f},{-.49f,.80f,.22f,.245f},
        {-.40f,.80f,.265f,.27f},{-.25f,.80f,.27f,.275f},{-.08f,.80f,.27f,.275f},
        {.10f,.81f,.25f,.265f},{.24f,.81f,.255f,.26f},{.36f,.80f,.26f,.265f},
        {.46f,.80f,.235f,.245f},{.52f,.80f,.17f,.20f},{.57f,.81f,.08f,.12f},
        {.60f,.81f,.025f,.055f}}};
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
    for(std::size_t legIndex=0;legIndex<pose.legs.size();++legIndex) {
        const auto& leg=pose.legs[legIndex];
        b.part=GoatPart::Leg;
        const bool front=legIndex%2==1;
        // Flesh follows the existing fixed-length chain; no foot or joint displacement.
        // Preserve longitudinal masses in both LODs, reducing only ring tessellation.
        const auto upper=Add(leg.root,Mul(Sub(leg.knee,leg.root),.36f));
        const auto upperLower=Add(leg.root,Mul(Sub(leg.knee,leg.root),.68f));
        const auto lower=Add(leg.knee,Mul(Sub(leg.hoof,leg.knee),.55f));
        // Bury a generous shoulder/haunch root inside the barrel, then taper
        // through a separate upper-limb station. Joint and hoof locations remain
        // the authored planted-foot chain; only the visible flesh silhouette grows.
        const std::array<Tube,7> limb{{{Add(leg.root,{0,.14f,0}),front?.105f:.125f},
            {leg.root,front?.095f:.115f},{upper,front?.075f:.090f},
            {upperLower,front?.060f:.072f},{leg.knee,front?.045f:.050f},
            {lower,front?.030f:.035f},{leg.hoof,.024f}}};
        b.Sweep(limb,detailed?8:5,GoatMaterial::Coat,Attachment::Leg);
        b.part=GoatPart::Hoof;
        b.hoofPivot=Add(leg.hoof,{0,-GoatLegPose::HoofHeight,GoatLegPose::SoleForward});
        b.hoofNormal=leg.soleNormal;
        for(float side:{-1.0f,1.0f}) {
            // Keep the sole at -.055 but avoid a hoof extending equally far above
            // the ankle: a low tapered wall and two separated toes, not round boots.
            const std::array<Ring,5> hoof{{{-.055f,-.012f,.016f,.027f},{-.025f,-.020f,.021f,.035f},
                {.030f,-.020f,.022f,.035f},{.075f,-.018f,.020f,.030f},{.085f,-.014f,.007f,.020f}}};
            b.Loft(hoof,detailed?8:5,GoatMaterial::Hoof,Attachment::Hoof,
                Add(leg.hoof,{side*.026f,0,GoatLegPose::SoleForward}));
        }
    }
    for(float side:{-1.0f,1.0f}) {
        b.part=GoatPart::Horn;
        const std::array<Tube,7> horn{{{{side*.062f,1.18f,.42f},.038f},{{side*.072f,1.29f,.38f},.032f},
            {{side*.090f,1.39f,.30f},.025f},{{side*.11f,1.45f,.20f},.018f},
            {{side*.13f,1.46f,.10f},.012f},{{side*.15f,1.42f,.015f},.007f},{{side*.16f,1.37f,-.03f},.002f}}};
        b.Sweep(horn,detailed?8:5,GoatMaterial::Horn,Attachment::Head);
        b.part=GoatPart::Ear;b.Ear(side,detailed);
        if(detailed) {
            b.part=GoatPart::Eye;b.Eye(side);
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
