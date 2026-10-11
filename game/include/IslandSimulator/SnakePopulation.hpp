#pragma once

#include <array>
#include <memory>
#include <span>
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    struct SnakePoint { float x = 0.0f, y = 0.0f, z = 0.0f; };
    enum class SnakeBehavior { Rest, Explore, Retreat };
    struct SnakeState
    {
        static constexpr std::size_t StationCount = 41;
        std::array<SnakePoint, StationCount> body{}; // Head first, tail last.
        float homeX = 0.0f, homeZ = 0.0f;
        float heading = 0.0f, phase = 0.0f, speed = 0.0f;
        SnakeBehavior behavior = SnakeBehavior::Rest;
        WildlifeSimulationTier tier = WildlifeSimulationTier::Dormant;
    };

    /** @brief Nonhostile ground snakes; body follows the head's path instead of rigid translation.
     * Tree habitat is a separate, still-unimplemented part of the snake feature.
     */
    class SnakePopulation final
    {
    public:
        static constexpr std::size_t Count = 4;
        static constexpr float Spacing = 0.055f;
        static constexpr float MaximumRadius = 0.05f;
        static constexpr float HomeRadius = 18.0f;
        SnakePopulation(const IslandTerrain& terrain, std::span<const CircularObstacle> obstacles = {});
        void Update(float deltaSeconds, float observerX, float observerY, float observerZ);
        [[nodiscard]] std::span<const SnakeState> Snakes() const noexcept { return states_; }
        [[nodiscard]] static float RadiusAt(std::size_t station) noexcept;
        [[nodiscard]] bool ValidBody(const SnakeState& state) const;

    private:
        [[nodiscard]] float GroundedY(float x, float z, float radius) const;
        [[nodiscard]] bool ValidPoint(float x, float z, float radius) const;
        const IslandTerrain& terrain_;
        WorldCollisionGrid obstacles_;
        std::unique_ptr<RenderedTerrainSurface> surface_;
        std::array<SnakeState, Count> states_{};
        std::array<float, Count> clocks_{};
    };
}
