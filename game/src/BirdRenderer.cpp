#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/BirdRenderer.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

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
        constexpr float NearLodDistance = 255.0f;
        constexpr int MaximumVerticesPerBird = 498;
        constexpr int MaximumVertexCount = static_cast<int>(BirdFlock::BirdCount)
            * MaximumVerticesPerBird;
        constexpr std::string_view NearBirdFeatherDiagnostic =
            "island-simulator: near birds use layered trailing flight-feather silhouettes";
        constexpr std::string_view BirdPerchSilhouetteDiagnostic =
            "island-simulator: perched birds fold wings and add near feet silhouettes";
        constexpr std::string_view NearBirdHeadDiagnostic =
            "island-simulator: near birds use closed lofted bodies, multi-ring heads, fitted beaks, and lateral eyes";
        constexpr std::string_view TropicalBirdDiagnostic =
            "island-simulator: canopy flock includes hooked-beak long-tailed parrots and compact tropical fruit doves";

        struct BirdPalette
        {
            Color body;
            Color upperWing;
            Color lowerWing;
            Color tail;
            Color head;
            Color beak;
            Color eye;
        };

        const std::array<BirdPalette, 3> Palettes = {{
            {Color(61, 70, 70, 255), Color(104, 113, 108, 255),
             Color(214, 218, 203, 255), Color(48, 57, 58, 255),
             Color(161, 168, 164, 255), Color(194, 145, 57, 255), Color(15, 14, 12, 255)},
            {Color(90, 72, 54, 255), Color(139, 117, 82, 255),
             Color(222, 204, 161, 255), Color(67, 56, 46, 255),
             Color(130, 105, 73, 255), Color(184, 127, 47, 255), Color(14, 12, 9, 255)},
            {Color(50, 62, 72, 255), Color(78, 99, 114, 255),
             Color(195, 211, 215, 255), Color(39, 50, 59, 255),
             Color(104, 124, 133, 255), Color(177, 122, 45, 255), Color(12, 13, 14, 255)}
        }};

        const std::array<BirdPalette, 2> ParrotPalettes = {{
            {Color(48, 130, 58, 255), Color(28, 96, 48, 255),
             Color(210, 176, 40, 255), Color(26, 82, 47, 255),
             Color(196, 54, 43, 255), Color(203, 187, 145, 255), Color(13, 12, 9, 255)},
            {Color(57, 140, 65, 255), Color(35, 92, 118, 255),
             Color(218, 182, 46, 255), Color(27, 84, 82, 255),
             Color(224, 174, 43, 255), Color(59, 53, 47, 255), Color(12, 11, 8, 255)}
        }};

        const std::array<BirdPalette, 2> DovePalettes = {{
            {Color(91, 137, 84, 255), Color(48, 102, 62, 255),
             Color(181, 191, 150, 255), Color(45, 88, 59, 255),
             Color(158, 86, 128, 255), Color(78, 70, 64, 255), Color(43, 20, 18, 255)},
            {Color(99, 143, 94, 255), Color(54, 108, 69, 255),
             Color(201, 196, 163, 255), Color(49, 92, 65, 255),
             Color(111, 126, 144, 255), Color(83, 72, 62, 255), Color(47, 21, 17, 255)}
        }};

        [[nodiscard]] const BirdPalette& PaletteForBird(const BirdState& bird)
        {
            if (bird.species == BirdSpecies::Parrot) {
                return ParrotPalettes[bird.colorVariant % ParrotPalettes.size()];
            }
            if (bird.species == BirdSpecies::FruitDove) {
                return DovePalettes[bird.colorVariant % DovePalettes.size()];
            }
            return Palettes[bird.colorVariant % Palettes.size()];
        }

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            vertices.emplace_back(a, color);
            vertices.emplace_back(b, color);
            vertices.emplace_back(c, color);
        }

        void AddDoubleSidedTriangle(std::vector<VertexPositionColor>& vertices,
                                    const Vector3& a, const Vector3& b, const Vector3& c,
                                    const Color& color)
        {
            AddTriangle(vertices, a, b, c, color);
            AddTriangle(vertices, c, b, a, color);
        }

        [[nodiscard]] Color Lit(const Color& base,const Vector3& normal)
        {
            const float light=.62f+.30f*std::max(0.0f,
                Vector3::Dot(normal,wildlife_render::FixedSunDirection))
                +.08f*std::max(0.0f,normal.Y);
            const auto channel=[light](int value) {
                return static_cast<int>(std::clamp(std::lround(value*light),0L,255L));
            };
            return Color(channel(base.getRProperty()),channel(base.getGProperty()),
                channel(base.getBProperty()),static_cast<int>(base.getAProperty()));
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const Vector3& right,
                                         const Vector3& up, const Vector3& forward,
                                         const float localX,
                                         const float localY, const float localZ,
                                         const float scale)
        {
            return centre + right * (localX * scale) + up * (localY * scale)
                + forward * (localZ * scale);
        }

        void AppendNearBody(std::vector<VertexPositionColor>& vertices,
                            const Vector3& centre,const Vector3& right,
                            const Vector3& up,const Vector3& forward,const float scale,
                            const BirdPalette& palette)
        {
            constexpr int Sides=12;
            const Vector3 tail=LocalPoint(centre,right,up,forward,0,0,-.49f,scale);
            const Vector3 neck=LocalPoint(centre,right,up,forward,0,.015f,.34f,scale);
            const auto ring=[&](float angle,float xRadius,float yRadius,float z) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*xRadius,
                    std::sin(angle)*yRadius,z,scale);
            };
            for(int side=0;side<Sides;++side) {
                const float a0=6.28318530718f*static_cast<float>(side)/Sides;
                const float a1=6.28318530718f*static_cast<float>(side+1)/Sides;
                const Vector3 rear0=ring(a0,.12f,.105f,-.31f),rear1=ring(a1,.12f,.105f,-.31f);
                const Vector3 middle0=ring(a0,.19f,.16f,-.02f),middle1=ring(a1,.19f,.16f,-.02f);
                const Vector3 front0=ring(a0,.115f,.105f,.27f),front1=ring(a1,.115f,.105f,.27f);
                const float middleAngle=(a0+a1)*.5f;
                const Vector3 normal=Vector3::Normalize(
                    right*std::cos(middleAngle)+up*std::sin(middleAngle));
                const Color color=Lit(std::sin(middleAngle)<-.05f
                    ?palette.lowerWing:palette.body,normal);
                AddTriangle(vertices,tail,rear1,rear0,color);
                AddTriangle(vertices,rear0,rear1,middle1,color);
                AddTriangle(vertices,rear0,middle1,middle0,color);
                AddTriangle(vertices,middle0,middle1,front1,color);
                AddTriangle(vertices,middle0,front1,front0,color);
                AddTriangle(vertices,neck,front0,front1,color);
            }
        }

        void AppendNearHead(std::vector<VertexPositionColor>& vertices,
                            const Vector3& centre,const Vector3& right,
                            const Vector3& up,const Vector3& forward,const float scale,
                            const BirdPalette& palette, const BirdSpecies species)
        {
            constexpr int Sides=12;
            const float headScale = species == BirdSpecies::Parrot ? 1.10f
                : (species == BirdSpecies::FruitDove ? .96f : 1.0f);
            const Vector3 top=LocalPoint(centre,right,up,forward,0,.195f*headScale,.405f,scale);
            const Vector3 bottom=LocalPoint(centre,right,up,forward,0,-.075f*headScale,.395f,scale);
            const auto ring=[&](float angle,float xRadius,float zRadius,float y) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*xRadius*headScale,
                    y*headScale,.405f+std::sin(angle)*zRadius*headScale,scale);
            };
            for(int side=0;side<Sides;++side) {
                const float a0=6.28318530718f*static_cast<float>(side)/Sides;
                const float a1=6.28318530718f*static_cast<float>(side+1)/Sides;
                const Vector3 upper0=ring(a0,.105f,.12f,.125f),upper1=ring(a1,.105f,.12f,.125f);
                const Vector3 middle0=ring(a0,.14f,.16f,.055f),middle1=ring(a1,.14f,.16f,.055f);
                const Vector3 lower0=ring(a0,.105f,.12f,-.015f),lower1=ring(a1,.105f,.12f,-.015f);
                const float middleAngle=(a0+a1)*.5f;
                const Vector3 normal=Vector3::Normalize(
                    right*std::cos(middleAngle)+forward*std::sin(middleAngle));
                const Color color=Lit(palette.head,normal);
                AddTriangle(vertices,top,upper0,upper1,color);
                AddTriangle(vertices,upper0,middle0,middle1,color);
                AddTriangle(vertices,upper0,middle1,upper1,color);
                AddTriangle(vertices,middle0,lower0,lower1,color);
                AddTriangle(vertices,middle0,lower1,middle1,color);
                AddTriangle(vertices,bottom,lower1,lower0,color);
            }
            const bool parrot = species == BirdSpecies::Parrot;
            const bool dove = species == BirdSpecies::FruitDove;
            const float beakWidth = parrot ? .070f : (dove ? .034f : .052f);
            const float beakBase = parrot ? .548f : (dove ? .525f : .535f);
            const float beakReach = parrot ? .665f : (dove ? .625f : .69f);
            const Vector3 beakTip=LocalPoint(centre,right,up,forward,0,
                parrot ? .010f : .055f,beakReach,scale);
            const Vector3 beakLeft=LocalPoint(centre,right,up,forward,-beakWidth,.058f,beakBase,scale);
            const Vector3 beakRight=LocalPoint(centre,right,up,forward,beakWidth,.058f,beakBase,scale);
            const Vector3 beakTop=LocalPoint(centre,right,up,forward,0,
                parrot ? .125f : .094f,beakBase,scale);
            const Vector3 beakBottom=LocalPoint(centre,right,up,forward,0,
                parrot ? .018f : .022f,beakBase,scale);
            AddTriangle(vertices,beakTip,beakLeft,beakTop,palette.beak);
            AddTriangle(vertices,beakTip,beakTop,beakRight,palette.beak);
            AddTriangle(vertices,beakTip,beakRight,beakBottom,palette.beak);
            AddTriangle(vertices,beakTip,beakBottom,beakLeft,palette.beak);
            for(const float side:{-1.0f,1.0f}) {
                const Vector3 eyeTop=LocalPoint(centre,right,up,forward,side*.107f,.113f,.482f,scale);
                const Vector3 eyeFront=LocalPoint(centre,right,up,forward,side*.109f,.091f,.508f,scale);
                const Vector3 eyeBack=LocalPoint(centre,right,up,forward,side*.109f,.087f,.475f,scale);
                AddDoubleSidedTriangle(vertices,eyeTop,eyeFront,eyeBack,palette.eye);
            }
        }

        void AppendNearBird(std::vector<VertexPositionColor>& vertices,
                            const BirdState& bird)
        {
            const BirdPalette& palette = PaletteForBird(bird);
            const Vector3 centre(bird.x, bird.y, bird.z);
            const Vector3 forward(std::cos(bird.headingRadians), 0.0f,
                                  std::sin(bird.headingRadians));
            const Vector3 levelRight(-forward.Z, 0.0f, forward.X);
            const float bankCosine = std::cos(bird.bankRadians);
            const float bankSine = std::sin(bird.bankRadians);
            const Vector3 right = levelRight * bankCosine + Vector3::Up * bankSine;
            const Vector3 up = Vector3::Up * bankCosine - levelRight * bankSine;
            const float scale = bird.scale;
            AppendNearBody(vertices,centre,right,up,forward,scale,palette);
            AppendNearHead(vertices,centre,right,up,forward,scale,palette,bird.species);

            const float perch = std::clamp(bird.perchAmount, 0.0f, 1.0f);
            const float wingLift = std::lerp(bird.wingPose * 0.38f, -0.025f, perch);
            const float wingSpan = std::lerp(1.04f, 0.21f, perch);
            const float wingRear = std::lerp(-0.06f, -0.29f, perch);
            const Vector3 leftFront = LocalPoint(
                centre, right, up, forward, -0.06f, 0.035f, 0.20f, scale);
            const Vector3 leftBack = LocalPoint(
                centre, right, up, forward, -0.08f, 0.015f, -0.24f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -wingSpan, wingLift, wingRear, scale);
            const Vector3 rightFront = LocalPoint(
                centre, right, up, forward, 0.06f, 0.035f, 0.20f, scale);
            const Vector3 rightBack = LocalPoint(
                centre, right, up, forward, 0.08f, 0.015f, -0.24f, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, wingSpan, wingLift, wingRear, scale);
            AddDoubleSidedTriangle(vertices, leftFront, leftTip, leftBack, palette.upperWing);
            AddDoubleSidedTriangle(vertices, rightFront, rightBack, rightTip, palette.upperWing);
            const Vector3 leftFeather = LocalPoint(
                centre, right, up, forward, -std::lerp(0.54f, 0.15f, perch),
                wingLift * 0.55f - 0.015f, std::lerp(-0.43f, -0.51f, perch), scale);
            const Vector3 rightFeather = LocalPoint(
                centre, right, up, forward, std::lerp(0.54f, 0.15f, perch),
                wingLift * 0.55f - 0.015f, std::lerp(-0.43f, -0.51f, perch), scale);
            AddDoubleSidedTriangle(
                vertices, leftBack, leftTip, leftFeather, palette.lowerWing);
            AddDoubleSidedTriangle(
                vertices, rightBack, rightFeather, rightTip, palette.lowerWing);

            const Vector3 tailRoot = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.40f, scale);
            const float tailHalfWidth = bird.species == BirdSpecies::Parrot ? .16f
                : (bird.species == BirdSpecies::FruitDove ? .29f : .34f);
            const float tailLength = bird.species == BirdSpecies::Parrot ? -1.08f
                : (bird.species == BirdSpecies::FruitDove ? -.75f : -.83f);
            const Vector3 tailLeft = LocalPoint(
                centre, right, up, forward, -tailHalfWidth, 0.0f, tailLength, scale);
            const Vector3 tailRight = LocalPoint(
                centre, right, up, forward, tailHalfWidth, 0.0f, tailLength, scale);
            AddDoubleSidedTriangle(vertices, tailRoot, tailLeft, tailRight, palette.tail);

            if (perch > 0.35f) {
                for (const float side : {-1.0f, 1.0f}) {
                    const Vector3 leg = LocalPoint(
                        centre, right, up, forward, side * 0.055f, -0.075f, 0.01f, scale);
                    const Vector3 foot = LocalPoint(
                        centre, right, up, forward, side * 0.055f, -0.12f, 0.01f, scale);
                    const Vector3 toe = LocalPoint(
                        centre, right, up, forward, side * 0.055f, -0.117f, 0.105f, scale);
                    AddDoubleSidedTriangle(vertices, leg, foot, toe, palette.tail);
                }
            }
        }

        void AppendFarBird(std::vector<VertexPositionColor>& vertices,
                           const BirdState& bird)
        {
            const BirdPalette& palette = PaletteForBird(bird);
            const Vector3 centre(bird.x, bird.y, bird.z);
            const Vector3 forward(std::cos(bird.headingRadians), 0.0f,
                                  std::sin(bird.headingRadians));
            const Vector3 levelRight(-forward.Z, 0.0f, forward.X);
            const float bankCosine = std::cos(bird.bankRadians);
            const float bankSine = std::sin(bird.bankRadians);
            const Vector3 right = levelRight * bankCosine + Vector3::Up * bankSine;
            const Vector3 up = Vector3::Up * bankCosine - levelRight * bankSine;
            const float scale = bird.scale;
            const float perch = std::clamp(bird.perchAmount, 0.0f, 1.0f);
            const float wingLift = std::lerp(bird.wingPose * 0.30f, -0.02f, perch);
            const float wingSpan = std::lerp(0.98f, 0.20f, perch);
            const float wingRear = std::lerp(-0.04f, -0.28f, perch);
            const Vector3 nose = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, 0.60f, scale);
            const Vector3 tail = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.55f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -wingSpan, wingLift, wingRear, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, wingSpan, wingLift, wingRear, scale);
            AddDoubleSidedTriangle(vertices, nose, leftTip, tail, palette.upperWing);
            AddDoubleSidedTriangle(vertices, nose, tail, rightTip, palette.upperWing);
        }
    }

    BirdRenderer::BirdRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Bird maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(310.0f);
        effect_->setFogEndProperty(700.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << NearBirdFeatherDiagnostic << '\n';
        std::cout << BirdPerchSilhouetteDiagnostic << '\n';
        std::cout << NearBirdHeadDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(NearBirdFeatherDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(BirdPerchSilhouetteDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(NearBirdHeadDiagnostic);
        std::cout << TropicalBirdDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TropicalBirdDiagnostic);
    }

    BirdRenderer::~BirdRenderer() = default;

    void BirdRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition,
                            const std::span<const BirdState> birds)
    {
        stagingVertices_.clear();
        visibleBirdCount_ = 0;
        visibleNearBirdCount_ = 0;
        visibleFarBirdCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);

        for (const BirdState& bird : birds) {
            const Vector3 centre(bird.x, bird.y, bird.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, bird.scale * 1.25f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearBird(stagingVertices_, bird);
                ++visibleNearBirdCount_;
            } else {
                AppendFarBird(stagingVertices_, bird);
                ++visibleFarBirdCount_;
            }
            ++visibleBirdCount_;
        }

        if (stagingVertices_.empty()) {
            return;
        }

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        vertices_->SetData(stagingVertices_.data(), 0,
                           static_cast<int>(stagingVertices_.size()), SetDataOptions::Discard);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0,
                               static_cast<int>(stagingVertices_.size() / 3U));
        ++drawCallCount_;
    }
}
