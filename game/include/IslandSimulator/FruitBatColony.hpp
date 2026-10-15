#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    enum class FruitBatBehavior : std::uint8_t
    {
        Roost,
        DayFlight
    };

    struct FruitBatRoost
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
    };

    struct FruitBatState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float wingPose = 0.0f;
        float hangAmount = 1.0f;
        float scale = 1.0f;
        FruitBatBehavior behavior = FruitBatBehavior::Roost;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Six bounded daytime fruit bats using real mature jungle crowns as roosts. */
    class FruitBatColony final
    {
    public:
        static constexpr std::size_t Count = 6U;
        static constexpr std::size_t MaximumDayFlights = 2U;
        static constexpr float FullSimulationDistance = 120.0f;
        static constexpr float ReducedSimulationDistance = 280.0f;

        FruitBatColony(const IslandTerrain& terrain,
                       std::span<const CanopyPlacement> canopy);

        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const FruitBatState> Bats() const noexcept { return states_; }
        [[nodiscard]] std::span<const FruitBatRoost> Roosts() const noexcept { return roosts_; }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullSimulationCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedSimulationCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantSimulationCount_; }

    private:
        struct Agent
        {
            FruitBatState state;
            FruitBatRoost roost;
            float orbitRadius = 0.0f;
            float orbitPhase = 0.0f;
            float orbitSpeed = 0.0f;
            float flapPhase = 0.0f;
            float accumulatedSeconds = 0.0f;
        };

        std::vector<Agent> agents_;
        std::vector<FruitBatState> states_;
        std::vector<FruitBatRoost> roosts_;
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
