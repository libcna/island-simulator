#include "IslandSimulator/SharkRenderer.hpp"

#include <algorithm>
#include <cmath>

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
        constexpr float MaximumDistance = 710.0f;
        constexpr int MaximumVertexCount = 66;

        struct PoseAxes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

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
                             const SharkState& shark)
        {
            const Color body = ActivityColor(Color(72, 91, 99, 255), shark.activity);
            const Color belly = ActivityColor(Color(147, 159, 156, 255), shark.activity);
            const Color fin = ActivityColor(Color(58, 76, 84, 255), shark.activity);
            const Vector3 centre(shark.x, shark.y, shark.z);
            const PoseAxes axes = MakePoseAxes(shark);
            const float scale = shark.scale;
            const float bend = shark.bodyWave * 0.075f;
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, 0.0f, 0.72f, scale);
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.18f, 0.10f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.15f, 0.10f, scale);
            const Vector3 left = LocalPoint(centre, axes, -0.18f, 0.0f, 0.10f, scale);
            const Vector3 rightSide = LocalPoint(centre, axes, 0.18f, 0.0f, 0.10f, scale);
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.56f, scale);
            AddTriangle(vertices, nose, top, left, body);
            AddTriangle(vertices, nose, rightSide, top, body);
            AddTriangle(vertices, nose, bottom, rightSide, belly);
            AddTriangle(vertices, nose, left, bottom, belly);
            AddTriangle(vertices, rear, left, top, body);
            AddTriangle(vertices, rear, top, rightSide, body);
            AddTriangle(vertices, rear, rightSide, bottom, belly);
            AddTriangle(vertices, rear, bottom, left, belly);

            const float tailBend = shark.bodyWave * 0.17f;
            const Vector3 tailTop = LocalPoint(
                centre, axes, tailBend, 0.34f, -0.94f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, tailBend * 1.06f, 0.0f, -0.78f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, tailBend, -0.34f, -0.94f, scale);
            AddDoubleSidedTriangle(vertices, rear, tailTop, tailNotch, fin);
            AddDoubleSidedTriangle(vertices, rear, tailNotch, tailBottom, fin);

            const Vector3 dorsalFront = LocalPoint(
                centre, axes, 0.0f, 0.14f, 0.23f, scale);
            const Vector3 dorsalTip = LocalPoint(
                centre, axes, 0.0f, 0.57f, -0.08f, scale);
            const Vector3 dorsalBack = LocalPoint(
                centre, axes, bend * 0.35f, 0.12f, -0.34f, scale);
            AddDoubleSidedTriangle(vertices, dorsalFront, dorsalTip, dorsalBack, fin);

            const Vector3 finRoot = LocalPoint(
                centre, axes, 0.0f, -0.02f, 0.14f, scale);
            const Vector3 leftFin = LocalPoint(
                centre, axes, -0.52f, -0.12f, -0.08f, scale);
            const Vector3 rightFin = LocalPoint(
                centre, axes, 0.52f, -0.12f, -0.08f, scale);
            AddDoubleSidedTriangle(vertices, finRoot, leftFin, rear, fin);
            AddDoubleSidedTriangle(vertices, finRoot, rear, rightFin, fin);

            const Color eye = ActivityColor(Color(15, 24, 27, 255), shark.activity);
            const Color gill = ActivityColor(Color(43, 59, 64, 255), shark.activity);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.128f, 0.064f, 0.458f, scale),
                        LocalPoint(centre, axes, -0.143f, 0.007f, 0.486f, scale),
                        LocalPoint(centre, axes, -0.148f, 0.010f, 0.414f, scale), eye);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.128f, 0.064f, 0.458f, scale),
                        LocalPoint(centre, axes, 0.148f, 0.010f, 0.414f, scale),
                        LocalPoint(centre, axes, 0.143f, 0.007f, 0.486f, scale), eye);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.183f, 0.082f, 0.205f, scale),
                        LocalPoint(centre, axes, -0.190f, -0.078f, 0.178f, scale),
                        LocalPoint(centre, axes, -0.186f, -0.058f, 0.218f, scale), gill);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.183f, 0.082f, 0.205f, scale),
                        LocalPoint(centre, axes, 0.186f, -0.058f, 0.218f, scale),
                        LocalPoint(centre, axes, 0.190f, -0.078f, 0.178f, scale), gill);
        }

        void AppendFarShark(std::vector<VertexPositionColor>& vertices,
                            const SharkState& shark)
        {
            const Color body = ActivityColor(Color(67, 88, 97, 255), shark.activity);
            const Color belly = ActivityColor(Color(139, 154, 153, 255), shark.activity);
            const Color fin = ActivityColor(Color(54, 72, 81, 255), shark.activity);
            const Vector3 centre(shark.x, shark.y, shark.z);
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
            AddDoubleSidedTriangle(vertices, nose, top, rear, body);
            AddDoubleSidedTriangle(vertices, nose, rear, bottom, belly);
            AddDoubleSidedTriangle(vertices, rear, tailTop, tailNotch, fin);
            AddDoubleSidedTriangle(vertices, rear, tailNotch, tailBottom, fin);
            AddDoubleSidedTriangle(vertices, top, dorsal, rear, fin);
        }
    }

    SharkRenderer::SharkRenderer(GraphicsDevice& device)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly))
    {
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
                             const Vector3& cameraPosition, const SharkState& shark)
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
        if (distanceSquared > MaximumDistance * MaximumDistance
            || !frustum.Intersects(BoundingSphere(centre, shark.scale * 1.15f))) {
            return;
        }
        nearLod_ = distanceSquared <= NearLodDistance * NearLodDistance;
        if (nearLod_) {
            AppendNearShark(stagingVertices_, shark);
        } else {
            AppendFarShark(stagingVertices_, shark);
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
