#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class BeetleBehavior : std::uint8_t { Rest, Forage, Hide };

    struct BeetleState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float normalX = 0.0f, normalY = 1.0f, normalZ = 0.0f;
        float headingRadians = 0.0f, gaitPhase = 0.0f, visibility = 1.0f, scale = 1.0f;
        BeetleBehavior behavior = BeetleBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct BeetleHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Fifteen large tropical ground beetles in three inland leaf-litter colonies. */
    class BeetlePopulation final
    {
    public:
        static constexpr std::size_t ColonyCount = 3U;
        static constexpr std::size_t PerColony = 5U;
        static constexpr std::size_t Count = ColonyCount * PerColony;
        static constexpr float FullSimulationDistance = 22.0f;
        static constexpr float ReducedSimulationDistance = 48.0f;

        explicit BeetlePopulation(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const BeetleState, Count> Beetles() const noexcept
        {
            return states_;
        }
        [[nodiscard]] std::span<const BeetleHome, ColonyCount> Homes() const noexcept
        {
            return homes_;
        }

    private:
        struct Agent
        {
            BeetleState state;
            std::uint8_t colony = 0U;
            float angle = 0.0f, radius = 0.0f, behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };

        void Place(Agent&) noexcept;
        void UpdateAgent(Agent&, float deltaSeconds, float observerX,
                         float observerZ, bool observerValid) noexcept;

        const IslandTerrain& terrain_;
        std::array<BeetleHome, ColonyCount> homes_{};
        std::array<Agent, Count> agents_{};
        std::array<BeetleState, Count> states_{};
        std::uint64_t frameIndex_ = 0U;
    };
}
