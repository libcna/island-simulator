#include "IslandSimulator/CaveRenderer.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
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
#include <cstddef>
#include <iostream>
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
out vec4 fragColor;
float hash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){
    vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),
                   mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),
                   mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}
void main(){
    vec3 local=localMaterial.xyz;
    float exterior=localMaterial.w;
    float broad=noise(local*vec3(.43,.55,.31)+vec3(2.7,1.1,4.3));
    float grain=noise(world*11.0);
    float fracturePhase=local.y*2.15+local.x*.31+local.z*.14
        +(noise(local*.46+vec3(7.1,2.4,5.3))-.5)*3.1;
    float fracture=1.0-smoothstep(.018,.16,abs(sin(fracturePhase)));
    vec3 stone=mix(vec3(.27,.285,.29),vec3(.52,.47,.38),broad);
    stone*=.87+.18*grain;
    stone*=1.0-.105*fracture;
    float upward=clamp(normal.y*.72+.28,0.0,1.0);
    float mossMask=smoothstep(.56,.78,noise(local*.29+vec3(11.2,4.0,1.8)))
        *upward*exterior;
    stone=mix(stone,vec3(.245,.31,.19),mossMask*.42);
    float damp=(1.0-exterior)*smoothstep(5.0,18.0,local.z)
        *(.55+.45*noise(local*.37+vec3(1.0,8.0,3.0)));
    stone=mix(stone,vec3(.20,.225,.215),damp*.22);
    float daylight=.42+.58*max(dot(normalize(normal),normalize(uToSun)),0.0);
    float mouthLight=.25+.67*exp(-max(local.z,0.0)*.18);
    float illumination=mix(mouthLight*(.84+.16*max(normal.y,0.0)),daylight,exterior);
    fragColor=vec4(stone*illumination,1.0);
}
)";
}
CaveRenderer::CaveRenderer(GraphicsDevice& device,const CaveLayout& cave):device_(device) {
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
    for(const auto& triangle:cave.Triangles()) {
        // The matching terrain replacement provides the visible floor, once only.
        if(triangle.surface==CaveSurface::Floor)continue;
        for(const auto p:triangle.points) {
            const auto local=cave.Local(p);
            const float material=triangle.surface==CaveSurface::Exterior?1.0f:
                triangle.surface==CaveSurface::Entrance?.62f:0.0f;
            vertices.push_back({{p.x,p.y,p.z},
                {triangle.normal.x,triangle.normal.y,triangle.normal.z},
                {local.x,local.y,local.z,material}});
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
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.SetVertexBuffer(vertices_.get());
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,count_/3);draws_=1;
}
}
