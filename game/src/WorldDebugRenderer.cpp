#include "IslandSimulator/WorldDebugRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int BoundarySegments = 192;
        constexpr int ObstacleSegments = 16;
        constexpr int PlayerSegments = 24;
        constexpr int AudioMarkerSegments = 16;
        constexpr int WildlifeTargetSegments = 16;
        constexpr int MonkeyHomeRangeSegments = 32;
        constexpr int CellEdgeSegments = 6;
        constexpr float ObstacleVisibleDistance = 52.0f;
        constexpr float CellVisibleDistance = 135.0f;
        constexpr float BirdMarkerDistance = 190.0f;
        constexpr float FishMarkerDistance = 105.0f;
        constexpr float MonkeyMarkerDistance = 115.0f;
        constexpr float OceanLineHeight = 0.72f;

        void AddLine(std::vector<VertexPositionColor>& vertices,
                     const Vector3& first, const Vector3& second, const Color& color)
        {
            vertices.emplace_back(first, color);
            vertices.emplace_back(second, color);
        }

        void AddCircle(std::vector<VertexPositionColor>& vertices, const float x, const float y,
                       const float z, const float radius, const int segments, const Color& color)
        {
            for (int index = 0; index < segments; ++index) {
                const float angleA = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index) / static_cast<float>(segments);
                const float angleB = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index + 1) / static_cast<float>(segments);
                AddLine(vertices,
                        Vector3(x + std::cos(angleA) * radius, y,
                                z + std::sin(angleA) * radius),
                        Vector3(x + std::cos(angleB) * radius, y,
                                z + std::sin(angleB) * radius), color);
            }
        }

        void AddTerrainCircle(std::vector<VertexPositionColor>& vertices,
                              const IslandTerrain& terrain, const float x, const float z,
                              const float radius, const int segments, const Color& color)
        {
            for (int index = 0; index < segments; ++index) {
                const float angleA = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index) / static_cast<float>(segments);
                const float angleB = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index + 1) / static_cast<float>(segments);
                const float xA = x + std::cos(angleA) * radius;
                const float zA = z + std::sin(angleA) * radius;
                const float xB = x + std::cos(angleB) * radius;
                const float zB = z + std::sin(angleB) * radius;
                AddLine(vertices,
                        Vector3(xA, terrain.Height(xA, zA) + 0.42f, zA),
                        Vector3(xB, terrain.Height(xB, zB) + 0.42f, zB), color);
            }
        }

        void AddTargetMarker(std::vector<VertexPositionColor>& vertices,
                             const Vector3& position, const float radius, const Color& color)
        {
            AddCircle(vertices, position.X, position.Y, position.Z, radius,
                      WildlifeTargetSegments, color);
            AddLine(vertices, position - Vector3::Right * radius,
                    position + Vector3::Right * radius, color);
            AddLine(vertices, position - Vector3::Forward * radius,
                    position + Vector3::Forward * radius, color);
            AddLine(vertices, position, position + Vector3::Up * radius, color);
        }

        void AddBoundary(std::vector<VertexPositionColor>& vertices,
                         const IslandTerrain& terrain, const float offshoreDistance,
                         const float height, const Color& color)
        {
            for (int index = 0; index < BoundarySegments; ++index) {
                const float angleA = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index) / static_cast<float>(BoundarySegments);
                const float angleB = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index + 1) / static_cast<float>(BoundarySegments);
                const float radiusA = terrain.ShoreRadius(angleA) + offshoreDistance;
                const float radiusB = terrain.ShoreRadius(angleB) + offshoreDistance;
                AddLine(vertices,
                        Vector3(std::cos(angleA) * radiusA, height,
                                std::sin(angleA) * radiusA),
                        Vector3(std::cos(angleB) * radiusB, height,
                                std::sin(angleB) * radiusB), color);
            }
        }

        void AddTerrainBoundary(std::vector<VertexPositionColor>& vertices,
                                const IslandTerrain& terrain, const float inlandDistance,
                                const Color& color)
        {
            for (int index = 0; index < BoundarySegments; ++index) {
                const float angleA = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index) / static_cast<float>(BoundarySegments);
                const float angleB = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(index + 1) / static_cast<float>(BoundarySegments);
                const float radiusA = terrain.ShoreRadius(angleA) - inlandDistance;
                const float radiusB = terrain.ShoreRadius(angleB) - inlandDistance;
                const float xA = std::cos(angleA) * radiusA;
                const float zA = std::sin(angleA) * radiusA;
                const float xB = std::cos(angleB) * radiusB;
                const float zB = std::sin(angleB) * radiusB;
                AddLine(vertices,
                        Vector3(xA, terrain.Height(xA, zA) + 0.38f, zA),
                        Vector3(xB, terrain.Height(xB, zB) + 0.38f, zB), color);
            }
        }

        void AddAudioSourceMarker(std::vector<VertexPositionColor>& vertices,
                                  const WildlifeAudioSource& source,
                                  const float radius, const Color& color)
        {
            if (!source.available) {
                return;
            }
            const Vector3 position(source.x, source.y, source.z);
            AddCircle(vertices, source.x, source.y, source.z, radius,
                      AudioMarkerSegments, color);
            AddLine(vertices, position - Vector3::Up * radius,
                    position + Vector3::Up * radius, color);
            AddLine(vertices, position - Vector3::Right * radius,
                    position + Vector3::Right * radius, color);
            AddLine(vertices, position - Vector3::Forward * radius,
                    position + Vector3::Forward * radius, color);
        }

        [[nodiscard]] Color TierColor(const WildlifeSimulationTier tier,
                                      const Color& fullColor)
        {
            if (tier == WildlifeSimulationTier::Full) {
                return fullColor;
            }
            if (tier == WildlifeSimulationTier::Reduced) {
                return Color(250, 220, 82, 255);
            }
            return Color(132, 140, 146, 255);
        }

        void AddHeadingMarker(std::vector<VertexPositionColor>& vertices,
                              const Vector3& position, const float headingRadians,
                              const float length, const Color& color)
        {
            const Vector3 forward(std::cos(headingRadians), 0.0f,
                                  std::sin(headingRadians));
            const Vector3 side(-forward.Z, 0.0f, forward.X);
            const Vector3 end = position + forward * length;
            AddLine(vertices, position, end, color);
            AddLine(vertices, end, end - forward * (length * 0.28f)
                                      + side * (length * 0.16f), color);
            AddLine(vertices, end, end - forward * (length * 0.28f)
                                      - side * (length * 0.16f), color);
            AddLine(vertices, position - Vector3::Up * 0.35f,
                    position + Vector3::Up * 0.35f, color);
        }

        [[nodiscard]] bool IsNear(const Vector3& cameraPosition, const float x, const float y,
                                  const float z, const float maximumDistance)
        {
            const float offsetX = x - cameraPosition.X;
            const float offsetY = y - cameraPosition.Y;
            const float offsetZ = z - cameraPosition.Z;
            return offsetX * offsetX + offsetY * offsetY + offsetZ * offsetZ
                <= maximumDistance * maximumDistance;
        }
    }

    WorldDebugRenderer::WorldDebugRenderer(GraphicsDevice& device, const IslandTerrain& terrain)
        : device_(device), terrain_(terrain), effect_(std::make_unique<BasicEffect>(device))
    {
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setTextureEnabledProperty(false);
        effect_->setFogEnabledProperty(false);
    }

    WorldDebugRenderer::~WorldDebugRenderer() = default;

    void WorldDebugRenderer::Draw(const Matrix& view, const Matrix& projection,
                                  const Vector3& cameraPosition,
                                  const PlayerPosition& playerPosition,
                                  const float playerGroundHeight,
                                  const bool playerSwimming,
                                  const std::span<const CircularObstacle> obstacles,
                                  const std::span<const BirdState> birds,
                                  const std::span<const FishState> fish,
                                  const std::span<const FishSchoolState> fishSchools,
                                  const SharkState& shark,
                                  const std::span<const MonkeyState> monkeys,
                                  const SoundscapeDebugState& soundscape)
    {
        std::vector<VertexPositionColor> surfaceVertices;
        std::vector<VertexPositionColor> overlayVertices;
        surfaceVertices.reserve(static_cast<std::size_t>(BoundarySegments * 10 * 2));
        overlayVertices.reserve(static_cast<std::size_t>(ObstacleSegments * 40 * 2
                                + PlayerSegments * 2 + 2048));

        AddBoundary(surfaceVertices, terrain_, 0.0f, OceanLineHeight,
                    Color(65, 239, 245, 255));
        AddBoundary(surfaceVertices, terrain_, PlayerMotion::SoftCurrentStart,
                    OceanLineHeight + 0.03f,
                    Color(252, 229, 80, 255));
        AddBoundary(surfaceVertices, terrain_, PlayerMotion::FullCurrentDistance,
                    OceanLineHeight + 0.06f,
                    Color(255, 145, 45, 255));
        AddBoundary(surfaceVertices, terrain_, PlayerMotion::SafetyDistance,
                    OceanLineHeight + 0.09f,
                    Color(255, 62, 105, 255));

        AddTerrainBoundary(surfaceVertices, terrain_,
                           SoundscapeModel::BreezeFadeStartDistance,
                           Color(205, 255, 112, 255));
        AddTerrainBoundary(surfaceVertices, terrain_,
                           SoundscapeModel::SurfFadeStartDistance,
                           Color(85, 225, 255, 255));
        AddTerrainBoundary(surfaceVertices, terrain_,
                           SoundscapeModel::JungleFadeStartDistance,
                           Color(112, 255, 150, 255));
        AddTerrainBoundary(surfaceVertices, terrain_,
                           SoundscapeModel::BreezeFadeEndDistance,
                           Color(130, 190, 70, 255));
        AddTerrainBoundary(surfaceVertices, terrain_,
                           SoundscapeModel::JungleFadeEndDistance,
                           Color(35, 190, 85, 255));
        AddTerrainBoundary(surfaceVertices, terrain_,
                           SoundscapeModel::SurfFadeEndDistance,
                           Color(50, 125, 235, 255));
        audioZoneCount_ = 6;

        visibleCellCount_ = 0;
        for (std::size_t cellIndex = 0; cellIndex < TerrainMeshLayout::CellCount; ++cellIndex) {
            const TerrainCellLayout cell = TerrainMeshLayout::Cell(cellIndex);
            const float centreX = (cell.minimumX + cell.maximumX) * 0.5f;
            const float centreZ = (cell.minimumZ + cell.maximumZ) * 0.5f;
            const float offsetX = centreX - cameraPosition.X;
            const float offsetZ = centreZ - cameraPosition.Z;
            if (offsetX * offsetX + offsetZ * offsetZ
                > CellVisibleDistance * CellVisibleDistance) {
                continue;
            }

            ++visibleCellCount_;
            const Color cellColor(75, 205, 145, 255);
            const auto addCellEdge = [&](const float fromX, const float fromZ,
                                         const float toX, const float toZ) {
                for (int segment = 0; segment < CellEdgeSegments; ++segment) {
                    const float tA = static_cast<float>(segment)
                        / static_cast<float>(CellEdgeSegments);
                    const float tB = static_cast<float>(segment + 1)
                        / static_cast<float>(CellEdgeSegments);
                    const float xA = fromX + (toX - fromX) * tA;
                    const float zA = fromZ + (toZ - fromZ) * tA;
                    const float xB = fromX + (toX - fromX) * tB;
                    const float zB = fromZ + (toZ - fromZ) * tB;
                    const float yA = std::max(terrain_.Height(xA, zA) + 0.32f,
                                              OceanLineHeight + 0.12f);
                    const float yB = std::max(terrain_.Height(xB, zB) + 0.32f,
                                              OceanLineHeight + 0.12f);
                    AddLine(overlayVertices, Vector3(xA, yA, zA),
                            Vector3(xB, yB, zB), cellColor);
                }
            };
            addCellEdge(cell.minimumX, cell.minimumZ, cell.maximumX, cell.minimumZ);
            addCellEdge(cell.maximumX, cell.minimumZ, cell.maximumX, cell.maximumZ);
            addCellEdge(cell.maximumX, cell.maximumZ, cell.minimumX, cell.maximumZ);
            addCellEdge(cell.minimumX, cell.maximumZ, cell.minimumX, cell.minimumZ);
        }

        visibleObstacleCount_ = 0;
        const float visibleDistanceSquared = ObstacleVisibleDistance * ObstacleVisibleDistance;
        for (const CircularObstacle& obstacle : obstacles) {
            const float offsetX = obstacle.x - cameraPosition.X;
            const float offsetZ = obstacle.z - cameraPosition.Z;
            if (offsetX * offsetX + offsetZ * offsetZ > visibleDistanceSquared) {
                continue;
            }

            ++visibleObstacleCount_;
            const float y = terrain_.Height(obstacle.x, obstacle.z) + 0.16f;
            const Color color(255, 132, 48, 255);
            AddCircle(overlayVertices, obstacle.x, y, obstacle.z, obstacle.radius,
                      ObstacleSegments, color);
            AddLine(overlayVertices, Vector3(obstacle.x - obstacle.radius, y, obstacle.z),
                    Vector3(obstacle.x + obstacle.radius, y, obstacle.z), color);
            AddLine(overlayVertices, Vector3(obstacle.x, y, obstacle.z - obstacle.radius),
                    Vector3(obstacle.x, y, obstacle.z + obstacle.radius), color);
            AddLine(overlayVertices, Vector3(obstacle.x, y, obstacle.z),
                    Vector3(obstacle.x, y + 1.1f, obstacle.z), color);
        }

        const float playerY = playerSwimming ? OceanLineHeight + 0.18f
                                             : playerGroundHeight + 0.18f;
        const Color playerColor(235, 255, 235, 255);
        AddCircle(overlayVertices, playerPosition.x, playerY, playerPosition.z,
                  PlayerMotion::CapsuleRadius, PlayerSegments, playerColor);
        AddLine(overlayVertices, Vector3(playerPosition.x - 0.65f, playerY, playerPosition.z),
                Vector3(playerPosition.x + 0.65f, playerY, playerPosition.z), playerColor);
        AddLine(overlayVertices, Vector3(playerPosition.x, playerY, playerPosition.z - 0.65f),
                Vector3(playerPosition.x, playerY, playerPosition.z + 0.65f), playerColor);

        groundContactSampleCount_ = 0;
        groundSupportSampleCount_ = 0;
        steepGroundSampleCount_ = 0;
        for (const PlayerGroundOffset& offset : PlayerMotion::GroundContactOffsets) {
            const float sampleX = playerPosition.x + offset.x * PlayerMotion::CapsuleRadius;
            const float sampleZ = playerPosition.z + offset.z * PlayerMotion::CapsuleRadius;
            const TerrainSample sample = terrain_.Sample(sampleX, sampleZ);
            const float markerY = sample.height + 0.06f;
            const bool supportsPlayer = std::fabs(sample.height - playerGroundHeight) < 0.0001f;
            const bool exceedsSlopeLimit = sample.slopeRadians
                > PlayerMotion::MaximumWalkSlopeRadians;
            const Color normalColor = exceedsSlopeLimit
                ? Color(255, 75, 65, 255) : Color(115, 255, 155, 255);
            const Vector3 contact(sampleX, markerY, sampleZ);
            AddLine(overlayVertices,
                    contact - Vector3::Right * 0.11f,
                    contact + Vector3::Right * 0.11f, normalColor);
            AddLine(overlayVertices,
                    contact - Vector3::Forward * 0.11f,
                    contact + Vector3::Forward * 0.11f, normalColor);
            AddLine(overlayVertices, contact,
                    contact + Vector3(sample.normal.x, sample.normal.y, sample.normal.z) * 1.5f,
                    normalColor);
            if (supportsPlayer) {
                AddCircle(overlayVertices, sampleX, markerY, sampleZ, 0.14f, 8,
                          Color(245, 255, 245, 255));
                ++groundSupportSampleCount_;
            }
            if (exceedsSlopeLimit) {
                ++steepGroundSampleCount_;
            }
            ++groundContactSampleCount_;
        }
        AddLine(overlayVertices,
                Vector3(playerPosition.x, playerGroundHeight + 0.06f, playerPosition.z),
                Vector3(playerPosition.x, playerPosition.y, playerPosition.z), playerColor);

        const float offshoreDistance = std::max(
            0.0f, -terrain_.SignedShoreDistance(playerPosition.x, playerPosition.z));
        if (offshoreDistance > PlayerMotion::SoftCurrentStart) {
            const float radius = std::hypot(playerPosition.x, playerPosition.z);
            if (radius > 0.001f) {
                const float inwardX = -playerPosition.x / radius;
                const float inwardZ = -playerPosition.z / radius;
                const float strength = std::clamp(
                    (offshoreDistance - PlayerMotion::SoftCurrentStart)
                        / (PlayerMotion::FullCurrentDistance - PlayerMotion::SoftCurrentStart),
                    0.0f, 1.0f);
                const float arrowLength = 5.0f + strength * 8.0f;
                const Vector3 start(playerPosition.x, OceanLineHeight + 0.18f,
                                    playerPosition.z);
                const Vector3 end = start + Vector3(inwardX, 0.0f, inwardZ) * arrowLength;
                const Vector3 side(-inwardZ, 0.0f, inwardX);
                const Color currentColor(255, 238, 96, 255);
                AddLine(overlayVertices, start, end, currentColor);
                AddLine(overlayVertices, end,
                        end - Vector3(inwardX, 0.0f, inwardZ) * 2.0f + side * 1.2f,
                        currentColor);
                AddLine(overlayVertices, end,
                        end - Vector3(inwardX, 0.0f, inwardZ) * 2.0f - side * 1.2f,
                        currentColor);
            }
        }

        wildlifeMarkerCount_ = 0;
        wildlifeTargetCount_ = 0;
        monkeyHomeRangeCount_ = 0;
        for (const BirdState& bird : birds) {
            if (!IsNear(cameraPosition, bird.x, bird.y, bird.z, BirdMarkerDistance)) {
                continue;
            }
            AddHeadingMarker(overlayVertices, Vector3(bird.x, bird.y, bird.z),
                             bird.headingRadians,
                             4.0f * bird.scale,
                             TierColor(bird.simulationTier, Color(120, 235, 255, 255)));
            ++wildlifeMarkerCount_;
        }
        for (const FishState& fishState : fish) {
            if (!IsNear(cameraPosition, fishState.x, fishState.y, fishState.z,
                        FishMarkerDistance)) {
                continue;
            }
            AddHeadingMarker(overlayVertices, Vector3(fishState.x, fishState.y, fishState.z),
                             fishState.headingRadians, 1.7f * fishState.scale,
                             TierColor(fishState.simulationTier, Color(70, 180, 255, 255)));
            ++wildlifeMarkerCount_;
        }
        for (const FishSchoolState& school : fishSchools) {
            if (!IsNear(cameraPosition, school.centreX, school.centreY, school.centreZ,
                        FishMarkerDistance)) {
                continue;
            }
            const Color color = TierColor(
                school.simulationTier, Color(80, 235, 255, 255));
            AddTargetMarker(overlayVertices,
                            Vector3(school.centreX, school.centreY, school.centreZ),
                            2.2f, color);
            ++wildlifeTargetCount_;
        }
        if (shark.present && IsNear(cameraPosition, shark.x, shark.y, shark.z,
                                    SharkPatrol::ReducedSimulationDistance)) {
            AddHeadingMarker(overlayVertices, Vector3(shark.x, shark.y, shark.z),
                             shark.headingRadians, 8.0f,
                             TierColor(shark.simulationTier, Color(255, 90, 180, 255)));
            ++wildlifeMarkerCount_;
        }
        const float sharkTargetY = OceanLineHeight + 0.24f;
        if (IsNear(cameraPosition, shark.targetX, sharkTargetY, shark.targetZ,
                   SharkPatrol::ReducedSimulationDistance)) {
            const Color targetColor(255, 120, 205, 255);
            const Vector3 target(shark.targetX, sharkTargetY, shark.targetZ);
            AddTargetMarker(overlayVertices, target, 3.2f, targetColor);
            if (shark.visible) {
                AddLine(overlayVertices, Vector3(shark.x, shark.y, shark.z),
                        target, targetColor);
            }
            ++wildlifeTargetCount_;
        }
        for (const MonkeyState& monkey : monkeys) {
            if (!IsNear(cameraPosition, monkey.x, monkey.y, monkey.z,
                        MonkeyMarkerDistance)) {
                continue;
            }
            Color fullColor(255, 205, 95, 255);
            if (monkey.behavior == MonkeyBehavior::RunAway) {
                fullColor = Color(255, 92, 72, 255);
            } else if (monkey.behavior == MonkeyBehavior::Observe) {
                fullColor = Color(215, 125, 255, 255);
            } else if (monkey.behavior == MonkeyBehavior::Idle) {
                fullColor = Color(105, 240, 155, 255);
            }
            AddHeadingMarker(overlayVertices,
                             Vector3(monkey.x, monkey.y + 0.65f, monkey.z),
                             monkey.headingRadians, 2.2f,
                             TierColor(monkey.simulationTier, fullColor));
            const Color navigationColor = TierColor(
                monkey.simulationTier, Color(255, 225, 105, 255));
            const float homeY = terrain_.Height(monkey.homeX, monkey.homeZ) + 0.48f;
            AddTargetMarker(overlayVertices,
                            Vector3(monkey.homeX, homeY, monkey.homeZ),
                            0.75f, navigationColor);
            AddTerrainCircle(overlayVertices, terrain_, monkey.homeX, monkey.homeZ,
                             MonkeyTroop::MaximumHomeDistance,
                             MonkeyHomeRangeSegments, navigationColor);
            ++monkeyHomeRangeCount_;
            if (monkey.behavior == MonkeyBehavior::Walk
                || monkey.behavior == MonkeyBehavior::RunAway) {
                const Vector3 target(
                    monkey.targetX,
                    terrain_.Height(monkey.targetX, monkey.targetZ) + 0.62f,
                    monkey.targetZ);
                AddTargetMarker(overlayVertices, target, 0.95f, navigationColor);
                AddLine(overlayVertices,
                        Vector3(monkey.x, monkey.y + 0.65f, monkey.z),
                        target, navigationColor);
                ++wildlifeTargetCount_;
            }
            ++wildlifeMarkerCount_;
        }

        audioCandidateCount_ = 0;
        if (soundscape.nearestBird.available) {
            AddAudioSourceMarker(overlayVertices, soundscape.nearestBird, 1.7f,
                                 Color(105, 225, 255, 255));
            ++audioCandidateCount_;
        }
        if (soundscape.nearestMonkey.available) {
            AddAudioSourceMarker(overlayVertices, soundscape.nearestMonkey, 1.4f,
                                 Color(225, 135, 255, 255));
            ++audioCandidateCount_;
        }

        activeAudioEmitterCount_ = 0;
        const WildlifeCallRequest& activeCall = soundscape.activeWildlifeCall;
        if (activeCall.type != WildlifeCallType::None && activeCall.source.available) {
            const Color activeColor = activeCall.type == WildlifeCallType::Bird
                ? Color(225, 250, 255, 255) : Color(255, 215, 105, 255);
            AddAudioSourceMarker(overlayVertices, activeCall.source, 2.8f, activeColor);
            AddCircle(overlayVertices, activeCall.source.x, activeCall.source.y,
                      activeCall.source.z, 3.6f, AudioMarkerSegments, activeColor);
            AddLine(overlayVertices, cameraPosition,
                    Vector3(activeCall.source.x, activeCall.source.y,
                            activeCall.source.z), activeColor);
            activeAudioEmitterCount_ = 1;
        }

        lineCount_ = static_cast<int>((surfaceVertices.size() + overlayVertices.size()) / 2U);
        drawCallCount_ = 0;
        if (surfaceVertices.empty() && overlayVertices.empty()) {
            return;
        }

        effect_->setWorldProperty(Matrix::getIdentityProperty());
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        const auto drawLines = [this](const std::vector<VertexPositionColor>& vertices,
                                      const DepthStencilState& depthState) {
            if (vertices.empty()) {
                return;
            }
            device_.setDepthStencilStateProperty(depthState);
            for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
                pass.Apply();
                device_.DrawUserPrimitives(PrimitiveType::LineList, vertices.data(), 0,
                                           static_cast<int>(vertices.size() / 2U));
                ++drawCallCount_;
            }
        };
        drawLines(surfaceVertices, DepthStencilState::DepthRead);
        drawLines(overlayVertices, DepthStencilState::None);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
    }
}
