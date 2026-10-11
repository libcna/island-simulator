#include "IslandSimulator/HutRenderer.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
HutRenderer::HutRenderer(GraphicsDevice& device,const HutPlacement& hut)
    :device_(device),effect_(std::make_unique<BasicEffect>(device)),centre_(hut.x,hut.floorY,hut.z)
{
    std::vector<VertexPositionColor> mesh;
    const float c=std::cos(hut.yawRadians),s=std::sin(hut.yawRadians);
    const auto world=[&](Vector3 p) { return Vector3(hut.x+c*p.X+s*p.Z,hut.floorY+p.Y,hut.z-s*p.X+c*p.Z); };
    const auto quad=[&](Vector3 a,Vector3 b,Vector3 d,Vector3 e,Vector3 pigment) {
        a=world(a); b=world(b); d=world(d); e=world(e);
        Vector3 normal=Vector3::Cross(b-a,d-a); normal.Normalize();
        const float light=0.48f+0.52f*std::max(0.0f,Vector3::Dot(normal,
            Vector3(environment::SunDirectionX,environment::SunDirectionY,environment::SunDirectionZ)));
        const Color color(static_cast<int>(pigment.X*light),static_cast<int>(pigment.Y*light),static_cast<int>(pigment.Z*light));
        for(auto p:{a,b,d,a,d,e}) mesh.emplace_back(p,color);
    };
    const auto beam=[&](Vector3 a,Vector3 b,float width,float depth,Vector3 pigment) {
        Vector3 along=b-a; along.Normalize();
        Vector3 across=Vector3::Cross(along,std::abs(along.Y)>0.9f?Vector3(0,0,1):Vector3(0,1,0)); across.Normalize();
        Vector3 up=Vector3::Cross(across,along); up.Normalize();
        const auto r=across*(width/2),t=up*(depth/2);
        const std::array<Vector3,8> p{a-r-t,a+r-t,a+r+t,a-r+t,b-r-t,b+r-t,b+r+t,b-r+t};
        quad(p[0],p[3],p[2],p[1],pigment); quad(p[4],p[5],p[6],p[7],pigment);
        quad(p[0],p[1],p[5],p[4],pigment); quad(p[1],p[2],p[6],p[5],pigment);
        quad(p[2],p[3],p[7],p[6],pigment); quad(p[3],p[0],p[4],p[7],pigment);
    };
    const Vector3 timber(131,100,65),dark(89,67,45);
    // Level deck with individually separated weathered boards.
    for(int i=0;i<24;++i) {
        const float x=-2.1f+(static_cast<float>(i)+0.5f)*4.2f/24;
        beam({x,-0.07f,-1.7f},{x,-0.07f,1.7f},0.166f,0.14f,timber+Vector3(static_cast<float>(i%5)*3));
    }
    std::size_t foot=0;
    for(float x:{-2.1f,2.1f}) for(float z:{-1.7f,0.0f,1.7f}) {
        const float bottom=hut.footings[foot++].groundY-hut.floorY-0.08f;
        beam({x,bottom,z},{x,2.25f,z},0.16f,0.16f,dark);
        if(z!=0) beam({x,1.45f,z},{x-std::copysign(0.6f,x),2.25f,z},0.095f,0.095f,timber);
    }
    for(float x:{-2.1f,2.1f}) beam({x,2.2f,-1.8f},{x,2.2f,1.8f},0.16f,0.16f,dark);
    for(float z:{-1.7f,1.7f}) beam({-2.18f,2.2f,z},{2.18f,2.2f,z},0.16f,0.16f,dark);
    // Slatted side/rear walls; front opening, porch rail and a low sill remain readable.
    for(int i=0;i<20;++i) {
        const float x=-2.0f+static_cast<float>(i)*4.0f/19;
        beam({x,0.08f,-1.7f},{x,1.8f,-1.7f},0.18f,0.055f,timber+Vector3(static_cast<float>(i%4)*5));
        if(std::abs(x)>0.65f) beam({x,0.08f,1.7f},{x,0.95f,1.7f},0.16f,0.055f,timber);
    }
    for(float x:{-2.1f,2.1f}) for(int i=0;i<16;++i) {
        const float z=-1.6f+static_cast<float>(i)*3.2f/15;
        beam({x,0.08f,z},{x,1.8f,z},0.18f,0.055f,timber+Vector3(static_cast<float>(i%3)*5));
    }
    // Exposed rafters and overlapping narrow thatch bundles, not a single roof pyramid.
    beam({0,3.55f,-2.3f},{0,3.55f,2.3f},0.16f,0.16f,dark);
    for(float side:{-1.0f,1.0f}) {
        for(int i=0;i<7;++i) {
            const float z=-2.1f+static_cast<float>(i)*0.7f;
            beam({0,3.5f,z},{side*2.65f,2.12f,z},0.10f,0.10f,timber);
        }
        for(int row=0;row<7;++row) for(int strip=0;strip<32;++strip) {
            const float u=static_cast<float>(row)/7, v=std::min(1.0f,u+0.19f);
            const float z=-2.25f+static_cast<float>(strip)*4.5f/32;
            const float rag=static_cast<float>((strip*7+row*3)%5)*0.008f;
            const float y=0.035f*static_cast<float>(7-row);
            beam({side*2.65f*u,3.55f-1.4f*u+y,z},
                 {side*(2.65f*v+rag),3.55f-1.4f*v+y,z},0.137f,0.045f,
                 Vector3(153,132,80)+Vector3(static_cast<float>((strip+row)%5)*4));
        }
    }
    if(mesh.empty()||mesh.size()>30000) throw std::runtime_error("Hut mesh exceeds budget");
    Vector3 low(std::numeric_limits<float>::max()),high(std::numeric_limits<float>::lowest());
    for(const auto& vertex:mesh) {
        if(!std::isfinite(vertex.Position.X)||!std::isfinite(vertex.Position.Y)||!std::isfinite(vertex.Position.Z))
            throw std::runtime_error("Nonfinite hut vertex");
        low=Vector3::Min(low,vertex.Position); high=Vector3::Max(high,vertex.Position);
    }
    for(std::size_t i=0;i<mesh.size();i+=3)
        if(Vector3::Cross(mesh[i+1].Position-mesh[i].Position,mesh[i+2].Position-mesh[i].Position).LengthSquared()<1e-12f)
            throw std::runtime_error("Degenerate hut triangle");
    bounds_=BoundingBox(low,high); triangles_=static_cast<int>(mesh.size()/3);
    vertices_=std::make_unique<VertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),static_cast<int>(mesh.size()),BufferUsage::WriteOnly);
    vertices_->SetData(mesh.data(),static_cast<int>(mesh.size()));
    effect_->setVertexColorEnabledProperty(true); effect_->setLightingEnabledProperty(false);
    effect_->setFogEnabledProperty(true); effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(180); effect_->setFogEndProperty(300);
    MirrorRuntimeDiagnosticToPlatformLog("island-simulator: hut timber frame and layered thatch, triangles="+std::to_string(triangles_));
}
HutRenderer::~HutRenderer()=default;
void HutRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera) {
    drawCalls_=0;
    if((centre_-camera).LengthSquared()>300*300 || BoundingFrustum(view*projection).Contains(bounds_)==ContainmentType::Disjoint) return;
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque); device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.SetVertexBuffer(vertices_.get()); device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty()); effect_->setViewProperty(view); effect_->setProjectionProperty(projection); effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_); drawCalls_=1;
}
}
