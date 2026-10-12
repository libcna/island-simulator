#include "IslandSimulator/GoatRenderer.hpp"
#include "IslandSimulator/GoatMesh.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
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
constexpr int Capacity=static_cast<int>(GoatHabitat::Count*GoatMesh::MaximumVertices);
Vector3 Point(GoatPoint p){return {p.x,p.y,p.z};}
// Temporary neutral materials. A real goat coat remains a separate visual task.
const std::array<Color,6> Materials{Color(184,165,134),Color(145,116,92),
    Color(108,91,66),Color(58,54,45),Color(18,16,13),Color(63,57,48)};
}
GoatRenderer::GoatRenderer(GraphicsDevice& device,const IslandTerrain& terrain,
    const CanopyLightField& canopy,float maximumDistance)
    :device_(device),terrain_(terrain),surface_(terrain),canopy_(canopy),
     maximumDistance_(maximumDistance),effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),
         Capacity,BufferUsage::WriteOnly))
{
    vertices_.reserve(Capacity);
    effect_->setVertexColorEnabledProperty(true);
    effect_->setLightingEnabledProperty(false);
    effect_->setTextureEnabledProperty(false);
    effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(maximumDistance_*0.65f);
    effect_->setFogEndProperty(maximumDistance_);
}
GoatRenderer::~GoatRenderer()=default;
void GoatRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,
    std::span<const GoatState> goats)
{
    if(goats.size()>GoatHabitat::Count)throw std::length_error("Goat population exceeds render capacity");
    vertices_.clear();visible_=draws_=0;
    const BoundingFrustum frustum(view*projection);
    for(const auto& goat:goats) {
        const Vector3 centre(goat.x,goat.y+.75f*goat.scale,goat.z);
        const float distanceSquared=Vector3::DistanceSquared(centre,camera);
        if(distanceSquared>maximumDistance_*maximumDistance_
            ||!frustum.Intersects(BoundingSphere(centre,1.8f*goat.scale)))continue;
        const auto pose=FitGoatGroundPose(terrain_,surface_,goat.x,goat.z,goat.heading,
            goat.phase,goat.speed,goat.scale);
        const auto mesh=BuildGoatMesh(pose,goat.grazingPose,distanceSquared<30*30);
        for(const auto& vertex:mesh.vertices) {
            const auto world=Point(pose.ToWorld(vertex.position));
            const auto n=Vector3::Normalize(Point(pose.right)*vertex.normal.x
                +Point(pose.up)*vertex.normal.y+Point(pose.forward)*vertex.normal.z);
            const float light=.61f+.37f*std::max(0.0f,Vector3::Dot(n,wildlife_render::FixedSunDirection))
                +.10f*std::max(0.0f,n.Y);
            const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(world.X,world.Y,world.Z,goat.y));
            const auto color=Materials.at(static_cast<std::size_t>(vertex.material));
            const auto channel=[light](int value,float transmission){
                return static_cast<int>(std::clamp(std::lround(value*light*transmission),0L,255L));};
            vertices_.emplace_back(world,Color(channel(color.getRProperty(),tint.red),
                channel(color.getGProperty(),tint.green),channel(color.getBProperty(),tint.blue)));
        }
        ++visible_;
    }
    if(vertices_.empty())return;
    if(vertices_.size()>Capacity)throw std::length_error("Goat vertex upload exceeds capacity");
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
    effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,static_cast<int>(vertices_.size()/3));draws_=1;
}
}
