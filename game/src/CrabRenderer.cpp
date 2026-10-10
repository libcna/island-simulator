#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/CrabRenderer.hpp"

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
        constexpr float NearLodDistance = 38.0f;
        constexpr int MaximumVerticesPerCrab = 192;
        constexpr int MaximumVertexCount = static_cast<int>(CrabColony::CrabCount)
            * MaximumVerticesPerCrab;
        constexpr std::string_view CrabDiagnostic =
            "island-simulator: beach crabs use articulated legs, claws, and one culled batch";

        struct CrabPalette
        {
            Color shellTop;
            Color shellSide;
            Color limb;
            Color claw;
        };

        const std::array<CrabPalette, 4> Palettes = {{
            {Color(139, 61, 39, 255), Color(91, 43, 33, 255),
             Color(116, 50, 35, 255), Color(164, 75, 47, 255)},
            {Color(177, 91, 46, 255), Color(111, 57, 38, 255),
             Color(145, 70, 42, 255), Color(202, 111, 58, 255)},
            {Color(111, 72, 52, 255), Color(75, 53, 42, 255),
             Color(92, 61, 45, 255), Color(142, 88, 57, 255)},
            {Color(187, 126, 80, 255), Color(117, 79, 58, 255),
             Color(153, 98, 65, 255), Color(213, 149, 93, 255)}
        }};

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

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const Vector3& right,
                                         const Vector3& up, const Vector3& forward,
                                         const float x, const float y, const float z,
                                         const float scale)
        {
            return centre + right * (x * scale) + up * (y * scale)
                + forward * (z * scale);
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
                centre, right, up, forward, 0.0f, 0.23f, 0.0f, scale);
            std::array<Vector3, 8> upper{};
            std::array<Vector3, 8> lower{};
            for (std::size_t index = 0; index < upper.size(); ++index) {
                const float angle = static_cast<float>(index) * 2.0f
                    * std::numbers::pi_v<float> / static_cast<float>(upper.size());
                const float x = std::cos(angle) * 0.34f;
                const float z = std::sin(angle) * 0.24f;
                upper[index] = LocalPoint(
                    centre, right, up, forward, x, 0.12f, z, scale);
                lower[index] = LocalPoint(
                    centre, right, up, forward, x * 0.90f, 0.025f, z * 0.90f, scale);
            }
            for (std::size_t index = 0; index < upper.size(); ++index) {
                const std::size_t next = (index + 1U) % upper.size();
                AddTriangle(vertices, top, upper[index], upper[next], palette.shellTop);
                AddTriangle(vertices, upper[index], lower[index], lower[next], palette.shellSide);
                AddTriangle(vertices, upper[index], lower[next], upper[next], palette.shellSide);
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
                    AddDoubleSidedTriangle(vertices, root, knee, foot, palette.limb);
                }

                const float clawSpread = 0.035f + crab.clawPose * 0.035f;
                const Vector3 clawRoot = LocalPoint(
                    centre, right, up, forward, side * 0.25f, 0.11f, 0.18f, scale);
                const Vector3 clawJoint = LocalPoint(
                    centre, right, up, forward, side * 0.40f, 0.13f, 0.29f, scale);
                const Vector3 outerTip = LocalPoint(
                    centre, right, up, forward, side * (0.46f + clawSpread),
                    0.16f, 0.39f, scale);
                const Vector3 innerTip = LocalPoint(
                    centre, right, up, forward, side * (0.39f - clawSpread * 0.20f),
                    0.12f, 0.42f, scale);
                AddDoubleSidedTriangle(vertices, clawRoot, clawJoint, outerTip, palette.claw);
                AddDoubleSidedTriangle(vertices, clawJoint, innerTip, outerTip, palette.claw);
            }

            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const Vector3 eyeRoot = LocalPoint(
                    centre, right, up, forward, side * 0.12f, 0.18f, 0.19f, scale);
                const Vector3 stalkLeft = LocalPoint(
                    centre, right, up, forward, side * 0.12f - 0.014f,
                    0.275f, 0.22f, scale);
                const Vector3 stalkRight = LocalPoint(
                    centre, right, up, forward, side * 0.12f + 0.014f,
                    0.275f, 0.22f, scale);
                const Vector3 eyeTop = LocalPoint(
                    centre, right, up, forward, side * 0.12f,
                    0.305f, 0.225f, scale);
                AddDoubleSidedTriangle(
                    vertices, eyeRoot, stalkLeft, stalkRight, palette.shellSide);
                AddDoubleSidedTriangle(
                    vertices, stalkLeft, eyeTop, stalkRight, Color(31, 25, 22, 255));
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
                AppendNearCrab(stagingVertices_, crab);
                ++visibleNearCrabCount_;
            } else {
                AppendFarCrab(stagingVertices_, crab);
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
