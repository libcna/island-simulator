#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class SkinkBehavior : std::uint8_t { Bask, Forage, Flee };

    struct SkinkState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float headingRadians = 0.0f, speed = 0.0f;
        float gaitPhase = 0.0f, breathPhase = 0.0f, scale = 1.0f;
        SkinkBehavior behavior = SkinkBehavior::Bask;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct SkinkHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Eight diurnal skinks that bask and forage on open inland ground. */
    class SkinkPopulation final
    {
    public:
        static constexpr std::size_t Count = 8U;
        static constexpr float FullSimulationDistance = 48.0f;
        static constexpr float ReducedSimulationDistance = 112.0f;

        explicit SkinkPopulation(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const SkinkState> Skinks() const noexcept { return states_; }
        [[nodiscard]] std::span<const SkinkHome> Homes() const noexcept { return homes_; }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantCount_; }

    private:
        struct Agent
        {
            SkinkState state;
            SkinkHome home;
            float behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent&, float, float, float, bool) noexcept;
        [[nodiscard]] bool IsValid(float x, float z, const SkinkHome& home) const noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<SkinkState, Count> states_{};
        std::array<SkinkHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
        int fullCount_ = 0, reducedCount_ = 0, dormantCount_ = 0;
    };
}
