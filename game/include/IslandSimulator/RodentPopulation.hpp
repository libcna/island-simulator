#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class RodentSpecies : std::uint8_t { Mouse, Rat };
    enum class RodentBehavior : std::uint8_t { Rest, Forage, Scurry, Hidden };

    struct RodentState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float headingRadians = 0.0f, speed = 0.0f;
        float gaitPhase = 0.0f, sniffPhase = 0.0f, visibility = 1.0f;
        RodentSpecies species = RodentSpecies::Mouse;
        RodentBehavior behavior = RodentBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct RodentHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Twelve field mice and eight rats in bounded inland edge habitats. */
    class RodentPopulation final
    {
    public:
        static constexpr std::size_t MouseCount = 12U;
        static constexpr std::size_t RatCount = 8U;
        static constexpr std::size_t Count = MouseCount + RatCount;
        static constexpr float FullSimulationDistance = 34.0f;
        static constexpr float ReducedSimulationDistance = 85.0f;

        explicit RodentPopulation(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const RodentState> Rodents() const noexcept { return states_; }
        [[nodiscard]] std::span<const RodentHome> Homes() const noexcept { return homes_; }
        [[nodiscard]] bool IsValidPosition(std::size_t index, float x, float z) const noexcept;

    private:
        struct Agent
        {
            RodentState state;
            RodentHome home;
            float behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent&, std::size_t, float, float, float, bool) noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<RodentState, Count> states_{};
        std::array<RodentHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
    };
}
