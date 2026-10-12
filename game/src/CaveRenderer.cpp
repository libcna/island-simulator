#include "IslandSimulator/CaveRenderer.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
constexpr const char* vertex=R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
uniform mat4 uViewProjection;
out vec3 world;out vec3 light;
void main(){
    world=aPosition;light=aColor.rgb;
    gl_Position=uViewProjection*vec4(aPosition,1.0);
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
constexpr const char* fragment=R"(
in vec3 world;in vec3 light;
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
    float broad=noise(world*.63),grain=noise(world*14.0);
    float bands=sin(world.y*5.0+noise(world*.8)*3.0+world.x*.13);
    float seam=1.0-smoothstep(.015,.10,abs(bands));
    vec3 stone=mix(vec3(.31,.32,.31),vec3(.51,.47,.39),broad);
    stone*=.86+.20*grain;stone*=1.0-.25*seam;
    fragColor=vec4(stone*light,1.0);
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
    std::vector<VertexPositionColor> vertices;
    for(const auto& triangle:cave.Triangles()) {
        // The matching terrain replacement provides the visible floor, once only.
        if(triangle.surface==CaveSurface::Floor)continue;
        for(const auto p:triangle.points) {
            const auto local=cave.Local(p);
            const bool inside=triangle.surface==CaveSurface::Interior||triangle.surface==CaveSurface::Back;
            float light=.40f+.60f*std::max(0.0f,triangle.normal.x*environment::SunDirectionX
                +triangle.normal.y*environment::SunDirectionY+triangle.normal.z*environment::SunDirectionZ);
            if(inside)light=(.23f+.60f*std::exp(-std::max(0.0f,local.z)*.20f))
                *(.82f+.18f*std::max(0.0f,triangle.normal.y));
            const int value=static_cast<int>(std::clamp(light,0.0f,1.0f)*255);
            vertices.emplace_back(Vector3(p.x,p.y,p.z),Color(value,value,value,255));
        }
    }
    const auto bounds=cave.Bounds();
    bounds_=BoundingBox(Vector3(bounds[0].x,bounds[0].y,bounds[0].z),Vector3(bounds[1].x,bounds[1].y,bounds[1].z));
    count_=static_cast<int>(vertices.size());
    vertices_=std::make_unique<VertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),count_,BufferUsage::WriteOnly);
    vertices_->SetData(vertices.data(),count_);
    std::cout<<"island-simulator: cave rock "<<count_/3<<" triangles in one culled draw; 20m walkable interior\n";
}
CaveRenderer::~CaveRenderer()=default;
void CaveRenderer::Draw(const Matrix& view,const Matrix& projection) {
    draws_=0;if(!BoundingFrustum(view*projection).Intersects(bounds_))return;
    static bool reported=false;
    if(!reported){std::cout<<"island-simulator: cave rock first visible draw\n";reported=true;}
    float matrix[16];(view*projection).ToColumnMajor(matrix);
    effect_->Apply();effect_->SetUniformMat4("uViewProjection",matrix);
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.SetVertexBuffer(vertices_.get());
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,count_/3);draws_=1;
}
}
