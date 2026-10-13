#include "IslandSimulator/SpiderWebRenderer.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island {
namespace {
constexpr int Capacity=static_cast<int>(SpiderWebLayout::MaximumSites
    *SpiderWebGeometry::MaximumStrands*6U);
constexpr float MaximumDistance=48.0f;

Vector3 Point(BranchPoint p){return {p[0],p[1],p[2]};}

void AddSilkRibbon(std::vector<VertexPositionColor>& vertices,Vector3 from,Vector3 to,
    const Vector3& camera,WebStrandKind kind,std::size_t identity) {
    const Vector3 delta=to-from;
    const float length=delta.Length();
    if(!std::isfinite(length)||length<1e-5f)return;
    const Vector3 direction=delta/length;
    const Vector3 middle=(from+to)*.5f;
    Vector3 side=Vector3::Cross(direction,camera-middle);
    if(side.LengthSquared()<1e-8f)side=Vector3::Cross(direction,Vector3::Up);
    if(side.LengthSquared()<1e-8f)side=Vector3::Cross(direction,Vector3::Right);
    side.Normalize();
    const float halfWidth=kind==WebStrandKind::Frame?.0026f:
        kind==WebStrandKind::Spiral?.00175f:.00145f;
    side*=halfWidth;
    const Vector3 toCamera=camera-middle;
    const Vector3 viewDirection=toCamera.LengthSquared()>1e-8f
        ?Vector3::Normalize(toCamera):Vector3::Up;
    const float facing=std::clamp(1.0f-std::abs(Vector3::Dot(direction,viewDirection)),0.0f,1.0f);
    const float glint=.72f+.28f*std::sin(static_cast<float>(identity)*1.618f
        +middle.X*.19f+middle.Y*.37f+middle.Z*.13f);
    const float base=kind==WebStrandKind::Frame?112.0f:kind==WebStrandKind::Spiral?104.0f:88.0f;
    const int alpha=static_cast<int>(std::clamp(base*(.60f+.40f*facing)*glint,38.0f,126.0f));
    const Color silk=Color::FromNonPremultiplied(226,235,224,alpha);
    vertices.emplace_back(from-side,silk);vertices.emplace_back(from+side,silk);
    vertices.emplace_back(to+side,silk);vertices.emplace_back(from-side,silk);
    vertices.emplace_back(to+side,silk);vertices.emplace_back(to-side,silk);
}
}

SpiderWebRenderer::SpiderWebRenderer(GraphicsDevice& device,const IslandTerrain& terrain,
    const JungleLayout& jungle,std::span<const SpiderWebSite> sites)
    :device_(device),jungle_(jungle),surface_(terrain),sites_(sites.begin(),sites.end()),
     effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,
        VertexPositionColor::getVertexDeclarationStatic(),Capacity,BufferUsage::WriteOnly)) {
    if(sites_.size()>SpiderWebLayout::MaximumSites)
        throw std::length_error("Spider web site count exceeds renderer capacity");
    geometry_.reserve(sites_.size());
    for(const auto& site:sites_)geometry_.emplace_back(
        SpiderWebLayout::Sample(site,jungle_,surface_,0));
    vertices_.reserve(Capacity);
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
    effect_->setTextureEnabledProperty(false);effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,
        environment::FogColorBlue));
    effect_->setFogStartProperty(24);effect_->setFogEndProperty(MaximumDistance);
}

SpiderWebRenderer::~SpiderWebRenderer()=default;

void SpiderWebRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,
    float timeSeconds) {
    vertices_.clear();visible_=strands_=draws_=0;
    const float time=std::isfinite(timeSeconds)?timeSeconds:0.0f;
    const BoundingFrustum frustum(view*projection);
    for(std::size_t siteIndex=0;siteIndex<sites_.size();++siteIndex) {
        const auto anchors=SpiderWebLayout::Sample(sites_[siteIndex],jungle_,surface_,time);
        const Vector3 centre=(Point(anchors[0])+Point(anchors[1])+Point(anchors[2]))/3.0f;
        float radius=0;
        for(const auto anchor:anchors)radius=std::max(radius,Vector3::Distance(centre,Point(anchor)));
        if(Vector3::DistanceSquared(centre,camera)>MaximumDistance*MaximumDistance
            ||!frustum.Intersects(BoundingSphere(centre,radius+.03f)))continue;
        const auto nodes=geometry_[siteIndex].Sample(anchors);
        const auto& strands=geometry_[siteIndex].Strands();
        for(std::size_t strandIndex=0;strandIndex<strands.size();++strandIndex) {
            const auto& strand=strands[strandIndex];
            AddSilkRibbon(vertices_,Point(nodes[strand.from]),Point(nodes[strand.to]),camera,
                strand.kind,siteIndex*SpiderWebGeometry::MaximumStrands+strandIndex);
        }
        ++visible_;strands_+=static_cast<int>(strands.size());
    }
    if(vertices_.empty())return;
    if(vertices_.size()>static_cast<std::size_t>(Capacity))
        throw std::length_error("Spider web vertex upload exceeds capacity");
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::AlphaBlend);
    device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
    effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,static_cast<int>(vertices_.size()/3U));
    draws_=1;
}

}
