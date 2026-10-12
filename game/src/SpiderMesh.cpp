#include "IslandSimulator/SpiderMesh.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
constexpr float Pi=std::numbers::pi_v<float>;
SpiderPoint Add(SpiderPoint a,SpiderPoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
SpiderPoint Mul(SpiderPoint a,float s){return {a.x*s,a.y*s,a.z*s};}
SpiderPoint Sub(SpiderPoint a,SpiderPoint b){return Add(a,Mul(b,-1));}
float Dot(SpiderPoint a,SpiderPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
SpiderPoint Cross(SpiderPoint a,SpiderPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
SpiderPoint Unit(SpiderPoint p) {
    const float n=std::hypot(p.x,p.y,p.z);
    if(!std::isfinite(n)||n<1e-9f)throw std::invalid_argument("Invalid spider mesh direction");
    return Mul(p,1/n);
}
struct Builder {
    SpiderMesh mesh;
    void Triangle(SpiderVertex a,SpiderVertex b,SpiderVertex c) {
        const auto cross=Cross(Sub(b.position,a.position),Sub(c.position,a.position));
        if(Dot(cross,cross)<1e-24f)return;
        if(Dot(cross,Add(Add(a.normal,b.normal),c.normal))<0)std::swap(b,c);
        if(mesh.vertices.size()+3>SpiderMesh::MaximumVertices)
            throw std::runtime_error("Spider mesh budget exceeded");
        mesh.vertices.insert(mesh.vertices.end(),{a,b,c});
    }
    void Tube(SpiderPoint a,SpiderPoint b,float radiusA,float radiusB,int sides,
            SpiderPart part,SpiderMaterial material) {
        const auto axis=Unit(Sub(b,a));
        const auto right=Unit(Cross(axis,std::abs(axis.y)<.9f?SpiderPoint{0,1,0}:SpiderPoint{1,0,0}));
        const auto up=Cross(axis,right);
        const float slope=(radiusB-radiusA)/std::hypot(b.x-a.x,b.y-a.y,b.z-a.z);
        const auto vertex=[&](bool end,int i) {
            const float angle=2*Pi*static_cast<float>(i)/sides;
            const auto radial=Add(Mul(right,std::cos(angle)),Mul(up,std::sin(angle)));
            return SpiderVertex{Add(end?b:a,Mul(radial,end?radiusB:radiusA)),
                Unit(Sub(radial,Mul(axis,slope))),part,material};
        };
        for(int i=0;i<sides;++i) {
            Triangle(vertex(false,i),vertex(true,i),vertex(true,i+1));
            Triangle(vertex(false,i),vertex(true,i+1),vertex(false,i+1));
            for(bool end:{false,true}) {
                const auto normal=Mul(axis,end?1.0f:-1.0f);
                auto v=vertex(end,i),w=vertex(end,i+1);v.normal=w.normal=normal;
                Triangle({end?b:a,normal,part,material},v,w);
            }
        }
    }
    void Ellipsoid(SpiderPoint centre,SpiderPoint radii,int rows,int sides,
            SpiderPart part,SpiderMaterial material,bool hairs=false) {
        const auto vertex=[&](int row,int side) {
            const float lat=Pi*static_cast<float>(row)/rows,lon=2*Pi*static_cast<float>(side)/sides;
            // Explicit poles prevent tiny nearly-degenerate triangles from sin(pi).
            const float ring=row==0||row==rows?0.0f:std::sin(lat);
            const SpiderPoint unit{ring*std::cos(lon),std::cos(lat),ring*std::sin(lon)};
            auto m=material;
            if(material==SpiderMaterial::Cuticle&&unit.y>.45f&&std::abs(unit.x)<.40f)m=SpiderMaterial::Marking;
            return SpiderVertex{Add(centre,{unit.x*radii.x,unit.y*radii.y,unit.z*radii.z}),
                Unit({unit.x/radii.x,unit.y/radii.y,unit.z/radii.z}),part,m};
        };
        for(int row=0;row<rows;++row)for(int side=0;side<sides;++side) {
            const auto a=vertex(row,side),b=vertex(row+1,side),c=vertex(row+1,side+1),d=vertex(row,side+1);
            Triangle(a,b,c);Triangle(a,c,d);
            if(hairs&&row>0&&row<rows-1&&row%2==1&&side%2==0) {
                // Starts at an actual tessellation vertex, not the outer analytic
                // ellipse, so the short bristle cannot float above the body mesh.
                Tube(a.position,Add(a.position,Mul(a.normal,.0024f)),.00018f,0,3,
                    SpiderPart::Hair,SpiderMaterial::Hair);
            }
        }
    }
};
}
SpiderMesh BuildSpiderMesh(const std::array<SpiderLegPose,SpiderPose::LegCount>& legs,bool detailed) {
    for(const auto& leg:legs)for(const auto p:{leg.root,leg.hip,leg.knee,leg.foot})
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
            throw std::invalid_argument("Nonfinite spider mesh joint");
    Builder b;
    const int sides=detailed?12:8,rows=detailed?8:6,limbSides=detailed?6:4;
    b.Ellipsoid(SpiderPose::ThoraxCentre,SpiderPose::ThoraxRadii,rows,sides,
        SpiderPart::FrontBody,SpiderMaterial::Cuticle,detailed);
    b.Ellipsoid(SpiderPose::AbdomenCentre,SpiderPose::AbdomenRadii,rows,sides,
        SpiderPart::Abdomen,SpiderMaterial::Cuticle,detailed);
    for(const auto& leg:legs) {
        b.Tube(leg.root,leg.hip,.0038f,.0032f,limbSides,SpiderPart::Leg,SpiderMaterial::Cuticle);
        b.Tube(leg.hip,leg.knee,.0032f,.0023f,limbSides,SpiderPart::Leg,SpiderMaterial::Cuticle);
        // Point contact: no cylindrical cap extends below a foot planted on y=0.
        b.Tube(leg.knee,leg.foot,.0023f,0,limbSides,SpiderPart::Leg,SpiderMaterial::Cuticle);
    }
    for(float side:{-1.0f,1.0f}) {
        const SpiderPoint root{side*.008f,.029f,.037f},elbow{side*.014f,.023f,.049f};
        b.Tube(root,elbow,.0026f,.002f,limbSides,SpiderPart::Palp,SpiderMaterial::Cuticle);
        b.Tube(elbow,{side*.011f,.015f,.057f},.002f,.0007f,limbSides,SpiderPart::Palp,SpiderMaterial::Cuticle);
    }
    if(detailed) {
        // Eight small embedded eye volumes; four forward, four higher/lateral.
        for(float side:{-1.0f,1.0f})for(int i=0;i<4;++i) {
            const float x=side*(i%2==0?.004f:.010f),y=i<2?.040f:.047f;
            // Fit the centre ray to the actual polygon surface, not its enclosing
            // ellipsoid: millimetre-scale eyes would otherwise float above facets.
            float z=-1;
            for(std::size_t triangle=0;triangle<b.mesh.vertices.size();triangle+=3) {
                const auto& a=b.mesh.vertices[triangle];
                if(a.part!=SpiderPart::FrontBody)continue;
                const auto d=Sub(b.mesh.vertices[triangle+1].position,a.position);
                const auto e=Sub(b.mesh.vertices[triangle+2].position,a.position);
                const float determinant=d.x*e.y-d.y*e.x;
                if(std::abs(determinant)<1e-12f)continue;
                const float qx=x-a.position.x,qy=y-a.position.y;
                const float u=(qx*e.y-qy*e.x)/determinant,v=(d.x*qy-d.y*qx)/determinant;
                if(u>=-1e-5f&&v>=-1e-5f&&u+v<=1.00001f)
                    z=std::max(z,a.position.z+u*d.z+v*e.z);
            }
            if(z<0)throw std::runtime_error("Spider eye has no polygon support");
            b.Ellipsoid({x,y,z-.0006f},{.0017f,.0017f,.002f},4,6,SpiderPart::Eye,SpiderMaterial::Eye);
        }
    }
    return std::move(b.mesh);
}
}
