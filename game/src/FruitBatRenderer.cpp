#include "IslandSimulator/FruitBatRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

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

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float NearDistance = 110.0f;
        constexpr int MaximumVerticesPerBat = 2300;
        constexpr int MaximumVertices = static_cast<int>(FruitBatColony::Count)
            * MaximumVerticesPerBat;

        struct Palette
        {
            Color fur, chest, membrane, finger, face, eye;
        };

        const std::array<Palette, 3> Palettes{{
            {Color(86, 58, 39, 255), Color(126, 88, 55, 255), Color(63, 43, 37, 255),
             Color(48, 34, 31, 255), Color(117, 76, 50, 255), Color(184, 137, 51, 255)},
            {Color(72, 50, 38, 255), Color(146, 99, 58, 255), Color(55, 42, 40, 255),
             Color(43, 34, 33, 255), Color(135, 86, 51, 255), Color(205, 151, 55, 255)},
            {Color(98, 70, 45, 255), Color(157, 111, 66, 255), Color(70, 48, 42, 255),
             Color(51, 37, 34, 255), Color(145, 94, 58, 255), Color(198, 145, 48, 255)}
        }};

        void Triangle(std::vector<VertexPositionColor>& out, const Vector3& a,
                      const Vector3& b, const Vector3& c, const Color color)
        {
            out.emplace_back(a, color); out.emplace_back(b, color); out.emplace_back(c, color);
        }

        void DoubleTriangle(std::vector<VertexPositionColor>& out, const Vector3& a,
                            const Vector3& b, const Vector3& c, const Color color)
        {
            Triangle(out, a, b, c, color); Triangle(out, c, b, a, color);
        }

        [[nodiscard]] Vector3 Point(const Vector3& centre, const Vector3& right,
                                    const Vector3& up, const Vector3& forward,
                                    const float x, const float y, const float z,
                                    const float scale)
        {
            return centre + right * (x * scale) + up * (y * scale)
                + forward * (z * scale);
        }

        void AppendEllipsoid(std::vector<VertexPositionColor>& out,const Vector3& centre,
                             const Vector3& right,const Vector3& up,const Vector3& forward,
                             const float rx,const float ry,const float rz,const float scale,
                             const Color& upper,const Color& lower,
                             const int rings,const int segments)
        {
            for(int ring=0;ring<rings;++ring) {
                const float v0=-.5f*std::numbers::pi_v<float>
                    +std::numbers::pi_v<float>*static_cast<float>(ring)/rings;
                const float v1=-.5f*std::numbers::pi_v<float>
                    +std::numbers::pi_v<float>*static_cast<float>(ring+1)/rings;
                for(int segment=0;segment<segments;++segment) {
                    const float u0=2.0f*std::numbers::pi_v<float>
                        *static_cast<float>(segment)/segments;
                    const float u1=2.0f*std::numbers::pi_v<float>
                        *static_cast<float>(segment+1)/segments;
                    const auto point=[&](const float u,const float v) {
                        return centre+right*(std::cos(v)*std::cos(u)*rx*scale)
                            +up*(std::sin(v)*ry*scale)
                            +forward*(std::cos(v)*std::sin(u)*rz*scale);
                    };
                    const Vector3 a=point(u0,v0),b=point(u0,v1);
                    const Vector3 c=point(u1,v1),d=point(u1,v0);
                    const Color base=(v0+v1)*.5f>0.0f?upper:lower;
                    Triangle(out,a,b,c,wildlife_render::FacetColor(a,b,c,base,true));
                    Triangle(out,a,c,d,wildlife_render::FacetColor(a,c,d,base,true));
                }
            }
        }

        template<std::size_t Count>
        void AppendClosedSurface(std::vector<VertexPositionColor>& out,
                                 const std::array<Vector3,Count>& outline,
                                 const Vector3& halfThickness,
                                 const Color& outer,const Color& inner)
        {
            static_assert(Count>=3U);
            std::array<Vector3,Count> top{},bottom{};
            for(std::size_t point=0;point<Count;++point) {
                top[point]=outline[point]+halfThickness;
                bottom[point]=outline[point]-halfThickness;
            }
            for(std::size_t point=1;point+1<Count;++point) {
                Triangle(out,top[0],top[point],top[point+1],
                    wildlife_render::FacetColor(top[0],top[point],top[point+1],outer,true));
                Triangle(out,bottom[0],bottom[point+1],bottom[point],
                    wildlife_render::FacetColor(bottom[0],bottom[point+1],bottom[point],inner,true));
            }
            const Color edge=Color::Lerp(outer,inner,.58f);
            for(std::size_t point=0;point<Count;++point) {
                const std::size_t next=(point+1U)%Count;
                Triangle(out,top[point],bottom[point],bottom[next],
                    wildlife_render::FacetColor(top[point],bottom[point],bottom[next],edge,true));
                Triangle(out,top[point],bottom[next],top[next],
                    wildlife_render::FacetColor(top[point],bottom[next],top[next],edge,true));
            }
        }

        void AppendTube(std::vector<VertexPositionColor>& out,const Vector3& start,
                        const Vector3& end,const float radiusStart,const float radiusEnd,
                        const Color& color,const int sides)
        {
            Vector3 axis=end-start;
            if(axis.LengthSquared()<1.0e-8f)return;
            axis.Normalize();
            Vector3 side=Vector3::Cross(axis,std::abs(axis.Y)<.88f
                ?Vector3::Up:Vector3::Right);
            side.Normalize();
            const Vector3 other=Vector3::Cross(axis,side);
            for(int index=0;index<sides;++index) {
                const float a0=2.0f*std::numbers::pi_v<float>*index/sides;
                const float a1=2.0f*std::numbers::pi_v<float>*(index+1)/sides;
                const Vector3 r0=side*std::cos(a0)+other*std::sin(a0);
                const Vector3 r1=side*std::cos(a1)+other*std::sin(a1);
                const Vector3 a=start+r0*radiusStart,b=start+r1*radiusStart;
                const Vector3 c=end+r1*radiusEnd,d=end+r0*radiusEnd;
                Triangle(out,a,b,c,wildlife_render::FacetColor(a,b,c,color,true));
                Triangle(out,a,c,d,wildlife_render::FacetColor(a,c,d,color,true));
            }
        }

        void AppendNear(std::vector<VertexPositionColor>& out, const FruitBatState& bat)
        {
            const std::size_t firstVertex=out.size();
            const Palette& p = Palettes[bat.colorVariant % Palettes.size()];
            const Vector3 horizontalForward(std::cos(bat.headingRadians), 0.0f,
                                            std::sin(bat.headingRadians));
            const Vector3 right(-horizontalForward.Z, 0.0f, horizontalForward.X);
            const float hang = std::clamp(bat.hangAmount, 0.0f, 1.0f);
            const Vector3 forward = Vector3::Normalize(
                Vector3::Lerp(horizontalForward, -Vector3::Up, hang));
            const Vector3 up = Vector3::Normalize(Vector3::Cross(forward, right));
            const Vector3 centre(bat.x, bat.y, bat.z);
            const float s = bat.scale;
            AppendEllipsoid(out,Point(centre,right,up,forward,0,0,-.02f,s),
                right,up,forward,.19f,.16f,.34f,s,p.fur,p.chest,5,12);
            const Vector3 head = Point(centre,right,up,forward,0,.015f,.38f,s);
            AppendEllipsoid(out,head,right,up,forward,.15f,.135f,.16f,s,
                p.face,p.fur,5,12);
            const Vector3 muzzle = Point(centre,right,up,forward,0,-.025f,.55f,s);
            AppendEllipsoid(out,muzzle,right,up,forward,.075f,.058f,.11f,s,
                p.face,Color::Lerp(p.face,p.fur,.22f),4,10);
            const Vector3 nose=Point(centre,right,up,forward,0,-.030f,.655f,s);
            AppendEllipsoid(out,nose,right,up,forward,.060f,.040f,.035f,s,
                p.finger,Color(25,20,20,255),3,8);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 earRoot=Point(centre,right,up,forward,side*.095f,.08f,.43f,s);
                const Vector3 earOuter=Point(centre,right,up,forward,side*.25f,.105f,.40f,s);
                const Vector3 earTip=Point(centre,right,up,forward,side*.17f,.27f,.43f,s);
                const Vector3 earInner=Point(centre,right,up,forward,side*.075f,.15f,.465f,s);
                const std::array<Vector3,4> ear{{earRoot,earOuter,earTip,earInner}};
                AppendClosedSurface(out,ear,forward*(.010f*s),p.face,p.chest);
                const Vector3 eye=Point(centre,right,up,forward,side*.137f,.068f,.505f,s);
                AppendEllipsoid(out,eye,right,up,forward,.027f,.024f,.020f,s,
                    p.eye,Color::Lerp(p.eye,p.finger,.30f),3,8);
                const Vector3 eyeGlint=eye+right*(side*.025f*s)+up*(.009f*s)
                    +forward*(.004f*s);
                DoubleTriangle(out,eyeGlint+up*(.005f*s),
                    eyeGlint-forward*(.005f*s)-up*(.003f*s),
                    eyeGlint+forward*(.005f*s)-up*(.003f*s),Color(236,207,139,255));

                const float wingLift = bat.wingPose * .30f * (1.0f - hang);
                const float span = std::lerp(.23f, 1.02f, 1.0f - hang);
                const Vector3 shoulder=Point(centre,right,up,forward,side*.14f,.04f,.20f,s);
                const Vector3 elbow=Point(centre,right,up,forward,side*span*.56f,
                    wingLift+.05f,-.02f,s);
                const Vector3 wrist=Point(centre,right,up,forward,side*span,
                    wingLift*.55f,-.13f,s);
                const Vector3 fingerA=Point(centre,right,up,forward,side*span*.88f,
                    wingLift*.25f,-.51f,s);
                const Vector3 fingerB=Point(centre,right,up,forward,side*span*.54f,
                    -.07f,-.58f,s);
                const Vector3 hip=Point(centre,right,up,forward,side*.12f,-.06f,-.29f,s);
                const Color innerMembrane=Color::Lerp(p.membrane,p.chest,.10f);
                const Color outerMembrane=Color::Lerp(p.membrane,Color(34,29,29,255),.12f);
                const std::array<Vector3,6> wing{{shoulder,elbow,wrist,fingerA,fingerB,hip}};
                AppendClosedSurface(out,wing,up*(.012f*s),outerMembrane,innerMembrane);
                AppendTube(out,shoulder,elbow,.016f*s,.013f*s,p.finger,6);
                AppendTube(out,elbow,wrist,.013f*s,.010f*s,p.finger,6);
                AppendTube(out,wrist,fingerA,.010f*s,.005f*s,p.finger,6);
                AppendTube(out,elbow,fingerB,.009f*s,.004f*s,p.finger,6);
                AppendTube(out,shoulder,hip,.012f*s,.006f*s,p.finger,6);
                const Vector3 ankle=Point(centre,right,up,forward,
                    side*.08f,-.08f,-.25f,s);
                AppendTube(out,hip,ankle,.026f*s,.015f*s,p.finger,6);
                for (const Vector3 clawOffset : {
                         Vector3(side*.07f,-.15f,-.30f), Vector3(side*.13f,-.18f,-.33f)}) {
                    const Vector3 claw=Point(centre,right,up,forward,
                        clawOffset.X,clawOffset.Y,clawOffset.Z,s);
                    const Vector3 toe=Point(centre,right,up,forward,
                        clawOffset.X+side*.035f,clawOffset.Y+.015f,clawOffset.Z+.045f,s);
                    AppendTube(out,ankle,claw,.012f*s,.007f*s,p.finger,5);
                    AppendTube(out,claw,toe,.007f*s,.003f*s,p.finger,5);
                }
            }
            if(out.size()-firstVertex!=2268U)
                throw std::runtime_error("Fruit-bat near mesh does not match its fixed vertex budget");
        }

        void AppendFar(std::vector<VertexPositionColor>& out, const FruitBatState& bat)
        {
            const Palette& p = Palettes[bat.colorVariant % Palettes.size()];
            const Vector3 forward(std::cos(bat.headingRadians),0,std::sin(bat.headingRadians));
            const Vector3 right(-forward.Z,0,forward.X);
            const Vector3 centre(bat.x,bat.y,bat.z);
            const float s=bat.scale;
            const Vector3 nose=centre+forward*(.48f*s),tail=centre-forward*(.42f*s);
            const float span=std::lerp(.24f,1.08f,1.0f-bat.hangAmount)*s;
            const Vector3 left=centre-right*span+Vector3::Up*(bat.wingPose*.18f*s);
            const Vector3 rightTip=centre+right*span+Vector3::Up*(bat.wingPose*.18f*s);
            DoubleTriangle(out,nose,left,tail,p.membrane);
            DoubleTriangle(out,nose,tail,rightTip,p.membrane);
        }
    }

    FruitBatRenderer::FruitBatRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device), effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(device,
              VertexPositionColor::getVertexDeclarationStatic(), MaximumVertices,
              BufferUsage::WriteOnly)), maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f)
            throw std::invalid_argument("Fruit-bat maximum distance must be finite and positive");
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed,
            environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(180.0f); effect_->setFogEndProperty(390.0f);
        staging_.reserve(MaximumVertices);
        constexpr auto diagnostic =
            "island-simulator: fruit bats use furred bodies, pointed ears, muzzles, eyes, fingered membranes, and hanging claws in one draw";
        std::cout << diagnostic << '\n'; MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        constexpr auto anatomyDiagnostic =
            "island-simulator: near fruit bats use rounded faceted body lofts, a fitted nose and eyes, closed ears, solid wing fingers, wrapped membranes, and tapered gripping toes";
        std::cout << anatomyDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(anatomyDiagnostic);
    }

    FruitBatRenderer::~FruitBatRenderer() = default;

    void FruitBatRenderer::Draw(const Matrix& view, const Matrix& projection,
                                const Vector3& cameraPosition,
                                const std::span<const FruitBatState> bats)
    {
        staging_.clear(); visibleCount_=nearCount_=farCount_=drawCallCount_=0;
        const BoundingFrustum frustum(view*projection);
        for(const FruitBatState& bat:bats) {
            const Vector3 centre(bat.x,bat.y,bat.z);
            const float distanceSquared=Vector3::DistanceSquared(centre,cameraPosition);
            if(distanceSquared>maximumDistance_*maximumDistance_
                ||!frustum.Intersects(BoundingSphere(centre,1.5f*bat.scale))) continue;
            if(distanceSquared<=NearDistance*NearDistance) { AppendNear(staging_,bat); ++nearCount_; }
            else { AppendFar(staging_,bat); ++farCount_; }
            ++visibleCount_;
        }
        if(staging_.empty()) return;
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        vertices_->SetData(staging_.data(),0,static_cast<int>(staging_.size()),SetDataOptions::Discard);
        device_.SetVertexBuffer(vertices_.get()); device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty()); effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection); effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList,0,static_cast<int>(staging_.size()/3U));
        ++drawCallCount_;
    }
}
