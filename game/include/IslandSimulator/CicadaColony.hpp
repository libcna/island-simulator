#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class JungleLayout;

    enum class CicadaBehavior : std::uint8_t { Rest, Call, Flight };

    struct CicadaState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float forwardX = 0.0f, forwardY = 1.0f, forwardZ = 0.0f;
        float normalX = 1.0f, normalY = 0.0f, normalZ = 0.0f;
        float wingPhase = 0.0f, scale = 1.0f;
        CicadaBehavior behavior = CicadaBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct CicadaHome
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float radius = 0.0f, azimuth = 0.0f;
    };

    /** Twelve trunk-resting cicadas with sparse calls and short crown-local flights. */
    class CicadaColony final
    {
    public:
        static constexpr std::size_t Count = 12U;
        static constexpr float FullSimulationDistance = 28.0f;
        static constexpr float ReducedSimulationDistance = 65.0f;

        explicit CicadaColony(const JungleLayout& jungle);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const CicadaState, Count> Cicadas() const noexcept
        {
            return states_;
        }
        [[nodiscard]] std::span<const CicadaHome, Count> Homes() const noexcept
        {
            return homes_;
        }

    private:
        struct Agent
        {
            CicadaState state;
            float stateSeconds = 0.0f;
            float flightProgress = 0.0f;
            float accumulatedSeconds = 0.0f;
        };

        void PlacePerched(std::size_t index) noexcept;
        void PlaceFlying(std::size_t index) noexcept;
        void UpdateAgent(std::size_t index, float deltaSeconds) noexcept;

        std::array<CicadaHome, Count> homes_{};
        std::array<Agent, Count> agents_{};
        std::array<CicadaState, Count> states_{};
        std::uint64_t frameIndex_ = 0U;
    };
}
