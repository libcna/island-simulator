#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "IslandSimulator/WorldCollision.hpp"

namespace island
{
    /** @brief Immutable broad-phase grid for the island's static circular obstacles. */
    class WorldCollisionGrid final
    {
    public:
        static constexpr float CellSize = 16.0f;

        WorldCollisionGrid() = default;
        explicit WorldCollisionGrid(std::span<const CircularObstacle> obstacles);

        /** @brief Rebuilds the index from a stable copy of the complete obstacle set. */
        void Reset(std::span<const CircularObstacle> obstacles);

        /** @brief Performs an exact circle test after selecting nearby grid candidates. */
        [[nodiscard]] bool IntersectsCircle(float x, float z, float radius) const noexcept;

        /** @brief Returns the broad-phase population considered by the same circle query. */
        [[nodiscard]] std::size_t CandidateCount(float x, float z,
                                                 float radius) const noexcept;

        [[nodiscard]] std::span<const CircularObstacle> Obstacles() const noexcept
        {
            return obstacles_;
        }

        [[nodiscard]] std::size_t CellCount() const noexcept { return cells_.size(); }

    private:
        [[nodiscard]] static int CellCoordinate(float coordinate) noexcept;
        [[nodiscard]] static std::uint64_t CellKey(int cellX, int cellZ) noexcept;
        [[nodiscard]] int QueryExtent(float radius) const noexcept;

        std::vector<CircularObstacle> obstacles_;
        std::unordered_map<std::uint64_t, std::vector<std::size_t>> cells_;
        float maximumObstacleRadius_ = 0.0f;
    };
}
