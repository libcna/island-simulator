#include "IslandSimulator/CampfireRenderer.hpp"
#include "IslandSimulator/CampfireLighting.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
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
layout(location=2) in vec2 aTexCoord;
uniform mat4 uViewProjection;
uniform vec3 uRight;uniform vec3 uForward;
uniform float uMoving;
out vec3 world;out vec4 pigment;out vec2 uv;out float life;
void main(){
    pigment=aColor;uv=aTexCoord;life=1.0;world=aPosition;
    if(uMoving>.5){
        vec3 local=aPosition;
        if(aColor.a>.6){
            float age=fract(uCampfire.w*.23+aColor.r)*4.35;
            local.y+=age*.45;local.x+=sin(age*3.0+aColor.r*37.0)*.10-age*.035;
            life=1.0-smoothstep(2.6,4.35,age);
        }else if(aColor.a>.35){
            local.x+=sin(uCampfire.w*.47+aColor.r*13.0)*.12-local.y*.09;
        }
        world=uCampfire.xyz-vec3(0,.45,0)+uRight*local.x+vec3(0,local.y,0)+uForward*local.z;
    }
    gl_Position=uViewProjection*vec4(world,1);
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
constexpr const char* fragment=R"(
in vec3 world;in vec4 pigment;in vec2 uv;in float life;
uniform float uMoving;
out vec4 fragColor;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float fbm(vec2 p){return noise(p)*.57+noise(p*2.13+13.1)*.28+noise(p*4.1-7.4)*.15;}
void main(){
    float t=uCampfire.w;
    if(uMoving<.5){
        int tag=int(pigment.a*255.0/36.0+.5);
        vec3 color;
        if(tag==0){color=mix(vec3(.27,.28,.27),vec3(.48,.46,.41),fbm(world.xz*7.0+world.y*3.0));}
        else if(tag==1){
            float fissure=pow(.5+.5*sin(uv.x*85.0+noise(uv*11.0)*2.0),12.0);
            color=mix(vec3(.035,.027,.024),vec3(.115,.071,.038),noise(uv*23.0));
            color+=vec3(.28,.035,.002)*fissure*campfirePulse(t);
        }else if(tag==2){
            float bark=noise(vec2(uv.x*81.0,uv.y*4.0));
            color=mix(vec3(.13,.072,.032),vec3(.36,.24,.12),bark);
        }else if(tag==3){
            float rings=.5+.5*sin(length(uv)*53.0+noise(uv*12.0)*2.0);
            color=mix(vec3(.37,.23,.11),vec3(.62,.46,.28),rings*.65);
        }else if(tag==4){color=mix(vec3(.12,.105,.087),vec3(.29,.26,.22),noise(world.xz*19.0));}
        else{color=mix(vec3(.37,.035,.004),vec3(1,.30,.015),noise(world.xz*31.0))*campfirePulse(t);}
        if(tag!=5)color=applyCampfireLight(color*pigment.rgb,world);
        fragColor=vec4(color,1);return;
    }
    vec2 p=vec2((uv.x-.5)*2.0,uv.y);
    float alpha;vec3 color;
    if(pigment.a<.35){
        float turbulence=fbm(vec2(p.x*3.7,p.y*5.2-t*1.9)+pigment.r*9.0);
        float bend=sin(p.y*7.0-t*2.3+pigment.r*11.0)*.10;
        float height=.88+.10*sin(t*2.7+pigment.r*23.0);
        float q=p.y/height;
        float width=(1.0-q)*(.72+.45*turbulence);
        float fine=noise(p*vec2(16.0,11.0)-vec2(t*.4,t*3.0)+pigment.r*17.0);
        float edge=width-abs(p.x+bend+(turbulence-.5)*.28)+(fine-.5)*.09;
        alpha=smoothstep(-.03,.14,edge)*smoothstep(0.0,.07,p.y)*(1.0-smoothstep(.82,1.0,q))
            *smoothstep(.20,.66,turbulence)*.74;
        float heat=clamp((1.0-p.y)*.95+(turbulence-.5)*.20,0.0,1.0);
        color=mix(vec3(.88,.055,.003),vec3(1,.65,.08),smoothstep(.15,.75,heat));
        color=mix(color,vec3(1,.95,.55),smoothstep(.78,1.0,heat));
    }else if(pigment.a<.6){
        float cloud=fbm(vec2(p.x*2.6,p.y*5.0-t*.43)+pigment.r*8.0);
        alpha=(1.0-smoothstep(.12,1.0,length(vec2(p.x,p.y*2.0-1.0))))
            *smoothstep(.25,.7,cloud)*.115;
        color=vec3(.33,.32,.29);
    }else{alpha=(1.0-smoothstep(.05,.8,length(uv*2.0-1.0)))*life*.85;color=vec3(1,.45,.04);}
    fragColor=vec4(color*alpha,alpha);
}
)";
}
CampfireRenderer::CampfireRenderer(GraphicsDevice& device,const CampfireLayout& layout):device_(device),centre_(layout.Centre()) {
    using CNA::Graphics::ShaderCodeEXT;
    const auto source=[](bool es,const char* body){return std::string(es?"#version 300 es\nprecision highp float;\nprecision highp int;\n":"#version 330 core\n")+CampfireLighting::Shader+body;};
    const CNA::Graphics::ShaderPackageEXT package({
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Vertex,"main","fire.es.vert",source(true,vertex)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Fragment,"main","fire.es.frag",source(true,fragment)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Vertex,"main","fire.vert",source(false,vertex)),
        ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Fragment,"main","fire.frag",source(false,fragment))},
        {CNA::ShaderStageEXT::Vertex,CNA::ShaderStageEXT::Fragment},{});
    effect_=std::make_unique<ShaderEffect>(device,package);
    if(!effect_->IsEffectValid())throw std::runtime_error("Campfire shader failed: "+effect_->GetCompileErrorEXT());
    std::vector<VertexPositionColorTexture> vertices;
    for(const auto v:layout.Vertices()) {
        const float light=.43f+.57f*std::max(0.0f,v.normal.x*environment::SunDirectionX
            +v.normal.y*environment::SunDirectionY+v.normal.z*environment::SunDirectionZ);
        const int value=static_cast<int>(light*255);
        vertices.emplace_back(Vector3(v.position.x,v.position.y,v.position.z),
            Color(value,value,value,static_cast<int>(v.material)*36),Vector2(v.u,v.v));
    }
    solidCount_=static_cast<int>(vertices.size());
    solid_=std::make_unique<VertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),solidCount_,BufferUsage::WriteOnly);
    solid_->SetData(vertices.data(),solidCount_);vertices.clear();
    const auto quad=[&](float x,float y,float z,float width,float height,int type,int phase) {
        const std::array uv{Vector2(0,0),Vector2(1,0),Vector2(1,1),Vector2(0,0),Vector2(1,1),Vector2(0,1)};
        for(const auto p:uv)vertices.emplace_back(Vector3(x+(p.X-.5f)*width,y+p.Y*height,z),Color(phase,255,255,type),p);
    };
    // Farther planes first; horizontal billboard depth stays ordered for every viewing yaw.
    for(int i=0;i<3;++i)quad(-.12f*i,.40f+i*.37f,.45f-i*.03f,1.3f+i*.2f,1.6f,128,i*73);
    for(int i=0;i<6;++i)quad((i%3-1)*.20f,.15f,.30f-i*.12f,.68f,1.0f+.13f*(i%3),64,i*43);
    for(int i=0;i<12;++i)quad((i%4-1.5f)*.16f,.30f,-.45f,.028f,.065f,192,i*21);
    effectCount_=static_cast<int>(vertices.size());
    moving_=std::make_unique<VertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),effectCount_,BufferUsage::WriteOnly);
    moving_->SetData(vertices.data(),effectCount_);
    bounds_=BoundingBox(Vector3(centre_.x-3,centre_.y-.4f,centre_.z-4.5f),Vector3(centre_.x+8,centre_.y+5,centre_.z+3));
    std::cout<<"island-simulator: campfire "<<solidCount_/3<<" solid triangles, "<<effectCount_/3
        <<" flame/smoke/spark triangles in two culled draws; 11 stored logs six metres away\n";
}
CampfireRenderer::~CampfireRenderer()=default;
void CampfireRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,float seconds,bool effects) {
    if(!effects)draws_=0;
    if(!BoundingFrustum(view*projection).Intersects(bounds_))return;
    static bool reported=false;if(!reported){std::cout<<"island-simulator: campfire first visible animated draw\n";reported=true;}
    float matrix[16];(view*projection).ToColumnMajor(matrix);
    effect_->Apply();effect_->SetUniformMat4("uViewProjection",matrix);CampfireLighting::Bind(*effect_,seconds);
    Vector3 toward(camera.X-centre_.x,0,camera.Z-centre_.z);
    if(toward.LengthSquared()<.001f)toward=Vector3(0,0,1);else toward.Normalize();
    effect_->SetUniformVec3("uRight",toward.Z,0,-toward.X);
    effect_->SetUniformVec3("uForward",-toward.X,0,-toward.Z);
    effect_->SetUniformFloat("uMoving",effects?1.0f:0.0f);
    device_.setBlendStateProperty(effects?BlendState::AlphaBlend:BlendState::Opaque);
    device_.setDepthStencilStateProperty(effects?DepthStencilState::DepthRead:DepthStencilState::Default);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.SetVertexBuffer(effects?moving_.get():solid_.get());
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,(effects?effectCount_:solidCount_)/3);++draws_;
}
}
