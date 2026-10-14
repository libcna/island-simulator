#include "IslandSimulator/SpiderRenderer.hpp"
#include "IslandSimulator/SpiderMesh.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <iostream>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
constexpr int Capacity=static_cast<int>(SpiderRenderer::MaximumVisibleCount*SpiderMesh::MaximumVertices);
Vector3 Point(SpiderPoint p){return {p.x,p.y,p.z};}
const std::array<Color,5> Materials{Color(220,211,195),Color(255,247,223),
    Color(12,10,8),Color(126,111,84),Color(87,71,51)};
}
SpiderRenderer::SpiderRenderer(GraphicsDevice& device,const CanopyLightField& canopy)
    :device_(device),canopy_(canopy),effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),Capacity,BufferUsage::WriteOnly)) {
    vertices_.reserve(Capacity);solid_.reserve(Capacity);
    constexpr int size=GeneratedTextureBudget::SpiderSetaeSize;
    const auto levels=LoadOpaqueImageMips(device_,"Content/wildlife/spider-setae.png",size);
    setae_=std::make_unique<Texture2D>(device_,size,size,true,SurfaceFormat::Color);
    for(std::size_t level=0;level<levels.size();++level) {
        const auto& pixels=levels[level].pixels;
        setae_->SetData(static_cast<int>(level),nullptr,reinterpret_cast<const std::uint8_t*>(pixels.data()),0,static_cast<int>(pixels.size()*4));
    }
    const char* message="island-simulator: spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes";
    std::cout<<message<<std::endl;MirrorRuntimeDiagnosticToPlatformLog(message);
    const char* eyeMessage="island-simulator: near spiders use tiered fitted eyes with upper-forward catchlights";
    std::cout<<eyeMessage<<std::endl;MirrorRuntimeDiagnosticToPlatformLog(eyeMessage);
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
    effect_->setTextureEnabledProperty(false);effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(12);effect_->setFogEndProperty(18);
}
SpiderRenderer::~SpiderRenderer()=default;
void SpiderRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,std::span<const SpiderState> spiders) {
    if(spiders.size()>SpiderHabitat::GroundCount)throw std::length_error("Spider population exceeds render capacity");
    vertices_.clear();solid_.clear();visible_=draws_=0;
    const BoundingFrustum frustum(view*projection);
    std::vector<std::pair<float,const SpiderState*>> candidates;
    candidates.reserve(spiders.size());
    for(const auto& spider:spiders) {
        const Vector3 centre(spider.x,spider.y+.04f,spider.z);
        const float distance=Vector3::DistanceSquared(centre,camera);
        if(distance>18*18||!frustum.Intersects(BoundingSphere(centre,.16f)))continue;
        candidates.emplace_back(distance,&spider);
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.first<b.first;});
    if(candidates.size()>SpiderRenderer::MaximumVisibleCount)candidates.resize(SpiderRenderer::MaximumVisibleCount);
    for(const auto& candidate:candidates) {
        const auto distance=candidate.first;
        const auto& spider=*candidate.second;
        const auto& pose=spider.pose;
        const auto mesh=BuildSpiderMesh(pose.legs,distance<4*4);
        for(const auto& vertex:mesh.vertices) {
            const auto p=Point(pose.ToWorld(vertex.position));
            const auto n=Vector3::Normalize(Point(pose.right)*vertex.normal.x+Point(pose.up)*vertex.normal.y+Point(pose.forward)*vertex.normal.z);
            const float light=.61f+.37f*std::max(0.0f,Vector3::Dot(n,wildlife_render::FixedSunDirection))+.10f*std::max(0.0f,n.Y);
            const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(p.X,p.Y,p.Z,spider.y));
            const auto color=Materials.at(static_cast<std::size_t>(vertex.material));
            const auto channel=[light](int value,float transmission){return static_cast<int>(std::clamp(std::lround(value*light*transmission),0L,255L));};
            auto& destination=vertex.material==SpiderMaterial::Eye
                ||vertex.material==SpiderMaterial::EyeHighlight
                ||vertex.material==SpiderMaterial::Hair?solid_:vertices_;
            destination.emplace_back(p,Color(channel(color.getRProperty(),tint.red),channel(color.getGProperty(),tint.green),channel(color.getBProperty(),tint.blue)),Vector2(vertex.u,vertex.v));
        }
        ++visible_;
    }
    if(vertices_.empty())return;
    const int texturedCount=static_cast<int>(vertices_.size());
    vertices_.insert(vertices_.end(),solid_.begin(),solid_.end());
    if(vertices_.size()>Capacity)throw std::length_error("Spider vertex upload exceeds capacity");
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);effect_->setProjectionProperty(projection);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;
    effect_->setTextureEnabledProperty(true);effect_->setTextureProperty(setae_.get());effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,texturedCount/3);draws_=1;
    if(!solid_.empty()) {
        effect_->setTextureEnabledProperty(false);effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList,texturedCount,static_cast<int>(solid_.size()/3));++draws_;
    }
}
}
