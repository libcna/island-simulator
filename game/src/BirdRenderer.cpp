#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/BirdRenderer.hpp"

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
        constexpr int MaximumVerticesPerBird = 54;
        constexpr int MaximumVertexCount = static_cast<int>(BirdFlock::BirdCount)
            * MaximumVerticesPerBird;
        constexpr std::string_view NearBirdFeatherDiagnostic =
            "island-simulator: near birds use layered trailing flight-feather silhouettes";

        struct BirdPalette
        {
            Color body;
            Color upperWing;
            Color lowerWing;
            Color tail;
        };

        const std::array<BirdPalette, 3> Palettes = {{
            {Color(61, 70, 70, 255), Color(104, 113, 108, 255),
             Color(214, 218, 203, 255), Color(48, 57, 58, 255)},
            {Color(90, 72, 54, 255), Color(139, 117, 82, 255),
             Color(222, 204, 161, 255), Color(67, 56, 46, 255)},
            {Color(50, 62, 72, 255), Color(78, 99, 114, 255),
             Color(195, 211, 215, 255), Color(39, 50, 59, 255)}
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
                                         const float localX,
                                         const float localY, const float localZ,
                                         const float scale)
        {
            return centre + right * (localX * scale) + up * (localY * scale)
                + forward * (localZ * scale);
        }

        void AppendNearBird(std::vector<VertexPositionColor>& vertices,
                            const BirdState& bird)
        {
            const BirdPalette& palette = Palettes[bird.colorVariant % Palettes.size()];
            const Vector3 centre(bird.x, bird.y, bird.z);
            const Vector3 forward(std::cos(bird.headingRadians), 0.0f,
                                  std::sin(bird.headingRadians));
            const Vector3 levelRight(-forward.Z, 0.0f, forward.X);
            const float bankCosine = std::cos(bird.bankRadians);
            const float bankSine = std::sin(bird.bankRadians);
            const Vector3 right = levelRight * bankCosine + Vector3::Up * bankSine;
            const Vector3 up = Vector3::Up * bankCosine - levelRight * bankSine;
            const float scale = bird.scale;
            const Vector3 nose = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, 0.66f, scale);
            const Vector3 tail = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.52f, scale);
            const Vector3 top = LocalPoint(
                centre, right, up, forward, 0.0f, 0.14f, 0.02f, scale);
            const Vector3 bottom = LocalPoint(
                centre, right, up, forward, 0.0f, -0.105f, 0.01f, scale);
            const Vector3 left = LocalPoint(
                centre, right, up, forward, -0.14f, 0.0f, 0.01f, scale);
            const Vector3 rightSide = LocalPoint(
                centre, right, up, forward, 0.14f, 0.0f, 0.01f, scale);

            AddTriangle(vertices, nose, top, left, palette.body);
            AddTriangle(vertices, nose, rightSide, top, palette.body);
            AddTriangle(vertices, nose, bottom, rightSide, palette.lowerWing);
            AddTriangle(vertices, nose, left, bottom, palette.lowerWing);
            AddTriangle(vertices, tail, left, top, palette.body);
            AddTriangle(vertices, tail, top, rightSide, palette.body);
            AddTriangle(vertices, tail, rightSide, bottom, palette.lowerWing);
            AddTriangle(vertices, tail, bottom, left, palette.lowerWing);

            const float wingLift = bird.wingPose * 0.38f;
            const Vector3 leftFront = LocalPoint(
                centre, right, up, forward, -0.06f, 0.035f, 0.20f, scale);
            const Vector3 leftBack = LocalPoint(
                centre, right, up, forward, -0.08f, 0.015f, -0.24f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -1.04f, wingLift, -0.06f, scale);
            const Vector3 rightFront = LocalPoint(
                centre, right, up, forward, 0.06f, 0.035f, 0.20f, scale);
            const Vector3 rightBack = LocalPoint(
                centre, right, up, forward, 0.08f, 0.015f, -0.24f, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, 1.04f, wingLift, -0.06f, scale);
            AddDoubleSidedTriangle(vertices, leftFront, leftTip, leftBack, palette.upperWing);
            AddDoubleSidedTriangle(vertices, rightFront, rightBack, rightTip, palette.upperWing);
            const Vector3 leftFeather = LocalPoint(
                centre, right, up, forward, -0.54f, wingLift * 0.55f - 0.015f,
                -0.43f, scale);
            const Vector3 rightFeather = LocalPoint(
                centre, right, up, forward, 0.54f, wingLift * 0.55f - 0.015f,
                -0.43f, scale);
            AddDoubleSidedTriangle(
                vertices, leftBack, leftTip, leftFeather, palette.lowerWing);
            AddDoubleSidedTriangle(
                vertices, rightBack, rightFeather, rightTip, palette.lowerWing);

            const Vector3 tailRoot = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.40f, scale);
            const Vector3 tailLeft = LocalPoint(
                centre, right, up, forward, -0.34f, 0.0f, -0.83f, scale);
            const Vector3 tailRight = LocalPoint(
                centre, right, up, forward, 0.34f, 0.0f, -0.83f, scale);
            AddDoubleSidedTriangle(vertices, tailRoot, tailLeft, tailRight, palette.tail);
        }

        void AppendFarBird(std::vector<VertexPositionColor>& vertices,
                           const BirdState& bird)
        {
            const BirdPalette& palette = Palettes[bird.colorVariant % Palettes.size()];
            const Vector3 centre(bird.x, bird.y, bird.z);
            const Vector3 forward(std::cos(bird.headingRadians), 0.0f,
                                  std::sin(bird.headingRadians));
            const Vector3 levelRight(-forward.Z, 0.0f, forward.X);
            const float bankCosine = std::cos(bird.bankRadians);
            const float bankSine = std::sin(bird.bankRadians);
            const Vector3 right = levelRight * bankCosine + Vector3::Up * bankSine;
            const Vector3 up = Vector3::Up * bankCosine - levelRight * bankSine;
            const float scale = bird.scale;
            const float wingLift = bird.wingPose * 0.30f;
            const Vector3 nose = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, 0.60f, scale);
            const Vector3 tail = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.55f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -0.98f, wingLift, -0.04f, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, 0.98f, wingLift, -0.04f, scale);
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
        MirrorRuntimeDiagnosticToPlatformLog(NearBirdFeatherDiagnostic);
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
