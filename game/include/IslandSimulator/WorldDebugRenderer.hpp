#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/WorldCollision.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class GraphicsDevice;
}

namespace island
{
    /** @brief Opt-in world-space lines for traversal and collision validation. */
    class WorldDebugRenderer final
    {
    public:
        WorldDebugRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                           const IslandTerrain& terrain);
        ~WorldDebugRenderer();

        WorldDebugRenderer(const WorldDebugRenderer&) = delete;
        WorldDebugRenderer& operator=(const WorldDebugRenderer&) = delete;
        WorldDebugRenderer(WorldDebugRenderer&&) = delete;
        WorldDebugRenderer& operator=(WorldDebugRenderer&&) = delete;

        /** @brief Draws traversal, collision, wildlife, and soundscape diagnostics. */
        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  const PlayerPosition& playerPosition,
                  float playerGroundHeight,
                  bool playerSwimming,
                  const PlayerMovementTrace& movementTrace,
                  std::span<const CircularObstacle> obstacles,
                  std::span<const BirdState> birds,
                  std::span<const BirdRouteState> birdRoutes,
                  std::span<const BirdRouteRegion> birdRouteRegions,
                  std::span<const FishState> fish,
                  std::span<const FishSchoolState> fishSchools,
                  const SharkState& shark,
                  std::span<const MonkeyState> monkeys,
                  const SoundscapeDebugState& soundscape);

        [[nodiscard]] int VisibleObstacleCount() const noexcept
        {
            return visibleObstacleCount_;
        }

        [[nodiscard]] int LineCount() const noexcept { return lineCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int WildlifeMarkerCount() const noexcept
        {
            return wildlifeMarkerCount_;
        }
        [[nodiscard]] int WildlifeTargetCount() const noexcept
        {
            return wildlifeTargetCount_;
        }
        [[nodiscard]] int MonkeyHomeRangeCount() const noexcept
        {
            return monkeyHomeRangeCount_;
        }
        [[nodiscard]] int BirdRouteRegionCount() const noexcept
        {
            return birdRouteRegionCount_;
        }
        [[nodiscard]] int BirdSpawnMarkerCount() const noexcept
        {
            return birdSpawnMarkerCount_;
        }
        [[nodiscard]] int BirdRouteTargetCount() const noexcept
        {
            return birdRouteTargetCount_;
        }
        [[nodiscard]] int AudioZoneCount() const noexcept { return audioZoneCount_; }
        [[nodiscard]] int AudioCandidateCount() const noexcept
        {
            return audioCandidateCount_;
        }
        [[nodiscard]] int ActiveAudioEmitterCount() const noexcept
        {
            return activeAudioEmitterCount_;
        }
        [[nodiscard]] int GroundContactSampleCount() const noexcept
        {
            return groundContactSampleCount_;
        }
        [[nodiscard]] int GroundSupportSampleCount() const noexcept
        {
            return groundSupportSampleCount_;
        }
        [[nodiscard]] int SteepGroundSampleCount() const noexcept
        {
            return steepGroundSampleCount_;
        }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const IslandTerrain& terrain_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        int visibleObstacleCount_ = 0;
        int visibleCellCount_ = 0;
        int wildlifeMarkerCount_ = 0;
        int wildlifeTargetCount_ = 0;
        int monkeyHomeRangeCount_ = 0;
        int birdRouteRegionCount_ = 0;
        int birdSpawnMarkerCount_ = 0;
        int birdRouteTargetCount_ = 0;
        int audioZoneCount_ = 0;
        int audioCandidateCount_ = 0;
        int activeAudioEmitterCount_ = 0;
        int groundContactSampleCount_ = 0;
        int groundSupportSampleCount_ = 0;
        int steepGroundSampleCount_ = 0;
        int lineCount_ = 0;
        int drawCallCount_ = 0;
    };
}
