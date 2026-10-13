#include "IslandSimulator/BearRenderer.hpp"

#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island {
namespace {
constexpr int VertexCapacity=36000,ShadowCapacity=180;
constexpr float Pi=std::numbers::pi_v<float>;
enum class Material:std::uint8_t {Fur,DarkFur,Muzzle,Nose,Bare,EyeHighlight};

Vector3 Point(BearPoint p){return {p.x,p.y,p.z};}
BearPoint Point(Vector3 p){return {p.X,p.Y,p.Z};}
Vector3 SafeUnit(Vector3 p) {if(p.LengthSquared()<1e-10f)return Vector3::Up;p.Normalize();return p;}
Vector2 AtlasUv(Material material,float u,float v) {
    u=std::clamp(u,0.0f,1.0f);v=std::clamp(v,0.0f,1.0f);
    if(material==Material::Fur)return {.025f+.95f*u,.025f+.95f*v};
    if(material==Material::DarkFur)return {.975f-.95f*u,.025f+.95f*v};
    // Short facial surfaces use bounded source windows rather than stretching one
    // long guard-hair clump over a nose, eye or mouth mark.
    if(material==Material::Muzzle)return {.40f+.20f*u,.40f+.20f*v};
    if(material==Material::EyeHighlight)return {.70f+.02f*u,.18f+.02f*v};
    return {.18f+.025f*u,.86f+.025f*v};
}
Color BaseColor(Material material) {
    switch(material) {
    case Material::Fur:return Color(245,245,245);
    case Material::DarkFur:return Color(176,166,154);
    case Material::Muzzle:return Color(222,211,198);
    case Material::Nose:return Color(39,34,30);
    case Material::Bare:return Color(15,13,12);
    case Material::EyeHighlight:return Color(184,168,142);
    }
    return Color::White;
}
class BearMeshBuilder {
public:
    BearMeshBuilder(std::vector<VertexPositionColorTexture>& output,const BearPose& pose,
        CanopyLightTint canopy):output_(output),pose_(pose),canopy_(canopy){}
    void Ellipsoid(Vector3 centre,Vector3 right,Vector3 up,Vector3 forward,float rx,float ry,float rz,
        Material material,int longitude,int latitude) {
        right=SafeUnit(right);up=SafeUnit(up);forward=SafeUnit(forward);
        const auto point=[&](float a,float b){const float c=std::cos(a);return centre+right*(rx*c*std::cos(b))
            +up*(ry*std::sin(a))+forward*(rz*c*std::sin(b));};
        const auto normal=[&](float a,float b){const float c=std::cos(a);return SafeUnit(
            right*(c*std::cos(b)/rx)+up*(std::sin(a)/ry)+forward*(c*std::sin(b)/rz));};
        for(int y=0;y<latitude;++y)for(int x=0;x<longitude;++x) {
            const float a0=-Pi*.5f+Pi*y/latitude,a1=-Pi*.5f+Pi*(y+1)/latitude;
            const float b0=2*Pi*x/longitude,b1=2*Pi*(x+1)/longitude;
            Quad(point(a0,b0),normal(a0,b0),{static_cast<float>(x)/longitude,static_cast<float>(y)/latitude},
                 point(a0,b1),normal(a0,b1),{static_cast<float>(x+1)/longitude,static_cast<float>(y)/latitude},
                 point(a1,b1),normal(a1,b1),{static_cast<float>(x+1)/longitude,static_cast<float>(y+1)/latitude},
                 point(a1,b0),normal(a1,b0),{static_cast<float>(x)/longitude,static_cast<float>(y+1)/latitude},material);
        }
    }
    void Tube(Vector3 a,Vector3 b,float radiusA,float radiusB,Material material,int segments) {
        const Vector3 axis=SafeUnit(b-a);
        const Vector3 side=SafeUnit(Vector3::Cross(axis,std::abs(axis.Y)<.86f?Vector3::Up:Vector3::Right));
        const Vector3 other=SafeUnit(Vector3::Cross(side,axis));
        for(int i=0;i<segments;++i) {
            const float t0=2*Pi*i/segments,t1=2*Pi*(i+1)/segments;
            const Vector3 n0=SafeUnit(side*std::cos(t0)+other*std::sin(t0));
            const Vector3 n1=SafeUnit(side*std::cos(t1)+other*std::sin(t1));
            Quad(a+n0*radiusA,n0,{static_cast<float>(i)/segments,0},a+n1*radiusA,n1,{static_cast<float>(i+1)/segments,0},
                b+n1*radiusB,n1,{static_cast<float>(i+1)/segments,1},b+n0*radiusB,n0,{static_cast<float>(i)/segments,1},material);
        }
    }
    void FurTuft(Vector3 root,float halfWidth,float length,float sweep) {
        const Vector3 across=Vector3::Right;
        const Vector3 normal=SafeUnit(Vector3::Up+Vector3(0,0,-.12f));
        const Vector3 tip=root+Vector3(0,length,sweep);
        Quad(root-across*halfWidth,normal,{.16f,.88f},
            root+across*halfWidth,normal,{.28f,.88f},
            tip+across*halfWidth*.28f,normal,{.28f,.18f},
            tip-across*halfWidth*.28f,normal,{.16f,.18f},Material::Fur);
    }
private:
    Vector3 World(Vector3 local)const{return Point(pose_.ToWorld(Point(local)));}
    Vector3 WorldNormal(Vector3 local)const{return SafeUnit(Point(pose_.right)*local.X
        +Point(pose_.up)*local.Y+Point(pose_.forward)*local.Z);}
    void Vertex(Vector3 p,Vector3 normal,Vector2 uv,Material material) {
        const auto worldNormal=WorldNormal(normal);const auto base=BaseColor(material);
        const float light=.55f+.37f*std::max(0.0f,Vector3::Dot(worldNormal,wildlife_render::FixedSunDirection))
            +.10f*std::max(0.0f,worldNormal.Y);
        const auto channel=[&](std::uint8_t value,float tint){return static_cast<int>(std::clamp(
            std::lround(value*light*tint),0L,255L));};
        output_.emplace_back(World(p),Color(channel(base.getRProperty(),canopy_.red),
            channel(base.getGProperty(),canopy_.green),channel(base.getBProperty(),canopy_.blue),255),
            AtlasUv(material,uv.X,uv.Y));
    }
    void Triangle(Vector3 a,Vector3 na,Vector2 ua,Vector3 b,Vector3 nb,Vector2 ub,
        Vector3 c,Vector3 nc,Vector2 uc,Material material) {
        Vertex(a,na,ua,material);Vertex(b,nb,ub,material);Vertex(c,nc,uc,material);
    }
    void Quad(Vector3 a,Vector3 na,Vector2 ua,Vector3 b,Vector3 nb,Vector2 ub,
        Vector3 c,Vector3 nc,Vector2 uc,Vector3 d,Vector3 nd,Vector2 ud,Material material) {
        Triangle(a,na,ua,b,nb,ub,c,nc,uc,material);Triangle(a,na,ua,c,nc,uc,d,nd,ud,material);
    }
    std::vector<VertexPositionColorTexture>& output_;const BearPose& pose_;CanopyLightTint canopy_;
};

void BuildBear(std::vector<VertexPositionColorTexture>& vertices,const BearState& state,
    CanopyLightTint canopy,bool near) {
    BearMeshBuilder mesh(vertices,state.pose,canopy);const int lon=near?22:10,lat=near?14:7;
    const float breath=.014f*std::sin(state.breathPhase),forage=state.foragePose;
    const Vector3 right=Vector3::Right,up=Vector3::Up,forward(0,0,1);
    mesh.Ellipsoid({0,1.04f+state.pose.bodyBob,-.20f},right,up,forward,.56f+breath,.53f+breath,.93f,
        Material::Fur,lon,lat);
    mesh.Ellipsoid({0,1.36f+state.pose.bodyBob,.31f},right,up,forward,.58f,.42f,.57f,
        Material::Fur,lon,lat);
    mesh.Ellipsoid({0,1.18f-forage*.18f,.70f+forage*.08f},right,up,forward,.45f,.43f,.52f,
        Material::Fur,lon,lat);
    const Vector3 head{0,1.28f-forage*.48f,1.12f+forage*.18f};
    mesh.Ellipsoid(head,right,up,forward,.38f,.35f,.39f,Material::Fur,lon,lat);
    if(near) {
        // Short overlapping wedges break the mathematically smooth top outline. Roots
        // follow the upper envelope of the same body volumes, so breathing, foraging
        // and the shoulder hump cannot expose floating fur cards.
        const auto top=[](float z,float centreZ,float centreY,float radiusY,float radiusZ) {
            const float t=(z-centreZ)/radiusZ;
            return std::abs(t)<=1.0f?centreY+radiusY*std::sqrt(std::max(0.0f,1.0f-t*t)):-100.0f;
        };
        for(int tuft=0;tuft<22;++tuft) {
            const float z=-1.02f+static_cast<float>(tuft)*.112f;
            const float y=std::max({
                top(z,-.20f,1.04f+state.pose.bodyBob,.53f+breath,.93f),
                top(z,.31f,1.36f+state.pose.bodyBob,.42f,.57f),
                top(z,.70f,1.18f-forage*.18f,.43f,.52f),
                top(z,head.Z,head.Y,.35f,.39f)});
            if(y>-50.0f)mesh.FurTuft({0,y-.004f,z},.020f+.003f*(tuft%3),
                .030f+.006f*(tuft%4),-.014f-.004f*(tuft%2));
        }
    }
    // Two overlapping, successively narrower volumes make a short tapered brown-bear muzzle.
    // The former single pale cylinder read as a toy snout from every oblique capture angle.
    const Vector3 muzzleRoot=head+forward*.315f-up*.105f;
    const Vector3 muzzle=muzzleRoot+forward*.155f-up*.008f;
    mesh.Ellipsoid(muzzleRoot,right,up,forward,.245f,.175f,.245f,
        Material::Muzzle,near?18:9,near?10:6);
    mesh.Ellipsoid(muzzle,right,up,forward,.195f,.137f,.170f,
        Material::Muzzle,near?16:9,near?9:6);
    const Vector3 nose=muzzle+forward*.158f+up*.035f;
    mesh.Ellipsoid(nose,right,up,forward,.105f,.066f,.047f,
        Material::Nose,near?14:8,near?8:5);
    mesh.Ellipsoid({0,1.10f,-1.08f},right,up,forward,.13f,.13f,.14f,Material::Fur,near?12:8,near?7:5);
    for(int side=-1;side<=1;side+=2) {
        const Vector3 ear=head+right*(side*.23f)+up*.285f-forward*.04f;
        mesh.Ellipsoid(ear,right,up,forward,.11f,.115f,.075f,Material::Fur,near?12:8,near?8:5);
        if(near) {
            mesh.Ellipsoid(ear+forward*.052f,right,up,forward,.068f,.075f,.018f,Material::DarkFur,10,7);
            const Vector3 eye=head+right*(side*.226f)+up*.050f+forward*.337f;
            mesh.Ellipsoid(eye,right,up,forward,.024f,.020f,.012f,Material::Bare,10,6);
            mesh.Ellipsoid(eye-right*(side*.005f)+up*.006f+forward*.011f,right,up,forward,
                .006f,.005f,.003f,Material::EyeHighlight,7,5);
        }
    }
    for(std::size_t leg=0;leg<4;++leg) {
        const auto& p=state.pose.legs[leg];const Vector3 root=Point(p.root),joint=Point(p.joint),ankle=Point(p.paw);
        const bool front=leg%2==1;
        mesh.Tube(root,joint,front?.19f:.21f,front?.145f:.16f,Material::Fur,near?14:8);
        mesh.Ellipsoid(joint,right,up,forward,front?.16f:.18f,.16f,.16f,Material::Fur,near?12:8,near?7:5);
        mesh.Tube(joint,ankle,front?.145f:.16f,.105f,Material::Fur,near?13:8);
        const Vector3 sole=ankle-up*BearLegPose::PawHeight;
        const Vector3 pawUp=SafeUnit(Point(p.soleNormal));
        const Vector3 pawRight=SafeUnit(Vector3::Cross(pawUp,forward));
        const Vector3 pawForward=SafeUnit(Vector3::Cross(pawRight,pawUp));
        const Vector3 paw=sole+pawUp*.075f+pawForward*.055f;
        mesh.Ellipsoid(paw,pawRight,pawUp,pawForward,.145f,.082f,.225f,Material::Fur,near?14:8,near?7:5);
        if(near)for(int claw=-1;claw<=1;++claw) {
            const Vector3 base=paw+pawRight*(claw*.075f)+pawForward*.195f-pawUp*.025f;
            mesh.Tube(base,base+pawForward*.065f-pawUp*.018f,.012f,.003f,Material::Bare,7);
        }
    }
    if(near) {
        // Paired nostrils, a short philtrum and divided mouth crease remain shallow facial marks,
        // rather than the previous thick horizontal rod projecting beyond the muzzle.
        for(int side=-1;side<=1;side+=2) {
            const Vector3 nostril=nose+right*(side*.045f)+up*.004f+forward*.044f;
            mesh.Ellipsoid(nostril,right,up,forward,.018f,.010f,.006f,Material::Bare,8,5);
        }
        const Vector3 philtrumTop=nose+forward*.045f-up*.030f;
        const Vector3 mouth=muzzle+forward*.166f-up*.058f;
        mesh.Tube(philtrumTop,mouth,.0045f,.0035f,Material::Bare,7);
        mesh.Tube(mouth,mouth-right*.088f-up*.004f-forward*.010f,
            .0040f,.0025f,Material::Bare,7);
        mesh.Tube(mouth,mouth+right*.088f-up*.004f-forward*.010f,
            .0040f,.0025f,Material::Bare,7);
        mesh.Ellipsoid(mouth-up*.038f-forward*.025f,right,up,forward,
            .105f,.033f,.050f,Material::Muzzle,12,6);
    }
}
bool IsVisible(const Matrix& view,const Matrix& projection,const Vector3& camera,
    const BearState& state,float maximumDistance) {
    const Vector3 centre(state.x,state.y+1.0f,state.z);
    return Vector3::DistanceSquared(centre,camera)<=maximumDistance*maximumDistance
        &&BoundingFrustum(view*projection).Intersects(BoundingSphere(centre,1.8f));
}
}

BearRenderer::BearRenderer(GraphicsDevice& device,const CanopyLightField& canopy,float maximumDistance)
    :device_(device),canopy_(canopy),maximumDistance_(maximumDistance),effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
        VertexCapacity,BufferUsage::WriteOnly)),
     shadowBuffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
        ShadowCapacity,BufferUsage::WriteOnly)),furTexture_(std::make_unique<Texture2D>(device,
        GeneratedTextureBudget::BearFurSize,GeneratedTextureBudget::BearFurSize,true,SurfaceFormat::Color)) {
    if(!std::isfinite(maximumDistance)||maximumDistance<=0)throw std::invalid_argument("Bear draw distance must be positive");
    vertices_.reserve(VertexCapacity);shadows_.reserve(ShadowCapacity);
    const auto levels=LoadOpaqueImageMips(device,"Content/wildlife/bear-fur.png",
        GeneratedTextureBudget::BearFurSize);
    for(std::size_t level=0;level<levels.size();++level) {
        const auto& pixels=levels[level].pixels;
        furTexture_->SetData(static_cast<int>(level),nullptr,
            reinterpret_cast<const std::uint8_t*>(pixels.data()),0,
            static_cast<int>(pixels.size()*4));
    }
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(maximumDistance*.65f);effect_->setFogEndProperty(maximumDistance);
    const char* material="island-simulator: bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
    const char* diagnostic="island-simulator: one large peaceful bear uses original articulated quadruped anatomy and a photographic-detail directional-fur material";
    std::cout<<diagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
    const char* faceDiagnostic="island-simulator: near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes";
    std::cout<<faceDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(faceDiagnostic);
    const char* furDiagnostic="island-simulator: near bear uses twenty-two body-envelope fur wedges without another draw";
    std::cout<<furDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(furDiagnostic);
}
BearRenderer::~BearRenderer()=default;

void BearRenderer::DrawShadow(const Matrix& view,const Matrix& projection,const Vector3& camera,const BearState& bear) {
    draws_=triangles_=shadowTriangles_=visible_=0;shadows_.clear();
    if(!IsVisible(view,projection,camera,bear,maximumDistance_))return;
    const Vector3 origin(bear.x,bear.y+.012f,bear.z),right=Point(bear.pose.right),forward=Point(bear.pose.forward);
    constexpr int segments=18;const Color centre(20,17,14,62),edge(20,17,14,0);
    for(int i=0;i<segments;++i) {
        const float a0=2*Pi*i/segments,a1=2*Pi*(i+1)/segments;
        shadows_.emplace_back(origin,centre,Vector2(.5f,.5f));
        shadows_.emplace_back(origin+right*(.64f*std::cos(a0))+forward*(1.18f*std::sin(a0)),edge,Vector2::Zero);
        shadows_.emplace_back(origin+right*(.64f*std::cos(a1))+forward*(1.18f*std::sin(a1)),edge,Vector2::Zero);
    }
    shadowTriangles_=static_cast<int>(shadows_.size()/3);
    shadowBuffer_->SetData(shadows_.data(),0,static_cast<int>(shadows_.size()),SetDataOptions::Discard);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::AlphaBlend);
    device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);device_.SetVertexBuffer(shadowBuffer_.get());
    device_.setIndicesProperty(nullptr);effect_->setTextureEnabledProperty(false);effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,shadowTriangles_);++draws_;
}
void BearRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,const BearState& bear) {
    vertices_.clear();visible_=0;triangles_=0;nearDetail_=false;
    if(!IsVisible(view,projection,camera,bear,maximumDistance_))return;
    const float distanceSquared=Vector3::DistanceSquared({bear.x,bear.y+1,bear.z},camera);
    nearDetail_=distanceSquared<55*55;
    const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(bear.x,bear.y+1,bear.z,bear.y));
    BuildBear(vertices_,bear,tint,nearDetail_);
    if(vertices_.empty()||vertices_.size()>VertexCapacity)throw std::length_error("Bear mesh exceeds bounded upload");
    triangles_=static_cast<int>(vertices_.size()/3);visible_=1;
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;effect_->setTextureEnabledProperty(true);
    effect_->setTextureProperty(furTexture_.get());effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_);++draws_;
}
}
