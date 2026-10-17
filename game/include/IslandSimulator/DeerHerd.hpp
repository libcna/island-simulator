#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/WildlifeSimulation.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    enum class DeerBehavior : std::uint8_t { Graze, Walk, Rest, Retreat };

    struct DeerState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float headingRadians = 0.0f, speed = 0.0f;
        float gaitPhase = 0.0f, grazeAmount = 0.0f, scale = 1.0f;
        DeerBehavior behavior = DeerBehavior::Graze;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t coatVariant = 0U;
        bool antlered = false;
    };

    struct DeerHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Four peaceful feral deer using open inland clearings and bounded retreat. */
    class DeerHerd final
    {
    public:
        static constexpr std::size_t Count = 4U;
        static constexpr float FullSimulationDistance = 90.0f;
        static constexpr float ReducedSimulationDistance = 190.0f;

        DeerHerd(const IslandTerrain& terrain, std::span<const CircularObstacle> obstacles);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const DeerState> Deer() const noexcept { return states_; }
        [[nodiscard]] std::span<const DeerHome> Homes() const noexcept { return homes_; }
        [[nodiscard]] bool IsValidPosition(std::size_t index, float x, float z) const noexcept;

    private:
        struct Agent
        {
            DeerState state;
            DeerHome home;
            float targetHeadingRadians = 0.0f;
            float behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent&, float, float, float, bool) noexcept;

        const IslandTerrain& terrain_;
        WorldCollisionGrid obstacles_;
        std::array<Agent, Count> agents_{};
        std::array<DeerState, Count> states_{};
        std::array<DeerHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
    };
}
