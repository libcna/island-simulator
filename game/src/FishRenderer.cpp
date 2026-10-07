#include "IslandSimulator/FishRenderer.hpp"

#include "IslandSimulator/WildlifeRenderLighting.hpp"

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
        constexpr int MaximumVerticesPerFish = FishRenderer::NearVertexCount;
        constexpr int MaximumVertexCount = static_cast<int>(FishSchools::FishCount)
            * MaximumVerticesPerFish;

        struct FishPalette
        {
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

        void AddLitTriangle(std::vector<VertexPositionColor>& vertices,
                            const Vector3& a, const Vector3& b, const Vector3& c,
                            const Color& color, const bool twoSided = false)
        {
            AddTriangle(vertices, a, b, c,
                        wildlife_render::FacetColor(a, b, c, color, twoSided));
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
            const Vector3 centre(fish.x, fish.y, fish.z);
            const FishPalette& sourcePalette = Palettes[fish.colorVariant % Palettes.size()];
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const FishPalette palette{
                wildlife_render::ApplyUnderwaterLight(sourcePalette.body, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.belly, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.fin, waterLight),
            };
            const PoseAxes axes = MakePoseAxes(fish);
            const float scale = fish.scale;
            const float bend = fish.bodyWave * 0.075f;
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, 0.0f, 0.49f, scale);
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.19f, 0.03f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.15f, 0.03f, scale);
            const Vector3 left = LocalPoint(centre, axes, -0.17f, 0.0f, 0.03f, scale);
            const Vector3 rightSide = LocalPoint(centre, axes, 0.17f, 0.0f, 0.03f, scale);
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.42f, scale);

            AddLitTriangle(vertices, nose, top, left, palette.body);
            AddLitTriangle(vertices, nose, rightSide, top, palette.body);
            AddLitTriangle(vertices, nose, bottom, rightSide, palette.belly);
            AddLitTriangle(vertices, nose, left, bottom, palette.belly);
            AddLitTriangle(vertices, rear, left, top, palette.body);
            AddLitTriangle(vertices, rear, top, rightSide, palette.body);
            AddLitTriangle(vertices, rear, rightSide, bottom, palette.belly);
            AddLitTriangle(vertices, rear, bottom, left, palette.belly);

            const float tailBend = fish.bodyWave * 0.17f;
            const Vector3 tailTop = LocalPoint(
                centre, axes, tailBend, 0.25f, -0.78f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, tailBend * 1.08f, 0.0f, -0.66f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, tailBend, -0.25f, -0.78f, scale);
            AddTriangle(vertices, rear, tailTop, tailNotch, palette.fin);
            AddTriangle(vertices, rear, tailNotch, tailBottom, palette.fin);

            const Vector3 dorsalFront = LocalPoint(
                centre, axes, 0.0f, 0.13f, 0.12f, scale);
            const Vector3 dorsalTip = LocalPoint(
                centre, axes, 0.0f, 0.34f, -0.08f, scale);
            const Vector3 dorsalBack = LocalPoint(
                centre, axes, bend * 0.45f, 0.12f, -0.27f, scale);
            AddTriangle(vertices, dorsalFront, dorsalTip, dorsalBack, palette.fin);

            const Vector3 finRoot = LocalPoint(
                centre, axes, 0.0f, -0.04f, 0.08f, scale);
            const Vector3 leftFin = LocalPoint(
                centre, axes, -0.34f, -0.10f, -0.08f, scale);
            const Vector3 rightFin = LocalPoint(
                centre, axes, 0.34f, -0.10f, -0.08f, scale);
            AddTriangle(vertices, finRoot, leftFin, rear, palette.fin);
            AddTriangle(vertices, finRoot, rear, rightFin, palette.fin);

            const Color eye(19, 31, 33, 255);
            const Color gill(65, 86, 83, 255);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.108f, 0.062f, 0.255f, scale),
                        LocalPoint(centre, axes, -0.117f, 0.012f, 0.278f, scale),
                        LocalPoint(centre, axes, -0.121f, 0.014f, 0.214f, scale), eye);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.108f, 0.062f, 0.255f, scale),
                        LocalPoint(centre, axes, 0.121f, 0.014f, 0.214f, scale),
                        LocalPoint(centre, axes, 0.117f, 0.012f, 0.278f, scale), eye);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.145f, 0.055f, 0.135f, scale),
                        LocalPoint(centre, axes, -0.151f, -0.047f, 0.117f, scale),
                        LocalPoint(centre, axes, -0.147f, -0.035f, 0.145f, scale), gill);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.145f, 0.055f, 0.135f, scale),
                        LocalPoint(centre, axes, 0.147f, -0.035f, 0.145f, scale),
                        LocalPoint(centre, axes, 0.151f, -0.047f, 0.117f, scale), gill);
        }

        void AppendFarFish(std::vector<VertexPositionColor>& vertices,
                           const FishState& fish, const float timeSeconds)
        {
            const Vector3 centre(fish.x, fish.y, fish.z);
            const FishPalette& sourcePalette = Palettes[fish.colorVariant % Palettes.size()];
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const FishPalette palette{
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
            if (distanceSquared > MaximumDistance * MaximumDistance
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
