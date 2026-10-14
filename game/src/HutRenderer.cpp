#include "IslandSimulator/HutRenderer.hpp"
#include "IslandSimulator/HutThatch.hpp"
#include "IslandSimulator/HutMaterial.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
HutRenderer::HutRenderer(GraphicsDevice& device,const HutPlacement& hut)
    :device_(device),effect_(std::make_unique<BasicEffect>(device)),centre_(hut.x,hut.floorY,hut.z)
{
    std::vector<VertexPositionColorTexture> mesh;
    Vector3 grainOrigin,grainAcross,grainAlong;
    int materialTag=64;
    const float c=std::cos(hut.yawRadians),s=std::sin(hut.yawRadians);
    const auto world=[&](Vector3 p) { return Vector3(hut.x+c*p.X+s*p.Z,hut.floorY+p.Y,hut.z-s*p.X+c*p.Z); };
    const auto quad=[&](Vector3 a,Vector3 b,Vector3 d,Vector3 e,Vector3 pigment) {
        const std::array<Vector3,6> local{a,b,d,a,d,e};
        a=world(a); b=world(b); d=world(d); e=world(e);
        Vector3 normal=Vector3::Cross(b-a,d-a); normal.Normalize();
        const float light=0.48f+0.52f*std::max(0.0f,Vector3::Dot(normal,
            Vector3(environment::SunDirectionX,environment::SunDirectionY,environment::SunDirectionZ)));
        const Color color(static_cast<int>(pigment.X*light),static_cast<int>(pigment.Y*light),static_cast<int>(pigment.Z*light),materialTag);
        for(const auto p:local) {
            const float phase=materialTag==128?0:Vector3::Dot(grainOrigin,Vector3(0.37f,0.19f,0.53f));
            Vector2 uv(Vector3::Dot(p-grainOrigin,grainAcross)+phase,
                Vector3::Dot(p-grainOrigin,grainAlong));
            if(materialTag==224) {
                // Interior atlas windows: no generated seamless-edge assumption.
                const float offset=phase-std::floor(phase);
                uv=Vector2(0.15f+0.7f*offset+Vector3::Dot(p-grainOrigin,grainAcross)*0.8f,
                    0.12f+Vector3::Dot(p-grainOrigin,grainAlong)*0.9f);
            }
            mesh.emplace_back(world(p),color,uv);
        }
    };
    const auto beam=[&](Vector3 a,Vector3 b,float width,float depth,Vector3 pigment) {
        Vector3 along=b-a; along.Normalize();
        Vector3 across=Vector3::Cross(along,std::abs(along.Y)>0.9f?Vector3(0,0,1):Vector3(0,1,0)); across.Normalize();
        Vector3 up=Vector3::Cross(across,along); up.Normalize();
        grainOrigin=a; grainAcross=across; grainAlong=up; materialTag=128;
        const auto r=across*(width/2),t=up*(depth/2);
        const std::array<Vector3,8> p{a-r-t,a+r-t,a+r+t,a-r+t,b-r-t,b+r-t,b+r+t,b-r+t};
        quad(p[0],p[3],p[2],p[1],pigment); quad(p[4],p[5],p[6],p[7],pigment);
        grainAcross=across+up; grainAlong=along; materialTag=64;
        quad(p[0],p[1],p[5],p[4],pigment); quad(p[1],p[2],p[6],p[5],pigment);
        quad(p[2],p[3],p[7],p[6],pigment); quad(p[3],p[0],p[4],p[7],pigment);
    };
    const Vector3 timber(131,100,65),dark(89,67,45);
    // Level deck with individually separated weathered boards.
    for(int i=0;i<24;++i) {
        const float x=-2.1f+(static_cast<float>(i)+0.5f)*4.2f/24;
        beam({x,-0.07f,-1.7f},{x,-0.07f,2.3f},0.166f,0.14f,
            timber+Vector3(static_cast<float>(i%5)*3));
    }
    std::size_t foot=0;
    for(float x:{-2.1f,2.1f}) for(float z:{-1.7f,0.0f,1.7f}) {
        const float bottom=hut.footings[foot++].groundY-hut.floorY-0.08f;
        beam({x,bottom,z},{x,2.25f,z},0.16f,0.16f,dark);
        if(z!=0) beam({x,1.45f,z},{x-std::copysign(0.6f,x),2.25f,z},0.095f,0.095f,timber);
    }
    for(float x:{-2.1f,2.1f}) beam({x,2.2f,-1.8f},{x,2.2f,1.8f},0.16f,0.16f,dark);
    for(float z:{-1.7f,1.7f}) beam({-2.18f,2.2f,z},{2.18f,2.2f,z},0.16f,0.16f,dark);
    // Slatted rear wall and a front facade with a framed central door and two windows.
    for(int i=0;i<20;++i) {
        const float x=-2.0f+static_cast<float>(i)*4.0f/19;
        beam({x,0.08f,-1.7f},{x,1.8f,-1.7f},0.18f,0.055f,timber+Vector3(static_cast<float>(i%4)*5));
        const float ax=std::abs(x);
        if(ax<0.53f) continue;
        if(ax>0.82f&&ax<1.72f) {
            beam({x,0.08f,1.7f},{x,0.55f,1.7f},0.16f,0.055f,timber);
            beam({x,1.35f,1.7f},{x,1.8f,1.7f},0.16f,0.055f,timber);
        } else beam({x,0.08f,1.7f},{x,1.8f,1.7f},0.16f,0.055f,timber);
    }
    for(float x:{-2.1f,2.1f}) for(int i=0;i<16;++i) {
        const float z=-1.6f+static_cast<float>(i)*3.2f/15;
        beam({x,0.08f,z},{x,1.8f,z},0.18f,0.055f,timber+Vector3(static_cast<float>(i%3)*5));
    }
    // Muted opaque glazing gives the facade readable windows without a sorted
    // transparency pass. Timber mullions sit slightly forward of the glass.
    grainAcross={1,0,0};grainAlong={0,1,0};materialTag=176;
    for(float x:{-1.26f,1.26f}) {
        grainOrigin={x-.43f,.55f,1.704f};
        quad({x-.43f,.55f,1.704f},{x+.43f,.55f,1.704f},
             {x+.43f,1.35f,1.704f},{x-.43f,1.35f,1.704f},{52,73,73});
        beam({x-.47f,.50f,1.73f},{x-.47f,1.40f,1.73f},.09f,.09f,dark);
        beam({x+.47f,.50f,1.73f},{x+.47f,1.40f,1.73f},.09f,.09f,dark);
        beam({x-.51f,.50f,1.73f},{x+.51f,.50f,1.73f},.09f,.09f,dark);
        beam({x-.51f,1.40f,1.73f},{x+.51f,1.40f,1.73f},.09f,.09f,dark);
        beam({x,.53f,1.79f},{x,1.37f,1.79f},.055f,.055f,dark);
        beam({x-.44f,.95f,1.79f},{x+.44f,.95f,1.79f},.055f,.055f,dark);
    }
    // A closed plank door suits the decorative, uninhabited building contract.
    for(int i=0;i<5;++i) {
        const float x=-.40f+static_cast<float>(i)*.20f;
        beam({x,.08f,1.72f},{x,1.68f,1.72f},.17f,.065f,dark+Vector3(15+3*i,11+2*i,6+i));
    }
    beam({-.54f,.04f,1.76f},{-.54f,1.78f,1.76f},.11f,.11f,dark);
    beam({ .54f,.04f,1.76f},{ .54f,1.78f,1.76f},.11f,.11f,dark);
    beam({-.59f,1.78f,1.76f},{.59f,1.78f,1.76f},.11f,.11f,dark);
    beam({-.39f,.30f,1.80f},{.39f,1.48f,1.80f},.065f,.055f,timber);
    beam({.31f,.82f,1.81f},{.31f,.82f,1.94f},.045f,.045f,Vector3(65,52,38));
    // Covered front veranda.  Tall outer posts and a shadowed fascia make the
    // platform read as part of the raised structure rather than three isolated
    // steps stuck onto the facade.
    beam({-2.10f,-.14f,2.25f},{2.10f,-.14f,2.25f},.12f,.16f,dark);
    for(float x:{-2.04f,2.04f})
        beam({x,-.10f,2.24f},{x,2.16f,2.24f},.14f,.14f,dark);
    for(const auto& span:{std::pair{-2.04f,-.72f},std::pair{.72f,2.04f}})
        beam({span.first,.86f,2.24f},{span.second,.86f,2.24f},.09f,.09f,dark);
    for(const auto& span:{std::pair{-2.00f,-.76f},std::pair{.76f,2.00f}})
        beam({span.first,.31f,2.24f},{span.second,.31f,2.24f},.065f,.065f,timber);
    for(float x:{-1.86f,-1.46f,-1.06f,1.06f,1.46f,1.86f})
        beam({x,.02f,2.24f},{x,.90f,2.24f},.07f,.07f,timber);
    for(float x:{-2.04f,2.04f}) {
        beam({x,.86f,1.76f},{x,.86f,2.24f},.09f,.09f,dark);
        beam({x,.02f,2.00f},{x,.90f,2.00f},.07f,.07f,timber);
    }
    beam({-.76f,-.16f,2.28f},{.76f,-.16f,2.28f},.34f,.14f,timber);
    beam({-.76f,-.31f,2.56f},{.76f,-.31f,2.56f},.34f,.14f,timber);
    beam({-.76f,-.46f,2.84f},{.76f,-.46f,2.84f},.34f,.14f,timber);
    // Close the front gable with individually varied vertical boards. The structural
    // frame remains slightly forward, so this reads as a timber house rather than an
    // unfinished open shelter while preserving the broad reference-house gable.
    for(int i=0;i<16;++i) {
        const float x=-1.76f+static_cast<float>(i)*3.52f/15;
        const float top=3.45f-.60f*std::abs(x);
        beam({x,2.23f,1.70f},{x,top,1.70f},.19f,.055f,
             timber+Vector3(static_cast<float>(i%4)*4));
    }
    // The front gable exposes the load-bearing frame seen in the reference house.
    beam({-2.08f,2.20f,1.74f},{0,3.52f,1.74f},.10f,.10f,timber);
    beam({ 2.08f,2.20f,1.74f},{0,3.52f,1.74f},.10f,.10f,timber);
    beam({-2.06f,2.20f,1.74f},{2.06f,2.20f,1.74f},.10f,.10f,dark);
    beam({0,2.20f,1.74f},{0,3.48f,1.74f},.09f,.09f,timber);
    // A small recessed attic light is present in the supplied reference. Muted glass
    // and a projecting timber frame break the broad gable without implying an interior.
    grainOrigin={-.28f,2.68f,1.79f}; grainAcross={1,0,0}; grainAlong={0,1,0}; materialTag=176;
    quad({-.28f,2.68f,1.79f},{.28f,2.68f,1.79f},{.28f,3.08f,1.79f},{-.28f,3.08f,1.79f},
         {48,67,67});
    const Vector3 paleTimber(176,139,91);
    beam({-.33f,2.63f,1.82f},{-.33f,3.13f,1.82f},.075f,.075f,paleTimber);
    beam({ .33f,2.63f,1.82f},{ .33f,3.13f,1.82f},.075f,.075f,paleTimber);
    beam({-.36f,2.63f,1.82f},{ .36f,2.63f,1.82f},.075f,.075f,paleTimber);
    beam({-.36f,3.13f,1.82f},{ .36f,3.13f,1.82f},.075f,.075f,paleTimber);
    beam({0,2.68f,1.825f},{0,3.08f,1.825f},.045f,.045f,paleTimber);
    beam({-.28f,2.88f,1.825f},{.28f,2.88f,1.825f},.045f,.045f,paleTimber);
    // Four short knee braces tie the veranda posts into the front header instead of
    // leaving two visually unsupported vertical poles below the roof.
    beam({-2.04f,1.72f,2.24f},{-1.48f,2.18f,1.78f},.075f,.075f,timber);
    beam({ 2.04f,1.72f,2.24f},{ 1.48f,2.18f,1.78f},.075f,.075f,timber);
    beam({-2.04f,1.72f,2.24f},{-2.04f,2.18f,1.78f},.075f,.075f,dark);
    beam({ 2.04f,1.72f,2.24f},{ 2.04f,2.18f,1.78f},.075f,.075f,dark);
    // Exposed rafters and overlapping narrow thatch bundles, not a single roof pyramid.
    beam({0,3.55f,-2.3f},{0,3.55f,2.3f},0.16f,0.16f,dark);
    for(float side:{-1.0f,1.0f}) {
        for(int i=0;i<7;++i) {
            const float z=-2.1f+static_cast<float>(i)*0.7f;
            beam({0,3.5f,z},{side*2.65f,2.12f,z},0.10f,0.10f,timber);
        }
        for(int row=0;row<7;++row) for(int strip=0;strip<48;++strip) {
            const auto blade=BuildHutThatchBlade(static_cast<int>(side),row,strip);
            const auto origin=blade.sections[0][1];
            grainOrigin=Vector3(origin.x,origin.y,origin.z);
            grainAcross=Vector3(0,0,1); grainAlong=Vector3(side*2.65f,-1.4f,0);
            grainAlong.Normalize(); materialTag=224;
            const Vector3 pigment=Vector3(177,163,131)+Vector3(blade.pigment*16,blade.pigment*14,blade.pigment*9);
            const auto point=[&](int station,int edge) {
                const auto p=blade.sections[static_cast<std::size_t>(station)][static_cast<std::size_t>(edge)];
                return Vector3(p.x,p.y,p.z);
            };
            for(int station=0;station<3;++station) for(int edge=0;edge<2;++edge) {
                const auto a=point(station,edge),b=point(station,edge+1),
                    d=point(station+1,edge+1),e=point(station+1,edge);
                if(side>0) quad(a,b,d,e,pigment);
                else quad(b,a,e,d,pigment);
            }
        }
    }
    if(mesh.empty()||mesh.size()>31800) throw std::runtime_error("Hut mesh exceeds budget");
    Vector3 low(std::numeric_limits<float>::max()),high(std::numeric_limits<float>::lowest());
    for(const auto& vertex:mesh) {
        if(!std::isfinite(vertex.TextureCoordinate.X)||!std::isfinite(vertex.TextureCoordinate.Y))
            throw std::runtime_error("Nonfinite hut grain coordinate");
        if(!std::isfinite(vertex.Position.X)||!std::isfinite(vertex.Position.Y)||!std::isfinite(vertex.Position.Z))
            throw std::runtime_error("Nonfinite hut vertex");
        low=Vector3::Min(low,vertex.Position); high=Vector3::Max(high,vertex.Position);
    }
    for(std::size_t i=0;i<mesh.size();i+=3)
        if(Vector3::Cross(mesh[i+1].Position-mesh[i].Position,mesh[i+2].Position-mesh[i].Position).LengthSquared()<1e-12f)
            throw std::runtime_error("Degenerate hut triangle");
    bounds_=BoundingBox(low,high); triangles_=static_cast<int>(mesh.size()/3);
    vertices_=std::make_unique<VertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),static_cast<int>(mesh.size()),BufferUsage::WriteOnly);
    vertices_->SetData(mesh.data(),static_cast<int>(mesh.size()));
    effect_->setVertexColorEnabledProperty(true); effect_->setLightingEnabledProperty(false);
    effect_->setFogEnabledProperty(true); effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(180); effect_->setFogEndProperty(300);
    const auto levels=LoadOpaqueImageMips(device_,"Content/structures/hut-palm-fibre.png",256);
    const auto& mean=levels.back().pixels[0];
    thatchMean_=Vector3(mean[0]/255.0f,mean[1]/255.0f,mean[2]/255.0f);
    if(std::min({thatchMean_.X,thatchMean_.Y,thatchMean_.Z})<0.01f)
        throw std::runtime_error("Hut thatch texture mean too dark");
    thatch_=std::make_unique<Texture2D>(device_,256,256,true,SurfaceFormat::Color);
    for(std::size_t level=0;level<levels.size();++level) {
        const auto& pixels=levels[level].pixels;
        thatch_->SetData(static_cast<int>(level),nullptr,
            reinterpret_cast<const std::uint8_t*>(pixels.data()),0,static_cast<int>(pixels.size()*4));
    }
    const char* textureMessage="island-simulator: hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes";
    std::cout<<textureMessage<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(textureMessage);
    try {
        if(!device.SupportsCapability(CNA::GraphicsCapability::CustomEffects))
            throw std::runtime_error("custom effects unavailable");
        using CNA::Graphics::ShaderCodeEXT;
        const auto es=[](const char* body){return std::string("#version 300 es\nprecision highp float;\n")+body;};
        const auto desktop=[](const char* body){return std::string("#version 330 core\n")+body;};
        const CNA::Graphics::ShaderPackageEXT package({
            ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Vertex,"main","hut/es.vert",es(HutMaterialVertex)),
            ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Fragment,"main","hut/es.frag",es(HutMaterialFragment)),
            ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Vertex,"main","hut/gl.vert",desktop(HutMaterialVertex)),
            ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Fragment,"main","hut/gl.frag",desktop(HutMaterialFragment))},
            {CNA::ShaderStageEXT::Vertex,CNA::ShaderStageEXT::Fragment});
        material_=std::make_unique<ShaderEffect>(device,package);
        if(!material_->IsEffectValid()) throw std::runtime_error(material_->GetCompileErrorEXT());
        const char* message="island-simulator: hut beam-aligned weathered grain and filtered thatch fibres";
        std::cout<<message<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(message);
        const char* timberMessage="island-simulator: hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation";
        std::cout<<timberMessage<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(timberMessage);
        const char* facadeMessage="island-simulator: hut four-pane muted glazing and braced plank door";
        std::cout<<facadeMessage<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(facadeMessage);
        const char* verandaMessage="island-simulator: hut raised veranda uses full-height outer posts, split rails, fascia, and three steps";
        std::cout<<verandaMessage<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(verandaMessage);
        const char* structureMessage="island-simulator: hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports";
        std::cout<<structureMessage<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(structureMessage);
    } catch(const std::exception& error) {
        material_.reset();
        const std::string message=std::string("island-simulator: hut material fallback: ")+error.what();
        std::cerr<<message<<'\n'; MirrorRuntimeDiagnosticToPlatformLog(message);
    }
    MirrorRuntimeDiagnosticToPlatformLog("island-simulator: hut timber frame and layered thatch, triangles="+std::to_string(triangles_));
}
HutRenderer::~HutRenderer()=default;
void HutRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera) {
    drawCalls_=0;
    if((centre_-camera).LengthSquared()>300*300 || BoundingFrustum(view*projection).Contains(bounds_)==ContainmentType::Disjoint) return;
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque); device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.SetVertexBuffer(vertices_.get()); device_.setIndicesProperty(nullptr);
    if(material_) {
        float matrix[16]; (view*projection).ToColumnMajor(matrix);
        material_->Apply(); material_->SetUniformMat4("uViewProjection",matrix);
        device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;
        material_->SetTexture(0,*thatch_); material_->SetUniformInt("uThatchTexture",0);
        material_->SetUniformVec3("uThatchMean",thatchMean_.X,thatchMean_.Y,thatchMean_.Z);
        material_->SetUniformVec3("uCameraPosition",camera.X,camera.Y,camera.Z);
        material_->SetUniformVec3("uHutCentre",centre_.X,centre_.Y,centre_.Z);
    } else {
        effect_->setWorldProperty(Matrix::getIdentityProperty()); effect_->setViewProperty(view); effect_->setProjectionProperty(projection); effect_->Apply();
    }
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_); drawCalls_=1;
}
}
