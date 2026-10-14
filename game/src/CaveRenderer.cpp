#include "IslandSimulator/CaveRenderer.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <numbers>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
struct CaveGpuVertex {
    float position[3];
    float normal[3];
    float localMaterial[4];
};
static_assert(std::is_trivially_copyable_v<CaveGpuVertex>);
static_assert(std::is_standard_layout_v<CaveGpuVertex> && sizeof(CaveGpuVertex)==40);
const VertexDeclaration& CaveDeclaration() {
    static const VertexDeclaration declaration{
        VertexElement(offsetof(CaveGpuVertex,position),VertexElementFormat::Vector3,
                      VertexElementUsage::Position,0),
        VertexElement(offsetof(CaveGpuVertex,normal),VertexElementFormat::Vector3,
                      VertexElementUsage::Normal,0),
        VertexElement(offsetof(CaveGpuVertex,localMaterial),VertexElementFormat::Vector4,
                      VertexElementUsage::TextureCoordinate,0)};
    return declaration;
}
constexpr const char* vertex=R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aLocalMaterial;
uniform mat4 uViewProjection;
out vec3 world;out vec3 normal;out vec4 localMaterial;
void main(){
    world=aPosition;normal=normalize(aNormal);localMaterial=aLocalMaterial;
    gl_Position=uViewProjection*vec4(aPosition,1.0);
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
constexpr const char* fragment=R"(
in vec3 world;in vec3 normal;in vec4 localMaterial;
uniform vec3 uToSun;
uniform sampler2D uRockTexture;
uniform vec3 uRockMean;
out vec4 fragColor;
float hash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){
    vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),
                   mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),
                   mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}
vec2 mirrored(vec2 p){
    vec2 f=fract(p*.5);
    return 1.0-abs(f*2.0-1.0);
}
void main(){
    vec3 local=localMaterial.xyz;
    float grassBlade=step(1.5,localMaterial.w);
    float exterior=min(localMaterial.w,1.0);
    float broad=noise(local*vec3(.43,.55,.31)+vec3(2.7,1.1,4.3));
    float grain=noise(world*11.0);
    float fracturePhase=local.y*2.15+local.x*.31+local.z*.14
        +(noise(local*.46+vec3(7.1,2.4,5.3))-.5)*3.1;
    float fracture=1.0-smoothstep(.018,.16,abs(sin(fracturePhase)));
    vec3 stone=mix(vec3(.27,.285,.29),vec3(.52,.47,.38),broad);
    stone*=.87+.18*grain;
    stone*=1.0-.105*fracture;
    vec3 weights=pow(abs(normalize(normal)),vec3(4.0));
    weights/=max(weights.x+weights.y+weights.z,.0001);
    vec3 rockSample=texture(uRockTexture,mirrored(local.yz*.72+vec2(.17,.31))).rgb*weights.x
        +texture(uRockTexture,mirrored(local.xz*.72+vec2(.43,.11))).rgb*weights.y
        +texture(uRockTexture,mirrored(local.xy*.72+vec2(.29,.57))).rgb*weights.z;
    vec3 rockDetail=clamp(rockSample/max(uRockMean,vec3(.05)),vec3(.55),vec3(1.55));
    stone*=mix(vec3(1.0),rockDetail,.62);
    float upward=clamp(normal.y*.72+.28,0.0,1.0);
    float mossMask=smoothstep(.56,.78,noise(local*.29+vec3(11.2,4.0,1.8)))
        *upward*exterior;
    stone=mix(stone,vec3(.245,.31,.19),mossMask*.42);
    // The cave is inside the hill: upward outer mantle becomes an earth/grass cap,
    // while the portal lip and steep side rock remain exposed and readable.
    float cap=exterior*smoothstep(-.05,.38,normal.y)*smoothstep(-.35,1.35,local.z);
    float earthNoise=noise(local*vec3(.38,.24,.41)+vec3(4.2,1.7,8.3));
    vec3 earth=mix(vec3(.285,.225,.125),vec3(.365,.305,.175),earthNoise);
    float turf=smoothstep(.47,.69,noise(local*.52+vec3(9.1,3.0,2.4)));
    vec3 covered=mix(earth,vec3(.205,.315,.095),turf*.38);
    stone=mix(stone,covered,cap*.985);
    if(grassBlade>0.5)
        stone=mix(vec3(.13,.225,.055),vec3(.225,.345,.085),
                  .35+.65*noise(local*1.7+vec3(3.0,7.0,2.0)));
    float damp=(1.0-exterior)*smoothstep(5.0,18.0,local.z)
        *(.55+.45*noise(local*.37+vec3(1.0,8.0,3.0)));
    stone=mix(stone,vec3(.20,.225,.215),damp*.22);
    float daylight=.42+.58*max(dot(normalize(normal),normalize(uToSun)),0.0);
    float mouthLight=.25+.67*exp(-max(local.z,0.0)*.18);
    float illumination=mix(mouthLight*(.84+.16*max(normal.y,0.0)),daylight,exterior);
    illumination=mix(illumination,.72+.28*daylight,grassBlade);
    fragColor=vec4(stone*illumination,1.0);
}
)";
}
CaveRenderer::CaveRenderer(GraphicsDevice& device,const CaveLayout& cave):device_(device) {
    constexpr int TextureSize=256;
    const auto levels=LoadOpaqueImageMips(device_,"Content/structures/cave-rock.png",TextureSize);
    const auto& mean=levels.back().pixels[0];
    rockMean_[0]=mean[0]/255.0f;rockMean_[1]=mean[1]/255.0f;rockMean_[2]=mean[2]/255.0f;
    rockTexture_=std::make_unique<Texture2D>(device_,TextureSize,TextureSize,true,SurfaceFormat::Color);
    for(std::size_t level=0;level<levels.size();++level) {
        const auto& pixels=levels[level].pixels;
        rockTexture_->SetData(static_cast<int>(level),nullptr,
            reinterpret_cast<const std::uint8_t*>(pixels.data()),0,static_cast<int>(pixels.size()*4));
    }
    using CNA::Graphics::ShaderCodeEXT;
    const auto source=[](bool es,const char* body){return std::string(es?"#version 300 es\nprecision highp float;\n":"#version 330 core\n")+body;};
    const CNA::Graphics::ShaderPackageEXT package({
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Vertex,"main","cave.es.vert",source(true,vertex)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Fragment,"main","cave.es.frag",source(true,fragment)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Vertex,"main","cave.vert",source(false,vertex)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Fragment,"main","cave.frag",source(false,fragment))},
        {CNA::ShaderStageEXT::Vertex,CNA::ShaderStageEXT::Fragment},{});
    effect_=std::make_unique<ShaderEffect>(device,package);
    if(!effect_->IsEffectValid())throw std::runtime_error("Cave shader failed: "+effect_->GetCompileErrorEXT());
    std::vector<CaveGpuVertex> vertices;
    // Share lighting normals across the connected arch strips while retaining the
    // eroded entrance rim and back wall as intentionally hard geological breaks.
    std::map<std::array<float,4>,Vector3> normalSums;
    for(const auto& triangle:cave.Triangles()) {
        if(triangle.surface!=CaveSurface::Exterior&&triangle.surface!=CaveSurface::Interior)continue;
        for(const auto p:triangle.points)
            normalSums[{static_cast<float>(triangle.surface),p.x,p.y,p.z}]
                +=Vector3(triangle.normal.x,triangle.normal.y,triangle.normal.z);
    }
    for(const auto& triangle:cave.Triangles()) {
        // The matching terrain replacement provides the visible floor, once only.
        if(triangle.surface==CaveSurface::Floor)continue;
        for(const auto p:triangle.points) {
            const auto local=cave.Local(p);
            const float material=triangle.surface==CaveSurface::Exterior?1.0f:
                triangle.surface==CaveSurface::Entrance?.62f:0.0f;
            Vector3 normal(triangle.normal.x,triangle.normal.y,triangle.normal.z);
            if(triangle.surface==CaveSurface::Exterior||triangle.surface==CaveSurface::Interior)
                normal=Vector3::Normalize(normalSums.at(
                    {static_cast<float>(triangle.surface),p.x,p.y,p.z}));
            vertices.push_back({{p.x,p.y,p.z},
                {normal.X,normal.Y,normal.Z},
                {local.x,local.y,local.z,material}});
        }
    }
    // Sparse crossed blades give the earth-toned crown a real grass silhouette from
    // above. They reuse this draw/shader and sit only on upward outer-shell triangles.
    int capCandidate=0,grassTuftCount=0;
    for(const auto& triangle:cave.Triangles()) {
        if(triangle.surface!=CaveSurface::Exterior||triangle.normal.y<.46f)continue;
        CavePoint centre{};
        for(const auto p:triangle.points) {
            centre.x+=p.x/3.0f;centre.y+=p.y/3.0f;centre.z+=p.z/3.0f;
        }
        const auto local=cave.Local(centre);
        if(local.z<.35f||local.z>CaveLayout::OuterLength-1.0f)continue;
        if(capCandidate++%2!=0)continue;
        const float angle=std::fmod(static_cast<float>(capCandidate)*2.3999632f,2.0f*std::numbers::pi_v<float>);
        const float height=.16f+.07f*std::sin(static_cast<float>(capCandidate)*1.71f)*.5f+.035f;
        const Vector3 c(centre.x,centre.y+.018f,centre.z);
        const Vector3 first(std::cos(angle)*.055f,0,std::sin(angle)*.055f);
        const Vector3 second(-first.Z,0,first.X);
        const auto blade=[&](const Vector3 side,const float lean) {
            const Vector3 a=c-side,b=c+side,tip=c+Vector3(side.Z*lean,height,-side.X*lean);
            Vector3 normal=Vector3::Cross(b-a,tip-a);normal.Normalize();
            for(const auto& p:{a,b,tip}) {
                const auto l=cave.Local({p.X,p.Y,p.Z});
                vertices.push_back({{p.X,p.Y,p.Z},{normal.X,normal.Y,normal.Z},
                    {l.x,l.y,l.z,2.0f}});
            }
        };
        blade(first,.35f);blade(second,-.25f);++grassTuftCount;
    }
    // Shallow eroded ceiling pendants break the otherwise continuous tunnel arch.
    // Their tips remain above 2.3m local height, clear of the walking capsule, so
    // they need no extra collision proxy and cannot narrow the traversable corridor.
    struct Pendant {float across,rootHeight,depth,radius,length,tipAcross;};
    constexpr std::array<Pendant,6> pendants{{
        {-.95f,2.88f,2.40f,.20f,.46f,.035f},
        { .65f,3.10f,4.10f,.17f,.42f,-.028f},
        {-1.25f,2.75f,5.80f,.22f,.38f,.042f},
        {1.05f,2.90f,7.50f,.18f,.56f,-.030f},
        {-.45f,3.15f,9.30f,.20f,.49f,.025f},
        { .70f,3.05f,11.0f,.14f,.35f,-.020f},
    }};
    constexpr int pendantSides=7;
    constexpr float tau=2.0f*std::numbers::pi_v<float>;
    const auto appendPendantTriangle=[&](CavePoint a,CavePoint b,CavePoint c,
                                         CavePoint centre) {
        Vector3 pa(a.x,a.y,a.z),pb(b.x,b.y,b.z),pc(c.x,c.y,c.z);
        Vector3 normal=Vector3::Cross(pb-pa,pc-pa);
        const Vector3 radial=(pa+pb)*.5f-Vector3(centre.x,centre.y,centre.z);
        if(Vector3::Dot(normal,radial)<0) {std::swap(b,c);std::swap(pb,pc);normal*=-1.0f;}
        normal.Normalize();
        for(const auto p:{a,b,c}) {
            const auto local=cave.Local(p);
            vertices.push_back({{p.x,p.y,p.z},{normal.X,normal.Y,normal.Z},
                {local.x,local.y,local.z,0.0f}});
        }
    };
    for(std::size_t pendantIndex=0;pendantIndex<pendants.size();++pendantIndex) {
        const auto& pendant=pendants[pendantIndex];
        const CavePoint centre=cave.World(pendant.across,pendant.rootHeight,pendant.depth);
        const CavePoint middleCentre=cave.World(pendant.across+pendant.tipAcross*.55f,
            pendant.rootHeight-pendant.length*.55f,pendant.depth+.020f);
        const CavePoint tip=cave.World(pendant.across+pendant.tipAcross,
            pendant.rootHeight-pendant.length,pendant.depth+.035f);
        for(int side=0;side<pendantSides;++side) {
            const float a=tau*static_cast<float>(side)/pendantSides;
            const float b=tau*static_cast<float>(side+1)/pendantSides;
            const auto radius=[&](const float angle,const float scale) {
                return pendant.radius*scale*(1.0f+.16f*std::sin(angle*3.0f
                    +static_cast<float>(pendantIndex)*1.7f));
            };
            const float rootA=radius(a,1.0f),rootB=radius(b,1.0f);
            const float middleA=radius(a+.12f,.48f),middleB=radius(b+.12f,.48f);
            const CavePoint first=cave.World(pendant.across+std::cos(a)*rootA,
                pendant.rootHeight,pendant.depth+std::sin(a)*rootA*.72f);
            const CavePoint second=cave.World(pendant.across+std::cos(b)*rootB,
                pendant.rootHeight,pendant.depth+std::sin(b)*rootB*.72f);
            const CavePoint middleFirst=cave.World(
                pendant.across+pendant.tipAcross*.55f+std::cos(a+.12f)*middleA,
                pendant.rootHeight-pendant.length*.55f,
                pendant.depth+.020f+std::sin(a+.12f)*middleA*.72f);
            const CavePoint middleSecond=cave.World(
                pendant.across+pendant.tipAcross*.55f+std::cos(b+.12f)*middleB,
                pendant.rootHeight-pendant.length*.55f,
                pendant.depth+.020f+std::sin(b+.12f)*middleB*.72f);
            appendPendantTriangle(first,middleFirst,middleSecond,centre);
            appendPendantTriangle(first,middleSecond,second,centre);
            appendPendantTriangle(middleFirst,tip,middleSecond,middleCentre);
        }
    }
    const auto bounds=cave.Bounds();
    bounds_=BoundingBox(Vector3(bounds[0].x,bounds[0].y,bounds[0].z),Vector3(bounds[1].x,bounds[1].y,bounds[1].z));
    count_=static_cast<int>(vertices.size());
    vertices_=std::make_unique<VertexBuffer>(device,CaveDeclaration(),count_,BufferUsage::WriteOnly);
    vertices_->SetData(vertices.data(),count_);
    std::cout<<"island-simulator: cave rock "<<count_/3<<" triangles in one culled draw; 20m walkable interior\n";
    std::cout<<"island-simulator: cave compact asymmetric mouth uses normal-lit fractured stone and restrained moss\n";
    std::cout<<"island-simulator: cave narrow eroded lip grows into an irregular closed rock mantle\n";
    std::cout<<"island-simulator: cave connected shell strips share smooth lighting normals while portal breaks stay sharp\n";
    std::cout<<"island-simulator: cave adds six high-clearance eroded ceiling pendants in the existing draw\n";
    std::cout<<"island-simulator: cave mouth is embedded in a rising inland mountain slope\n";
    std::cout<<"island-simulator: cave crown carries an earth-and-grass cap with "
             <<grassTuftCount<<" sparse blade tufts in the existing draw\n";
    const char* material="island-simulator: cave rock loaded Content/structures/cave-rock.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored triplanar sampling";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
}
CaveRenderer::~CaveRenderer()=default;
void CaveRenderer::Draw(const Matrix& view,const Matrix& projection) {
    draws_=0;if(!BoundingFrustum(view*projection).Intersects(bounds_))return;
    static bool reported=false;
    if(!reported){std::cout<<"island-simulator: cave rock first visible draw\n";reported=true;}
    float matrix[16];(view*projection).ToColumnMajor(matrix);
    effect_->Apply();effect_->SetUniformMat4("uViewProjection",matrix);
    effect_->SetUniformVec3("uToSun",environment::SunDirectionX,environment::SunDirectionY,
                            environment::SunDirectionZ);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;
    effect_->SetTexture(0,*rockTexture_);effect_->SetUniformInt("uRockTexture",0);
    effect_->SetUniformVec3("uRockMean",rockMean_[0],rockMean_[1],rockMean_[2]);
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.SetVertexBuffer(vertices_.get());
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,count_/3);draws_=1;
}
}
