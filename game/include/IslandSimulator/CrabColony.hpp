#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Compact autonomous behaviour used by the beach-crab population. */
    enum class CrabBehavior : std::uint8_t
    {
        Pause,
        Forage,
        Scuttle
    };

    /** @brief Renderer-independent state for one procedural beach crab. */
    struct CrabState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float bodyYawRadians = 0.0f;
        float travelHeadingRadians = 0.0f;
        float speed = 0.0f;
        float legPhase = 0.0f;
        float clawPose = 0.0f;
        float scale = 1.0f;
        CrabBehavior behavior = CrabBehavior::Pause;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Immutable home range used to keep a crab inside suitable beach habitat. */
    struct CrabHomeRange
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    struct CrabHabitatCue { float x = 0.0f, z = 0.0f; };

    /** @brief Visible sandy retreat shared by the five members of one beach colony. */
    struct CrabBurrow
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** @brief Deterministic beach-constrained crab simulation with mild player avoidance. */
    class CrabColony final
    {
    public:
        static constexpr std::size_t CrabCount = 30U;
        static constexpr float MinimumShoreDistance = 1.4f;
        static constexpr float MaximumShoreDistance = 17.0f;
        static constexpr float FullSimulationDistance = 82.0f;
        static constexpr float ReducedSimulationDistance = 210.0f;
        static constexpr float MinimumAgentSeparation = 0.72f;

        explicit CrabColony(const IslandTerrain& terrain);
        /** @brief Transactionally reanchors six colonies beside real non-solid beach debris. */
        void SetDebrisHabitats(std::span<const CrabHabitatCue> cues);
        [[nodiscard]] static bool IsNearDebris(
            const CrabHomeRange& home, std::span<const CrabHabitatCue> cues) noexcept;

        /** @brief Advances autonomous beach movement and observer-relative activation. */
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const CrabState> Crabs() const noexcept;
        [[nodiscard]] std::span<const CrabHomeRange> Homes() const noexcept;
        [[nodiscard]] std::span<const CrabBurrow> Burrows() const noexcept { return burrows_; }
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
            CrabState state;
            CrabHomeRange home;
            float behaviorSeconds = 0.0f;
            float movementPhase = 0.0f;
            float animationPhase = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitionCount = 0U;
            int preferredDirection = 1;
        };

        void InitializeAgent(std::size_t index);
        void RebuildBurrows() noexcept;
        void UpdateAgent(Agent& agent, std::size_t index, float deltaSeconds,
                         float observerX, float observerZ,
                         bool observerValid) noexcept;
        [[nodiscard]] bool IsValidHabitat(float x, float z,
                                          const CrabHomeRange& home) const noexcept;

        const IslandTerrain& terrain_;
        std::vector<Agent> agents_;
        std::vector<CrabState> states_;
        std::vector<CrabHomeRange> homes_;
        std::array<CrabBurrow, 6> burrows_{};
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
        std::array<CrabHabitatCue, 6> debrisAnchors_{};
        bool debrisAnchored_ = false;
    };
}
