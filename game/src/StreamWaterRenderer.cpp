#include "IslandSimulator/StreamWaterRenderer.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include <iostream>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
constexpr const char* vertex=R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
uniform mat4 uViewProjection;
out vec3 world;out float opacity;
void main(){
    world=aPosition;opacity=aColor.a;
    gl_Position=uViewProjection*vec4(aPosition,1.0);
    // XNA projection uses [0,1] depth; desktop GL and GLES require [-1,1].
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
constexpr const char* fragment=R"(
in vec3 world;in float opacity;
uniform float uTime;uniform vec3 uCamera;
out vec4 fragColor;
void main(){
    vec2 flow=normalize(vec2(-1.0,.23));
    float along=dot(world.xz,flow),across=dot(world.xz,vec2(-flow.y,flow.x));
    float ripple=sin(along*5.0-uTime*1.5+sin(across*3.0))*.045;
    float crossRipple=sin(across*8.0+along*2.0-uTime*.7)*.018;
    vec3 n=normalize(vec3(ripple,1.0,crossRipple));
    vec3 eye=normalize(uCamera-world);
    float fresnel=.04+.40*pow(1.0-abs(dot(n,eye)),4.0);
    float glint=pow(max(0.0,dot(n,normalize(eye+normalize(vec3(-.4,.8,.3))))),64.0);
    float dapple=.5+.5*sin(along*.57+sin(across*1.9-uTime*.12));
    vec3 water=mix(vec3(.16,.36,.30),vec3(.43,.66,.69),fresnel);
    water*=mix(.94,1.06,dapple);
    water+=vec3(.35,.36,.29)*glint*.35;
    float alpha=opacity*(.57+fresnel*.40);
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
    std::vector<VertexPositionColor> vertices;vertices.reserve(mesh.vertices.size());
    Vector3 low(1000,1000,1000),high(-1000,-1000,-1000);
    for(const auto p:mesh.vertices) {
        const Vector3 position(p.x,p.y,p.z);low=Vector3::Min(low,position);high=Vector3::Max(high,position);
        vertices.emplace_back(position,Color(255,255,255,static_cast<int>(p.alpha*255)));
    }
    bounds_=BoundingBox(low-Vector3(.05f,.05f,.05f),high+Vector3(.05f,.05f,.05f));
    count_=static_cast<int>(vertices.size());triangles_=static_cast<int>(mesh.indices.size()/3);
    vertices_=std::make_unique<VertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),count_,BufferUsage::WriteOnly);
    vertices_->SetData(vertices.data(),count_);
    indices_=std::make_unique<IndexBuffer>(device,IndexElementSize::SixteenBits,static_cast<int>(mesh.indices.size()),BufferUsage::WriteOnly);
    indices_->SetData(mesh.indices.data(),static_cast<int>(mesh.indices.size()));
    std::cout<<"island-simulator: stream water "<<count_<<" vertices, "<<triangles_<<" triangles, portable moving-ripple shader\n";
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
