#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;
    class JungleLayout;

    enum class AntBehavior : std::uint8_t { Walk, Carry, Pause };

    struct AntState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float forwardX = 1.0f, forwardY = 0.0f, forwardZ = 0.0f;
        float normalX = 0.0f, normalY = 1.0f, normalZ = 0.0f;
        float gaitPhase = 0.0f, scale = 1.0f;
        AntBehavior behavior = AntBehavior::Walk;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
        std::uint8_t trailIndex = 0U;
    };

    struct AntTrailHome
    {
        float x = 0.0f, y = 0.0f, z = 0.0f, halfLength = 0.0f;
        bool onTrunk = false;
    };

    /** Four close-range ant trails; three cross soil and one climbs a real canopy trunk. */
    class AntTrails final
    {
    public:
        static constexpr std::size_t TrailCount = 4U;
        static constexpr std::size_t PerTrail = 16U;
        static constexpr std::size_t Count = TrailCount * PerTrail;
        static constexpr float FullSimulationDistance = 16.0f;
        static constexpr float ReducedSimulationDistance = 34.0f;

        AntTrails(const IslandTerrain& terrain, const JungleLayout& jungle);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const AntState> Ants() const noexcept { return states_; }
        [[nodiscard]] std::span<const AntTrailHome, TrailCount> Trails() const noexcept
        {
            return trails_;
        }

    private:
        struct Agent
        {
            AntState state;
            float progress = 0.0f, speed = 0.0f, pauseSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            bool outward = true;
        };

        void Initialize(std::size_t index);
        void Place(Agent&) noexcept;
        void UpdateAgent(Agent&, float deltaSeconds) noexcept;

        const IslandTerrain& terrain_;
        std::array<AntTrailHome, TrailCount> trails_{};
        std::array<float, TrailCount> trailAngles_{};
        std::array<float, TrailCount> trunkScales_{};
        std::array<std::uint8_t, TrailCount> trunkVariants_{};
        std::array<Agent, Count> agents_{};
        std::array<AntState, Count> states_{};
        std::uint64_t frameIndex_ = 0U;
    };
}
