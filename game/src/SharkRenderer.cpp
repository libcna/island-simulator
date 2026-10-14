#include "IslandSimulator/SharkRenderer.hpp"

#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

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

        constexpr std::array<BodySection, 14> BodySections = {{
            {0.80f, 0.060f, 0.045f},
            {0.74f, 0.140f, 0.090f},
            {0.64f, 0.215f, 0.145f},
            {0.50f, 0.280f, 0.185f},
            {0.34f, 0.320f, 0.205f},
            {0.15f, 0.330f, 0.215f},
            {-0.05f, 0.315f, 0.208f},
            {-0.24f, 0.280f, 0.190f},
            {-0.42f, 0.225f, 0.160f},
            {-0.57f, 0.170f, 0.120f},
            {-0.68f, 0.125f, 0.088f},
            {-0.76f, 0.090f, 0.065f},
            {-0.81f, 0.065f, 0.050f},
            {-0.84f, 0.052f, 0.040f},
        }};
        constexpr int BodySegments = 18;
        constexpr std::string_view SharkDetailDiagnostic =
            "island-simulator: near shark uses a fourteen-station eighteen-sided capped body, closed fins, five paired gill slits, and fitted eyes";
        constexpr std::string_view SharkCountershadeDiagnostic =
            "island-simulator: shark retains deep dorsal and pale ventral countershading through water tint";
        constexpr std::string_view SharkProportionDiagnostic =
            "island-simulator: shark uses a long low spindle, reinforced caudal peduncle, and swept restrained fins";

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

        [[nodiscard]] Color BodyColor(const Color& back, const Color& body,
                                      const Color& belly, const float vertical)
        {
            if (vertical < 0.0f)
                return Color::Lerp(body, belly,
                                   std::clamp(-vertical * .98f, 0.0f, 1.0f));
            return Color::Lerp(body, back,
                               std::clamp(vertical * .96f, 0.0f, 1.0f));
        }

        [[nodiscard]] Color SmoothBodyColor(const Color& back, const Color& body,
                                            const Color& belly, const PoseAxes& axes,
                                            const float angle)
        {
            const Vector3 normal = Vector3::Normalize(
                axes.right * std::cos(angle) + axes.up * std::sin(angle));
            const float direct = std::max(
                0.0f, Vector3::Dot(normal, wildlife_render::FixedSunDirection));
            const float sky = std::max(0.0f, normal.Y);
            return wildlife_render::ScaleColor(
                BodyColor(back, body, belly, std::sin(angle)),
                .76f + .25f * direct + .05f * sky);
        }

        void AddGradientTriangle(std::vector<VertexPositionColor>& vertices,
                                 const Vector3& a, const Vector3& b, const Vector3& c,
                                 const Color& colorA, const Color& colorB,
                                 const Color& colorC)
        {
            vertices.emplace_back(a, colorA);
            vertices.emplace_back(b, colorB);
            vertices.emplace_back(c, colorC);
        }

        void AppendClosedFin(std::vector<VertexPositionColor>& vertices,
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
                ActivityColor(Color(78, 96, 103, 255), shark.activity), waterLight);
            const Color back = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(24, 43, 52, 255), shark.activity), waterLight);
            const Color belly = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(185, 188, 174, 255), shark.activity), waterLight);
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
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, -0.008f, 0.88f, scale);
            for (int segment = 0; segment < BodySegments; ++segment) {
                const int next = (segment + 1) % BodySegments;
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(segment) / static_cast<float>(BodySegments);
                const float nextAngle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(next) / static_cast<float>(BodySegments);
                AddGradientTriangle(vertices, nose, rings.front()[segment],
                                    rings.front()[next], body,
                                    SmoothBodyColor(back, body, belly, axes, angle),
                                    SmoothBodyColor(back, body, belly, axes, nextAngle));
            }
            for (std::size_t row = 0; row + 1U < BodySections.size(); ++row) {
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const int next = (segment + 1) % BodySegments;
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(BodySegments);
                    const float nextAngle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(next) / static_cast<float>(BodySegments);
                    const Color color = SmoothBodyColor(back, body, belly, axes, angle);
                    const Color nextColor = SmoothBodyColor(back, body, belly, axes, nextAngle);
                    AddGradientTriangle(vertices, rings[row][segment], rings[row + 1U][segment],
                                        rings[row + 1U][next], color, color, nextColor);
                    AddGradientTriangle(vertices, rings[row][segment], rings[row + 1U][next],
                                        rings[row][next], color, nextColor, nextColor);
                }
            }

            const float rearBend = shark.bodyWave * 0.125f;
            const Vector3 rearCap = LocalPoint(centre, axes, rearBend, 0.0f, -0.87f, scale);
            for (int segment = 0; segment < BodySegments; ++segment) {
                const int next = (segment + 1) % BodySegments;
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(segment) / static_cast<float>(BodySegments);
                const float nextAngle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(next) / static_cast<float>(BodySegments);
                AddGradientTriangle(vertices, rings.back()[segment], rearCap,
                                    rings.back()[next],
                                    SmoothBodyColor(back, body, belly, axes, angle), body,
                                    SmoothBodyColor(back, body, belly, axes, nextAngle));
            }

            const float bend = shark.bodyWave * 0.125f;
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.84f, scale);

            const float tailBend = shark.bodyWave * 0.21f;
            const Vector3 upperKnee = LocalPoint(
                centre, axes, tailBend, 0.24f, -0.98f, scale);
            const Vector3 upperTip = LocalPoint(
                centre, axes, tailBend, 0.50f, -1.28f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, tailBend * 1.05f, 0.02f, -1.05f, scale);
            const Vector3 lowerKnee = LocalPoint(
                centre, axes, tailBend, -0.18f, -0.99f, scale);
            const Vector3 lowerTip = LocalPoint(
                centre, axes, tailBend, -0.30f, -1.19f, scale);
            AppendClosedFin(vertices, rear, upperKnee, upperTip, tailNotch,
                            axes.right * (0.007f * scale), body, fin);
            AppendClosedFin(vertices, rear, tailNotch, lowerTip, lowerKnee,
                            axes.right * (0.006f * scale), body, fin);

            const Vector3 dorsalFront = LocalPoint(
                centre, axes, 0.0f, 0.18f, 0.25f, scale);
            const Vector3 dorsalTip = LocalPoint(
                centre, axes, 0.0f, 0.47f, -0.02f, scale);
            const Vector3 dorsalBack = LocalPoint(
                centre, axes, bend * 0.35f, 0.13f, -0.38f, scale);
            const Vector3 dorsalKnee = LocalPoint(
                centre, axes, bend * 0.17f, 0.33f, -0.22f, scale);
            AppendClosedFin(vertices, dorsalFront, dorsalTip, dorsalKnee, dorsalBack,
                            axes.right * (0.012f * scale), body, fin);

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 pectoralFront = LocalPoint(
                    centre, axes, side * 0.27f, -0.015f, 0.19f, scale);
                const Vector3 pectoralTip = LocalPoint(
                    centre, axes, side * 0.66f, -0.13f, -0.13f, scale);
                const Vector3 pectoralElbow = LocalPoint(
                    centre, axes, side * 0.51f, -0.13f, -0.23f, scale);
                const Vector3 pectoralBack = LocalPoint(
                    centre, axes, side * 0.23f, -0.09f, -0.27f, scale);
                AppendClosedFin(vertices, pectoralFront, pectoralTip,
                                pectoralElbow, pectoralBack,
                                axes.up * (0.010f * scale), body, fin);

                const Vector3 pelvicFront = LocalPoint(
                    centre, axes, side * 0.17f, -0.08f, -0.32f, scale);
                const Vector3 pelvicTip = LocalPoint(
                    centre, axes, side * 0.32f, -0.12f, -0.54f, scale);
                const Vector3 pelvicElbow = LocalPoint(
                    centre, axes, side * 0.26f, -0.12f, -0.59f, scale);
                const Vector3 pelvicBack = LocalPoint(
                    centre, axes, side * 0.12f, -0.09f, -0.56f, scale);
                AppendClosedFin(vertices, pelvicFront, pelvicTip,
                                pelvicElbow, pelvicBack,
                                axes.up * (0.007f * scale), body, fin);
            }
            const Vector3 secondaryFront = LocalPoint(
                centre, axes, 0.0f, 0.13f, -0.31f, scale);
            const Vector3 secondaryTip = LocalPoint(
                centre, axes, bend * 0.45f, 0.27f, -0.46f, scale);
            const Vector3 secondaryKnee = LocalPoint(
                centre, axes, bend * 0.60f, 0.18f, -0.52f, scale);
            const Vector3 secondaryBack = LocalPoint(
                centre, axes, bend * 0.72f, 0.09f, -0.57f, scale);
            AppendClosedFin(vertices, secondaryFront, secondaryTip,
                            secondaryKnee, secondaryBack,
                            axes.right * (0.006f * scale), body, fin);

            const Color eye = ActivityColor(Color(15, 24, 27, 255), shark.activity);
            const Color eyeShine = ActivityColor(Color(116, 137, 139, 255), shark.activity);
            const Color gill = ActivityColor(Color(43, 59, 64, 255), shark.activity);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 eyeTop = LocalPoint(
                    centre, axes, side * 0.245f, 0.084f, 0.535f, scale);
                const Vector3 eyeFront = LocalPoint(
                    centre, axes, side * 0.250f, 0.050f, 0.564f, scale);
                const Vector3 eyeBottom = LocalPoint(
                    centre, axes, side * 0.253f, 0.017f, 0.530f, scale);
                const Vector3 eyeRear = LocalPoint(
                    centre, axes, side * 0.249f, 0.050f, 0.495f, scale);
                if (side > 0.0f) {
                    AddTriangle(vertices, eyeTop, eyeBottom, eyeFront, eye);
                    AddTriangle(vertices, eyeTop, eyeRear, eyeBottom, eye);
                } else {
                    AddTriangle(vertices, eyeTop, eyeFront, eyeBottom, eye);
                    AddTriangle(vertices, eyeTop, eyeBottom, eyeRear, eye);
                }
                AddTriangle(vertices,
                    LocalPoint(centre, axes, side * 0.256f, 0.072f, 0.539f, scale),
                    LocalPoint(centre, axes, side * 0.257f, 0.057f, 0.551f, scale),
                    LocalPoint(centre, axes, side * 0.257f, 0.056f, 0.527f, scale), eyeShine);

                for (int slit = 0; slit < 5; ++slit) {
                    const float z = 0.335f - static_cast<float>(slit) * 0.057f;
                    const Vector3 topFront = LocalPoint(
                        centre, axes, side * 0.316f, 0.064f, z + 0.010f, scale);
                    const Vector3 topBack = LocalPoint(
                        centre, axes, side * 0.319f, 0.057f, z - 0.010f, scale);
                    const Vector3 bottomFront = LocalPoint(
                        centre, axes, side * 0.319f, -0.030f, z - 0.020f, scale);
                    const Vector3 bottomBack = LocalPoint(
                        centre, axes, side * 0.321f, -0.037f, z - 0.040f, scale);
                    if (side > 0.0f) {
                        AddTriangle(vertices, topFront, bottomFront, topBack, gill);
                        AddTriangle(vertices, topBack, bottomFront, bottomBack, gill);
                    } else {
                        AddTriangle(vertices, topFront, topBack, bottomFront, gill);
                        AddTriangle(vertices, topBack, bottomBack, bottomFront, gill);
                    }
                }
                const Vector3 mouthFront = LocalPoint(
                    centre, axes, side * 0.055f, -0.072f, 0.754f, scale);
                const Vector3 mouthOuter = LocalPoint(
                    centre, axes, side * 0.180f, -0.102f, 0.625f, scale);
                const Vector3 mouthKnee = LocalPoint(
                    centre, axes, side * 0.125f, -0.105f, 0.682f, scale);
                const Vector3 mouthInner = LocalPoint(
                    centre, axes, side * 0.065f, -0.094f, 0.724f, scale);
                AddTriangle(vertices, mouthFront, mouthOuter, mouthKnee, gill);
                AddTriangle(vertices, mouthFront, mouthKnee, mouthInner, gill);
                AddTriangle(vertices,
                    LocalPoint(centre, axes, side * 0.072f, -0.052f, 0.770f, scale),
                    LocalPoint(centre, axes, side * 0.104f, -0.061f, 0.744f, scale),
                    LocalPoint(centre, axes, side * 0.076f, -0.068f, 0.746f, scale), eye);
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
                ActivityColor(Color(75, 94, 102, 255), shark.activity), waterLight);
            const Color belly = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(174, 180, 169, 255), shark.activity), waterLight);
            const Color fin = wildlife_render::ApplyUnderwaterLight(
                ActivityColor(Color(54, 72, 81, 255), shark.activity), waterLight);
            const PoseAxes axes = MakePoseAxes(shark);
            const float scale = shark.scale;
            const float bend = shark.bodyWave * 0.12f;
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, 0.0f, 0.77f, scale);
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.16f, 0.06f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.12f, 0.06f, scale);
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.65f, scale);
            const Vector3 tailTop = LocalPoint(
                centre, axes, bend * 1.5f, 0.27f, -1.01f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, bend * 1.62f, 0.0f, -0.84f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, bend * 1.5f, -0.23f, -0.95f, scale);
            const Vector3 dorsal = LocalPoint(
                centre, axes, 0.0f, 0.41f, -0.09f, scale);
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
        std::cout << SharkDetailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(SharkDetailDiagnostic);
        std::cout << SharkCountershadeDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(SharkCountershadeDiagnostic);
        std::cout << SharkProportionDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(SharkProportionDiagnostic);
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
