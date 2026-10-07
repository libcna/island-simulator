#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Calm near-shore activity states for a sea turtle. */
    enum class SeaTurtleBehavior : std::uint8_t
    {
        Cruise,
        Forage,
        Surface
    };

    /** @brief Renderer-independent pose for one non-hostile sea turtle. */
    struct SeaTurtleState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float pitchRadians = 0.0f;
        float bankRadians = 0.0f;
        float speed = 0.0f;
        float flipperStroke = 0.0f;
        float scale = 1.0f;
        SeaTurtleBehavior behavior = SeaTurtleBehavior::Cruise;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Immutable water-space range that bounds one turtle's local route. */
    struct SeaTurtleHomeRange
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** @brief Deterministic near-shore turtles with slow cruising and occasional surfacing. */
    class SeaTurtleGroup final
    {
    public:
        static constexpr std::size_t TurtleCount = 5U;
        static constexpr float MinimumShoreDistance = -38.0f;
        static constexpr float MaximumShoreDistance = -7.0f;
        static constexpr float FullSimulationDistance = 135.0f;
        static constexpr float ReducedSimulationDistance = 330.0f;

        explicit SeaTurtleGroup(const IslandTerrain& terrain);

        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const SeaTurtleState> Turtles() const noexcept;
        [[nodiscard]] std::span<const SeaTurtleHomeRange> Homes() const noexcept;
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
            SeaTurtleState state;
            SeaTurtleHomeRange home;
            float behaviorSeconds = 0.0f;
            float wanderPhase = 0.0f;
            float depthPhase = 0.0f;
            float flipperPhase = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t transitionCount = 0U;
        };

        void InitializeAgent(std::size_t index);
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ,
                         bool observerValid) noexcept;
        [[nodiscard]] bool IsValidHabitat(float x, float z,
                                          const SeaTurtleHomeRange& home) const noexcept;
        [[nodiscard]] float DesiredDepth(const Agent& agent,
                                         float seabedHeight) const noexcept;

        const IslandTerrain& terrain_;
        std::vector<Agent> agents_;
        std::vector<SeaTurtleState> states_;
        std::vector<SeaTurtleHomeRange> homes_;
        float elapsedSeconds_ = 0.0f;
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
