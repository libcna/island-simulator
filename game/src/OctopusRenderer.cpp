#include "IslandSimulator/OctopusRenderer.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
using namespace Microsoft::Xna::Framework;using namespace Microsoft::Xna::Framework::Graphics;
namespace island{namespace{
constexpr int MaxPer=3100,Max=static_cast<int>(OctopusGroup::Count)*MaxPer;constexpr float NearDistance=105;
struct Palette{Color mantle,highlight,shadow,arm,sucker,eye,iris;};
const std::array<Palette,3> P{{{Color(137,66,45,255),Color(181,101,69,255),Color(91,43,37,255),Color(151,74,50,255),Color(213,177,142,255),Color(158,126,69,255),Color(25,18,13,255)},{Color(111,76,57,255),Color(157,112,81,255),Color(72,49,42,255),Color(126,88,65,255),Color(200,174,145,255),Color(145,123,79,255),Color(22,18,14,255)},{Color(126,82,48,255),Color(176,126,73,255),Color(80,51,34,255),Color(143,93,55,255),Color(220,183,133,255),Color(169,134,67,255),Color(25,18,12,255)}}};
void Tri(std::vector<VertexPositionColor>& o,const Vector3&a,const Vector3&b,const Vector3&c,Color x){o.emplace_back(a,x);o.emplace_back(b,x);o.emplace_back(c,x);}void DTri(std::vector<VertexPositionColor>&o,const Vector3&a,const Vector3&b,const Vector3&c,Color x){Tri(o,a,b,c,x);Tri(o,c,b,a,x);}
struct Basis{Vector3 c,r,u,f;float s;Vector3 point(float x,float y,float z)const{return c+r*(x*s)+u*(y*s)+f*(z*s);}};
Basis basis(const OctopusState&o){Vector3 f(std::cos(o.headingRadians),0,std::sin(o.headingRadians));return{{o.x,o.y,o.z},{-f.Z,0,f.X},Vector3::Up,f,o.scale};}
void Ellipsoid(std::vector<VertexPositionColor>&out,const Basis&b,Vector3 local,float rx,float ry,float rz,Color top,Color side){constexpr int rings=8,sides=16;std::array<std::array<Vector3,sides>,rings> p{};for(int j=0;j<rings;++j){float v=-std::numbers::pi_v<float>*.5f+std::numbers::pi_v<float>*j/(rings-1);for(int i=0;i<sides;++i){float a=2*std::numbers::pi_v<float>*i/sides;p[j][i]=b.point(local.X+std::cos(a)*std::cos(v)*rx,local.Y+std::sin(v)*ry,local.Z+std::sin(a)*std::cos(v)*rz);}}for(int j=0;j<rings-1;++j)for(int i=0;i<sides;++i){int n=(i+1)%sides;Color c=j>=3?top:side;Tri(out,p[j][i],p[j+1][i],p[j+1][n],c);Tri(out,p[j][i],p[j+1][n],p[j][n],c);}}
void EyeLens(std::vector<VertexPositionColor>&out,const Basis&b,float side,const Palette&p){constexpr int sides=8;const Vector3 centre=b.point(side*.145f,.285f,.375f);const Vector3 irisApex=centre+b.f*(.031f*b.s),pupilApex=centre+b.f*(.038f*b.s);for(int i=0;i<sides;++i){float a=2*std::numbers::pi_v<float>*i/sides,n=2*std::numbers::pi_v<float>*(i+1)/sides;Vector3 ra=b.r*(std::cos(a)*.046f*b.s)+b.u*(std::sin(a)*.029f*b.s),rn=b.r*(std::cos(n)*.046f*b.s)+b.u*(std::sin(n)*.029f*b.s);Tri(out,irisApex,centre+ra,centre+rn,p.eye);Vector3 pa=b.r*(std::cos(a)*.022f*b.s)+b.u*(std::sin(a)*.006f*b.s),pn=b.r*(std::cos(n)*.022f*b.s)+b.u*(std::sin(n)*.006f*b.s);Tri(out,pupilApex,centre+pa+b.f*(.034f*b.s),centre+pn+b.f*(.034f*b.s),p.iris);}}
void Near(std::vector<VertexPositionColor>&out,const OctopusState&o){const auto&p=P[o.colorVariant%P.size()];const Basis b=basis(o);float hide=std::clamp(o.hideAmount,0.f,1.f);float pulse=1+o.mantlePulse*.045f;Ellipsoid(out,b,{0,.36f*(1-hide*.48f),-.10f},.29f*pulse,.39f*(1-hide*.45f),.27f*pulse,p.highlight,p.mantle);Ellipsoid(out,b,{0,.19f,.16f},.25f,.20f,.23f,p.mantle,p.shadow);
for(float side:{-1.f,1.f})EyeLens(out,b,side,p);
for(int arm=0;arm<8;++arm){float angle=2*std::numbers::pi_v<float>*arm/8+.18f;Vector3 last=b.point(std::cos(angle)*.13f,.09f,std::sin(angle)*.13f);float lastWidth=.075f*b.s;Vector3 lastLateral=b.r*std::cos(angle)*lastWidth-b.f*std::sin(angle)*lastWidth,lastUp=b.u*(lastWidth*.55f);for(int seg=1;seg<=7;++seg){float t=seg/7.f,length=(.35f+.55f*(1-hide))*t;float curl=std::sin(o.armWave+arm*.93f+seg*.62f)*.10f*t*t;float a=angle+curl;Vector3 next=b.point(std::cos(a)*length,.055f+.028f*std::sin(o.armWave+seg*.8f)*(1-t),std::sin(a)*length);float w=(.068f*(1-t)+.014f)*b.s;Vector3 lateral=b.r*std::cos(a)*w-b.f*std::sin(a)*w,up=b.u*(w*.55f);Tri(out,last+lastLateral,last+lastUp,next+up,p.arm);Tri(out,last+lastLateral,next+up,next+lateral,p.arm);Tri(out,last+lastUp,last-lastLateral,next-lateral,p.highlight);Tri(out,last+lastUp,next-lateral,next+up,p.highlight);Tri(out,last-lastLateral,last-lastUp,next-up,p.shadow);Tri(out,last-lastLateral,next-up,next-lateral,p.shadow);Tri(out,last-lastUp,last+lastLateral,next+lateral,p.arm);Tri(out,last-lastUp,next+lateral,next-up,p.arm);if((arm+seg)%3==0){Vector3 d=up+b.u*(.005f*b.s);DTri(out,next-d,next+lateral*.36f-d,next-lateral*.36f-d,p.sucker);}last=next;lastLateral=lateral;lastUp=up;}}
}
void Far(std::vector<VertexPositionColor>&out,const OctopusState&o){const auto&p=P[o.colorVariant%P.size()];const Basis b=basis(o);Vector3 top=b.point(0,.62f,0),left=b.point(-.32f,.08f,.05f),right=b.point(.32f,.08f,.05f),rear=b.point(0,.08f,-.30f);DTri(out,top,left,right,p.mantle);DTri(out,left,rear,right,p.shadow);for(float side:{-1.f,1.f})DTri(out,rear,b.point(side*.58f,.04f,-.20f),b.point(side*.85f,.02f,-.38f),p.arm);}
}
OctopusRenderer::OctopusRenderer(GraphicsDevice&d,float max):device_(d),effect_(std::make_unique<BasicEffect>(d)),vertices_(std::make_unique<DynamicVertexBuffer>(d,VertexPositionColor::getVertexDeclarationStatic(),Max,BufferUsage::WriteOnly)),maximumDistance_(max){if(!std::isfinite(max)||max<=0)throw std::invalid_argument("Octopus maximum distance must be finite and positive");effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);effect_->setFogEnabledProperty(true);effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));effect_->setFogStartProperty(145);effect_->setFogEndProperty(310);staging_.reserve(Max);constexpr auto marker="island-simulator: octopuses use pulsing mantles, expressive eyes, eight independently curled arms, and pale sucker rows in one draw";std::cout<<marker<<'\n';MirrorRuntimeDiagnosticToPlatformLog(marker);}OctopusRenderer::~OctopusRenderer()=default;
void OctopusRenderer::Draw(const Matrix&v,const Matrix&p,const Vector3&camera,std::span<const OctopusState> animals){staging_.clear();visible_=near_=far_=draws_=0;BoundingFrustum frustum(v*p);for(const auto&o:animals){Vector3 c(o.x,o.y+.3f*o.scale,o.z);float ds=Vector3::DistanceSquared(c,camera);if(ds>maximumDistance_*maximumDistance_||!frustum.Intersects(BoundingSphere(c,1.4f*o.scale)))continue;if(ds<=NearDistance*NearDistance){Near(staging_,o);++near_;}else{Far(staging_,o);++far_;}++visible_;}if(staging_.empty())return;device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::Opaque);device_.setDepthStencilStateProperty(DepthStencilState::Default);vertices_->SetData(staging_.data(),0,static_cast<int>(staging_.size()),SetDataOptions::Discard);device_.SetVertexBuffer(vertices_.get());device_.setIndicesProperty(nullptr);effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(v);effect_->setProjectionProperty(p);effect_->Apply();device_.DrawPrimitives(PrimitiveType::TriangleList,0,static_cast<int>(staging_.size()/3));++draws_;}
}
