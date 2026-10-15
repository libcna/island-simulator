#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class FlyBehavior : std::uint8_t { Hover, Orbit, Dart };

    struct FlyState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float headingRadians = 0.0f, wingPhase = 0.0f, scale = 1.0f;
        FlyBehavior behavior = FlyBehavior::Hover;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct FlyHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Four close-range swarms of six harmless flies over inland organic ground. */
    class FlySwarms final
    {
    public:
        static constexpr std::size_t SwarmCount = 4U;
        static constexpr std::size_t PerSwarm = 6U;
        static constexpr std::size_t Count = SwarmCount * PerSwarm;
        static constexpr float FullSimulationDistance = 22.0f;
        static constexpr float ReducedSimulationDistance = 48.0f;

        explicit FlySwarms(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const FlyState> Flies() const noexcept { return states_; }
        [[nodiscard]] std::span<const FlyHome> Homes() const noexcept { return homes_; }

    private:
        struct Agent
        {
            FlyState state;
            FlyHome home;
            float angle = 0.0f, radius = 0.0f, altitude = 0.0f;
            float behaviorSeconds = 0.0f, accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent&, float deltaSeconds, float observerX,
                         float observerZ, bool observerValid) noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<FlyState, Count> states_{};
        std::array<FlyHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
    };
}
