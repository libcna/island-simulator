#include "IslandSimulator/FishRenderer.hpp"

#include <array>
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
        constexpr float NearLodDistance = 72.0f;
        constexpr float MaximumDistance = 215.0f;
        constexpr int MaximumVerticesPerFish = 54;
        constexpr int MaximumVertexCount = static_cast<int>(FishSchools::FishCount)
            * MaximumVerticesPerFish;

        struct FishPalette
        {
            Color body;
            Color belly;
            Color fin;
        };

        const std::array<FishPalette, 4> Palettes = {{
            {Color(126, 166, 166, 255), Color(207, 214, 184, 255),
             Color(218, 177, 67, 255)},
            {Color(41, 126, 138, 255), Color(125, 191, 175, 255),
             Color(35, 83, 102, 255)},
            {Color(174, 133, 74, 255), Color(223, 194, 129, 255),
             Color(116, 83, 52, 255)},
            {Color(59, 82, 126, 255), Color(140, 168, 180, 255),
             Color(224, 183, 58, 255)}
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
                                         const Vector3& forward, const float localX,
                                         const float localY, const float localZ,
                                         const float scale)
        {
            return centre + right * (localX * scale) + Vector3::Up * (localY * scale)
                + forward * (localZ * scale);
        }

        void AppendNearFish(std::vector<VertexPositionColor>& vertices,
                            const FishState& fish)
        {
            const FishPalette& palette = Palettes[fish.colorVariant % Palettes.size()];
            const Vector3 centre(fish.x, fish.y, fish.z);
            const Vector3 forward(std::cos(fish.headingRadians), 0.0f,
                                  std::sin(fish.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const float scale = fish.scale;
            const float bend = fish.bodyWave * 0.075f;
            const Vector3 nose = LocalPoint(centre, right, forward, 0.0f, 0.0f, 0.49f, scale);
            const Vector3 top = LocalPoint(centre, right, forward, 0.0f, 0.19f, 0.03f, scale);
            const Vector3 bottom = LocalPoint(centre, right, forward, 0.0f, -0.15f, 0.03f, scale);
            const Vector3 left = LocalPoint(centre, right, forward, -0.17f, 0.0f, 0.03f, scale);
            const Vector3 rightSide = LocalPoint(centre, right, forward, 0.17f, 0.0f, 0.03f, scale);
            const Vector3 rear = LocalPoint(centre, right, forward, bend, 0.0f, -0.42f, scale);

            AddTriangle(vertices, nose, top, left, palette.body);
            AddTriangle(vertices, nose, rightSide, top, palette.body);
            AddTriangle(vertices, nose, bottom, rightSide, palette.belly);
            AddTriangle(vertices, nose, left, bottom, palette.belly);
            AddTriangle(vertices, rear, left, top, palette.body);
            AddTriangle(vertices, rear, top, rightSide, palette.body);
            AddTriangle(vertices, rear, rightSide, bottom, palette.belly);
            AddTriangle(vertices, rear, bottom, left, palette.belly);

            const float tailBend = fish.bodyWave * 0.17f;
            const Vector3 tailTop = LocalPoint(centre, right, forward, tailBend, 0.25f,
                                                -0.75f, scale);
            const Vector3 tailBottom = LocalPoint(centre, right, forward, tailBend, -0.25f,
                                                   -0.75f, scale);
            AddDoubleSidedTriangle(vertices, rear, tailTop, tailBottom, palette.fin);

            const Vector3 dorsalFront = LocalPoint(centre, right, forward, 0.0f, 0.13f,
                                                    0.12f, scale);
            const Vector3 dorsalTip = LocalPoint(centre, right, forward, 0.0f, 0.34f,
                                                  -0.08f, scale);
            const Vector3 dorsalBack = LocalPoint(centre, right, forward, bend * 0.45f, 0.12f,
                                                   -0.27f, scale);
            AddDoubleSidedTriangle(vertices, dorsalFront, dorsalTip, dorsalBack, palette.fin);

            const Vector3 finRoot = LocalPoint(centre, right, forward, 0.0f, -0.04f,
                                                0.08f, scale);
            const Vector3 leftFin = LocalPoint(centre, right, forward, -0.34f, -0.10f,
                                                -0.08f, scale);
            const Vector3 rightFin = LocalPoint(centre, right, forward, 0.34f, -0.10f,
                                                 -0.08f, scale);
            AddDoubleSidedTriangle(vertices, finRoot, leftFin, rear, palette.fin);
            AddDoubleSidedTriangle(vertices, finRoot, rear, rightFin, palette.fin);
        }

        void AppendFarFish(std::vector<VertexPositionColor>& vertices,
                           const FishState& fish)
        {
            const FishPalette& palette = Palettes[fish.colorVariant % Palettes.size()];
            const Vector3 centre(fish.x, fish.y, fish.z);
            const Vector3 forward(std::cos(fish.headingRadians), 0.0f,
                                  std::sin(fish.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const float scale = fish.scale;
            const float bend = fish.bodyWave * 0.12f;
            const Vector3 nose = LocalPoint(centre, right, forward, 0.0f, 0.0f, 0.46f, scale);
            const Vector3 top = LocalPoint(centre, right, forward, 0.0f, 0.17f, 0.0f, scale);
            const Vector3 bottom = LocalPoint(centre, right, forward, 0.0f, -0.14f, 0.0f, scale);
            const Vector3 rear = LocalPoint(centre, right, forward, bend, 0.0f, -0.43f, scale);
            const Vector3 tailTop = LocalPoint(centre, right, forward, bend * 1.5f, 0.21f,
                                                -0.70f, scale);
            const Vector3 tailBottom = LocalPoint(centre, right, forward, bend * 1.5f, -0.21f,
                                                   -0.70f, scale);
            AddDoubleSidedTriangle(vertices, nose, top, rear, palette.body);
            AddDoubleSidedTriangle(vertices, nose, rear, bottom, palette.belly);
            AddDoubleSidedTriangle(vertices, rear, tailTop, tailBottom, palette.fin);
        }
    }

    FishRenderer::FishRenderer(GraphicsDevice& device)
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
        effect_->setFogStartProperty(70.0f);
        effect_->setFogEndProperty(220.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
    }

    FishRenderer::~FishRenderer() = default;

    void FishRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition,
                            const std::span<const FishState> fish)
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
            if (distanceSquared > MaximumDistance * MaximumDistance
                || !frustum.Intersects(BoundingSphere(centre, state.scale * 0.85f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearFish(stagingVertices_, state);
                ++visibleNearFishCount_;
            } else {
                AppendFarFish(stagingVertices_, state);
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
