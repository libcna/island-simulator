#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"
#include "IslandSimulator/WorldCollision.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Compact autonomous behaviour states used by the island monkeys. */
    enum class MonkeyBehavior : std::uint8_t
    {
        Idle,
        Observe,
        Wander,
        Walk,
        RunAway
    };

    /** @brief Renderer-independent pose and animation state for one monkey. */
    struct MonkeyState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float headLookRadians = 0.0f;
        float speed = 0.0f;
        float animationSeconds = 0.0f;
        float scale = 1.0f;
        float homeX = 0.0f;
        float homeZ = 0.0f;
        float targetX = 0.0f;
        float targetZ = 0.0f;
        MonkeyBehavior behavior = MonkeyBehavior::Idle;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Deterministic small troop constrained to traversable interior terrain. */
    class MonkeyTroop final
    {
    public:
        static constexpr std::size_t MonkeyCount = 7U;
        static constexpr float FullSimulationDistance = 95.0f;
        static constexpr float ReducedSimulationDistance = 245.0f;
        static constexpr float CollisionRadius = 0.32f;
        static constexpr float MaximumHomeDistance = 34.0f;
        static constexpr float MaximumHeadLookRadians = 0.34f;

        explicit MonkeyTroop(const IslandTerrain& terrain);

        /** @brief Installs solid world proxies and relocates any newly obstructed agents. */
        void SetObstacles(std::span<const CircularObstacle> obstacles);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const MonkeyState> Monkeys() const noexcept;
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullSimulationCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept {
            return reducedSimulationCount_;
        }
        [[nodiscard]] int DormantSimulationCount() const noexcept {
            return dormantSimulationCount_;
        }
        [[nodiscard]] std::uint64_t ObstacleDeflectionCount() const noexcept {
            return obstacleDeflectionCount_;
        }

    private:
        struct Agent
        {
            MonkeyState state;
            float stateSecondsRemaining = 0.0f;
            float accumulatedSeconds = 0.0f;
            std::uint32_t decisionCounter = 0U;
        };

        void InitializeAgent(std::size_t index) noexcept;
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ) noexcept;
        void EnterState(Agent& agent, MonkeyBehavior behavior) noexcept;
        void ChooseWanderTarget(Agent& agent) noexcept;
        void ChooseEscapeTarget(Agent& agent, float observerX, float observerZ) noexcept;
        [[nodiscard]] bool TryMove(Agent& agent, float headingRadians,
                                   float distance) noexcept;
        [[nodiscard]] bool IsClearOfObstacles(float x, float z) const noexcept;
        [[nodiscard]] bool IsValidGround(float x, float z) const noexcept;

        const IslandTerrain& terrain_;
        std::vector<CircularObstacle> obstacles_;
        std::vector<Agent> agents_;
        std::vector<MonkeyState> states_;
        std::uint64_t frameIndex_ = 0U;
        std::uint64_t obstacleDeflectionCount_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
