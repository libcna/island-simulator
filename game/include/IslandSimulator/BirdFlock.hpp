#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Broad route family used to compose varied island bird traffic. */
    enum class BirdPathFamily : std::uint8_t
    {
        Shore,
        Canopy,
        High
    };

    /** @brief Renderer-independent pose for one procedural bird. */
    struct BirdState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float wingPose = 0.0f;
        float scale = 1.0f;
        BirdPathFamily family = BirdPathFamily::Shore;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Deterministic, bounded and distance-throttled flying-bird simulation. */
    class BirdFlock final
    {
    public:
        static constexpr std::size_t BirdCount = 24U;
        static constexpr float FullSimulationDistance = 190.0f;
        static constexpr float ReducedSimulationDistance = 560.0f;

        explicit BirdFlock(const IslandTerrain& terrain);

        /** @brief Advances autonomous flight and updates observer-relative simulation tiers. */
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const BirdState> Birds() const noexcept;
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullSimulationCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept {
            return reducedSimulationCount_;
        }
        [[nodiscard]] int DormantSimulationCount() const noexcept {
            return dormantSimulationCount_;
        }

    private:
        struct Agent
        {
            BirdState state;
            float preferredRadius = 0.0f;
            float baseAltitude = 0.0f;
            float minimumClearance = 0.0f;
            float verticalAmplitude = 0.0f;
            float verticalPhase = 0.0f;
            float verticalFrequency = 0.0f;
            float wanderPhaseA = 0.0f;
            float wanderPhaseB = 0.0f;
            float wanderFrequencyA = 0.0f;
            float wanderFrequencyB = 0.0f;
            float flapPhase = 0.0f;
            float flapSpeed = 0.0f;
            float accumulatedSeconds = 0.0f;
            int direction = 1;
        };

        void InitializeAgent(std::size_t index);
        void UpdateAgent(Agent& agent, float deltaSeconds) noexcept;

        const IslandTerrain& terrain_;
        std::vector<Agent> agents_;
        std::vector<BirdState> states_;
        float elapsedSeconds_ = 0.0f;
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
