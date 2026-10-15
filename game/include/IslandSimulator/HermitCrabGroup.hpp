#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class HermitCrabBehavior : std::uint8_t
    {
        Pause,
        Forage,
        Retreat
    };

    struct HermitCrabState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float legPhase = 0.0f;
        float antennaPhase = 0.0f;
        float scale = 1.0f;
        HermitCrabBehavior behavior = HermitCrabBehavior::Pause;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t shellVariant = 0U;
    };

    struct HermitCrabHome
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** Ten deterministic shell-bearing crabs on sheltered sandy beach sections. */
    class HermitCrabGroup final
    {
    public:
        static constexpr std::size_t Count = 10U;
        static constexpr float MinimumShoreDistance = 2.0f;
        static constexpr float MaximumShoreDistance = 14.0f;
        static constexpr float FullSimulationDistance = 70.0f;
        static constexpr float ReducedSimulationDistance = 170.0f;

        explicit HermitCrabGroup(const IslandTerrain& terrain);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const HermitCrabState> Crabs() const noexcept {
            return states_;
        }
        [[nodiscard]] std::span<const HermitCrabHome> Homes() const noexcept {
            return homes_;
        }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantCount_; }

    private:
        struct Agent
        {
            HermitCrabState state;
            HermitCrabHome home;
            float behaviorSeconds = 0.0f;
            float steeringPhase = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitionCount = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ,
                         bool observerValid) noexcept;
        [[nodiscard]] bool IsValid(float x, float z,
                                   const HermitCrabHome& home) const noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<HermitCrabState, Count> states_{};
        std::array<HermitCrabHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
        int fullCount_ = 0;
        int reducedCount_ = 0;
        int dormantCount_ = 0;
    };
}
