#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ButterflyRenderer.hpp"

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
        constexpr float NearLodDistance = 25.0f;
        constexpr int MaximumVerticesPerButterfly = 228;
        constexpr int MaximumVertexCount = static_cast<int>(ButterflyMeadow::ButterflyCount)
            * MaximumVerticesPerButterfly;
        constexpr std::string_view ButterflyDiagnostic =
            "island-simulator: clearing butterflies use hinged patterned wings in one culled batch";
        constexpr std::string_view ButterflyAnatomyDiagnostic =
            "island-simulator: near butterflies use closed bodies, paired antennae, rounded wing lobes, and dark margins";

        struct ButterflyPalette
        {
            Color forewing;
            Color hindwing;
            Color accent;
            Color body;
        };

        const std::array<ButterflyPalette, 5> Palettes = {{
            {Color(49, 111, 156, 255), Color(74, 153, 183, 255),
             Color(205, 225, 190, 255), Color(32, 37, 34, 255)},
            {Color(211, 110, 39, 255), Color(236, 159, 54, 255),
             Color(45, 39, 32, 255), Color(38, 32, 29, 255)},
            {Color(221, 202, 94, 255), Color(235, 220, 129, 255),
             Color(87, 69, 40, 255), Color(43, 39, 30, 255)},
            {Color(55, 143, 126, 255), Color(91, 181, 146, 255),
             Color(220, 219, 142, 255), Color(31, 42, 36, 255)},
            {Color(213, 217, 205, 255), Color(178, 194, 187, 255),
             Color(75, 93, 87, 255), Color(38, 42, 40, 255)}
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

        void AppendNearButterfly(std::vector<VertexPositionColor>& vertices,
                                 const ButterflyState& butterfly)
        {
            const ButterflyPalette& palette =
                Palettes[butterfly.colorVariant % Palettes.size()];
            const Vector3 centre(butterfly.x, butterfly.y, butterfly.z);
            const Vector3 forward(std::cos(butterfly.headingRadians), 0.0f,
                                  std::sin(butterfly.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const float scale = butterfly.scale;
            const float foldAngle = butterfly.wingFold * 1.22f;
            const float horizontal = std::cos(foldAngle);
            const float lift = std::sin(foldAngle);

            constexpr int BodySides=6;
            const Vector3 bodyFront=LocalPoint(
                centre,right,up,forward,0,.012f,.21f,scale);
            const Vector3 bodyRear=LocalPoint(
                centre,right,up,forward,0,-.012f,-.38f,scale);
            const auto bodyRing=[&](float angle,float z,float radius) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*radius,
                    std::sin(angle)*radius,z,scale);
            };
            for(int sideIndex=0;sideIndex<BodySides;++sideIndex) {
                const float a0=6.28318530718f*static_cast<float>(sideIndex)/BodySides;
                const float a1=6.28318530718f*static_cast<float>(sideIndex+1)/BodySides;
                const Vector3 front0=bodyRing(a0,.12f,.055f),front1=bodyRing(a1,.12f,.055f);
                const Vector3 rear0=bodyRing(a0,-.15f,.050f),rear1=bodyRing(a1,-.15f,.050f);
                AddTriangle(vertices,bodyFront,front0,front1,palette.body);
                AddTriangle(vertices,front0,rear0,rear1,palette.body);
                AddTriangle(vertices,front0,rear1,front1,palette.body);
                AddTriangle(vertices,bodyRear,rear1,rear0,palette.body);
            }
            const Vector3 headCentre=LocalPoint(
                centre,right,up,forward,0,.012f,.255f,scale);
            const Vector3 headTop=headCentre+up*(.065f*scale);
            const Vector3 headBottom=headCentre-up*(.065f*scale);
            const Vector3 headFront=headCentre+forward*(.070f*scale);
            const Vector3 headBack=headCentre-forward*(.060f*scale);
            const Vector3 headLeft=headCentre-right*(.062f*scale);
            const Vector3 headRight=headCentre+right*(.062f*scale);
            AddTriangle(vertices,headTop,headFront,headLeft,palette.body);
            AddTriangle(vertices,headTop,headRight,headFront,palette.body);
            AddTriangle(vertices,headTop,headBack,headRight,palette.body);
            AddTriangle(vertices,headTop,headLeft,headBack,palette.body);
            AddTriangle(vertices,headBottom,headLeft,headFront,palette.body);
            AddTriangle(vertices,headBottom,headFront,headRight,palette.body);
            AddTriangle(vertices,headBottom,headRight,headBack,palette.body);
            AddTriangle(vertices,headBottom,headBack,headLeft,palette.body);
            for(const float side:{-1.0f,1.0f}) {
                const Vector3 antennaRoot=LocalPoint(
                    centre,right,up,forward,side*.025f,.045f,.30f,scale);
                const Vector3 antennaTip=LocalPoint(
                    centre,right,up,forward,side*.17f,.085f,.52f,scale);
                const Vector3 antennaEdge=LocalPoint(
                    centre,right,up,forward,side*.155f,.073f,.505f,scale);
                AddDoubleSidedTriangle(vertices,antennaRoot,antennaTip,antennaEdge,
                    palette.body);
            }

            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const auto wingPoint=[&](float span,float z,float extraLift=0.0f) {
                    return LocalPoint(centre,right,up,forward,side*span*horizontal,
                        span*lift+extraLift,z,scale);
                };
                const Vector3 foreRoot = LocalPoint(
                    centre, right, up, forward, side * 0.035f, 0.0f, 0.12f, scale);
                const Vector3 hindRoot = LocalPoint(
                    centre, right, up, forward, side * 0.035f, 0.0f, -0.13f, scale);
                const Vector3 leadingTip=wingPoint(.40f,.31f);
                const Vector3 foreTip=wingPoint(.64f,.18f);
                const Vector3 outerTip=wingPoint(.68f,-.05f);
                const Vector3 hindTip=wingPoint(.52f,-.33f);
                const Vector3 rearTip=wingPoint(.28f,-.42f);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,leadingTip,foreTip,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,foreTip,outerTip,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,outerTip,hindRoot,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,hindRoot,outerTip,hindTip,palette.hindwing);
                AddDoubleSidedTriangle(
                    vertices,hindRoot,hindTip,rearTip,palette.hindwing);

                const Vector3 foreInner=wingPoint(.51f,.13f,.004f);
                const Vector3 outerInner=wingPoint(.55f,-.055f,.004f);
                const Vector3 hindInner=wingPoint(.41f,-.275f,.004f);
                AddDoubleSidedTriangle(
                    vertices,foreTip,outerTip,outerInner,palette.body);
                AddDoubleSidedTriangle(
                    vertices,foreTip,outerInner,foreInner,palette.body);
                AddDoubleSidedTriangle(
                    vertices,outerTip,hindTip,hindInner,palette.body);
                AddDoubleSidedTriangle(
                    vertices,outerTip,hindInner,outerInner,palette.body);
                const Vector3 accentTip=wingPoint(.43f,-.02f,.008f);
                AddDoubleSidedTriangle(vertices,foreRoot,accentTip,hindRoot,
                    palette.accent);
            }
        }

        void AppendFarButterfly(std::vector<VertexPositionColor>& vertices,
                                const ButterflyState& butterfly)
        {
            const ButterflyPalette& palette =
                Palettes[butterfly.colorVariant % Palettes.size()];
            const Vector3 centre(butterfly.x, butterfly.y, butterfly.z);
            const Vector3 forward(std::cos(butterfly.headingRadians), 0.0f,
                                  std::sin(butterfly.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const float scale = butterfly.scale;
            const float foldAngle = butterfly.wingFold * 1.12f;
            const float horizontal = std::cos(foldAngle);
            const float lift = std::sin(foldAngle);
            const Vector3 front = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, 0.22f, scale);
            const Vector3 back = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.25f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -0.57f * horizontal,
                0.57f * lift, -0.02f, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, 0.57f * horizontal,
                0.57f * lift, -0.02f, scale);
            AddDoubleSidedTriangle(vertices, front, leftTip, back, palette.forewing);
            AddDoubleSidedTriangle(vertices, front, back, rightTip, palette.hindwing);
        }
    }

    ButterflyRenderer::ButterflyRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument(
                "Butterfly maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(42.0f);
        effect_->setFogEndProperty(82.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << ButterflyDiagnostic << '\n';
        std::cout << ButterflyAnatomyDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyAnatomyDiagnostic);
    }

    ButterflyRenderer::~ButterflyRenderer() = default;

    void ButterflyRenderer::Draw(const Matrix& view, const Matrix& projection,
                                  const Vector3& cameraPosition,
                                  const std::span<const ButterflyState> butterflies)
    {
        stagingVertices_.clear();
        visibleButterflyCount_ = 0;
        visibleNearButterflyCount_ = 0;
        visibleFarButterflyCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const ButterflyState& butterfly : butterflies) {
            const Vector3 centre(butterfly.x, butterfly.y, butterfly.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, butterfly.scale * 0.82f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearButterfly(stagingVertices_, butterfly);
                ++visibleNearButterflyCount_;
            } else {
                AppendFarButterfly(stagingVertices_, butterfly);
                ++visibleFarButterflyCount_;
            }
            ++visibleButterflyCount_;
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
