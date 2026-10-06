#pragma once

#include <cstdint>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Renderer-independent pose and presence state for the offshore shark. */
    struct SharkState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float pitchRadians = 0.0f;
        float bankRadians = 0.0f;
        float bodyWave = 0.0f;
        float scale = 2.8f;
        float activity = 1.0f;
        float targetX = 0.0f;
        float targetZ = 0.0f;
        bool present = true;
        bool visible = true;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
    };

    /** @brief Rare non-hostile offshore patrol with deterministic dive/appearance timing. */
    class SharkPatrol final
    {
    public:
        static constexpr float FullSimulationDistance = 180.0f;
        static constexpr float ReducedSimulationDistance = 560.0f;

        explicit SharkPatrol(const IslandTerrain& terrain);

        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] const SharkState& State() const noexcept { return state_; }
        [[nodiscard]] std::uint32_t PresenceCycleCount() const noexcept {
            return presenceCycleCount_;
        }

    private:
        void ResolveRouteTarget() noexcept;
        void UpdateMotion(float deltaSeconds) noexcept;
        [[nodiscard]] float NextPresentDuration() const noexcept;
        [[nodiscard]] float NextHiddenDuration() const noexcept;

        const IslandTerrain& terrain_;
        SharkState state_;
        float routeAngle_ = 0.0f;
        float elapsedSeconds_ = 0.0f;
        float presenceSecondsRemaining_ = 0.0f;
        float accumulatedSeconds_ = 0.0f;
        float bodyPhase_ = 0.0f;
        std::uint64_t frameIndex_ = 0U;
        std::uint32_t presenceCycleCount_ = 0U;
        int direction_ = 1;
    };
}
