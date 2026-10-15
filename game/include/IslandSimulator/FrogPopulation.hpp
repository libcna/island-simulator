#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class FrogBehavior : std::uint8_t { Rest, Hop, Flee };

    struct FrogState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float headingRadians = 0.0f, hopPhase = 0.0f, breathPhase = 0.0f;
        float scale = 1.0f;
        FrogBehavior behavior = FrogBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct FrogHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Ten peaceful stream/wetland frogs with bounded rest, hop and player-retreat states. */
    class FrogPopulation final
    {
    public:
        static constexpr std::size_t Count = 10U;
        static constexpr float FullSimulationDistance = 48.0f;
        static constexpr float ReducedSimulationDistance = 105.0f;

        explicit FrogPopulation(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const FrogState> Frogs() const noexcept { return states_; }
        [[nodiscard]] std::span<const FrogHome> Homes() const noexcept { return homes_; }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantCount_; }

    private:
        struct Agent
        {
            FrogState state;
            FrogHome home;
            float groundY = 0.0f;
            float behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent&, float, float, float, bool) noexcept;
        [[nodiscard]] bool IsValid(float x, float z, const FrogHome& home) const noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<FrogState, Count> states_{};
        std::array<FrogHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
        int fullCount_ = 0, reducedCount_ = 0, dormantCount_ = 0;
    };
}
