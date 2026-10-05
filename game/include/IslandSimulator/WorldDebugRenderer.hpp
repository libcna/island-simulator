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

        /** @brief Draws traversal boundaries, local scene cells, collision, and wildlife markers. */
        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  const PlayerPosition& playerPosition,
                  float playerGroundHeight,
                  bool playerSwimming,
                  std::span<const CircularObstacle> obstacles,
                  std::span<const BirdState> birds,
                  std::span<const FishState> fish,
                  const SharkState& shark,
                  std::span<const MonkeyState> monkeys);

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

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const IslandTerrain& terrain_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        int visibleObstacleCount_ = 0;
        int visibleCellCount_ = 0;
        int wildlifeMarkerCount_ = 0;
        int lineCount_ = 0;
        int drawCallCount_ = 0;
    };
}
