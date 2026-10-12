#include "IslandSimulator/ContactShadowRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <limits>
#include <stdexcept>

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ContactShadowPatch.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/SnakeContactShadow.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
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
                + MonkeyTroop::MonkeyCount + BoarHerd::BoarCount;
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
        const NaturalPropLayout& naturalProps, const ShipwreckLayout& shipwreck,
        const float maximumDistanceScale, const HutPlacement* hut)
        : device_(device),
          terrain_(terrain),
          effect_(std::make_unique<BasicEffect>(device)),
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
        staticShadows_.reserve(maximumShadowCount - MonkeyTroop::MonkeyCount - BoarHerd::BoarCount);
        staticVertices_.reserve((maximumShadowCount - MonkeyTroop::MonkeyCount - BoarHerd::BoarCount)
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
                                      firstVertex, VerticesPerShadow, kind});
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
        const auto cachePatch = [&](const ContactFootprint& footprint, const StaticShadowKind kind,
                                    const float distance, const ContactPatchDetail detail) {
            const std::size_t first = staticVertices_.size();
            for (const auto& vertex : BuildContactPatch(terrain_, footprint, detail)) {
                staticVertices_.emplace_back(Vector3(vertex.x, vertex.y, vertex.z),
                    Color(0, 0, 0, static_cast<int>(std::lround(vertex.opacity * 255))));
            }
            if (staticVertices_.size() == first) return;
            staticShadows_.push_back({{}, 0, distance * maximumDistanceScale, first,
                                      staticVertices_.size() - first, kind});
        };
        for (const auto& rock : naturalProps.Rocks()) {
            if (!rock.offshore) cachePatch(LandRockContactFootprint(rock), StaticShadowKind::Rock,
                                          PropMaximumDistance, ContactPatchDetail::Compact);
        }
        for (const auto& log : naturalProps.FallenLogs()) {
            cachePatch(LogContactFootprint(log), StaticShadowKind::Wood,
                       PropMaximumDistance, ContactPatchDetail::Compact);
        }
        for (const auto& wood : naturalProps.Driftwood()) {
            cachePatch(DriftwoodContactFootprint(wood), StaticShadowKind::Wood,
                       PropMaximumDistance, ContactPatchDetail::Compact);
        }
        const auto cacheCoastal = [&](const ContactFootprint& footprint) {
            cachePatch(footprint, StaticShadowKind::Coastal, 110.0f, ContactPatchDetail::Coastal);
        };
        for (const auto& rock : naturalProps.Rocks()) {
            if (rock.offshore) cacheCoastal(RockContactFootprint(rock));
        }
        for (const auto& footprint : HullContactFootprints(shipwreck.Ship())) cacheCoastal(footprint);
        for (const auto& debris : shipwreck.Debris()) cacheCoastal(DebrisContactFootprint(debris));
        if(hut) for(const auto& footprint:HutContactFootprints(*hut))
            cachePatch(footprint,StaticShadowKind::Hut,90.0f,ContactPatchDetail::Coastal);

        // Bounds cover the actual projected/slope-conforming patch, not the source object's pivot.
        for (auto& shadow : staticShadows_) {
            Vector3 minimum(std::numeric_limits<float>::max()), maximum(std::numeric_limits<float>::lowest());
            for (std::size_t index = shadow.firstVertex; index < shadow.firstVertex + shadow.vertexCount; ++index) {
                minimum = Vector3::Min(minimum, staticVertices_[index].Position);
                maximum = Vector3::Max(maximum, staticVertices_[index].Position);
            }
            shadow.centre = (minimum + maximum) * 0.5f;
            shadow.radius = Vector3::Distance(minimum, maximum) * 0.5f + 0.001f;
        }
        vertexCapacity_ = staticVertices_.size()
            + (MonkeyTroop::MonkeyCount + BoarHerd::BoarCount + GoatHabitat::Count) * VerticesPerShadow
            + SnakePopulation::Count*SnakeShadowVertexLimit;
        if (vertexCapacity_ > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error("Contact-shadow batch exceeds supported vertex capacity");
        }
        stagingVertices_.reserve(vertexCapacity_);
        vertices_ = std::make_unique<DynamicVertexBuffer>(device,
            VertexPositionColor::getVertexDeclarationStatic(), static_cast<int>(vertexCapacity_), BufferUsage::WriteOnly);
    }

    ContactShadowRenderer::~ContactShadowRenderer() = default;

    void ContactShadowRenderer::Draw(const Matrix& view, const Matrix& projection,
                                     const Vector3& cameraPosition,
                                     const std::span<const MonkeyState> monkeys,
                                     const std::span<const BoarState> boars, const SnakePopulation& population,
                                     const std::span<const GoatState> goats)
    {
        const auto snakes=population.Snakes();
        drawCallCount_ = 0;
        if (monkeys.size() > MonkeyTroop::MonkeyCount || boars.size() > BoarHerd::BoarCount
            || goats.size()>GoatHabitat::Count) {
            throw std::invalid_argument("Contact-shadow population exceeds the reserved GPU capacity");
        }
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
            || cachedMonkeyPositions_.size() != monkeys.size()
            || cachedBoarPoses_.size() != boars.size()
            || cachedGoatPoses_.size() != goats.size()
            || cachedSnakePoints_.size() != snakes.size()*SnakeState::StationCount;
        if (!rebuild) {
            for (std::size_t index = 0; index < monkeys.size(); ++index) {
                const auto centre=MonkeyVisualCentre(monkeys[index]);
                const Vector3 position(centre.x,centre.y,centre.z);
                if (Vector3::DistanceSquared(position, cachedMonkeyPositions_[index])
                    > MonkeyRefreshDistanceSquared) {
                    rebuild = true;
                    break;
                }
            }
        }

        if (!rebuild) {
            for (std::size_t index = 0; index < boars.size(); ++index) {
                const auto& boar = boars[index];
                const auto& cached = cachedBoarPoses_[index];
                const float dx = boar.x - cached.X, dz = boar.z - cached.Z;
                if (dx * dx + dz * dz > MonkeyRefreshDistanceSquared
                    || std::abs(boar.y - cached.Y) > 0.04f
                    || std::abs(std::remainder(boar.headingRadians - cached.W,
                                              2.0f * std::numbers::pi_v<float>)) > 0.06f) {
                    rebuild = true;
                    break;
                }
            }
        }

        if(!rebuild) {
            std::size_t i=0;
            for(const auto& snake:snakes) for(const auto& p:snake.body) {
                if(Vector3::DistanceSquared(Vector3(p.x,p.y,p.z),cachedSnakePoints_[i++])>0.000064f)
                    rebuild=true;
            }
        }
        if(!rebuild) {
            for(std::size_t i=0;i<goats.size();++i) {
                const auto& goat=goats[i];const auto& cached=cachedGoatPoses_[i];
                const float dx=goat.x-cached.X,dz=goat.z-cached.Z;
                if(dx*dx+dz*dz>MonkeyRefreshDistanceSquared||std::abs(goat.y-cached.Y)>.04f
                    ||std::abs(std::remainder(goat.heading-cached.W,2*std::numbers::pi_v<float>))>.06f)
                    rebuild=true;
            }
        }
        if (rebuild) {
            stagingVertices_.clear();
            visibleShadowCount_ = 0;
            visiblePalmShadowCount_ = 0;
            visibleCanopyShadowCount_ = 0;
            visibleRockShadowCount_ = 0;
            visibleWoodShadowCount_ = 0;
            visibleHutShadowCount_ = 0;
            visibleMonkeyShadowCount_ = 0;
            visibleBoarShadowCount_ = 0;
            visibleGoatShadowCount_ = 0;
            visibleSnakeShadowCount_ = 0;
            visibleCoastalShadowCount_ = 0;
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
                for (std::size_t vertexIndex = 0; vertexIndex < shadow.vertexCount; ++vertexIndex) {
                    const VertexPositionColor& source = staticVertices_[
                        shadow.firstVertex + vertexIndex];
                    stagingVertices_.emplace_back(source.Position,
                        Color(0, 0, 0, static_cast<int>(FadedAlpha(source.Color.getAProperty(), fade))));
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
                case StaticShadowKind::Hut:
                    ++visibleHutShadowCount_;
                    break;
                case StaticShadowKind::Coastal:
                    ++visibleCoastalShadowCount_;
                    break;
                }
            }

            for (const MonkeyState& monkey : monkeys) {
                const auto centre=MonkeyVisualCentre(monkey);
                float fade = 0.0f;
                if (!IsVisible(frustum, cameraPosition, centre.x, centre.y, centre.z,
                               0.85f * monkey.scale, monkeyMaximumDistance_, fade)) {
                    continue;
                }
                const float elevation=std::max(0.0f,centre.y-monkey.y-.72f*monkey.scale);
                fade*=std::clamp(1-elevation/3.0f,0.0f,1.0f);
                AppendTerrainEllipse(stagingVertices_, terrain_, centre.x, centre.z,
                                     shadowDirectionX, shadowDirectionZ,
                                     0.62f * monkey.scale, 0.38f * monkey.scale,
                                     0.12f * monkey.scale, FadedAlpha(48, fade));
                ++visibleMonkeyShadowCount_;
            }

            for (const auto& boar : boars) {
                float fade = 0.0f;
                if (!IsVisible(frustum, cameraPosition, boar.x, boar.y, boar.z,
                               1.3f * boar.scale, monkeyMaximumDistance_, fade)) continue;
                AppendTerrainEllipse(stagingVertices_, terrain_, boar.x, boar.z,
                                     std::cos(boar.headingRadians), std::sin(boar.headingRadians),
                                     1.05f * boar.scale, 0.48f * boar.scale,
                                     0.10f * boar.scale, FadedAlpha(90, fade));
                ++visibleBoarShadowCount_;
            }

            for(const auto& goat:goats) {
                float fade=0;
                if(!IsVisible(frustum,cameraPosition,goat.x,goat.y,goat.z,1.8f*goat.scale,
                    monkeyMaximumDistance_,fade))continue;
                AppendTerrainEllipse(stagingVertices_,terrain_,goat.x,goat.z,
                    std::cos(goat.heading),std::sin(goat.heading),
                    .70f*goat.scale,.30f*goat.scale,.08f*goat.scale,FadedAlpha(70,fade));
                ++visibleGoatShadowCount_;
            }
            for(const auto& snake:snakes) {
                const auto head=snake.body.front();
                float fade=0;
                if(!IsVisible(frustum,cameraPosition,head.x,head.y,head.z,2.5f,
                    monkeyMaximumDistance_*(60.0f/105.0f),fade)) continue;
                for(const auto& v:BuildSnakeContactShadow(snake,population.Surface()))
                    stagingVertices_.emplace_back(Vector3(v.x,v.y,v.z),
                        Color(0,0,0,static_cast<int>(std::lround(v.opacity*fade*255.0f))));
                ++visibleSnakeShadowCount_;
            }
            visibleShadowCount_ = visiblePalmShadowCount_ + visibleCanopyShadowCount_
                + visibleRockShadowCount_ + visibleWoodShadowCount_ + visibleHutShadowCount_
                + visibleMonkeyShadowCount_ + visibleBoarShadowCount_ + visibleCoastalShadowCount_
                + visibleSnakeShadowCount_ + visibleGoatShadowCount_;
            primitiveCount_ = static_cast<int>(stagingVertices_.size() / 3U);
            if (stagingVertices_.size() > vertexCapacity_) {
                throw std::length_error("Contact-shadow upload exceeds reserved vertex capacity");
            }
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
                const auto centre=MonkeyVisualCentre(monkey);
                cachedMonkeyPositions_.emplace_back(centre.x,centre.y,centre.z);
            }
            cachedBoarPoses_.clear();
            for (const auto& boar : boars) {
                cachedBoarPoses_.emplace_back(boar.x, boar.y, boar.z, boar.headingRadians);
            }
            cachedSnakePoints_.clear();
            cachedGoatPoses_.clear();
            for(const auto& goat:goats)cachedGoatPoses_.emplace_back(goat.x,goat.y,goat.z,goat.heading);
            for(const auto& snake:snakes) for(const auto& p:snake.body)
                cachedSnakePoints_.emplace_back(p.x,p.y,p.z);
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
