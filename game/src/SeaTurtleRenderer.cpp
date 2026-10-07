#include "IslandSimulator/SeaTurtleRenderer.hpp"

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
        constexpr float NearLodDistance = 74.0f;
        constexpr int MaximumVertexCount = static_cast<int>(SeaTurtleGroup::TurtleCount)
            * SeaTurtleRenderer::NearVertexCount;
        constexpr std::string_view TurtleDiagnostic =
            "island-simulator: sea turtles use faceted shells and articulated flippers in one culled batch";

        struct TurtlePalette
        {
            Color shell;
            Color scute;
            Color underside;
            Color skin;
        };

        struct PoseAxes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

        const std::array<TurtlePalette, 3> Palettes = {{
            {Color(75, 104, 69, 255), Color(113, 132, 75, 255),
             Color(171, 164, 105, 255), Color(91, 119, 82, 255)},
            {Color(73, 91, 65, 255), Color(132, 119, 72, 255),
             Color(183, 158, 101, 255), Color(103, 111, 75, 255)},
            {Color(57, 92, 81, 255), Color(102, 132, 91, 255),
             Color(162, 169, 121, 255), Color(72, 113, 97, 255)}
        }};

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

        void AddDoubleSidedTriangle(std::vector<VertexPositionColor>& vertices,
                                    const Vector3& a, const Vector3& b, const Vector3& c,
                                    const Color& color)
        {
            const Color lit = wildlife_render::FacetColor(a, b, c, color, true);
            AddTriangle(vertices, a, b, c, lit);
            AddTriangle(vertices, c, b, a, lit);
        }

        [[nodiscard]] PoseAxes MakePoseAxes(const SeaTurtleState& turtle)
        {
            const Vector3 levelForward(std::cos(turtle.headingRadians), 0.0f,
                                       std::sin(turtle.headingRadians));
            const Vector3 levelRight(-levelForward.Z, 0.0f, levelForward.X);
            const float pitchCosine = std::cos(turtle.pitchRadians);
            const float pitchSine = std::sin(turtle.pitchRadians);
            const Vector3 forward = levelForward * pitchCosine + Vector3::Up * pitchSine;
            const Vector3 pitchUp = Vector3::Up * pitchCosine - levelForward * pitchSine;
            const float bankCosine = std::cos(turtle.bankRadians);
            const float bankSine = std::sin(turtle.bankRadians);
            return PoseAxes{
                levelRight * bankCosine + pitchUp * bankSine,
                pitchUp * bankCosine - levelRight * bankSine,
                forward,
            };
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const PoseAxes& axes,
                                         const float x, const float y, const float z,
                                         const float scale)
        {
            return centre + axes.right * (x * scale) + axes.up * (y * scale)
                + axes.forward * (z * scale);
        }

        [[nodiscard]] TurtlePalette LitPalette(const SeaTurtleState& turtle,
                                               const float timeSeconds)
        {
            const TurtlePalette& source = Palettes[turtle.colorVariant % Palettes.size()];
            const AquaticLightProfile light = AquaticLightAt(
                turtle.x, turtle.y, turtle.z, timeSeconds);
            return TurtlePalette{
                wildlife_render::ApplyUnderwaterLight(source.shell, light),
                wildlife_render::ApplyUnderwaterLight(source.scute, light),
                wildlife_render::ApplyUnderwaterLight(source.underside, light),
                wildlife_render::ApplyUnderwaterLight(source.skin, light),
            };
        }

        void AppendNearTurtle(std::vector<VertexPositionColor>& vertices,
                              const SeaTurtleState& turtle, const float timeSeconds)
        {
            const Vector3 centre(turtle.x, turtle.y, turtle.z);
            const PoseAxes axes = MakePoseAxes(turtle);
            const TurtlePalette palette = LitPalette(turtle, timeSeconds);
            const float scale = turtle.scale;
            constexpr std::size_t SegmentCount = 10U;
            std::array<Vector3, SegmentCount> upper{};
            std::array<Vector3, SegmentCount> lower{};
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.20f, -0.03f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.105f, -0.03f, scale);
            for (std::size_t index = 0; index < SegmentCount; ++index) {
                const float angle = static_cast<float>(index) * 2.0f
                    * std::numbers::pi_v<float> / static_cast<float>(SegmentCount);
                const float x = std::cos(angle) * 0.39f;
                const float z = std::sin(angle) * 0.52f - 0.03f;
                upper[index] = LocalPoint(centre, axes, x, 0.055f, z, scale);
                lower[index] = LocalPoint(
                    centre, axes, x * 0.91f, -0.055f, z * 0.93f, scale);
            }
            for (std::size_t index = 0; index < SegmentCount; ++index) {
                const std::size_t next = (index + 1U) % SegmentCount;
                const Color topColor = index % 2U == 0U ? palette.shell : palette.scute;
                AddLitTriangle(vertices, top, upper[index], upper[next], topColor);
                AddLitTriangle(vertices, upper[index], lower[index], lower[next], palette.shell);
                AddLitTriangle(vertices, upper[index], lower[next], upper[next], palette.shell);
                AddLitTriangle(vertices, bottom, lower[next], lower[index], palette.underside);
            }

            constexpr std::size_t HeadSegments = 6U;
            std::array<Vector3, HeadSegments> headRing{};
            const Vector3 headNose = LocalPoint(
                centre, axes, 0.0f, 0.015f, 0.77f, scale);
            const Vector3 headNeck = LocalPoint(
                centre, axes, 0.0f, 0.01f, 0.46f, scale);
            for (std::size_t index = 0; index < HeadSegments; ++index) {
                const float angle = static_cast<float>(index) * 2.0f
                    * std::numbers::pi_v<float> / static_cast<float>(HeadSegments);
                headRing[index] = LocalPoint(
                    centre, axes, std::cos(angle) * 0.13f,
                    std::sin(angle) * 0.105f + 0.02f, 0.61f, scale);
            }
            for (std::size_t index = 0; index < HeadSegments; ++index) {
                const std::size_t next = (index + 1U) % HeadSegments;
                AddLitTriangle(vertices, headNose, headRing[index], headRing[next], palette.skin);
                AddLitTriangle(vertices, headNeck, headRing[next], headRing[index], palette.skin);
            }

            const float stroke = turtle.flipperStroke;
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const Vector3 frontRoot = LocalPoint(
                    centre, axes, side * 0.30f, 0.0f, 0.23f, scale);
                const Vector3 frontMid = LocalPoint(
                    centre, axes, side * 0.58f, -0.02f + stroke * 0.105f,
                    0.11f - std::fabs(stroke) * 0.04f, scale);
                const Vector3 frontTip = LocalPoint(
                    centre, axes, side * 0.78f, -0.06f + stroke * 0.16f,
                    -0.04f - std::fabs(stroke) * 0.08f, scale);
                const Vector3 frontRear = LocalPoint(
                    centre, axes, side * 0.31f, -0.035f, -0.06f, scale);
                AddDoubleSidedTriangle(
                    vertices, frontRoot, frontMid, frontTip, palette.skin);
                AddDoubleSidedTriangle(
                    vertices, frontRoot, frontTip, frontRear, palette.skin);

                const Vector3 rearRoot = LocalPoint(
                    centre, axes, side * 0.26f, -0.035f, -0.35f, scale);
                const Vector3 rearTip = LocalPoint(
                    centre, axes, side * 0.50f, -0.055f - stroke * 0.045f,
                    -0.54f, scale);
                const Vector3 rearBack = LocalPoint(
                    centre, axes, side * 0.18f, -0.055f, -0.50f, scale);
                AddDoubleSidedTriangle(
                    vertices, rearRoot, rearTip, rearBack, palette.skin);
            }

            AddDoubleSidedTriangle(
                vertices,
                LocalPoint(centre, axes, 0.0f, -0.04f, -0.47f, scale),
                LocalPoint(centre, axes, -0.045f, -0.05f, -0.63f, scale),
                LocalPoint(centre, axes, 0.045f, -0.05f, -0.63f, scale),
                palette.skin);

            const Color eye(19, 28, 24, 255);
            AddTriangle(
                vertices,
                LocalPoint(centre, axes, -0.105f, 0.073f, 0.67f, scale),
                LocalPoint(centre, axes, -0.118f, 0.038f, 0.70f, scale),
                LocalPoint(centre, axes, -0.119f, 0.041f, 0.65f, scale), eye);
            AddTriangle(
                vertices,
                LocalPoint(centre, axes, 0.105f, 0.073f, 0.67f, scale),
                LocalPoint(centre, axes, 0.119f, 0.041f, 0.65f, scale),
                LocalPoint(centre, axes, 0.118f, 0.038f, 0.70f, scale), eye);
        }

        void AppendFarTurtle(std::vector<VertexPositionColor>& vertices,
                             const SeaTurtleState& turtle, const float timeSeconds)
        {
            const Vector3 centre(turtle.x, turtle.y, turtle.z);
            const PoseAxes axes = MakePoseAxes(turtle);
            const TurtlePalette palette = LitPalette(turtle, timeSeconds);
            const float scale = turtle.scale;
            const Vector3 front = LocalPoint(centre, axes, 0.0f, 0.03f, 0.68f, scale);
            const Vector3 rear = LocalPoint(centre, axes, 0.0f, 0.0f, -0.55f, scale);
            const Vector3 left = LocalPoint(centre, axes, -0.42f, 0.05f, -0.02f, scale);
            const Vector3 right = LocalPoint(centre, axes, 0.42f, 0.05f, -0.02f, scale);
            AddDoubleSidedTriangle(vertices, front, left, rear, palette.shell);
            AddDoubleSidedTriangle(vertices, front, rear, right, palette.scute);
            const float stroke = turtle.flipperStroke;
            AddDoubleSidedTriangle(
                vertices, front, left,
                LocalPoint(centre, axes, -0.74f, stroke * 0.13f, 0.02f, scale),
                palette.skin);
            AddDoubleSidedTriangle(
                vertices, front,
                LocalPoint(centre, axes, 0.74f, stroke * 0.13f, 0.02f, scale),
                right, palette.skin);
        }
    }

    SeaTurtleRenderer::SeaTurtleRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument(
                "Sea-turtle maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.16f, 0.55f, 0.61f));
        effect_->setFogStartProperty(76.0f);
        effect_->setFogEndProperty(205.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << TurtleDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TurtleDiagnostic);
    }

    SeaTurtleRenderer::~SeaTurtleRenderer() = default;

    void SeaTurtleRenderer::Draw(const Matrix& view, const Matrix& projection,
                                 const Vector3& cameraPosition,
                                 const std::span<const SeaTurtleState> turtles,
                                 const float timeSeconds)
    {
        stagingVertices_.clear();
        visibleTurtleCount_ = 0;
        visibleNearTurtleCount_ = 0;
        visibleFarTurtleCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const SeaTurtleState& turtle : turtles) {
            const Vector3 centre(turtle.x, turtle.y, turtle.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, turtle.scale * 1.08f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearTurtle(stagingVertices_, turtle, timeSeconds);
                ++visibleNearTurtleCount_;
            } else {
                AppendFarTurtle(stagingVertices_, turtle, timeSeconds);
                ++visibleFarTurtleCount_;
            }
            ++visibleTurtleCount_;
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
