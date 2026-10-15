#include "IslandSimulator/GoatRenderer.hpp"
#include "IslandSimulator/GoatMesh.hpp"
#include "IslandSimulator/GoatCoatUv.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
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
constexpr int Capacity=static_cast<int>(GoatHabitat::Count*GoatMesh::MaximumVertices);
Vector3 Point(GoatPoint p){return {p.x,p.y,p.z};}
// Coat artwork supplies local strand albedo; bare parts must not sample fur.
const std::array<Color,7> Materials{Color(244,238,222),Color(145,116,92),
    Color(108,91,66),Color(58,54,45),Color(18,16,13),Color(63,57,48),Color(166,122,53)};
// Feral herds are not six copies of one white animal.  The shared neutral fur
// artwork supplies hair detail while these bounded vertex tints provide natural
// cream, fawn, grey and sable individuals without another texture or draw.
const std::array<Color,6> CoatPalettes{Color(221,199,161),Color(242,235,216),
    Color(169,158,139),Color(190,151,111),Color(126,112,94),Color(232,220,198)};
}
GoatRenderer::GoatRenderer(GraphicsDevice& device,
    const CanopyLightField& canopy,float maximumDistance)
    :device_(device),canopy_(canopy),
     maximumDistance_(maximumDistance),effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
         Capacity,BufferUsage::WriteOnly))
{
    vertices_.reserve(Capacity);
    solidVertices_.reserve(Capacity);
    constexpr int size=GeneratedTextureBudget::GoatFurSize;
    const auto levels=LoadOpaqueImageMips(device_,"Content/wildlife/goat-fur.png",size);
    coat_=std::make_unique<Texture2D>(device_,size,size,true,SurfaceFormat::Color);
    for(std::size_t level=0;level<levels.size();++level) {
        const auto& pixels=levels[level].pixels;
        coat_->SetData(static_cast<int>(level),nullptr,reinterpret_cast<const std::uint8_t*>(pixels.data()),
            0,static_cast<int>(pixels.size()*4));
    }
    const char* message="island-simulator: goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes";
    std::cout<<message<<'\n';MirrorRuntimeDiagnosticToPlatformLog(message);
    const char* anatomy="island-simulator: goats use separated haunch, waist and raised shoulder masses, six-station necks, and six deterministic natural coat tints";
    std::cout<<anatomy<<'\n';MirrorRuntimeDiagnosticToPlatformLog(anatomy);
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
    vertices_.clear();solidVertices_.clear();visible_=draws_=0;
    const BoundingFrustum frustum(view*projection);
    for(std::size_t goatIndex=0;goatIndex<goats.size();++goatIndex) {
        const auto& goat=goats[goatIndex];
        const Vector3 centre(goat.x,goat.y+.75f*goat.scale,goat.z);
        const float distanceSquared=Vector3::DistanceSquared(centre,camera);
        if(distanceSquared>maximumDistance_*maximumDistance_
            ||!frustum.Intersects(BoundingSphere(centre,1.8f*goat.scale)))continue;
        const auto& pose=goat.pose;
        const auto mesh=BuildGoatMesh(pose,goat.grazingPose,distanceSquared<30*30);
        for(const auto& vertex:mesh.vertices) {
            const auto world=Point(pose.ToWorld(vertex.position));
            const auto n=Vector3::Normalize(Point(pose.right)*vertex.normal.x
                +Point(pose.up)*vertex.normal.y+Point(pose.forward)*vertex.normal.z);
            const float light=.61f+.37f*std::max(0.0f,Vector3::Dot(n,wildlife_render::FixedSunDirection))
                +.10f*std::max(0.0f,n.Y);
            const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(world.X,world.Y,world.Z,goat.y));
            const auto color=vertex.material==GoatMaterial::Coat
                ?CoatPalettes[goatIndex%CoatPalettes.size()]
                :Materials.at(static_cast<std::size_t>(vertex.material));
            // A restrained darker belly/warmer topline prevents the textured barrel
            // from reading as one flat pale sphere; non-body fur keeps its palette.
            const float coatTone=vertex.material==GoatMaterial::Coat&&vertex.part==GoatPart::Body
                ?.91f+.10f*std::clamp((vertex.position.y-.53f)/.58f,0.0f,1.0f):1.0f;
            const auto channel=[light,coatTone](int value,float transmission){
                return static_cast<int>(std::clamp(std::lround(value*light*coatTone*transmission),0L,255L));};
            const auto uv=GoatCoatUv(vertex);
            auto& target=vertex.material==GoatMaterial::Coat?vertices_:solidVertices_;
            target.emplace_back(world,Color(channel(color.getRProperty(),tint.red),
                channel(color.getGProperty(),tint.green),channel(color.getBProperty(),tint.blue)),Vector2(uv[0],uv[1]));
        }
        ++visible_;
    }
    if(vertices_.empty())return;
    const int coatVertices=static_cast<int>(vertices_.size());
    vertices_.insert(vertices_.end(),solidVertices_.begin(),solidVertices_.end());
    if(vertices_.size()>Capacity)throw std::length_error("Goat vertex upload exceeds capacity");
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
    effect_->setProjectionProperty(projection);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;
    effect_->setTextureEnabledProperty(true);effect_->setTextureProperty(coat_.get());effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,coatVertices/3);draws_=1;
    if(!solidVertices_.empty()) {
        effect_->setTextureEnabledProperty(false);effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList,coatVertices,static_cast<int>(solidVertices_.size()/3));
        ++draws_;
    }
}
}
