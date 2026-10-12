#include "IslandSimulator/SharkRenderer.hpp"

#include "IslandSimulator/WildlifeRenderLighting.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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
        constexpr float NearLodDistance = 240.0f;
        constexpr int MaximumVertexCount = SharkRenderer::NearVertexCount;

        struct PoseAxes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

        struct BodySection
        {
            float z;
            float halfWidth;
            float halfHeight;
        };

        constexpr std::array<BodySection, 7> BodySections = {{
            {0.68f, 0.140f, 0.110f},
            {0.53f, 0.230f, 0.170f},
            {0.28f, 0.300f, 0.240f},
            {0.00f, 0.320f, 0.260f},
            {-0.28f, 0.260f, 0.210f},
            {-0.50f, 0.150f, 0.130f},
            {-0.66f, 0.060f, 0.060f},
        }};
        constexpr int BodySegments = 8;

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

        [[nodiscard]] Color ActivityColor(const Color& color, const float activity)
        {
            constexpr int waterR = 42;
            constexpr int waterG = 121;
            constexpr int waterB = 132;
            const float blend = std::clamp(activity, 0.0f, 1.0f);
            const auto channel = [blend](const int water, const std::uint8_t source) {
                return water + static_cast<int>(std::lround(
                    (static_cast<float>(source) - static_cast<float>(water)) * blend));
            };
            return Color(channel(waterR, color.getRProperty()),
                         channel(waterG, color.getGProperty()),
                         channel(waterB, color.getBProperty()), 255);
        }

        [[nodiscard]] PoseAxes MakePoseAxes(const SharkState& shark)
        {
            const Vector3 levelForward(std::cos(shark.headingRadians), 0.0f,
                                       std::sin(shark.headingRadians));
            const Vector3 levelRight(-levelForward.Z, 0.0f, levelForward.X);
            const float pitchCosine = std::cos(shark.pitchRadians);
            const float pitchSine = std::sin(shark.pitchRadians);
            const Vector3 forward = levelForward * pitchCosine + Vector3::Up * pitchSine;
            const Vector3 pitchUp = Vector3::Up * pitchCosine - levelForward * pitchSine;
            const float bankCosine = std::cos(shark.bankRadians);
            const float bankSine = std::sin(shark.bankRadians);
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

        void AppendNearShark(std::vector<VertexPositionColor>& vertices,
                             const SharkState& shark, const float timeSeconds)
        {
            const std::size_t startingVertexCount = vertices.size();
            const Vector3 centre(shark.x, shark.y, shark.z);
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const Color body = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(72, 91, 99, 255), shark.activity), waterLight);
            const Color back = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(43, 61, 70, 255), shark.activity), waterLight);
            const Color belly = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(147, 159, 156, 255), shark.activity), waterLight);
            const Color fin = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(58, 76, 84, 255), shark.activity), waterLight);
            const PoseAxes axes = MakePoseAxes(shark);
            const float scale = shark.scale;
            std::array<std::array<Vector3, BodySegments>, BodySections.size()> rings{};
            for (std::size_t row = 0; row < BodySections.size(); ++row) {
                const BodySection& section = BodySections[row];
                const float rearWeight = std::clamp((0.18f - section.z) / 0.84f, 0.0f, 1.0f);
                const float bend = shark.bodyWave * 0.125f * rearWeight * rearWeight;
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(BodySegments);
                    rings[row][segment] = LocalPoint(
                        centre, axes, bend + section.halfWidth * std::cos(angle),
                        section.halfHeight * std::sin(angle), section.z, scale);
                }
            }
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, -0.012f, 0.78f, scale);
            for (int segment = 0; segment < BodySegments; ++segment) {
                AddLitTriangle(vertices, nose, rings.front()[segment],
                               rings.front()[(segment + 1) % BodySegments], body);
            }
            for (std::size_t row = 0; row + 1U < BodySections.size(); ++row) {
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const int next = (segment + 1) % BodySegments;
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * (static_cast<float>(segment) + 0.5f)
                        / static_cast<float>(BodySegments);
                    const Color color = std::sin(angle) > 0.30f ? back
                        : (std::sin(angle) < -0.22f ? belly : body);
                    AddLitTriangle(vertices, rings[row][segment], rings[row + 1U][segment],
                                   rings[row + 1U][next], color);
                    AddLitTriangle(vertices, rings[row][segment], rings[row + 1U][next],
                                   rings[row][next], color);
                }
            }

            const float bend = shark.bodyWave * 0.125f;
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.66f, scale);

            const float tailBend = shark.bodyWave * 0.21f;
            const Vector3 upperKnee = LocalPoint(
                centre, axes, tailBend, 0.31f, -0.86f, scale);
            const Vector3 upperTip = LocalPoint(
                centre, axes, tailBend, 0.57f, -1.12f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, tailBend * 1.05f, 0.02f, -0.92f, scale);
            const Vector3 lowerKnee = LocalPoint(
                centre, axes, tailBend, -0.20f, -0.87f, scale);
            const Vector3 lowerTip = LocalPoint(
                centre, axes, tailBend, -0.34f, -1.04f, scale);
            AddTriangle(vertices, rear, upperKnee, tailNotch, fin);
            AddTriangle(vertices, upperKnee, upperTip, tailNotch, fin);
            AddTriangle(vertices, rear, tailNotch, lowerKnee, fin);
            AddTriangle(vertices, lowerKnee, tailNotch, lowerTip, fin);

            const Vector3 dorsalFront = LocalPoint(
                centre, axes, 0.0f, 0.14f, 0.23f, scale);
            const Vector3 dorsalTip = LocalPoint(
                centre, axes, 0.0f, 0.57f, -0.08f, scale);
            const Vector3 dorsalBack = LocalPoint(
                centre, axes, bend * 0.35f, 0.12f, -0.34f, scale);
            const Vector3 dorsalKnee = LocalPoint(
                centre, axes, bend * 0.17f, 0.38f, -0.18f, scale);
            AddTriangle(vertices, dorsalFront, dorsalTip, dorsalKnee, fin);
            AddTriangle(vertices, dorsalFront, dorsalKnee, dorsalBack, fin);

            for (const float side : {-1.0f, 1.0f}) {
                AddTriangle(vertices,
                            LocalPoint(centre, axes, side * 0.27f, -0.015f, 0.19f, scale),
                            LocalPoint(centre, axes, side * 0.76f, -0.17f, -0.10f, scale),
                            LocalPoint(centre, axes, side * 0.23f, -0.10f, -0.20f, scale), fin);
                AddTriangle(vertices,
                            LocalPoint(centre, axes, side * 0.17f, -0.08f, -0.33f, scale),
                            LocalPoint(centre, axes, side * 0.36f, -0.13f, -0.53f, scale),
                            LocalPoint(centre, axes, side * 0.12f, -0.09f, -0.55f, scale), fin);
            }
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.0f, 0.13f, -0.31f, scale),
                        LocalPoint(centre, axes, bend * 0.45f, 0.27f, -0.46f, scale),
                        LocalPoint(centre, axes, bend * 0.72f, 0.09f, -0.57f, scale), fin);

            const Color eye = ActivityColor(Color(15, 24, 27, 255), shark.activity);
            const Color gill = ActivityColor(Color(43, 59, 64, 255), shark.activity);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.236f, 0.082f, 0.505f, scale),
                        LocalPoint(centre, axes, -0.244f, 0.018f, 0.527f, scale),
                        LocalPoint(centre, axes, -0.246f, 0.020f, 0.442f, scale), eye);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.236f, 0.082f, 0.505f, scale),
                        LocalPoint(centre, axes, 0.246f, 0.020f, 0.442f, scale),
                        LocalPoint(centre, axes, 0.244f, 0.018f, 0.527f, scale), eye);
            for (const float side : {-1.0f, 1.0f}) {
                for (int slit = 0; slit < 3; ++slit) {
                    const float z = 0.30f - static_cast<float>(slit) * 0.085f;
                    AddTriangle(vertices,
                                LocalPoint(centre, axes, side * 0.307f, 0.075f, z, scale),
                                LocalPoint(centre, axes, side * 0.312f, -0.075f,
                                           z - 0.025f, scale),
                                LocalPoint(centre, axes, side * 0.314f, -0.045f,
                                           z + 0.010f, scale), gill);
                }
                AddTriangle(vertices,
                            LocalPoint(centre, axes, side * 0.075f, -0.075f, 0.755f, scale),
                            LocalPoint(centre, axes, side * 0.175f, -0.090f, 0.625f, scale),
                            LocalPoint(centre, axes, side * 0.080f, -0.100f, 0.720f, scale), gill);
            }
            if (vertices.size() - startingVertexCount != SharkRenderer::NearVertexCount) {
                throw std::runtime_error("Shark near mesh does not match its fixed vertex budget");
            }
        }

        void AppendFarShark(std::vector<VertexPositionColor>& vertices,
                            const SharkState& shark, const float timeSeconds)
        {
            const Vector3 centre(shark.x, shark.y, shark.z);
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const Color body = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(67, 88, 97, 255), shark.activity), waterLight);
            const Color belly = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(139, 154, 153, 255), shark.activity), waterLight);
            const Color fin = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(54, 72, 81, 255), shark.activity), waterLight);
            const PoseAxes axes = MakePoseAxes(shark);
            const float scale = shark.scale;
            const float bend = shark.bodyWave * 0.12f;
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, 0.0f, 0.68f, scale);
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.18f, 0.06f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.14f, 0.06f, scale);
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.55f, scale);
            const Vector3 tailTop = LocalPoint(
                centre, axes, bend * 1.5f, 0.30f, -0.91f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, bend * 1.62f, 0.0f, -0.76f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, bend * 1.5f, -0.30f, -0.91f, scale);
            const Vector3 dorsal = LocalPoint(
                centre, axes, 0.0f, 0.49f, -0.06f, scale);
            AddLitTriangle(vertices, nose, top, rear, body, true);
            AddLitTriangle(vertices, nose, rear, bottom, belly, true);
            AddTriangle(vertices, rear, tailTop, tailNotch, fin);
            AddTriangle(vertices, rear, tailNotch, tailBottom, fin);
            AddTriangle(vertices, top, dorsal, rear, fin);
        }
    }

    SharkRenderer::SharkRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Shark maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.16f, 0.55f, 0.61f));
        effect_->setFogStartProperty(260.0f);
        effect_->setFogEndProperty(700.0f);
        stagingVertices_.reserve(MaximumVertexCount);
    }

    SharkRenderer::~SharkRenderer() = default;

    void SharkRenderer::Draw(const Matrix& view, const Matrix& projection,
                             const Vector3& cameraPosition, const SharkState& shark,
                             const float timeSeconds)
    {
        stagingVertices_.clear();
        visible_ = false;
        nearLod_ = false;
        drawCallCount_ = 0;
        if (!shark.visible) {
            return;
        }

        const Vector3 centre(shark.x, shark.y, shark.z);
        const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
        const BoundingFrustum frustum(view * projection);
        if (distanceSquared > maximumDistance_ * maximumDistance_
            || !frustum.Intersects(BoundingSphere(centre, shark.scale * 1.15f))) {
            return;
        }
        nearLod_ = distanceSquared <= NearLodDistance * NearLodDistance;
        if (nearLod_) {
            AppendNearShark(stagingVertices_, shark, timeSeconds);
        } else {
            AppendFarShark(stagingVertices_, shark, timeSeconds);
        }
        visible_ = true;

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
