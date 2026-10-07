#include "IslandSimulator/ContactShadowRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <stdexcept>

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
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
        constexpr int EllipseSegments = 12;
        constexpr int VerticesPerShadow = EllipseSegments * 3;
        constexpr float SurfaceOffset = 0.035f;
        constexpr float PalmMaximumDistance = 92.0f;
        constexpr float CanopyMaximumDistance = 88.0f;
        constexpr float PropMaximumDistance = 78.0f;
        constexpr float MonkeyMaximumDistance = 105.0f;
        constexpr float CameraRefreshDistanceSquared = 0.25f * 0.25f;
        constexpr float MonkeyRefreshDistanceSquared = 0.08f * 0.08f;
        constexpr float ViewRefreshThreshold = 0.008f;
        constexpr float ProjectionRefreshThreshold = 0.001f;

        [[nodiscard]] std::size_t MaximumShadowCount(
            const VegetationLayout& vegetation, const JungleLayout& jungle,
            const NaturalPropLayout& naturalProps) noexcept
        {
            const auto landRockCount = static_cast<std::size_t>(std::count_if(
                naturalProps.Rocks().begin(), naturalProps.Rocks().end(),
                [](const RockPlacement& rock) { return !rock.offshore; }));
            return vegetation.Palms().size() + jungle.Canopy().size() + landRockCount
                + naturalProps.FallenLogs().size() + naturalProps.Driftwood().size()
                + MonkeyTroop::MonkeyCount;
        }

        [[nodiscard]] float DistanceFade(const float distanceSquared,
                                         const float maximumDistance) noexcept
        {
            const float fadeStart = maximumDistance * 0.78f;
            if (distanceSquared <= fadeStart * fadeStart) {
                return 1.0f;
            }
            const float distance = std::sqrt(distanceSquared);
            return std::clamp((maximumDistance - distance)
                                  / (maximumDistance - fadeStart),
                              0.0f, 1.0f);
        }

        [[nodiscard]] std::uint8_t FadedAlpha(const int alpha, const float fade) noexcept
        {
            return static_cast<std::uint8_t>(
                std::clamp(static_cast<int>(std::lround(static_cast<float>(alpha) * fade)),
                           0, 255));
        }

        void AppendTerrainEllipse(std::vector<VertexPositionColor>& vertices,
                                  const IslandTerrain& terrain,
                                  const float x, const float z,
                                  const float forwardX, const float forwardZ,
                                  const float forwardRadius, const float sideRadius,
                                  const float forwardOffset, const std::uint8_t centreAlpha)
        {
            const float centreX = x + forwardX * forwardOffset;
            const float centreZ = z + forwardZ * forwardOffset;
            const float sideX = -forwardZ;
            const float sideZ = forwardX;
            const Vector3 centre(centreX, terrain.Height(centreX, centreZ) + SurfaceOffset,
                                 centreZ);
            const Color centreColor(0, 0, 0, static_cast<int>(centreAlpha));
            const Color edgeColor(0, 0, 0, 0);

            std::array<Vector3, EllipseSegments> edgePoints;
            for (int segment = 0; segment < EllipseSegments; ++segment) {
                const float angle = static_cast<float>(segment)
                    * 2.0f * std::numbers::pi_v<float>
                    / static_cast<float>(EllipseSegments);
                const float along = std::cos(angle) * forwardRadius;
                const float across = std::sin(angle) * sideRadius;
                const float edgeX = centreX + forwardX * along + sideX * across;
                const float edgeZ = centreZ + forwardZ * along + sideZ * across;
                edgePoints[static_cast<std::size_t>(segment)] = Vector3(
                    edgeX, terrain.Height(edgeX, edgeZ) + SurfaceOffset, edgeZ);
            }
            for (int segment = 0; segment < EllipseSegments; ++segment) {
                const int nextSegment = (segment + 1) % EllipseSegments;
                vertices.emplace_back(centre, centreColor);
                vertices.emplace_back(edgePoints[static_cast<std::size_t>(segment)], edgeColor);
                vertices.emplace_back(edgePoints[static_cast<std::size_t>(nextSegment)], edgeColor);
            }
        }

        [[nodiscard]] bool IsVisible(const BoundingFrustum& frustum,
                                     const Vector3& cameraPosition,
                                     const float x, const float y, const float z,
                                     const float radius, const float maximumDistance,
                                     float& fade) noexcept
        {
            const Vector3 centre(x, y, z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance * maximumDistance
                || !frustum.Intersects(BoundingSphere(centre, radius))) {
                return false;
            }
            fade = DistanceFade(distanceSquared, maximumDistance);
            return fade > 0.0f;
        }
    }

    ContactShadowRenderer::ContactShadowRenderer(
        GraphicsDevice& device, const IslandTerrain& terrain,
        const VegetationLayout& vegetation, const JungleLayout& jungle,
        const NaturalPropLayout& naturalProps, const float maximumDistanceScale)
        : device_(device),
          terrain_(terrain),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(),
              static_cast<int>(MaximumShadowCount(vegetation, jungle, naturalProps)
                               * static_cast<std::size_t>(VerticesPerShadow)),
              BufferUsage::WriteOnly)),
          monkeyMaximumDistance_(MonkeyMaximumDistance * maximumDistanceScale)
    {
        if (!std::isfinite(maximumDistanceScale) || maximumDistanceScale <= 0.0f
            || maximumDistanceScale > 1.0f) {
            throw std::invalid_argument(
                "Contact-shadow distance scale must be finite and in (0, 1]");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(false);
        const std::size_t maximumShadowCount = MaximumShadowCount(
            vegetation, jungle, naturalProps);
        staticShadows_.reserve(maximumShadowCount - MonkeyTroop::MonkeyCount);
        staticVertices_.reserve((maximumShadowCount - MonkeyTroop::MonkeyCount)
                                * static_cast<std::size_t>(VerticesPerShadow));
        stagingVertices_.reserve(maximumShadowCount
                                 * static_cast<std::size_t>(VerticesPerShadow));

        const auto cacheShadow = [this](const StaticShadowKind kind,
                                        const float x, const float y, const float z,
                                        const float radius, const float maximumDistance,
                                        const float forwardX, const float forwardZ,
                                        const float forwardRadius, const float sideRadius,
                                        const float forwardOffset,
                                        const std::uint8_t centreAlpha) {
            const std::size_t firstVertex = staticVertices_.size();
            AppendTerrainEllipse(staticVertices_, terrain_, x, z, forwardX, forwardZ,
                                 forwardRadius, sideRadius, forwardOffset, centreAlpha);
            staticShadows_.push_back({Vector3(x, y, z), radius, maximumDistance,
                                      firstVertex, centreAlpha, kind});
        };

        const float horizontalSunLength = std::hypot(environment::SunDirectionX,
                                                      environment::SunDirectionZ);
        const float shadowDirectionX = -environment::SunDirectionX / horizontalSunLength;
        const float shadowDirectionZ = -environment::SunDirectionZ / horizontalSunLength;

        for (const PalmPlacement& palm : vegetation.Palms()) {
            cacheShadow(StaticShadowKind::Palm, palm.x, palm.y, palm.z,
                        1.2f * palm.scale, PalmMaximumDistance * maximumDistanceScale,
                        shadowDirectionX, shadowDirectionZ,
                        0.95f * palm.scale, 0.48f * palm.scale,
                        0.20f * palm.scale, 44U);
        }
        for (const CanopyPlacement& tree : jungle.Canopy()) {
            cacheShadow(StaticShadowKind::Canopy, tree.x, tree.y, tree.z,
                        1.5f * tree.scale, CanopyMaximumDistance * maximumDistanceScale,
                        shadowDirectionX, shadowDirectionZ,
                        1.18f * tree.scale, 0.72f * tree.scale,
                        0.24f * tree.scale, 40U);
        }
        for (const RockPlacement& rock : naturalProps.Rocks()) {
            if (rock.offshore) {
                continue;
            }
            const float footprint = std::max(rock.scaleX, rock.scaleZ);
            cacheShadow(StaticShadowKind::Rock, rock.x, rock.y, rock.z,
                        footprint, PropMaximumDistance * maximumDistanceScale,
                        std::cos(rock.yawRadians), std::sin(rock.yawRadians),
                        0.78f * rock.scaleX, 0.68f * rock.scaleZ,
                        0.08f * footprint, 52U);
        }
        for (const FallenLogPlacement& log : naturalProps.FallenLogs()) {
            cacheShadow(StaticShadowKind::Wood, log.x, log.y, log.z,
                        log.length * 0.55f, PropMaximumDistance * maximumDistanceScale,
                        std::cos(log.yawRadians), std::sin(log.yawRadians),
                        log.length * 0.52f, log.radius * 1.45f, 0.0f, 45U);
        }
        for (const DriftwoodPlacement& wood : naturalProps.Driftwood()) {
            cacheShadow(StaticShadowKind::Wood, wood.x, wood.y, wood.z,
                        wood.length * 0.55f, PropMaximumDistance * maximumDistanceScale,
                        std::cos(wood.yawRadians), std::sin(wood.yawRadians),
                        wood.length * 0.52f, wood.radius * 1.70f, 0.0f, 30U);
        }
    }

    ContactShadowRenderer::~ContactShadowRenderer() = default;

    void ContactShadowRenderer::Draw(const Matrix& view, const Matrix& projection,
                                     const Vector3& cameraPosition,
                                     const std::span<const MonkeyState> monkeys)
    {
        drawCallCount_ = 0;
        // Contact shadows are diffuse enough to tolerate sub-decimetre refresh thresholds. This
        // avoids repeating culling and a dynamic-buffer orphan for nearly identical frames.
        bool rebuild = !cacheValid_
            || Vector3::DistanceSquared(cameraPosition, cachedCameraPosition_)
                > CameraRefreshDistanceSquared
            || std::abs(view.M11 - cachedView_.M11) > ViewRefreshThreshold
            || std::abs(view.M12 - cachedView_.M12) > ViewRefreshThreshold
            || std::abs(view.M13 - cachedView_.M13) > ViewRefreshThreshold
            || std::abs(view.M21 - cachedView_.M21) > ViewRefreshThreshold
            || std::abs(view.M22 - cachedView_.M22) > ViewRefreshThreshold
            || std::abs(view.M23 - cachedView_.M23) > ViewRefreshThreshold
            || std::abs(view.M31 - cachedView_.M31) > ViewRefreshThreshold
            || std::abs(view.M32 - cachedView_.M32) > ViewRefreshThreshold
            || std::abs(view.M33 - cachedView_.M33) > ViewRefreshThreshold
            || std::abs(projection.M11 - cachedProjection_.M11)
                > ProjectionRefreshThreshold
            || std::abs(projection.M22 - cachedProjection_.M22)
                > ProjectionRefreshThreshold
            || cachedMonkeyPositions_.size() != monkeys.size();
        if (!rebuild) {
            for (std::size_t index = 0; index < monkeys.size(); ++index) {
                const Vector3 position(monkeys[index].x, monkeys[index].y, monkeys[index].z);
                if (Vector3::DistanceSquared(position, cachedMonkeyPositions_[index])
                    > MonkeyRefreshDistanceSquared) {
                    rebuild = true;
                    break;
                }
            }
        }

        if (rebuild) {
            stagingVertices_.clear();
            visibleShadowCount_ = 0;
            visiblePalmShadowCount_ = 0;
            visibleCanopyShadowCount_ = 0;
            visibleRockShadowCount_ = 0;
            visibleWoodShadowCount_ = 0;
            visibleMonkeyShadowCount_ = 0;
            primitiveCount_ = 0;
            const BoundingFrustum frustum(view * projection);

            const float horizontalSunLength = std::hypot(environment::SunDirectionX,
                                                          environment::SunDirectionZ);
            const float shadowDirectionX = -environment::SunDirectionX / horizontalSunLength;
            const float shadowDirectionZ = -environment::SunDirectionZ / horizontalSunLength;

            for (const StaticShadow& shadow : staticShadows_) {
                float fade = 0.0f;
                if (!IsVisible(frustum, cameraPosition,
                               shadow.centre.X, shadow.centre.Y, shadow.centre.Z,
                               shadow.radius, shadow.maximumDistance, fade)) {
                    continue;
                }
                const std::uint8_t alpha = FadedAlpha(shadow.centreAlpha, fade);
                for (int vertexIndex = 0; vertexIndex < VerticesPerShadow; ++vertexIndex) {
                    const VertexPositionColor& source = staticVertices_[
                        shadow.firstVertex + static_cast<std::size_t>(vertexIndex)];
                    if ((vertexIndex % 3) == 0) {
                        stagingVertices_.emplace_back(
                            source.Position, Color(0, 0, 0, static_cast<int>(alpha)));
                    } else {
                        stagingVertices_.push_back(source);
                    }
                }
                switch (shadow.kind) {
                case StaticShadowKind::Palm:
                    ++visiblePalmShadowCount_;
                    break;
                case StaticShadowKind::Canopy:
                    ++visibleCanopyShadowCount_;
                    break;
                case StaticShadowKind::Rock:
                    ++visibleRockShadowCount_;
                    break;
                case StaticShadowKind::Wood:
                    ++visibleWoodShadowCount_;
                    break;
                }
            }

            for (const MonkeyState& monkey : monkeys) {
                float fade = 0.0f;
                if (!IsVisible(frustum, cameraPosition, monkey.x, monkey.y, monkey.z,
                               0.85f * monkey.scale, monkeyMaximumDistance_, fade)) {
                    continue;
                }
                AppendTerrainEllipse(stagingVertices_, terrain_, monkey.x, monkey.z,
                                     shadowDirectionX, shadowDirectionZ,
                                     0.62f * monkey.scale, 0.38f * monkey.scale,
                                     0.12f * monkey.scale, FadedAlpha(48, fade));
                ++visibleMonkeyShadowCount_;
            }

            visibleShadowCount_ = visiblePalmShadowCount_ + visibleCanopyShadowCount_
                + visibleRockShadowCount_ + visibleWoodShadowCount_
                + visibleMonkeyShadowCount_;
            primitiveCount_ = static_cast<int>(stagingVertices_.size() / 3U);
            if (!stagingVertices_.empty()) {
                vertices_->SetData(stagingVertices_.data(), 0,
                                   static_cast<int>(stagingVertices_.size()),
                                   SetDataOptions::Discard);
            }
            cachedView_ = view;
            cachedProjection_ = projection;
            cachedCameraPosition_ = cameraPosition;
            cachedMonkeyPositions_.clear();
            cachedMonkeyPositions_.reserve(monkeys.size());
            for (const MonkeyState& monkey : monkeys) {
                cachedMonkeyPositions_.emplace_back(monkey.x, monkey.y, monkey.z);
            }
            cacheValid_ = true;
        }

        if (stagingVertices_.empty()) {
            return;
        }

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::AlphaBlend);
        device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
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
