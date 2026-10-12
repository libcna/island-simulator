#include "IslandSimulator/SpiderRenderer.hpp"
#include "IslandSimulator/SpiderMesh.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
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
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
constexpr int Capacity=static_cast<int>(SpiderHabitat::GroundCount*SpiderMesh::MaximumVertices);
Vector3 Point(SpiderPoint p){return {p.x,p.y,p.z};}
const std::array<Color,4> Materials{Color(100,77,51),Color(169,139,93),Color(15,13,11),Color(87,71,51)};
}
SpiderRenderer::SpiderRenderer(GraphicsDevice& device,const CanopyLightField& canopy)
    :device_(device),canopy_(canopy),effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),Capacity,BufferUsage::WriteOnly)) {
    vertices_.reserve(Capacity);
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
    effect_->setTextureEnabledProperty(false);effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(12);effect_->setFogEndProperty(18);
}
SpiderRenderer::~SpiderRenderer()=default;
void SpiderRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,std::span<const SpiderState> spiders) {
    if(spiders.size()>SpiderHabitat::GroundCount)throw std::length_error("Spider population exceeds render capacity");
    vertices_.clear();visible_=draws_=0;
    const BoundingFrustum frustum(view*projection);
    for(const auto& spider:spiders) {
        const Vector3 centre(spider.x,spider.y+.04f,spider.z);
        const float distance=Vector3::DistanceSquared(centre,camera);
        if(distance>18*18||!frustum.Intersects(BoundingSphere(centre,.16f)))continue;
        const auto& pose=spider.pose;
        const auto mesh=BuildSpiderMesh(pose.legs,distance<4*4);
        for(const auto& vertex:mesh.vertices) {
            const auto p=Point(pose.ToWorld(vertex.position));
            const auto n=Vector3::Normalize(Point(pose.right)*vertex.normal.x+Point(pose.up)*vertex.normal.y+Point(pose.forward)*vertex.normal.z);
            const float light=.61f+.37f*std::max(0.0f,Vector3::Dot(n,wildlife_render::FixedSunDirection))+.10f*std::max(0.0f,n.Y);
            const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(p.X,p.Y,p.Z,spider.y));
            const auto color=Materials.at(static_cast<std::size_t>(vertex.material));
            const auto channel=[light](int value,float transmission){return static_cast<int>(std::clamp(std::lround(value*light*transmission),0L,255L));};
            vertices_.emplace_back(p,Color(channel(color.getRProperty(),tint.red),channel(color.getGProperty(),tint.green),channel(color.getBProperty(),tint.blue)));
        }
        ++visible_;
    }
    if(vertices_.empty())return;
    if(vertices_.size()>Capacity)throw std::length_error("Spider vertex upload exceeds capacity");
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,static_cast<int>(vertices_.size()/3));draws_=1;
}
}
