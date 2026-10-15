#include "IslandSimulator/FruitBatRenderer.hpp"

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

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float NearDistance = 110.0f;
        constexpr int MaximumVerticesPerBat = 360;
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

        void Diamond(std::vector<VertexPositionColor>& out, const Vector3& centre,
                     const Vector3& right, const Vector3& up, const Vector3& forward,
                     const float rx, const float ry, const float rz, const float scale,
                     const Color top, const Color side)
        {
            constexpr int Sides = 10;
            const Vector3 crown = centre + up * (ry * scale);
            const Vector3 belly = centre - up * (ry * scale);
            for (int i = 0; i < Sides; ++i) {
                const float a0 = 2.0f * std::numbers::pi_v<float> * i / Sides;
                const float a1 = 2.0f * std::numbers::pi_v<float> * (i + 1) / Sides;
                const Vector3 p0 = centre + right * (std::cos(a0) * rx * scale)
                    + forward * (std::sin(a0) * rz * scale);
                const Vector3 p1 = centre + right * (std::cos(a1) * rx * scale)
                    + forward * (std::sin(a1) * rz * scale);
                Triangle(out, crown, p0, p1, top);
                Triangle(out, belly, p1, p0, side);
            }
        }

        void AppendNear(std::vector<VertexPositionColor>& out, const FruitBatState& bat)
        {
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
            Diamond(out, Point(centre,right,up,forward,0,0,-.02f,s), right,up,forward,
                    .19f,.15f,.34f,s,p.fur,p.chest);
            const Vector3 head = Point(centre,right,up,forward,0,.015f,.38f,s);
            Diamond(out, head,right,up,forward,.15f,.13f,.16f,s,p.face,p.fur);
            const Vector3 muzzle = Point(centre,right,up,forward,0,-.025f,.55f,s);
            Diamond(out,muzzle,right,up,forward,.075f,.055f,.11f,s,p.face,p.face);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 earRoot=Point(centre,right,up,forward,side*.095f,.08f,.43f,s);
                const Vector3 earOuter=Point(centre,right,up,forward,side*.25f,.105f,.40f,s);
                const Vector3 earTip=Point(centre,right,up,forward,side*.17f,.27f,.43f,s);
                DoubleTriangle(out,earRoot,earOuter,earTip,p.face);
                const Vector3 eyeA=Point(centre,right,up,forward,side*.135f,.045f,.50f,s);
                const Vector3 eyeB=Point(centre,right,up,forward,side*.137f,.09f,.485f,s);
                const Vector3 eyeC=Point(centre,right,up,forward,side*.137f,.07f,.525f,s);
                DoubleTriangle(out,eyeA,eyeB,eyeC,p.eye);

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
                DoubleTriangle(out,shoulder,elbow,fingerB,innerMembrane);
                DoubleTriangle(out,elbow,wrist,fingerA,outerMembrane);
                DoubleTriangle(out,elbow,fingerA,fingerB,p.membrane);
                DoubleTriangle(out,shoulder,fingerB,hip,innerMembrane);
                const auto bone=[&](const Vector3& start,const Vector3& end) {
                    const Vector3 halfWidth=up*(.018f*s);
                    DoubleTriangle(out,start,end+halfWidth,end-halfWidth,p.finger);
                };
                bone(shoulder,elbow);
                bone(elbow,wrist);
                bone(wrist,fingerA);
                for (const Vector3 clawOffset : {
                         Vector3(side*.07f,-.15f,-.30f), Vector3(side*.13f,-.18f,-.33f)}) {
                    const Vector3 ankle=Point(centre,right,up,forward,
                        side*.08f,-.08f,-.25f,s);
                    const Vector3 claw=Point(centre,right,up,forward,
                        clawOffset.X,clawOffset.Y,clawOffset.Z,s);
                    const Vector3 toe=Point(centre,right,up,forward,
                        clawOffset.X+side*.035f,clawOffset.Y+.015f,clawOffset.Z+.045f,s);
                    DoubleTriangle(out,ankle,claw,toe,p.finger);
                }
            }
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
