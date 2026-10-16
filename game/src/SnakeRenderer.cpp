#include "IslandSimulator/SnakeRenderer.hpp"
#include "IslandSimulator/SnakeHeadProfile.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
namespace island {
namespace {
    constexpr int Capacity=static_cast<int>(SnakeRenderer::MaximumVisibleCount)*5000;
    constexpr int Sides=16;
    constexpr float Tau=2.0f*std::numbers::pi_v<float>;
    Vector3 Point(SnakePoint p) { return {p.x,p.y,p.z}; }
}
SnakeRenderer::SnakeRenderer(GraphicsDevice& device, const CanopyLightField& canopyLight)
    : device_(device), canopy_(canopyLight),
      texture_(std::make_unique<Texture2D>(device,GeneratedTextureBudget::SnakeScaleSize,GeneratedTextureBudget::SnakeScaleSize)),
      effect_(std::make_unique<BasicEffect>(device)),
      buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),Capacity,BufferUsage::WriteOnly))
{
    constexpr int size=GeneratedTextureBudget::SnakeScaleSize;
    std::vector<Color> pixels;
    pixels.reserve(size*size);
    for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
        const int row=y/16;
        const float u=static_cast<float>((x+(row%2)*8)%16)/16.0f;
        const float v=static_cast<float>(y%16)/16.0f;
        const float edge=std::abs(u-0.5f)+std::abs(v-0.5f)*0.6f;
        const int shade=static_cast<int>(std::clamp(240.0f-42.0f*std::clamp((edge-0.43f)*12.0f,0.0f,1.0f)
            -18.0f*v,0.0f,255.0f));
        pixels.emplace_back(shade,shade,shade,255);
    }
    texture_->SetData(pixels.data(),static_cast<int>(pixels.size()));
    vertices_.reserve(Capacity);
    effect_->setTextureEnabledProperty(true); effect_->setTextureProperty(texture_.get());
    effect_->setVertexColorEnabledProperty(true); effect_->setLightingEnabledProperty(false);
    effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(45.0f); effect_->setFogEndProperty(80.0f);
    const char* anatomy="island-simulator: snakes use a readable dorsal saddle and active forked-tongue flicks";
    std::cout<<anatomy<<'\n';MirrorRuntimeDiagnosticToPlatformLog(anatomy);
    const char* headAnatomy="island-simulator: near snakes use a broad low temple, tapered blunt muzzle, raised amber eyes, and fitted vertical pupils";
    std::cout<<headAnatomy<<'\n';MirrorRuntimeDiagnosticToPlatformLog(headAnatomy);
}
SnakeRenderer::~SnakeRenderer()=default;
void SnakeRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,
                         const std::span<const SnakeState> snakes)
{
    if(snakes.size()>SnakePopulation::Count)throw std::length_error("Snake population exceeds render capacity");
    vertices_.clear(); visible_=draws_=0;
    const BoundingFrustum frustum(view*projection);
    std::vector<std::pair<float,const SnakeState*>> candidates;
    candidates.reserve(snakes.size());
    for(const auto& snake:snakes) {
        const auto origin=Point(snake.body[0]);
        const float distance=Vector3::DistanceSquared(origin,camera);
        if (distance>70.0f*70.0f
            || !frustum.Intersects(BoundingSphere(origin,2.5f))) continue;
        candidates.emplace_back(distance,&snake);
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.first<b.first;});
    if(candidates.size()>SnakeRenderer::MaximumVisibleCount)candidates.resize(SnakeRenderer::MaximumVisibleCount);
    for (const auto& candidate:candidates) {
        const auto& snake=*candidate.second;
        const auto origin=Point(snake.body[0]);
        using Ring=std::array<VertexPositionColorTexture,Sides+1>;
        std::array<Ring,SnakeState::StationCount> rings;
        auto vertex=[&](Vector3 point,Vector3 normal,Color color,Vector2 uv) {
            const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(point.X,point.Y,point.Z,origin.Y));
            const float light=0.58f+0.38f*std::max(0.0f,Vector3::Dot(normal,wildlife_render::FixedSunDirection));
            return VertexPositionColorTexture(point,Color(
                static_cast<int>(color.getRProperty()*light*tint.red),
                static_cast<int>(color.getGProperty()*light*tint.green),
                static_cast<int>(color.getBProperty()*light*tint.blue)),uv);
        };
        for (std::size_t j=0;j<snake.body.size();++j) {
            const auto centre=Point(snake.body[j]);
            const auto before=Point(snake.body[j==0 ? 0 : j-1]);
            const auto after=Point(snake.body[std::min(j+1,snake.body.size()-1)]);
            const auto tangent=Vector3::Normalize(before-after);
            const auto right=Vector3::Normalize(Vector3::Cross(Vector3::Up,tangent));
            const auto up=Vector3::Normalize(Vector3::Cross(tangent,right));
            for (int k=0;k<=Sides;++k) {
                const float angle=Tau*static_cast<float>(k)/Sides;
                const auto normal=right*std::cos(angle)+up*std::sin(angle);
                const float band=0.80f+0.20f*std::cos(static_cast<float>(j)*1.65f);
                const float dorsalBand=.84f+.16f*std::cos(static_cast<float>(j)*1.65f);
                const Color color=std::sin(angle)<-0.25f ? Color(177,165,105)
                    : std::sin(angle)>.32f
                        ? Color(static_cast<int>(126*dorsalBand),static_cast<int>(139*dorsalBand),
                            static_cast<int>(61*dorsalBand))
                        : Color(static_cast<int>(104*band),static_cast<int>(119*band),static_cast<int>(49*band));
                rings[j][static_cast<std::size_t>(k)]=vertex(centre+normal*SnakePopulation::RadiusAt(j),normal,color,
                    Vector2(SnakeSkinU(static_cast<float>(k),Sides),
                        SnakeSkinV(static_cast<float>(j)*SnakePopulation::Spacing,SnakePopulation::Spacing)));
            }
        }
        auto triangle=[&](const auto& a,const auto& b,const auto& c) {
            vertices_.push_back(a); vertices_.push_back(b); vertices_.push_back(c);
        };
        for (std::size_t j=1;j<rings.size();++j) for(int k=0;k<Sides;++k) {
            const auto a=static_cast<std::size_t>(k),b=a+1;
            triangle(rings[j-1][a],rings[j][a],rings[j][b]);
            triangle(rings[j-1][a],rings[j][b],rings[j-1][b]);
        }
        const auto forward=Vector3::Normalize(Point(snake.body[0])-Point(snake.body[1]));
        const auto right=Vector3::Normalize(Vector3::Cross(Vector3::Up,forward));
        const auto up=Vector3::Normalize(Vector3::Cross(forward,right));
        Ring muzzle=rings[0];
        for(std::size_t section=1;section<SnakeHeadSections.size();++section) {
            const auto profile=SnakeHeadSections[section];
            Ring next;
            for(int k=0;k<=Sides;++k) {
                const float angle=Tau*static_cast<float>(k)/Sides;
                const auto normal=Vector3::Normalize(right*(std::cos(angle)/profile.width)
                    +up*(std::sin(angle)/profile.height)+forward*12.0f);
                const auto position=origin+forward*profile.forward
                    +right*(std::cos(angle)*profile.width)+up*(profile.centreY+std::sin(angle)*profile.height);
                next[static_cast<std::size_t>(k)]=vertex(position,normal,
                    std::sin(angle)<-0.25f ? Color(192,177,116) : Color(117,136,62),
                    Vector2(SnakeSkinU(static_cast<float>(k),Sides),
                        SnakeSkinV(-profile.forward,SnakePopulation::Spacing)));
            }
            for(int k=0;k<Sides;++k) {
                const auto a=static_cast<std::size_t>(k),b=a+1;
                triangle(muzzle[a],next[a],next[b]); triangle(muzzle[a],next[b],muzzle[b]);
            }
            muzzle=next;
        }
        const auto tail=vertex(Point(snake.body.back()),-forward,Color(89,104,43),Vector2(0,0));
        for(int k=0;k<Sides;++k) {
            // Each cap triangle needs a local pole UV. Reusing (0,0) across the fan
            // dragged up to four texture wraps across a single tiny muzzle triangle.
            const auto nose=vertex(origin+forward*SnakeNoseForward-up*0.008f,forward,Color(111,128,57),
                Vector2(SnakeSkinU(static_cast<float>(k)+0.5f,Sides),
                    SnakeSkinV(-SnakeNoseForward,SnakePopulation::Spacing)));
            triangle(nose,muzzle[static_cast<std::size_t>(k)],muzzle[static_cast<std::size_t>(k+1)]);
            triangle(tail,rings.back()[static_cast<std::size_t>(k+1)],rings.back()[static_cast<std::size_t>(k)]);
        }
        for (const float side:{-1.0f,1.0f}) {
            // Fine mouth seam follows the actual muzzle surface, not a floating bar.
            for(std::size_t section=1;section<SnakeHeadSections.size();++section) {
                auto lip=[&](const SnakeHeadSection& p,const float vertical) {
                    const float y=-0.20f*p.height+vertical;
                    const float width=p.width*std::sqrt(std::max(0.0f,1.0f-y*y/(p.height*p.height)))+0.0003f;
                    return vertex(origin+forward*p.forward+right*(side*width)+up*(p.centreY+y),
                        right*side,Color(55,53,30),Vector2(0,0));
                };
                const auto a=lip(SnakeHeadSections[section-1],-0.0006f);
                const auto b=lip(SnakeHeadSections[section-1],0.0006f);
                const auto c=lip(SnakeHeadSections[section],-0.0006f);
                const auto d=lip(SnakeHeadSections[section],0.0006f);
                triangle(a,b,c); triangle(b,d,c);
            }
            const auto eye=origin+right*(side*SnakeEyeSide)+up*SnakeEyeHeight+forward*SnakeEyeForward;
            const auto n=Vector3::Normalize(right*side+up*0.4f);
            const auto eyeUp=Vector3::Normalize(Vector3::Cross(n,forward));
            auto eyeVertex=[&](const float angle,const float elevation) {
                const auto normal=(forward*std::cos(angle)+eyeUp*std::sin(angle))*std::cos(elevation)
                    +n*std::sin(elevation);
                return vertex(eye+normal*SnakeEyeRadius,normal,Color(171,133,53),Vector2(0,0));
            };
            for(int ring=0;ring<3;++ring) for(int k=0;k<10;++k) {
                const float a=Tau*static_cast<float>(k)/10,b=Tau*static_cast<float>(k+1)/10;
                const float low=static_cast<float>(ring)*Tau/12,high=static_cast<float>(ring+1)*Tau/12;
                triangle(eyeVertex(a,low),eyeVertex(b,low),eyeVertex(a,high));
                if(ring<2) triangle(eyeVertex(b,low),eyeVertex(b,high),eyeVertex(a,high));
            }
            // A fitted vertical pupil follows the outward eye tangent. The old black
            // polar cap read as a dark polygon rather than a snake eye from the side.
            const Vector3 pupilCentre=eye+n*(SnakeEyeRadius+.00035f);
            const auto pupilVertex=[&](const float angle) {
                const Vector3 offset=forward*(std::cos(angle)*SnakeEyeRadius*.42f)
                    +eyeUp*(std::sin(angle)*SnakeEyeRadius*.82f);
                return vertex(pupilCentre+offset,n,Color(6,7,4),Vector2(0,0));
            };
            const auto pupilMiddle=vertex(pupilCentre,n,Color(6,7,4),Vector2(0,0));
            for(int k=0;k<10;++k) {
                const float a=Tau*static_cast<float>(k)/10.0f;
                const float b=Tau*static_cast<float>(k+1)/10.0f;
                triangle(pupilMiddle,pupilVertex(a),pupilVertex(b));
            }
        }
        // A brief split-tongue flick follows the existing locomotion phase. It is absent at
        // rest and costs only four triangles when active, without a separate draw or texture.
        const float flick=.5f+.5f*std::sin(snake.phase*5.0f+snake.homeX*.17f+snake.homeZ*.11f);
        if(snake.behavior!=SnakeBehavior::Rest&&flick>.62f) {
            const float extension=.019f+.026f*(flick-.62f)/.38f;
            const Vector3 base=origin+forward*(SnakeNoseForward+.002f)-up*.004f;
            const Vector3 fork=base+forward*(extension*.68f);
            const auto tongueVertex=[&](const Vector3& point) {
                return vertex(point,up,Color(184,43,38),Vector2(0,0));
            };
            const Vector3 baseLeft=base-right*.0022f,baseRight=base+right*.0022f;
            const Vector3 forkLeft=fork-right*.0015f,forkRight=fork+right*.0015f;
            triangle(tongueVertex(baseLeft),tongueVertex(baseRight),tongueVertex(forkRight));
            triangle(tongueVertex(baseLeft),tongueVertex(forkRight),tongueVertex(forkLeft));
            triangle(tongueVertex(forkLeft),tongueVertex(fork),
                tongueVertex(fork+forward*(extension*.32f)-right*.008f));
            triangle(tongueVertex(fork),tongueVertex(forkRight),
                tongueVertex(fork+forward*(extension*.32f)+right*.008f));
        }
        ++visible_;
    }
    if(vertices_.empty()) return;
    if(vertices_.size()>Capacity) throw std::runtime_error("Snake vertex budget exceeded");
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque); device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearWrap;
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.SetVertexBuffer(buffer_.get()); device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty()); effect_->setViewProperty(view); effect_->setProjectionProperty(projection);
    effect_->Apply(); device_.DrawPrimitives(PrimitiveType::TriangleList,0,static_cast<int>(vertices_.size()/3)); draws_=1;
}
}
