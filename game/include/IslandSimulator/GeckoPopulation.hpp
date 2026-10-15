#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island
{
    class IslandTerrain;

    enum class GeckoBehavior : std::uint8_t
    {
        Rest,
        Scurry,
        Hidden
    };

    struct GeckoState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float gaitPhase = 0.0f;
        float visibility = 1.0f;
        float scale = 1.0f;
        GeckoBehavior behavior = GeckoBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    struct GeckoHome
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** @brief Small deterministic inland lizards that rest, flee and hide. */
    class GeckoPopulation final
    {
    public:
        static constexpr std::size_t Count = 8U;
        static constexpr float FullSimulationDistance = 44.0f;
        static constexpr float ReducedSimulationDistance = 105.0f;

        explicit GeckoPopulation(const IslandTerrain& terrain);

        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const GeckoState> Geckos() const noexcept { return states_; }
        [[nodiscard]] std::span<const GeckoHome> Homes() const noexcept { return homes_; }
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullSimulationCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept { return reducedSimulationCount_; }
        [[nodiscard]] int DormantSimulationCount() const noexcept { return dormantSimulationCount_; }

    private:
        struct Agent
        {
            GeckoState state;
            GeckoHome home;
            float behaviorSeconds = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitionCount = 0U;
        };

        void Initialize(std::size_t index);
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ, bool observerValid) noexcept;
        [[nodiscard]] bool ValidGround(float x, float z, const GeckoHome& home) const noexcept;

        const IslandTerrain& terrain_;
        std::array<Agent, Count> agents_{};
        std::array<GeckoState, Count> states_{};
        std::array<GeckoHome, Count> homes_{};
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
