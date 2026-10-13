#include "IslandSimulator/FishRenderer.hpp"

#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
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
        constexpr float NearLodDistance = 72.0f;
        constexpr int MaximumVerticesPerFish = FishRenderer::NearVertexCount;
        constexpr int MaximumVertexCount = static_cast<int>(FishSchools::FishCount)
            * MaximumVerticesPerFish;

        struct FishPalette
        {
            Color back;
            Color body;
            Color belly;
            Color fin;
        };

        struct PoseAxes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

        const std::array<FishPalette, 4> Palettes = {{
            {Color(66, 111, 117, 255), Color(126, 166, 166, 255),
             Color(207, 214, 184, 255),
             Color(218, 177, 67, 255)},
            {Color(21, 76, 91, 255), Color(41, 126, 138, 255),
             Color(125, 191, 175, 255),
             Color(35, 83, 102, 255)},
            {Color(102, 79, 51, 255), Color(174, 133, 74, 255),
             Color(223, 194, 129, 255),
             Color(116, 83, 52, 255)},
            {Color(28, 48, 91, 255), Color(59, 82, 126, 255),
             Color(140, 168, 180, 255),
             Color(224, 183, 58, 255)}
        }};

        struct BodySection
        {
            float z;
            float halfWidth;
            float halfHeight;
        };

        constexpr std::array<BodySection, 7> BodySections = {{
            {0.52f, 0.018f, 0.024f},
            {0.44f, 0.074f, 0.068f},
            {0.28f, 0.145f, 0.135f},
            {0.08f, 0.188f, 0.180f},
            {-0.14f, 0.170f, 0.160f},
            {-0.33f, 0.115f, 0.105f},
            {-0.45f, 0.050f, 0.052f},
        }};
        constexpr int BodySegments = 10;
        constexpr std::string_view FishDetailDiagnostic =
            "island-simulator: near fish use rounded ten-facet bodies, closed-volume fins, and two-facet eyes";

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            vertices.emplace_back(a, color);
            vertices.emplace_back(b, color);
            vertices.emplace_back(c, color);
        }

        void AddLitTriangle(std::vector<VertexPositionColor>& vertices,
                            const Vector3& a, const Vector3& b, const Vector3& c,
                            const Color& color, const bool twoSided = false)
        {
            AddTriangle(vertices, a, b, c,
                        wildlife_render::FacetColor(a, b, c, color, twoSided));
        }

        void AddLitQuad(std::vector<VertexPositionColor>& vertices,
                        const Vector3& a, const Vector3& b,
                        const Vector3& c, const Vector3& d, const Color& color)
        {
            AddLitTriangle(vertices, a, b, c, color);
            AddLitTriangle(vertices, a, c, d, color);
        }

        void AppendClosedTriangle(std::vector<VertexPositionColor>& vertices,
                                  const Vector3& a, const Vector3& b, const Vector3& c,
                                  const Vector3& halfThickness,
                                  const Color& face, const Color& edge)
        {
            const Vector3 ap = a + halfThickness, bp = b + halfThickness;
            const Vector3 cp = c + halfThickness;
            const Vector3 am = a - halfThickness, bm = b - halfThickness;
            const Vector3 cm = c - halfThickness;
            AddLitTriangle(vertices, ap, bp, cp, face);
            AddLitTriangle(vertices, am, cm, bm, face);
            AddLitQuad(vertices, ap, am, bm, bp, edge);
            AddLitQuad(vertices, bp, bm, cm, cp, edge);
            AddLitQuad(vertices, cp, cm, am, ap, edge);
        }

        void AppendClosedQuad(std::vector<VertexPositionColor>& vertices,
                              const Vector3& a, const Vector3& b,
                              const Vector3& c, const Vector3& d,
                              const Vector3& halfThickness,
                              const Color& face, const Color& edge)
        {
            const Vector3 ap = a + halfThickness, bp = b + halfThickness;
            const Vector3 cp = c + halfThickness, dp = d + halfThickness;
            const Vector3 am = a - halfThickness, bm = b - halfThickness;
            const Vector3 cm = c - halfThickness, dm = d - halfThickness;
            AddLitQuad(vertices, ap, bp, cp, dp, face);
            AddLitQuad(vertices, am, dm, cm, bm, face);
            AddLitQuad(vertices, ap, am, bm, bp, edge);
            AddLitQuad(vertices, bp, bm, cm, cp, edge);
            AddLitQuad(vertices, cp, cm, dm, dp, edge);
            AddLitQuad(vertices, dp, dm, am, ap, edge);
        }

        [[nodiscard]] PoseAxes MakePoseAxes(const FishState& fish)
        {
            const Vector3 levelForward(std::cos(fish.headingRadians), 0.0f,
                                       std::sin(fish.headingRadians));
            const Vector3 levelRight(-levelForward.Z, 0.0f, levelForward.X);
            const float pitchCosine = std::cos(fish.pitchRadians);
            const float pitchSine = std::sin(fish.pitchRadians);
            const Vector3 forward = levelForward * pitchCosine + Vector3::Up * pitchSine;
            const Vector3 pitchUp = Vector3::Up * pitchCosine - levelForward * pitchSine;
            const float bankCosine = std::cos(fish.bankRadians);
            const float bankSine = std::sin(fish.bankRadians);
            return PoseAxes{
                levelRight * bankCosine + pitchUp * bankSine,
                pitchUp * bankCosine - levelRight * bankSine,
                forward,
            };
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const PoseAxes& axes,
                                         const float localX,
                                         const float localY, const float localZ,
                                         const float scale)
        {
            return centre + axes.right * (localX * scale) + axes.up * (localY * scale)
                + axes.forward * (localZ * scale);
        }

        void AppendNearFish(std::vector<VertexPositionColor>& vertices,
                            const FishState& fish, const float timeSeconds)
        {
            const std::size_t startingVertexCount = vertices.size();
            const Vector3 centre(fish.x, fish.y, fish.z);
            const FishPalette& sourcePalette = Palettes[fish.colorVariant % Palettes.size()];
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const FishPalette palette{
                wildlife_render::ApplyUnderwaterLight(sourcePalette.back, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.body, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.belly, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.fin, waterLight),
            };
            const PoseAxes axes = MakePoseAxes(fish);
            const float scale = fish.scale;
            const float widthScale = fish.colorVariant % 4U == 1U ? 0.88f : 1.0f;
            const float heightScale = fish.colorVariant % 4U == 2U ? 1.12f : 1.0f;
            std::array<std::array<Vector3, BodySegments>, BodySections.size()> rings{};
            for (std::size_t row = 0; row < BodySections.size(); ++row) {
                const BodySection& section = BodySections[row];
                const float rearWeight = std::clamp((0.20f - section.z) / 0.65f, 0.0f, 1.0f);
                const float bend = fish.bodyWave * 0.105f * rearWeight * rearWeight;
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(BodySegments);
                    rings[row][segment] = LocalPoint(
                        centre, axes,
                        bend + section.halfWidth * widthScale * std::cos(angle),
                        section.halfHeight * heightScale * std::sin(angle), section.z, scale);
                }
            }
            for (std::size_t row = 0; row + 1U < BodySections.size(); ++row) {
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const int next = (segment + 1) % BodySegments;
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * (static_cast<float>(segment) + 0.5f)
                        / static_cast<float>(BodySegments);
                    const Color color = std::sin(angle) > 0.36f ? palette.back
                        : (std::sin(angle) < -0.20f ? palette.belly : palette.body);
                    AddLitTriangle(vertices, rings[row][segment], rings[row + 1U][segment],
                                   rings[row + 1U][next], color);
                    AddLitTriangle(vertices, rings[row][segment], rings[row + 1U][next],
                                   rings[row][next], color);
                }
            }

            const float bend = fish.bodyWave * 0.105f;
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.45f, scale);

            const float tailBend = fish.bodyWave * 0.17f;
            const Vector3 tailTop = LocalPoint(
                centre, axes, tailBend, 0.25f, -0.78f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, tailBend * 1.08f, 0.0f, -0.66f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, tailBend, -0.25f, -0.78f, scale);
            AppendClosedTriangle(vertices, rear, tailTop, tailNotch,
                                 axes.right * (0.004f * scale),
                                 palette.fin, palette.back);
            AppendClosedTriangle(vertices, rear, tailNotch, tailBottom,
                                 axes.right * (0.004f * scale),
                                 palette.fin, palette.back);

            const Vector3 dorsalFront = LocalPoint(
                centre, axes, 0.0f, 0.13f, 0.12f, scale);
            const Vector3 dorsalTip = LocalPoint(
                centre, axes, 0.0f, 0.34f, -0.08f, scale);
            const Vector3 dorsalBack = LocalPoint(
                centre, axes, bend * 0.45f, 0.12f, -0.27f, scale);
            const Vector3 dorsalKnee = LocalPoint(
                centre, axes, bend * 0.25f, 0.25f, -0.19f, scale);
            AppendClosedQuad(vertices, dorsalFront, dorsalTip, dorsalKnee, dorsalBack,
                             axes.right * (0.006f * scale),
                             palette.fin, palette.back);

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 rootFront = LocalPoint(
                    centre, axes, side * 0.15f, -0.015f, 0.14f, scale);
                const Vector3 tip = LocalPoint(
                    centre, axes, side * 0.37f, -0.09f, -0.10f, scale);
                const Vector3 elbow = LocalPoint(
                    centre, axes, side * 0.27f, -0.075f, -0.17f, scale);
                const Vector3 rootBack = LocalPoint(
                    centre, axes, side * 0.13f, -0.055f, -0.21f, scale);
                AppendClosedQuad(vertices, rootFront, tip, elbow, rootBack,
                                 axes.up * (0.004f * scale),
                                 palette.fin, palette.back);
            }

            const Color eye(19, 31, 33, 255);
            const Color eyeShine(132, 158, 151, 255);
            const Color gill(65, 86, 83, 255);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 eyeTop = LocalPoint(centre, axes, side * 0.153f, 0.082f, 0.288f, scale);
                const Vector3 eyeFront = LocalPoint(centre, axes, side * 0.158f, 0.044f, 0.318f, scale);
                const Vector3 eyeBottom = LocalPoint(centre, axes, side * 0.160f, 0.008f, 0.282f, scale);
                const Vector3 eyeRear = LocalPoint(centre, axes, side * 0.157f, 0.044f, 0.247f, scale);
                if (side > 0.0f) {
                    AddTriangle(vertices, eyeTop, eyeBottom, eyeFront, eye);
                    AddTriangle(vertices, eyeTop, eyeRear, eyeBottom, eye);
                } else {
                    AddTriangle(vertices, eyeTop, eyeFront, eyeBottom, eye);
                    AddTriangle(vertices, eyeTop, eyeBottom, eyeRear, eye);
                }
                AddTriangle(vertices,
                    LocalPoint(centre, axes, side * 0.162f, 0.070f, 0.292f, scale),
                    LocalPoint(centre, axes, side * 0.163f, 0.052f, 0.305f, scale),
                    LocalPoint(centre, axes, side * 0.163f, 0.051f, 0.279f, scale), eyeShine);
            }
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.190f, 0.070f, 0.145f, scale),
                        LocalPoint(centre, axes, -0.192f, -0.060f, 0.115f, scale),
                        LocalPoint(centre, axes, -0.193f, -0.042f, 0.158f, scale), gill);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.190f, 0.070f, 0.145f, scale),
                        LocalPoint(centre, axes, 0.193f, -0.042f, 0.158f, scale),
                        LocalPoint(centre, axes, 0.192f, -0.060f, 0.115f, scale), gill);
            for (const float side : {-1.0f, 1.0f}) {
                AddTriangle(vertices,
                            LocalPoint(centre, axes, side * 0.035f, -0.027f, 0.486f, scale),
                            LocalPoint(centre, axes, side * 0.110f, -0.034f, 0.405f, scale),
                            LocalPoint(centre, axes, side * 0.040f, -0.045f, 0.465f, scale),
                            gill);
            }
            if (vertices.size() - startingVertexCount != FishRenderer::NearVertexCount) {
                throw std::runtime_error("Fish near mesh does not match its fixed vertex budget");
            }
        }

        void AppendFarFish(std::vector<VertexPositionColor>& vertices,
                           const FishState& fish, const float timeSeconds)
        {
            const Vector3 centre(fish.x, fish.y, fish.z);
            const FishPalette& sourcePalette = Palettes[fish.colorVariant % Palettes.size()];
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const FishPalette palette{
                wildlife_render::ApplyUnderwaterLight(sourcePalette.back, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.body, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.belly, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.fin, waterLight),
            };
            const PoseAxes axes = MakePoseAxes(fish);
            const float scale = fish.scale;
            const float bend = fish.bodyWave * 0.12f;
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, 0.0f, 0.46f, scale);
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.17f, 0.0f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.14f, 0.0f, scale);
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.43f, scale);
            const Vector3 tailTop = LocalPoint(
                centre, axes, bend * 1.5f, 0.21f, -0.72f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, bend * 1.62f, 0.0f, -0.62f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, bend * 1.5f, -0.21f, -0.72f, scale);
            AddLitTriangle(vertices, nose, top, rear, palette.body, true);
            AddLitTriangle(vertices, nose, rear, bottom, palette.belly, true);
            AddTriangle(vertices, rear, tailTop, tailNotch, palette.fin);
            AddTriangle(vertices, rear, tailNotch, tailBottom, palette.fin);
        }
    }

    FishRenderer::FishRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Fish maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.16f, 0.55f, 0.61f));
        effect_->setFogStartProperty(70.0f);
        effect_->setFogEndProperty(220.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << FishDetailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishDetailDiagnostic);
    }

    FishRenderer::~FishRenderer() = default;

    void FishRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition,
                            const std::span<const FishState> fish,
                            const float timeSeconds)
    {
        stagingVertices_.clear();
        visibleFishCount_ = 0;
        visibleNearFishCount_ = 0;
        visibleFarFishCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);

        for (const FishState& state : fish) {
            const Vector3 centre(state.x, state.y, state.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, state.scale * 0.85f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearFish(stagingVertices_, state, timeSeconds);
                ++visibleNearFishCount_;
            } else {
                AppendFarFish(stagingVertices_, state, timeSeconds);
                ++visibleFarFishCount_;
            }
            ++visibleFishCount_;
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
