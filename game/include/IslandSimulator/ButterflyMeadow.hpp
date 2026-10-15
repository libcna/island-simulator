#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Compact low-altitude activity states for clearing butterflies. */
    enum class ButterflyBehavior : std::uint8_t
    {
        Rest,
        Flutter,
        NectarHover
    };

    /** @brief Renderer-independent pose for one procedural butterfly. */
    struct ButterflyState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float wingFold = 0.0f;
        float scale = 1.0f;
        ButterflyBehavior behavior = ButterflyBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Immutable clearing or jungle-edge range used by one butterfly. */
    struct ButterflyHomeRange
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** @brief One persistent cluster of nectar flowers anchoring a butterfly habitat. */
    struct ButterflyFlowerPatch
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
        float groundSlopeX = 0.0f;
        float groundSlopeZ = 0.0f;
    };

    /** @brief Deterministic low-altitude butterfly activity around three sunny habitats. */
    class ButterflyMeadow final
    {
    public:
        static constexpr std::size_t ButterflyCount = 18U;
        static constexpr std::size_t HabitatCount = 3U;
        static constexpr float MinimumInlandDistance = 28.0f;
        static constexpr float FullSimulationDistance = 70.0f;
        static constexpr float ReducedSimulationDistance = 165.0f;

        explicit ButterflyMeadow(const IslandTerrain& terrain);

        /** @brief Advances bounded flutter/rest activity and observer-relative activation. */
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const ButterflyState> Butterflies() const noexcept;
        [[nodiscard]] std::span<const ButterflyHomeRange> Homes() const noexcept;
        [[nodiscard]] std::span<const ButterflyFlowerPatch> FlowerPatches() const noexcept
        {
            return flowerPatches_;
        }
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
            ButterflyState state;
            ButterflyHomeRange home;
            float behaviorSeconds = 0.0f;
            float baseAltitude = 0.0f;
            float turnPhaseA = 0.0f;
            float turnPhaseB = 0.0f;
            float verticalPhase = 0.0f;
            float wingPhase = 0.0f;
            float accumulatedSeconds = 0.0f;
            float nectarX = 0.0f;
            float nectarZ = 0.0f;
            std::uint32_t transitionCount = 0U;
        };

        void InitializeAgent(std::size_t index);
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ,
                         bool observerValid) noexcept;
        [[nodiscard]] bool IsValidHabitat(float x, float z,
                                          const ButterflyHomeRange& home) const noexcept;

        const IslandTerrain& terrain_;
        std::vector<Agent> agents_;
        std::vector<ButterflyState> states_;
        std::vector<ButterflyHomeRange> homes_;
        std::array<ButterflyFlowerPatch, HabitatCount> flowerPatches_{};
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
