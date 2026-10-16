#pragma once

#include <array>
#include <memory>
#include <deque>
#include <span>
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class JungleLayout;
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
        bool arboreal = false;
    };

    /** @brief Nonhostile ground and trunk snakes with independent bounded motion. */
    class SnakePopulation final
    {
    public:
        static constexpr std::size_t Count = 50;
        static constexpr std::size_t TreeCount = 6;
        static constexpr float Spacing = 0.055f;
        static constexpr float MaximumRadius = 0.05f;
        static constexpr float HomeRadius = 18.0f;
        static constexpr float MinimumHomeSeparation = 14.0f;
        SnakePopulation(const IslandTerrain& terrain, std::span<const CircularObstacle> obstacles = {});
        SnakePopulation(const IslandTerrain& terrain, const JungleLayout& jungle,
                        std::span<const CircularObstacle> obstacles = {});
        void Update(float deltaSeconds, float observerX, float observerY, float observerZ);
        [[nodiscard]] std::span<const SnakeState> Snakes() const noexcept { return states_; }
        [[nodiscard]] std::span<const SnakeState> TreeSnakes() const noexcept
        {
            return std::span<const SnakeState>(treeStates_).first(treeCount_);
        }
        [[nodiscard]] const RenderedTerrainSurface& Surface() const noexcept { return *surface_; }
        [[nodiscard]] static float RadiusAt(std::size_t station) noexcept;
        [[nodiscard]] static bool IsUnderstoryMargin(
            const JungleLayout& jungle, float x, float z) noexcept;
        [[nodiscard]] bool ValidBody(const SnakeState& state) const;

    private:
        [[nodiscard]] float GroundedY(float x, float z, float radius) const;
        [[nodiscard]] bool ValidPoint(float x, float z, float radius) const;
        void Initialize(const JungleLayout* jungle);
        void UpdateTreeSnake(std::size_t index, float clock);
        struct TreeHabitat
        {
            float x=0, y=0, z=0, yaw=0, scale=1, baseHeight=0, baseAngle=0;
            std::uint8_t variant=0;
        };
        const IslandTerrain& terrain_;
        WorldCollisionGrid obstacles_;
        std::unique_ptr<RenderedTerrainSurface> surface_;
        std::array<SnakeState, Count> states_{};
        std::array<SnakeState, TreeCount> treeStates_{};
        std::array<TreeHabitat, TreeCount> treeHabitats_{};
        std::array<float, TreeCount> treeClocks_{};
        std::size_t treeCount_=0;
        std::array<float, Count> clocks_{};
        std::array<float, Count> accumulatedSeconds_{};
        std::uint64_t frameIndex_ = 0;
        // Newest first, >=6 mm between committed samples; bounded independently of dt.
        std::array<std::deque<SnakePoint>, Count> trails_{};
    };
}
