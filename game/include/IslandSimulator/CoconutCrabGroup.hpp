#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class CoconutCrabBehavior : std::uint8_t { Rest, Forage, Retreat };

    struct CoconutCrabState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float headingRadians = 0.0f, speed = 0.0f;
        float legPhase = 0.0f, clawPose = 0.0f, scale = 1.0f;
        CoconutCrabBehavior behavior = CoconutCrabBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct CoconutCrabHome { float x = 0.0f, z = 0.0f, radius = 0.0f; };

    /** Six large terrestrial coconut crabs in coastal palm-zone habitats. */
    class CoconutCrabGroup final
    {
    public:
        static constexpr std::size_t Count = 6U;
        static constexpr float MinimumShoreDistance = 22.0f;
        static constexpr float MaximumShoreDistance = 58.0f;
        static constexpr float FullSimulationDistance = 78.0f;
        static constexpr float ReducedSimulationDistance = 185.0f;

        explicit CoconutCrabGroup(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;
        [[nodiscard]] std::span<const CoconutCrabState> Crabs() const noexcept {
            return states_;
        }
        [[nodiscard]] std::span<const CoconutCrabHome> Homes() const noexcept {
            return homes_;
        }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantCount_; }

    private:
        struct Agent
        {
            CoconutCrabState state;
            CoconutCrabHome home;
            float behaviorSeconds = 0.0f;
            float steeringPhase = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitions = 0U;
        };
        void Initialize(std::size_t index);
        void UpdateAgent(Agent&, float, float, float, bool) noexcept;
        [[nodiscard]] bool IsValid(float x, float z,
                                   const CoconutCrabHome& home) const noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<CoconutCrabState, Count> states_{};
        std::array<CoconutCrabHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
        int fullCount_ = 0, reducedCount_ = 0, dormantCount_ = 0;
    };
}
