#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    enum class StingrayBehavior : std::uint8_t
    {
        Glide,
        Forage,
        Settle
    };

    struct StingrayState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float bankRadians = 0.0f;
        float speed = 0.0f;
        float wingWave = 0.0f;
        float scale = 1.0f;
        StingrayBehavior behavior = StingrayBehavior::Glide;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct StingrayHomeRange
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** @brief Five peaceful stingrays confined to shallow sandy water. */
    class StingrayGroup final
    {
    public:
        static constexpr std::size_t Count = 5U;
        static constexpr float MinimumShoreDistance = -34.0f;
        static constexpr float MaximumShoreDistance = -6.0f;
        static constexpr float FullSimulationDistance = 140.0f;
        static constexpr float ReducedSimulationDistance = 330.0f;

        explicit StingrayGroup(const IslandTerrain& terrain);

        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const StingrayState> Stingrays() const noexcept {
            return states_;
        }
        [[nodiscard]] std::span<const StingrayHomeRange> Homes() const noexcept {
            return homes_;
        }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantCount_; }

    private:
        struct Agent
        {
            StingrayState state;
            StingrayHomeRange home;
            float routePhase = 0.0f;
            float flapPhase = 0.0f;
            float behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitionCount = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ,
                         bool observerValid) noexcept;
        [[nodiscard]] bool IsSandyHabitat(float x, float z,
                                          const StingrayHomeRange& home) const noexcept;
        [[nodiscard]] float DesiredHeight(const Agent& agent) const noexcept;

        const IslandTerrain& terrain_;
        std::vector<Agent> agents_;
        std::vector<StingrayState> states_;
        std::vector<StingrayHomeRange> homes_;
        float elapsedSeconds_ = 0.0f;
        std::uint64_t frameIndex_ = 0U;
        int fullCount_ = 0;
        int reducedCount_ = 0;
        int dormantCount_ = 0;
    };
}
