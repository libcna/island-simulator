#include "IslandSimulator/BearRenderer.hpp"

#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
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
enum class Material:std::uint8_t {Fur,DarkFur,Muzzle,Bare};
struct Texel {std::uint8_t r=255,g=255,b=255,a=255;};

Vector3 Point(BearPoint p){return {p.x,p.y,p.z};}
BearPoint Point(Vector3 p){return {p.X,p.Y,p.Z};}
Vector3 SafeUnit(Vector3 p) {if(p.LengthSquared()<1e-10f)return Vector3::Up;p.Normalize();return p;}
Vector2 AtlasUv(Material material,float u,float v) {
    const int column=(material==Material::DarkFur||material==Material::Bare)?1:0;
    const int row=(material==Material::Muzzle||material==Material::Bare)?1:0;
    return {column*.5f+.012f+std::clamp(u,0.0f,1.0f)*.476f,
            row*.5f+.012f+std::clamp(v,0.0f,1.0f)*.476f};
}
Color BaseColor(Material material) {
    switch(material) {
    case Material::Fur:return Color(92,68,45);
    case Material::DarkFur:return Color(67,50,36);
    case Material::Muzzle:return Color(116,96,74);
    case Material::Bare:return Color(31,27,24);
    }
    return Color::White;
}
std::vector<Texel> MakeAtlasBase() {
    constexpr int size=GeneratedTextureBudget::BearFurSize;
    std::vector<Texel> pixels(static_cast<std::size_t>(size*size));
    for(int y=0;y<size;++y)for(int x=0;x<size;++x) {
        const int quadrant=(x>=size/2?1:0)+(y>=size/2?2:0);
        const int lx=x%(size/2),ly=y%(size/2);
        std::uint32_t hash=static_cast<std::uint32_t>(x*2246822519U+y*3266489917U+0x5f356495U);
        hash^=hash>>15U;hash*=0x2c1b3c6dU;hash^=hash>>12U;
        int value=230;
        if(quadrant<2) {
            // Fine tapered vertical guard hairs over a denser irregular undercoat.
            const float guard=std::sin(lx*.71f+std::sin(ly*.19f)*2.4f);
            const int strand=((lx+2*(ly/3))%11<2)?24:0;
            value=quadrant==0?205:183;
            value+=static_cast<int>(guard*13)+strand+static_cast<int>(hash%17U)-8;
        } else if(quadrant==2) {
            value=218+static_cast<int>(hash%19U)-9+((lx+ly)%13<2?11:0);
        } else {
            value=205+static_cast<int>(hash%13U)-6;
        }
        value=std::clamp(value,120,255);
        pixels[static_cast<std::size_t>(y*size+x)]={static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value),static_cast<std::uint8_t>(value),255};
    }
    return pixels;
}
std::vector<Texel> Downsample(const std::vector<Texel>& source,int size) {
    const int next=std::max(1,size/2);std::vector<Texel> result(static_cast<std::size_t>(next*next));
    for(int y=0;y<next;++y)for(int x=0;x<next;++x) {
        unsigned r=0,g=0,b=0,a=0;
        for(int oy=0;oy<2;++oy)for(int ox=0;ox<2;++ox) {
            const auto& p=source[static_cast<std::size_t>(std::min(size-1,2*y+oy)*size+std::min(size-1,2*x+ox))];
            r+=p.r;g+=p.g;b+=p.b;a+=p.a;
        }
        result[static_cast<std::size_t>(y*next+x)]={static_cast<std::uint8_t>(r/4),
            static_cast<std::uint8_t>(g/4),static_cast<std::uint8_t>(b/4),static_cast<std::uint8_t>(a/4)};
    }
    return result;
}
std::unique_ptr<Texture2D> CreateFurAtlas(GraphicsDevice& device) {
    constexpr int size=GeneratedTextureBudget::BearFurSize;
    auto texture=std::make_unique<Texture2D>(device,size,size,true,SurfaceFormat::Color);
    auto pixels=MakeAtlasBase();int levelSize=size,level=0;
    while(true) {
        texture->SetData(level,nullptr,reinterpret_cast<const std::uint8_t*>(pixels.data()),0,
            static_cast<int>(pixels.size()*sizeof(Texel)));
        if(levelSize==1)break;
        pixels=Downsample(pixels,levelSize);levelSize=std::max(1,levelSize/2);++level;
    }
    return texture;
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
    mesh.Ellipsoid(head,right,up,forward,.36f,.34f,.39f,Material::Fur,lon,lat);
    const Vector3 muzzle=head+forward*.37f-up*.10f;
    mesh.Ellipsoid(muzzle,right,up,forward,.25f,.18f,.27f,Material::Muzzle,near?16:9,near?9:6);
    mesh.Ellipsoid(muzzle+forward*.245f+up*.035f,right,up,forward,.12f,.075f,.06f,
        Material::Bare,near?14:8,near?8:5);
    mesh.Ellipsoid({0,1.10f,-1.08f},right,up,forward,.13f,.13f,.14f,Material::Fur,near?12:8,near?7:5);
    for(int side=-1;side<=1;side+=2) {
        const Vector3 ear=head+right*(side*.23f)+up*.285f-forward*.04f;
        mesh.Ellipsoid(ear,right,up,forward,.11f,.115f,.075f,Material::Fur,near?12:8,near?8:5);
        if(near) {
            mesh.Ellipsoid(ear+forward*.052f,right,up,forward,.068f,.075f,.018f,Material::DarkFur,10,7);
            const Vector3 eye=head+right*(side*.225f)+up*.045f+forward*.335f;
            mesh.Ellipsoid(eye,right,up,forward,.019f,.015f,.010f,Material::Bare,10,6);
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
        // Split lower lip and short mouth corners keep the muzzle readable without cartoon outlines.
        const Vector3 lip=muzzle+forward*.257f-up*.07f;
        mesh.Tube(lip-right*.12f,lip+right*.12f,.011f,.009f,Material::Bare,8);
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
        ShadowCapacity,BufferUsage::WriteOnly)),furAtlas_(CreateFurAtlas(device)) {
    if(!std::isfinite(maximumDistance)||maximumDistance<=0)throw std::invalid_argument("Bear draw distance must be positive");
    vertices_.reserve(VertexCapacity);shadows_.reserve(ShadowCapacity);
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(maximumDistance*.65f);effect_->setFogEndProperty(maximumDistance);
    const char* diagnostic="island-simulator: one large peaceful bear uses original articulated quadruped anatomy and a generated directional-fur atlas";
    std::cout<<diagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
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
    effect_->setTextureProperty(furAtlas_.get());effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_);++draws_;
}
}
