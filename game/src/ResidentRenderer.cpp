#include "IslandSimulator/ResidentRenderer.hpp"

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
constexpr int VertexCapacity=40000;
constexpr int ShadowCapacity=144;
constexpr float Pi=std::numbers::pi_v<float>;

enum class Material:std::uint8_t {Skin,Cloth,Hair,Fibre,Eye,Lip};
struct Texel {std::uint8_t r=255,g=255,b=255,a=255;};

Vector3 Point(ResidentPoint p){return {p.x,p.y,p.z};}
Vector3 SafeUnit(Vector3 p) {
    if(p.LengthSquared()<1e-10f)return Vector3::Up;
    p.Normalize();return p;
}
Vector3 Lerp(Vector3 a,Vector3 b,float t){return a+(b-a)*t;}

Vector2 AtlasUv(Material material,float u,float v) {
    int column=0,row=0;
    switch(material) {
    case Material::Skin: case Material::Eye: case Material::Lip:break;
    case Material::Cloth:column=1;break;
    case Material::Hair:row=1;break;
    case Material::Fibre:column=1;row=1;break;
    }
    // Inset each quadrant so bilinear filtering cannot cross material borders.
    return {column*.5f+.018f+std::clamp(u,0.0f,1.0f)*.464f,
            row*.5f+.018f+std::clamp(v,0.0f,1.0f)*.464f};
}

Color BaseColor(Material material) {
    switch(material) {
    case Material::Skin:return Color(116,72,49);
    case Material::Cloth:return Color(204,163,91);
    case Material::Hair:return Color(27,20,16);
    case Material::Fibre:return Color(101,74,42);
    case Material::Eye:return Color(220,207,177);
    case Material::Lip:return Color(104,48,43);
    }
    return Color::White;
}

std::vector<Texel> MakeAtlasBase() {
    constexpr int size=GeneratedTextureBudget::ResidentMaterialSize;
    std::vector<Texel> pixels(static_cast<std::size_t>(size*size));
    for(int y=0;y<size;++y)for(int x=0;x<size;++x) {
        const int quadrant=(x>=size/2?1:0)+(y>=size/2?2:0);
        const int lx=x%(size/2),ly=y%(size/2);
        std::uint32_t hash=static_cast<std::uint32_t>(x*374761393U+y*668265263U+0x9e3779b9U);
        hash=(hash^(hash>>13U))*1274126177U;hash^=hash>>16U;
        int value=235;
        if(quadrant==0) {
            // Fine, restrained skin grain with sparse pores and no painted facial features.
            value=238+static_cast<int>(hash%13U)-6;
            if(hash%97U==0U)value-=23;
        } else if(quadrant==1) {
            // Unequal warp and weft threads make the plain tropical cloth visibly woven.
            value=218+((lx%7)==0?23:0)+((ly%5)==0?14:0)+static_cast<int>(hash%7U)-3;
        } else if(quadrant==2) {
            const float curls=std::sin(lx*.47f+std::sin(ly*.31f)*2.2f)
                *std::sin(ly*.51f+std::sin(lx*.27f)*1.7f);
            value=212+static_cast<int>(24*curls)+static_cast<int>(hash%9U)-4;
        } else {
            // Plant-fibre diagonals for belt, sandal soles and straps.
            value=210+(((lx+2*ly)%9)<2?30:0)+(((2*lx-ly+128)%17)<2?12:0);
        }
        value=std::clamp(value,155,255);
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
            const auto& p=source[static_cast<std::size_t>(std::min(size-1,y*2+oy)*size+std::min(size-1,x*2+ox))];
            r+=p.r;g+=p.g;b+=p.b;a+=p.a;
        }
        result[static_cast<std::size_t>(y*next+x)]={static_cast<std::uint8_t>(r/4),
            static_cast<std::uint8_t>(g/4),static_cast<std::uint8_t>(b/4),static_cast<std::uint8_t>(a/4)};
    }
    return result;
}

std::unique_ptr<Texture2D> CreateMaterialAtlas(GraphicsDevice& device) {
    constexpr int size=GeneratedTextureBudget::ResidentMaterialSize;
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

class HumanMeshBuilder {
public:
    HumanMeshBuilder(std::vector<VertexPositionColorTexture>& output,CanopyLightTint canopy)
        :output_(output),canopy_(canopy){}

    void Ellipsoid(Vector3 centre,Vector3 right,Vector3 up,Vector3 forward,
        float rx,float ry,float rz,Material material,int longitude=12,int latitude=8) {
        right=SafeUnit(right);up=SafeUnit(up);forward=SafeUnit(forward);
        const auto point=[&](float a,float b){const float c=std::cos(a);return centre
            +right*(rx*c*std::cos(b))+up*(ry*std::sin(a))+forward*(rz*c*std::sin(b));};
        const auto normal=[&](float a,float b){const float c=std::cos(a);return SafeUnit(
            right*(c*std::cos(b)/rx)+up*(std::sin(a)/ry)+forward*(c*std::sin(b)/rz));};
        for(int y=0;y<latitude;++y) {
            const float a0=-Pi*.5f+Pi*y/latitude,a1=-Pi*.5f+Pi*(y+1)/latitude;
            for(int x=0;x<longitude;++x) {
                const float b0=2*Pi*x/longitude,b1=2*Pi*(x+1)/longitude;
                Quad(point(a0,b0),normal(a0,b0),{static_cast<float>(x)/longitude,static_cast<float>(y)/latitude},
                     point(a0,b1),normal(a0,b1),{static_cast<float>(x+1)/longitude,static_cast<float>(y)/latitude},
                     point(a1,b1),normal(a1,b1),{static_cast<float>(x+1)/longitude,static_cast<float>(y+1)/latitude},
                     point(a1,b0),normal(a1,b0),{static_cast<float>(x)/longitude,static_cast<float>(y+1)/latitude},material);
            }
        }
    }

    void Tube(Vector3 a,Vector3 b,float radiusA,float radiusB,Material material,int segments=10) {
        const Vector3 axis=SafeUnit(b-a);
        Vector3 side=SafeUnit(Vector3::Cross(axis,std::abs(axis.Y)<.85f?Vector3::Up:Vector3::Right));
        const Vector3 other=SafeUnit(Vector3::Cross(side,axis));
        for(int i=0;i<segments;++i) {
            const float t0=2*Pi*i/segments,t1=2*Pi*(i+1)/segments;
            const Vector3 n0=SafeUnit(side*std::cos(t0)+other*std::sin(t0));
            const Vector3 n1=SafeUnit(side*std::cos(t1)+other*std::sin(t1));
            Quad(a+n0*radiusA,n0,{static_cast<float>(i)/segments,0},
                 a+n1*radiusA,n1,{static_cast<float>(i+1)/segments,0},
                 b+n1*radiusB,n1,{static_cast<float>(i+1)/segments,1},
                 b+n0*radiusB,n0,{static_cast<float>(i)/segments,1},material);
        }
    }

    void Loft(std::span<const Vector3> centres,std::span<const float> rx,std::span<const float> rz,
        Vector3 right,Vector3 forward,Material material,int segments=14) {
        right=SafeUnit(right);forward=SafeUnit(forward);
        for(std::size_t ring=0;ring+1<centres.size();++ring)for(int i=0;i<segments;++i) {
            const float a0=2*Pi*i/segments,a1=2*Pi*(i+1)/segments;
            const auto p=[&](std::size_t r,float a){return centres[r]+right*(rx[r]*std::cos(a))+forward*(rz[r]*std::sin(a));};
            const auto n=[&](std::size_t r,float a){return SafeUnit(right*(std::cos(a)/rx[r])+forward*(std::sin(a)/rz[r]));};
            Quad(p(ring,a0),n(ring,a0),{static_cast<float>(i)/segments,static_cast<float>(ring)/(centres.size()-1)},
                 p(ring,a1),n(ring,a1),{static_cast<float>(i+1)/segments,static_cast<float>(ring)/(centres.size()-1)},
                 p(ring+1,a1),n(ring+1,a1),{static_cast<float>(i+1)/segments,static_cast<float>(ring+1)/(centres.size()-1)},
                 p(ring+1,a0),n(ring+1,a0),{static_cast<float>(i)/segments,static_cast<float>(ring+1)/(centres.size()-1)},material);
        }
    }
private:
    void Vertex(Vector3 p,Vector3 normal,Vector2 uv,Material material) {
        const auto base=BaseColor(material);
        const float direct=std::max(0.0f,Vector3::Dot(SafeUnit(normal),wildlife_render::FixedSunDirection));
        const float sky=std::max(0.0f,normal.Y);
        const float light=.57f+.35f*direct+.10f*sky;
        const auto channel=[&](std::uint8_t value,float tint){return static_cast<int>(std::clamp(
            std::lround(value*light*tint),0L,255L));};
        output_.emplace_back(p,Color(channel(base.getRProperty(),canopy_.red),
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
    std::vector<VertexPositionColorTexture>& output_;
    CanopyLightTint canopy_;
};

void BuildHuman(std::vector<VertexPositionColorTexture>& vertices,const ResidentState& state,
    CanopyLightTint canopy) {
    HumanMeshBuilder mesh(vertices,canopy);const auto& pose=state.pose;
    const Vector3 up=Vector3::Up;
    const Vector3 right(std::cos(pose.heading),0,-std::sin(pose.heading));
    const Vector3 forward(std::sin(pose.heading),0,std::cos(pose.heading));
    const Vector3 root(state.x,pose.pelvisY,state.z);

    const std::array torsoCentres{root+up*-.02f,root+up*.08f,root+up*.20f,root+up*.35f,root+up*.47f};
    const std::array torsoRx{.155f,.17f,.15f,.18f,.202f};
    const std::array torsoRz{.105f,.115f,.105f,.13f,.115f};
    mesh.Loft(torsoCentres,torsoRx,torsoRz,right,forward,Material::Cloth);
    mesh.Tube(root+up*.045f,root+up*.092f,.178f,.17f,Material::Fibre,14);

    const Vector3 neckBase=root+up*.48f,neckTop=root+up*.565f;
    mesh.Tube(neckBase,neckTop,.068f,.063f,Material::Skin,12);
    const Vector3 head=root+up*.705f+forward*.009f;
    mesh.Ellipsoid(head,right,up,forward,.105f,.145f,.098f,Material::Skin,16,11);
    // Recessed close-cropped hair preserves the forehead and jaw instead of reading as a helmet.
    const Vector3 hairCentre=head+up*.061f-forward*.037f;
    mesh.Ellipsoid(hairCentre,right,up,forward,.109f,.108f,.083f,Material::Hair,16,9);
    for(int curl=0;curl<5;++curl) {
        const Vector3 c=head+right*((curl-2)*.034f)+up*(.142f-std::abs(curl-2)*.004f)-forward*.035f;
        mesh.Ellipsoid(c,right,up,forward,.019f,.017f,.018f,Material::Hair,8,5);
    }
    mesh.Ellipsoid(head+right*.108f-up*.004f,right,up,forward,.015f,.031f,.012f,Material::Skin,9,6);
    mesh.Ellipsoid(head-right*.108f-up*.004f,right,up,forward,.015f,.031f,.012f,Material::Skin,9,6);
    for(int side=-1;side<=1;side+=2) {
        const Vector3 eye=head+right*(side*.038f)+up*.031f+forward*.094f;
        mesh.Ellipsoid(eye,right,up,forward,.017f,.008f,.006f,Material::Eye,10,5);
        mesh.Ellipsoid(eye+forward*.006f,right,up,forward,.0055f,.0055f,.003f,Material::Hair,8,5);
        const Vector3 browA=head+right*(side*.018f)+up*.054f+forward*.095f;
        const Vector3 browB=head+right*(side*.058f)+up*.051f+forward*.091f;
        mesh.Tube(browA,browB,.0035f,.003f,Material::Hair,7);
    }
    mesh.Ellipsoid(head-up*.005f+forward*.100f,right,up,forward,.020f,.035f,.025f,Material::Skin,11,7);
    const Vector3 mouth=head-up*.055f+forward*.096f;
    mesh.Tube(mouth-right*.026f,mouth+right*.026f,.0055f,.005f,Material::Lip,8);

    for(std::size_t side=0;side<2;++side) {
        const Vector3 shoulder=Point(pose.shoulders[side]),elbow=Point(pose.elbows[side]),wrist=Point(pose.wrists[side]);
        const Vector3 sleeveEnd=Lerp(shoulder,elbow,.22f);
        mesh.Tube(shoulder,sleeveEnd,.058f,.055f,Material::Cloth,11);
        mesh.Tube(sleeveEnd,elbow,.050f,.045f,Material::Skin,11);
        mesh.Ellipsoid(elbow,right,up,forward,.047f,.048f,.047f,Material::Skin,10,6);
        mesh.Tube(elbow,wrist,.044f,.033f,Material::Skin,11);
        const Vector3 hand=wrist+up*-.055f+forward*.006f;
        mesh.Ellipsoid(hand,right,up,forward,.038f,.076f,.030f,Material::Skin,10,7);

        const auto& leg=pose.legs[side];const Vector3 hip=Point(leg.hip),knee=Point(leg.knee),ankle=Point(leg.ankle);
        const Vector3 shortEnd=Lerp(hip,knee,.53f);
        mesh.Tube(hip,shortEnd,.105f,.092f,Material::Cloth,12);
        mesh.Tube(shortEnd,knee,.080f,.070f,Material::Skin,11);
        mesh.Ellipsoid(knee,right,up,forward,.071f,.072f,.068f,Material::Skin,10,6);
        mesh.Tube(knee,ankle,.067f,.044f,Material::Skin,11);
        const Vector3 footRight=Point(leg.foot.right),footUp=Point(leg.foot.up),footForward=Point(leg.foot.forward);
        const Vector3 foot=Point(leg.foot.centre)+footUp*.038f+footForward*.025f;
        mesh.Ellipsoid(foot,footRight,footUp,footForward,.068f,.043f,.145f,Material::Skin,12,7);
        const Vector3 sole=Point(leg.foot.centre)+footUp*.012f+footForward*.025f;
        mesh.Ellipsoid(sole,footRight,footUp,footForward,.073f,.017f,.154f,Material::Fibre,12,5);
        mesh.Tube(foot-footRight*.068f+footUp*.015f,foot+footRight*.068f+footUp*.015f,
            .012f,.012f,Material::Fibre,7);
        mesh.Tube(foot-footRight*.055f+footForward*.055f+footUp*.012f,
            foot+footRight*.055f+footForward*.055f+footUp*.012f,.010f,.010f,Material::Fibre,7);
    }
}

bool IsVisible(const Matrix& view,const Matrix& projection,const Vector3& camera,
    const ResidentState& state,float maximumDistance) {
    const Vector3 centre(state.x,state.y+.9f,state.z);
    return Vector3::DistanceSquared(centre,camera)<=maximumDistance*maximumDistance
        &&BoundingFrustum(view*projection).Intersects(BoundingSphere(centre,1.25f));
}
}

ResidentRenderer::ResidentRenderer(GraphicsDevice& device,const CanopyLightField& canopy,float maximumDistance)
    :device_(device),canopy_(canopy),maximumDistance_(maximumDistance),
     effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
         VertexCapacity,BufferUsage::WriteOnly)),
     shadowBuffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
         ShadowCapacity,BufferUsage::WriteOnly)),materialAtlas_(CreateMaterialAtlas(device)) {
    if(!std::isfinite(maximumDistance)||maximumDistance<=0)throw std::invalid_argument("Resident draw distance must be positive");
    vertices_.reserve(VertexCapacity);shadows_.reserve(ShadowCapacity);
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
    effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(maximumDistance*.65f);effect_->setFogEndProperty(maximumDistance);
    const char* diagnostic="island-simulator: one adult resident uses original articulated human geometry and a 128x128 skin, hair, cloth and plant-fibre material atlas";
    std::cout<<diagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
}
ResidentRenderer::~ResidentRenderer()=default;

void ResidentRenderer::DrawShadow(const Matrix& view,const Matrix& projection,const Vector3& camera,
    const ResidentState& resident) {
    draws_=triangles_=shadowTriangles_=visible_=0;shadows_.clear();
    if(!IsVisible(view,projection,camera,resident,maximumDistance_))return;
    for(const auto& leg:resident.pose.legs) {
        const Vector3 c=Point(leg.foot.centre)+Point(leg.foot.up)*.006f;
        const Vector3 r=Point(leg.foot.right)*.085f,f=Point(leg.foot.forward)*.19f;
        constexpr int segments=12;const Color edge(22,19,15,0),centre(22,19,15,54);
        for(int i=0;i<segments;++i) {
            const float a0=2*Pi*i/segments,a1=2*Pi*(i+1)/segments;
            shadows_.emplace_back(c,centre,Vector2(.5f,.5f));
            shadows_.emplace_back(c+r*std::cos(a0)+f*std::sin(a0),edge,Vector2::Zero);
            shadows_.emplace_back(c+r*std::cos(a1)+f*std::sin(a1),edge,Vector2::Zero);
        }
    }
    shadowTriangles_=static_cast<int>(shadows_.size()/3);
    shadowBuffer_->SetData(shadows_.data(),0,static_cast<int>(shadows_.size()),SetDataOptions::Discard);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::AlphaBlend);
    device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
    device_.SetVertexBuffer(shadowBuffer_.get());device_.setIndicesProperty(nullptr);
    effect_->setTextureEnabledProperty(false);effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,shadowTriangles_);++draws_;
}

void ResidentRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,
    const ResidentState& resident) {
    vertices_.clear();visible_=0;triangles_=0;
    if(!IsVisible(view,projection,camera,resident,maximumDistance_))return;
    const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(resident.x,resident.y+1.0f,resident.z,resident.y));
    BuildHuman(vertices_,resident,tint);
    if(vertices_.empty()||vertices_.size()>VertexCapacity)throw std::length_error("Resident mesh exceeds bounded upload");
    triangles_=static_cast<int>(vertices_.size()/3);visible_=1;
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;
    effect_->setTextureEnabledProperty(true);effect_->setTextureProperty(materialAtlas_.get());
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
    effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_);++draws_;
}
}
