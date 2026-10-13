#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/CrabRenderer.hpp"

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
        constexpr float NearLodDistance = 38.0f;
        constexpr int NearCrabVertexCount = 738;
        constexpr int FarCrabVertexCount = 12;
        constexpr int MaximumVerticesPerCrab = NearCrabVertexCount;
        constexpr int MaximumVertexCount = static_cast<int>(CrabColony::CrabCount)
            * MaximumVerticesPerCrab;
        constexpr std::string_view CrabDiagnostic =
            "island-simulator: beach crabs use rounded carapaces, volumetric jointed legs, eye stalks, and one culled batch";
        constexpr std::string_view CrabDetailDiagnostic =
            "island-simulator: near crabs use fixed-sun facets, tapered distal toes, and socketed eyes";

        struct CrabPalette
        {
            Color shellTop;
            Color shellHighlight;
            Color shellSide;
            Color limb;
            Color joint;
            Color claw;
        };

        const std::array<CrabPalette, 4> Palettes = {{
            {Color(139, 61, 39, 255), Color(167, 79, 50, 255),
             Color(91, 43, 33, 255), Color(116, 50, 35, 255),
             Color(72, 35, 30, 255), Color(171, 80, 50, 255)},
            {Color(177, 91, 46, 255), Color(208, 119, 63, 255),
             Color(111, 57, 38, 255), Color(145, 70, 42, 255),
             Color(91, 49, 36, 255), Color(211, 119, 61, 255)},
            {Color(111, 72, 52, 255), Color(143, 96, 67, 255),
             Color(75, 53, 42, 255), Color(92, 61, 45, 255),
             Color(61, 43, 36, 255), Color(148, 93, 60, 255)},
            {Color(187, 126, 80, 255), Color(218, 158, 102, 255),
             Color(117, 79, 58, 255), Color(153, 98, 65, 255),
             Color(96, 65, 51, 255), Color(220, 154, 96, 255)}
        }};

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            const Color lit = wildlife_render::FacetColor(a, b, c, color, true);
            vertices.emplace_back(a, lit);
            vertices.emplace_back(b, lit);
            vertices.emplace_back(c, lit);
        }

        void AddDoubleSidedTriangle(std::vector<VertexPositionColor>& vertices,
                                    const Vector3& a, const Vector3& b, const Vector3& c,
                                    const Color& color)
        {
            AddTriangle(vertices, a, b, c, color);
            AddTriangle(vertices, c, b, a, color);
        }

        void AddQuad(std::vector<VertexPositionColor>& vertices,
                     const Vector3& a, const Vector3& b,
                     const Vector3& c, const Vector3& d, const Color& color)
        {
            AddTriangle(vertices, a, b, c, color);
            AddTriangle(vertices, a, c, d, color);
        }

        void AppendCrossedSegment(std::vector<VertexPositionColor>& vertices,
                                  const Vector3& a, const Vector3& b,
                                  const float halfWidth, const Color& color)
        {
            const Vector3 delta = b - a;
            const float horizontalLength = std::hypot(delta.X, delta.Z);
            const Vector3 side = horizontalLength > 0.0001f
                ? Vector3(-delta.Z * halfWidth / horizontalLength, 0.0f,
                          delta.X * halfWidth / horizontalLength)
                : Vector3(halfWidth, 0.0f, 0.0f);
            const Vector3 vertical(0.0f, halfWidth * 0.62f, 0.0f);
            AddQuad(vertices, a + side, a - side, b - side, b + side, color);
            AddQuad(vertices, a + vertical, a - vertical,
                    b - vertical, b + vertical, color);
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const Vector3& right,
                                         const Vector3& up, const Vector3& forward,
                                         const float x, const float y, const float z,
                                         const float scale)
        {
            return centre + right * (x * scale) + up * (y * scale)
                + forward * (z * scale);
        }

        void AppendOctahedron(std::vector<VertexPositionColor>& vertices,
                              const Vector3& centre, const Vector3& right,
                              const Vector3& up, const Vector3& forward,
                              const float halfX, const float halfY, const float halfZ,
                              const Color& topColor, const Color& sideColor)
        {
            const Vector3 top = centre + up * halfY;
            const Vector3 bottom = centre - up * halfY;
            const Vector3 left = centre - right * halfX;
            const Vector3 rightPoint = centre + right * halfX;
            const Vector3 front = centre + forward * halfZ;
            const Vector3 back = centre - forward * halfZ;
            AddTriangle(vertices, top, front, rightPoint, topColor);
            AddTriangle(vertices, top, rightPoint, back, topColor);
            AddTriangle(vertices, top, back, left, topColor);
            AddTriangle(vertices, top, left, front, topColor);
            AddTriangle(vertices, bottom, rightPoint, front, sideColor);
            AddTriangle(vertices, bottom, back, rightPoint, sideColor);
            AddTriangle(vertices, bottom, left, back, sideColor);
            AddTriangle(vertices, bottom, front, left, sideColor);
        }

        void AppendNearCrab(std::vector<VertexPositionColor>& vertices,
                            const CrabState& crab)
        {
            const CrabPalette& palette = Palettes[crab.colorVariant % Palettes.size()];
            const Vector3 centre(crab.x, crab.y, crab.z);
            const Vector3 forward(std::cos(crab.bodyYawRadians), 0.0f,
                                  std::sin(crab.bodyYawRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const float scale = crab.scale;
            const Vector3 top = LocalPoint(
                centre, right, up, forward, 0.0f, 0.245f, -0.01f, scale);
            std::array<Vector3, 10> crown{};
            std::array<Vector3, 10> shoulder{};
            std::array<Vector3, 10> rim{};
            std::array<Vector3, 10> lower{};
            for (std::size_t index = 0; index < crown.size(); ++index) {
                const float angle = static_cast<float>(index) * 2.0f
                    * std::numbers::pi_v<float> / static_cast<float>(crown.size());
                const float cosine = std::cos(angle);
                const float sine = std::sin(angle);
                const float frontTaper = sine > 0.0f ? 0.94f : 1.0f;
                crown[index] = LocalPoint(centre, right, up, forward,
                    cosine * 0.105f, 0.232f, sine * 0.071f - 0.01f, scale);
                shoulder[index] = LocalPoint(centre, right, up, forward,
                    cosine * 0.255f, 0.192f,
                    sine * 0.168f * frontTaper - 0.005f, scale);
                rim[index] = LocalPoint(centre, right, up, forward,
                    cosine * 0.36f, 0.105f, sine * 0.245f * frontTaper, scale);
                lower[index] = LocalPoint(
                    centre, right, up, forward, cosine * 0.33f, 0.035f,
                    sine * 0.22f * frontTaper, scale);
            }
            for (std::size_t index = 0; index < crown.size(); ++index) {
                const std::size_t next = (index + 1U) % crown.size();
                const Color panel = index % 3U == 0U
                    ? palette.shellHighlight : palette.shellTop;
                AddTriangle(vertices, top, crown[index], crown[next], panel);
                AddQuad(vertices, crown[index], shoulder[index], shoulder[next], crown[next],
                        panel);
                AddQuad(vertices, shoulder[index], rim[index], rim[next], shoulder[next],
                        palette.shellTop);
                AddQuad(vertices, rim[index], lower[index], lower[next], rim[next],
                        palette.shellSide);
            }

            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                for (int leg = 0; leg < 4; ++leg) {
                    const float z = -0.17f + static_cast<float>(leg) * 0.115f;
                    const float stride = crab.legPhase * (leg % 2 == 0 ? 0.045f : -0.045f);
                    const float bend = (leg < 2 ? -0.045f : 0.045f)
                        + (leg % 2 == 0 ? 0.018f : -0.018f);
                    const Vector3 root = LocalPoint(
                        centre, right, up, forward, side * 0.25f, 0.08f, z, scale);
                    const Vector3 knee = LocalPoint(
                        centre, right, up, forward, side * 0.43f, 0.035f,
                        z + bend + stride, scale);
                    const Vector3 foot = LocalPoint(
                        centre, right, up, forward, side * 0.58f, -0.015f,
                        z + bend * 1.35f + stride * 1.55f, scale);
                    const Vector3 toe = LocalPoint(
                        centre, right, up, forward, side * 0.68f, -0.020f,
                        z + bend * 1.55f + stride * 1.72f
                            + (leg % 2 == 0 ? -0.018f : 0.018f), scale);
                    AppendCrossedSegment(vertices, root, knee, 0.018f * scale,
                                         palette.limb);
                    AppendCrossedSegment(vertices, knee, foot, 0.012f * scale,
                                         palette.joint);
                    AppendCrossedSegment(vertices, foot, toe, 0.007f * scale,
                                         palette.limb);
                }

                const float clawSpread = 0.025f + crab.clawPose * 0.035f;
                const Vector3 clawRoot = LocalPoint(
                    centre, right, up, forward, side * 0.24f, 0.11f, 0.16f, scale);
                const Vector3 clawJoint = LocalPoint(
                    centre, right, up, forward, side * 0.40f, 0.13f, 0.28f, scale);
                AppendCrossedSegment(vertices, clawRoot, clawJoint, 0.027f * scale,
                                     palette.limb);
                const Vector3 palm = LocalPoint(
                    centre, right, up, forward, side * 0.47f, 0.15f, 0.345f, scale);
                AppendOctahedron(vertices, palm, right, up, forward,
                                 0.075f * scale, 0.052f * scale, 0.082f * scale,
                                 palette.claw, palette.shellSide);
                const Vector3 outerTip = LocalPoint(
                    centre, right, up, forward, side * (0.46f + clawSpread),
                    0.165f, 0.485f, scale);
                const Vector3 innerTip = LocalPoint(
                    centre, right, up, forward, side * (0.44f - clawSpread * 0.45f),
                    0.125f, 0.49f, scale);
                const Vector3 outerBase = palm + right * (side * 0.035f * scale)
                    + forward * (0.035f * scale);
                const Vector3 innerBase = palm - right * (side * 0.025f * scale)
                    + forward * (0.035f * scale);
                AddDoubleSidedTriangle(vertices, outerBase, palm, outerTip, palette.claw);
                AddDoubleSidedTriangle(vertices, palm, innerBase, innerTip,
                                       palette.shellHighlight);
            }

            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const Vector3 eyeRoot = LocalPoint(
                    centre, right, up, forward, side * 0.12f, 0.19f, 0.19f, scale);
                const Vector3 eyeCentre = LocalPoint(
                    centre, right, up, forward, side * 0.12f,
                    0.255f, 0.225f, scale);
                AppendCrossedSegment(vertices, eyeRoot, eyeCentre,
                                     0.012f * scale, palette.shellSide);
                AppendOctahedron(vertices, eyeCentre, right, up, forward,
                                 0.024f * scale, 0.021f * scale, 0.022f * scale,
                                 palette.shellHighlight, palette.shellSide);
                const Vector3 pupilCentre = eyeCentre + forward * (0.014f * scale)
                    + up * (0.006f * scale);
                AppendOctahedron(vertices, pupilCentre, right, up, forward,
                                 0.012f * scale, 0.012f * scale, 0.011f * scale,
                                 Color(38, 30, 24, 255), Color(20, 18, 17, 255));
            }

            const Vector3 mouthCentre = LocalPoint(
                centre, right, up, forward, 0.0f, 0.072f, 0.236f, scale);
            const Vector3 mouthLeft = LocalPoint(
                centre, right, up, forward, -0.07f, 0.068f, 0.244f, scale);
            const Vector3 mouthRight = LocalPoint(
                centre, right, up, forward, 0.07f, 0.068f, 0.244f, scale);
            const Vector3 mouthLow = LocalPoint(
                centre, right, up, forward, 0.0f, 0.035f, 0.255f, scale);
            AddDoubleSidedTriangle(vertices, mouthLeft, mouthCentre, mouthLow,
                                   palette.joint);
            AddDoubleSidedTriangle(vertices, mouthCentre, mouthRight, mouthLow,
                                   palette.joint);

            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                AddDoubleSidedTriangle(
                    vertices,
                    LocalPoint(centre, right, up, forward, side * 0.30f, 0.11f,
                               -0.16f, scale),
                    LocalPoint(centre, right, up, forward, side * 0.41f, 0.085f,
                               -0.10f, scale),
                    LocalPoint(centre, right, up, forward, side * 0.34f, 0.10f,
                               -0.035f, scale),
                    palette.shellSide);
            }
        }

        void AppendFarCrab(std::vector<VertexPositionColor>& vertices,
                           const CrabState& crab)
        {
            const CrabPalette& palette = Palettes[crab.colorVariant % Palettes.size()];
            const Vector3 centre(crab.x, crab.y, crab.z);
            const Vector3 forward(std::cos(crab.bodyYawRadians), 0.0f,
                                  std::sin(crab.bodyYawRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const float scale = crab.scale;
            const Vector3 front = LocalPoint(
                centre, right, up, forward, 0.0f, 0.10f, 0.27f, scale);
            const Vector3 back = LocalPoint(
                centre, right, up, forward, 0.0f, 0.08f, -0.25f, scale);
            const Vector3 left = LocalPoint(
                centre, right, up, forward, -0.38f, 0.07f, 0.0f, scale);
            const Vector3 rightPoint = LocalPoint(
                centre, right, up, forward, 0.38f, 0.07f, 0.0f, scale);
            AddDoubleSidedTriangle(vertices, front, left, back, palette.shellTop);
            AddDoubleSidedTriangle(vertices, front, back, rightPoint, palette.shellSide);
        }
    }

    CrabRenderer::CrabRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Crab maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(58.0f);
        effect_->setFogEndProperty(105.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << CrabDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(CrabDiagnostic);
        std::cout << CrabDetailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(CrabDetailDiagnostic);
    }

    CrabRenderer::~CrabRenderer() = default;

    void CrabRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition,
                            const std::span<const CrabState> crabs)
    {
        stagingVertices_.clear();
        visibleCrabCount_ = 0;
        visibleNearCrabCount_ = 0;
        visibleFarCrabCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const CrabState& crab : crabs) {
            const Vector3 centre(crab.x, crab.y + 0.15f * crab.scale, crab.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, crab.scale * 0.75f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                const std::size_t firstVertex = stagingVertices_.size();
                AppendNearCrab(stagingVertices_, crab);
                if (stagingVertices_.size() - firstVertex != NearCrabVertexCount) {
                    throw std::logic_error("Near crab geometry violated its exact vertex budget");
                }
                ++visibleNearCrabCount_;
            } else {
                const std::size_t firstVertex = stagingVertices_.size();
                AppendFarCrab(stagingVertices_, crab);
                if (stagingVertices_.size() - firstVertex != FarCrabVertexCount) {
                    throw std::logic_error("Far crab geometry violated its exact vertex budget");
                }
                ++visibleFarCrabCount_;
            }
            ++visibleCrabCount_;
        }
        if (stagingVertices_.empty()) return;

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
