#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/DistantSceneryRenderer.hpp"

#include "IslandSimulator/DistantSceneryLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numbers>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int RockSegments = 14;
        constexpr std::array<float, 4> RingRadius = {1.0f, 0.94f, 0.70f, 0.39f};
        constexpr std::array<float, 4> RingHeight = {-0.18f, 0.03f, 0.38f, 0.74f};

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& first, const Vector3& second,
                         const Vector3& third, const Color& color)
        {
            vertices.emplace_back(first, color);
            vertices.emplace_back(second, color);
            vertices.emplace_back(third, color);
        }

        [[nodiscard]] Vector3 IsletPoint(const DistantIsletPlacement& islet,
                                         const int ring, const int segment)
        {
            const float angle = 2.0f * std::numbers::pi_v<float>
                * static_cast<float>(segment) / static_cast<float>(RockSegments);
            const float irregularity = 1.0f + 0.075f * std::sin(
                angle * static_cast<float>(3U + islet.variant) + islet.yawRadians * 4.0f)
                + 0.035f * std::sin(angle * 7.0f + static_cast<float>(islet.variant));
            const float localX = std::cos(angle) * islet.radiusX
                * RingRadius[static_cast<std::size_t>(ring)] * irregularity;
            const float localZ = std::sin(angle) * islet.radiusZ
                * RingRadius[static_cast<std::size_t>(ring)] * irregularity;
            const float cosine = std::cos(islet.yawRadians);
            const float sine = std::sin(islet.yawRadians);
            const float ridgeVariation = ring >= 2
                ? 0.94f + 0.06f * std::sin(
                    angle * 2.0f + static_cast<float>(islet.variant) * 1.7f)
                : 1.0f;
            return Vector3(islet.x + localX * cosine - localZ * sine,
                           RingHeight[static_cast<std::size_t>(ring)] * islet.height
                               * ridgeVariation,
                           islet.z + localX * sine + localZ * cosine);
        }

        // Aerial perspective baked per islet: far silhouettes lose contrast toward the shared
        // haze before the renderer's distance fog, so the horizon reads as depth, not a row.
        [[nodiscard]] float HazeBlend(const DistantIsletPlacement& islet)
        {
            const float distance = std::hypot(islet.x, islet.z);
            const float t = std::clamp((distance - 480.0f) / (1100.0f - 480.0f), 0.0f, 1.0f);
            return 0.22f + 0.62f * t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] Color Hazed(const Color& color, const DistantIsletPlacement& islet)
        {
            const float blend = HazeBlend(islet);
            const auto channel = [blend](const std::uint8_t value, const float fog) {
                return static_cast<int>(std::lround(static_cast<float>(value)
                    + (fog * 255.0f - static_cast<float>(value)) * blend));
            };
            return Color(channel(color.getRProperty(), environment::FogColorRed),
                         channel(color.getGProperty(), environment::FogColorGreen),
                         channel(color.getBProperty(), environment::FogColorBlue), 255);
        }

        [[nodiscard]] Color RockColor(const DistantIsletPlacement& islet,
                                      const int ring, const int segment)
        {
            static const std::array<std::array<Color, 4>, 4> palettes = {{
                {Color(69, 88, 82), Color(83, 101, 87), Color(100, 113, 91),
                 Color(111, 124, 94)},
                {Color(75, 85, 86), Color(91, 102, 96), Color(107, 116, 101),
                 Color(118, 128, 105)},
                {Color(67, 82, 78), Color(79, 96, 83), Color(93, 108, 87),
                 Color(104, 119, 90)},
                // Treeless sandbar: wet dark sand low down, dry pale sand on top.
                {Color(146, 136, 108), Color(176, 164, 128), Color(206, 192, 150),
                 Color(224, 210, 166)},
            }};
            const Color base = palettes[islet.variant % palettes.size()]
                                       [static_cast<std::size_t>(ring)];
            const float facet = 0.90f + 0.10f * std::max(
                0.0f, std::cos(static_cast<float>(segment) * 0.78f - 0.65f));
            return Color(static_cast<int>(std::lround(
                             static_cast<float>(base.getRProperty()) * facet)),
                         static_cast<int>(std::lround(
                             static_cast<float>(base.getGProperty()) * facet)),
                         static_cast<int>(std::lround(
                             static_cast<float>(base.getBProperty()) * facet)), 255);
        }

        void AppendRock(std::vector<VertexPositionColor>& vertices,
                        const DistantIsletPlacement& islet)
        {
            for (int ring = 0; ring < static_cast<int>(RingRadius.size()) - 1; ++ring) {
                for (int segment = 0; segment < RockSegments; ++segment) {
                    const int next = (segment + 1) % RockSegments;
                    const Vector3 lower = IsletPoint(islet, ring, segment);
                    const Vector3 lowerNext = IsletPoint(islet, ring, next);
                    const Vector3 upper = IsletPoint(islet, ring + 1, segment);
                    const Vector3 upperNext = IsletPoint(islet, ring + 1, next);
                    const Color color = Hazed(RockColor(islet, ring, segment), islet);
                    AddTriangle(vertices, lower, upper, lowerNext, color);
                    AddTriangle(vertices, lowerNext, upper, upperNext, color);
                }
            }

            const Vector3 apex(islet.x + std::sin(islet.yawRadians * 3.0f + 0.8f)
                                               * islet.radiusX * 0.18f,
                               islet.height,
                               islet.z + std::cos(islet.yawRadians * 2.0f + 0.3f)
                                               * islet.radiusZ * 0.12f);
            for (int segment = 0; segment < RockSegments; ++segment) {
                const int next = (segment + 1) % RockSegments;
                AddTriangle(vertices, IsletPoint(islet, 3, segment), apex,
                            IsletPoint(islet, 3, next), Hazed(RockColor(islet, 3, segment), islet));
            }
        }

        void AppendPalm(std::vector<VertexPositionColor>& vertices,
                        const DistantIsletPlacement& islet, const int palmIndex)
        {
            const float phase = islet.yawRadians + static_cast<float>(palmIndex) * 2.31f
                + static_cast<float>(islet.variant) * 0.73f;
            const float offsetScale = palmIndex == 0 ? 0.16f : 0.27f;
            const Vector3 base(islet.x + std::cos(phase) * islet.radiusX * offsetScale,
                               islet.height * (0.78f - offsetScale * 0.22f),
                               islet.z + std::sin(phase) * islet.radiusZ * offsetScale);
            const float trunkHeight = 7.4f + static_cast<float>(
                (palmIndex + static_cast<int>(islet.variant)) % 3) * 1.35f;
            const float leanX = std::cos(phase + 0.8f) * 0.62f;
            const float leanZ = std::sin(phase + 0.8f) * 0.62f;
            const Vector3 crown = base + Vector3(leanX, trunkHeight, leanZ);
            const float halfWidth = 0.35f;
            const Color trunkColor = Hazed(Color(92, 79, 54, 255), islet);
            AddTriangle(vertices, base + Vector3(-halfWidth, 0.0f, 0.0f),
                        crown + Vector3(-halfWidth * 0.45f, 0.0f, 0.0f),
                        base + Vector3(halfWidth, 0.0f, 0.0f), trunkColor);
            AddTriangle(vertices, base + Vector3(halfWidth, 0.0f, 0.0f),
                        crown + Vector3(-halfWidth * 0.45f, 0.0f, 0.0f),
                        crown + Vector3(halfWidth * 0.45f, 0.0f, 0.0f), trunkColor);
            AddTriangle(vertices, base + Vector3(0.0f, 0.0f, -halfWidth),
                        crown + Vector3(0.0f, 0.0f, -halfWidth * 0.45f),
                        base + Vector3(0.0f, 0.0f, halfWidth), trunkColor);
            AddTriangle(vertices, base + Vector3(0.0f, 0.0f, halfWidth),
                        crown + Vector3(0.0f, 0.0f, -halfWidth * 0.45f),
                        crown + Vector3(0.0f, 0.0f, halfWidth * 0.45f), trunkColor);

            const Color frondColor = Hazed(islet.variant == 1U
                ? Color(58, 105, 69, 255) : Color(48, 112, 67, 255), islet);
            constexpr int crownSegments = 8;
            const Vector3 crownTop = crown + Vector3::Up * 1.05f;
            const Vector3 crownBottom = crown - Vector3::Up * 0.85f;
            for (int segment = 0; segment < crownSegments; ++segment) {
                const float angle = phase + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(segment) / static_cast<float>(crownSegments);
                const float nextAngle = phase + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(segment + 1) / static_cast<float>(crownSegments);
                const Vector3 edge = crown + Vector3(std::cos(angle) * 2.05f, 0.0f,
                                                      std::sin(angle) * 2.05f);
                const Vector3 nextEdge = crown + Vector3(std::cos(nextAngle) * 2.05f, 0.0f,
                                                          std::sin(nextAngle) * 2.05f);
                AddTriangle(vertices, crownTop, edge, nextEdge, frondColor);
                AddTriangle(vertices, crownBottom, nextEdge, edge, frondColor);
            }
            constexpr int frondCount = 7;
            for (int frond = 0; frond < frondCount; ++frond) {
                const float angle = phase + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(frond) / static_cast<float>(frondCount);
                const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
                const Vector3 side(-direction.Z, 0.0f, direction.X);
                const float length = 4.8f + 0.60f * static_cast<float>((frond + palmIndex) % 3);
                const Vector3 root = crown + Vector3::Up * 0.25f;
                const Vector3 tip = crown + direction * length
                    - Vector3::Up * (0.82f + 0.16f * static_cast<float>(frond % 2));
                AddTriangle(vertices, root - side * 0.75f, tip,
                            root + side * 0.75f, frondColor);
            }
        }
    }

    DistantSceneryRenderer::DistantSceneryRenderer(GraphicsDevice& device,
                                                   const DistantSceneryLayout& layout)
        : device_(device), effect_(std::make_unique<BasicEffect>(device))
    {
        std::vector<VertexPositionColor> vertices;
        vertices.reserve(layout.Islets().size() * 512U);
        for (const DistantIsletPlacement& islet : layout.Islets()) {
            AppendRock(vertices, islet);
            const int palmCount = islet.variant == DistantSceneryLayout::SandbarVariant
                ? 0 : 1 + static_cast<int>(islet.variant % 3U);
            for (int palm = 0; palm < palmCount; ++palm) {
                AppendPalm(vertices, islet, palm);
            }
        }

        isletCount_ = static_cast<int>(layout.Islets().size());
        vertexCount_ = static_cast<int>(vertices.size());
        primitiveCount_ = vertexCount_ / 3;
        vertices_ = std::make_unique<VertexBuffer>(
            device_, VertexPositionColor::getVertexDeclarationStatic(), vertexCount_,
            BufferUsage::WriteOnly);
        vertices_->SetData(vertices.data(), vertexCount_);

        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(260.0f);
        effect_->setFogEndProperty(1300.0f); // Far silhouettes stay faintly visible.
        std::cout << "island-simulator: distant scenery " << isletCount_
                  << " unreachable low islets, one portable batch, "
                  << primitiveCount_ << " triangles\n";
    }

    DistantSceneryRenderer::~DistantSceneryRenderer() = default;

    void DistantSceneryRenderer::Draw(const Matrix& view, const Matrix& projection)
    {
        drawCallCount_ = 0;
        if (primitiveCount_ <= 0) return;

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0, primitiveCount_);
        drawCallCount_ = 1;
    }
}
