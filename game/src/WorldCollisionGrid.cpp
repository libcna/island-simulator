#include "IslandSimulator/WorldCollisionGrid.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    WorldCollisionGrid::WorldCollisionGrid(const std::span<const CircularObstacle> obstacles)
    {
        Reset(obstacles);
    }

    int WorldCollisionGrid::CellCoordinate(const float coordinate) noexcept
    {
        return static_cast<int>(std::floor(coordinate / CellSize));
    }

    std::uint64_t WorldCollisionGrid::CellKey(const int cellX, const int cellZ) noexcept
    {
        const auto xBits = static_cast<std::uint32_t>(cellX);
        const auto zBits = static_cast<std::uint32_t>(cellZ);
        return (static_cast<std::uint64_t>(xBits) << 32U) | zBits;
    }

    int WorldCollisionGrid::QueryExtent(const float radius) const noexcept
    {
        const float safeRadius = std::max(0.0f, radius) + maximumObstacleRadius_;
        return std::max(1, static_cast<int>(std::ceil(safeRadius / CellSize)));
    }

    void WorldCollisionGrid::Reset(const std::span<const CircularObstacle> obstacles)
    {
        obstacles_.assign(obstacles.begin(), obstacles.end());
        cells_.clear();
        cells_.reserve(obstacles_.size());
        maximumObstacleRadius_ = 0.0f;

        for (std::size_t index = 0; index < obstacles_.size(); ++index) {
            const CircularObstacle& obstacle = obstacles_[index];
            maximumObstacleRadius_ = std::max(
                maximumObstacleRadius_, std::max(0.0f, obstacle.radius));
            cells_[CellKey(CellCoordinate(obstacle.x), CellCoordinate(obstacle.z))]
                .push_back(index);
        }
    }

    std::size_t WorldCollisionGrid::CandidateCount(const float x, const float z,
                                                   const float radius) const noexcept
    {
        const int centreX = CellCoordinate(x);
        const int centreZ = CellCoordinate(z);
        const int extent = QueryExtent(radius);
        std::size_t count = 0U;
        for (int offsetZ = -extent; offsetZ <= extent; ++offsetZ) {
            for (int offsetX = -extent; offsetX <= extent; ++offsetX) {
                const auto found = cells_.find(CellKey(centreX + offsetX, centreZ + offsetZ));
                if (found != cells_.end()) {
                    count += found->second.size();
                }
            }
        }
        return count;
    }

    bool WorldCollisionGrid::IntersectsCircle(const float x, const float z,
                                              const float radius) const noexcept
    {
        const float safeRadius = std::max(0.0f, radius);
        const int centreX = CellCoordinate(x);
        const int centreZ = CellCoordinate(z);
        const int extent = QueryExtent(safeRadius);
        for (int offsetZ = -extent; offsetZ <= extent; ++offsetZ) {
            for (int offsetX = -extent; offsetX <= extent; ++offsetX) {
                const auto found = cells_.find(CellKey(centreX + offsetX, centreZ + offsetZ));
                if (found == cells_.end()) {
                    continue;
                }
                for (const std::size_t index : found->second) {
                    const CircularObstacle& obstacle = obstacles_[index];
                    const float combinedRadius = safeRadius
                        + std::max(0.0f, obstacle.radius);
                    const float separationX = x - obstacle.x;
                    const float separationZ = z - obstacle.z;
                    if (separationX * separationX + separationZ * separationZ
                        < combinedRadius * combinedRadius) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
}
