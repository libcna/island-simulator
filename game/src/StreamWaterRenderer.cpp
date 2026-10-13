#include "IslandSimulator/StreamWaterRenderer.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include <cstddef>
#include <iostream>
#include <type_traits>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
struct StreamGpuVertex {
    float position[3];
    float flowDistanceOpacity[4];
    float lateral;
};
static_assert(std::is_trivially_copyable_v<StreamGpuVertex>);
static_assert(std::is_standard_layout_v<StreamGpuVertex>&&sizeof(StreamGpuVertex)==32);
const VertexDeclaration& StreamDeclaration() {
    static const VertexDeclaration declaration{
        VertexElement(offsetof(StreamGpuVertex,position),VertexElementFormat::Vector3,
                      VertexElementUsage::Position,0),
        VertexElement(offsetof(StreamGpuVertex,flowDistanceOpacity),VertexElementFormat::Vector4,
                      VertexElementUsage::TextureCoordinate,0),
        VertexElement(offsetof(StreamGpuVertex,lateral),VertexElementFormat::Single,
                      VertexElementUsage::TextureCoordinate,1)};
    return declaration;
}
constexpr const char* vertex=R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aFlowDistanceOpacity;
layout(location=2) in float aLateral;
uniform mat4 uViewProjection;
out vec3 world;out vec2 flow;out float courseDistance;out float opacity;out float lateral;
void main(){
    world=aPosition;flow=aFlowDistanceOpacity.xy;
    courseDistance=aFlowDistanceOpacity.z;opacity=aFlowDistanceOpacity.w;lateral=aLateral;
    gl_Position=uViewProjection*vec4(aPosition,1.0);
    // XNA projection uses [0,1] depth; desktop GL and GLES require [-1,1].
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
constexpr const char* fragment=R"(
in vec3 world;in vec2 flow;in float courseDistance;in float opacity;in float lateral;
uniform float uTime;uniform vec3 uCamera;
out vec4 fragColor;
void main(){
    vec2 downstream=normalize(flow);
    vec2 bankNormal=vec2(-downstream.y,downstream.x);
    float across=dot(world.xz,bankNormal);
    float primary=sin(courseDistance*5.1-uTime*1.65+sin(across*3.3))*.040;
    float crossRipple=sin(across*8.7+courseDistance*1.7-uTime*.73)*.017;
    vec2 slope=downstream*primary+bankNormal*crossRipple;
    vec3 n=normalize(vec3(slope.x,1.0,slope.y));
    vec3 eye=normalize(uCamera-world);
    float fresnel=.035+.43*pow(1.0-abs(dot(n,eye)),4.0);
    float glint=pow(max(0.0,dot(n,normalize(eye+normalize(vec3(-.4,.8,.3))))),64.0);
    float broad=.5+.5*sin(courseDistance*.57+sin(across*1.9-uTime*.12));
    float streakWave=.5+.5*sin(courseDistance*8.3-uTime*2.45+sin(across*5.2)*.45);
    float streak=smoothstep(.76,.98,streakWave)*(.45+.55*broad);
    float bankBand=smoothstep(.55,.75,abs(lateral))*(1.0-smoothstep(.91,.99,abs(lateral)));
    float riffleCarrier=.5+.5*sin(courseDistance*1.17+sin(across*4.1)*.7);
    float riffle=smoothstep(.78,.95,riffleCarrier)*(.5+.5*sin(courseDistance*12.4-uTime*3.0+across*7.0));
    vec3 water=mix(vec3(.105,.285,.235),vec3(.42,.68,.72),fresnel);
    water*=mix(.91,1.055,broad);
    water+=vec3(.20,.28,.25)*streak*.24+vec3(.38,.38,.31)*glint*.38;
    water+=vec3(.36,.43,.35)*(bankBand*(.15+.12*streak)+riffle*.11);
    float alpha=opacity*(.47+fresnel*.46+streak*.065+bankBand*.14+riffle*.08);
    fragColor=vec4(water*alpha,alpha);
}
)";
}
StreamWaterRenderer::StreamWaterRenderer(GraphicsDevice& device,const IslandTerrain& terrain,const StreamBedPatch& bed):device_(device) {
    using CNA::Graphics::ShaderCodeEXT;
    const auto source=[](bool es,const char* body){return std::string(es?"#version 300 es\nprecision highp float;\n":"#version 330 core\n")+body;};
    const CNA::Graphics::ShaderPackageEXT package({
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Vertex,"main","stream.es.vert",source(true,vertex)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Fragment,"main","stream.es.frag",source(true,fragment)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Vertex,"main","stream.vert",source(false,vertex)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Fragment,"main","stream.frag",source(false,fragment))},
        {CNA::ShaderStageEXT::Vertex,CNA::ShaderStageEXT::Fragment},{});
    effect_=std::make_unique<ShaderEffect>(device,package);
    if(!effect_->IsEffectValid())throw std::runtime_error("Stream water shader failed: "+effect_->GetCompileErrorEXT());
    const StreamWaterMesh mesh(StreamCourse(terrain),bed);
    std::vector<StreamGpuVertex> vertices;vertices.reserve(mesh.vertices.size());
    Vector3 low(1000,1000,1000),high(-1000,-1000,-1000);
    for(const auto p:mesh.vertices) {
        const Vector3 position(p.x,p.y,p.z);low=Vector3::Min(low,position);high=Vector3::Max(high,position);
        vertices.push_back({{p.x,p.y,p.z},{p.flowX,p.flowZ,p.distance,p.alpha},p.lateral});
    }
    bounds_=BoundingBox(low-Vector3(.05f,.05f,.05f),high+Vector3(.05f,.05f,.05f));
    count_=static_cast<int>(vertices.size());triangles_=static_cast<int>(mesh.indices.size()/3);
    vertices_=std::make_unique<VertexBuffer>(device,StreamDeclaration(),count_,BufferUsage::WriteOnly);
    vertices_->SetData(vertices.data(),count_);
    indices_=std::make_unique<IndexBuffer>(device,IndexElementSize::SixteenBits,static_cast<int>(mesh.indices.size()),BufferUsage::WriteOnly);
    indices_->SetData(mesh.indices.data(),static_cast<int>(mesh.indices.size()));
    std::cout<<"island-simulator: stream water "<<count_<<" vertices, "<<triangles_
        <<" triangles, route-following downstream ripple and translucent current streaks\n";
    std::cout<<"island-simulator: stream water uses bank-edge sheen and broken shallow riffles\n";
}
StreamWaterRenderer::~StreamWaterRenderer()=default;
void StreamWaterRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,float seconds) {
    draws_=0;if(!BoundingFrustum(view*projection).Intersects(bounds_))return;
    static bool reported=false;
    if(!reported){std::cout<<"island-simulator: stream water first visible draw\n";reported=true;}
    float matrix[16];(view*projection).ToColumnMajor(matrix);
    effect_->Apply();effect_->SetUniformMat4("uViewProjection",matrix);
    effect_->SetUniformFloat("uTime",std::isfinite(seconds)?seconds:0);
    effect_->SetUniformVec3("uCamera",camera.X,camera.Y,camera.Z);
    device_.setBlendStateProperty(BlendState::AlphaBlend);device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.SetVertexBuffer(vertices_.get());device_.setIndicesProperty(indices_.get());
    device_.DrawIndexedPrimitives(PrimitiveType::TriangleList,0,0,count_,0,triangles_);draws_=1;
}
}
