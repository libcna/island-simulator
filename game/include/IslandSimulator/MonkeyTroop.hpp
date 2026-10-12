#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <memory>
#include "IslandSimulator/MonkeyTreeExcursion.hpp"

#include "IslandSimulator/WildlifeSimulation.hpp"
#include "IslandSimulator/WorldCollision.hpp"
#include "IslandSimulator/JungleLayout.hpp"

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
        RunAway,
        ApproachTree,
        InspectTree
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
        float gaitPhase = 0.0f;
        float gaitAmount = 0.0f;
        float scale = 1.0f;
        float homeX = 0.0f;
        float homeZ = 0.0f;
        float targetX = 0.0f;
        float targetZ = 0.0f;
        // Index into the installed canopy placements; -1 when no tree is selected.
        int treeIndex = -1;
        CanopyPlacement selectedTree{};
        float treeInspectionSeconds = 0.0f;
        float treeInspectionRemaining = 0.0f;
        std::shared_ptr<const MonkeyTreeExcursion::Plan> treeExcursion;
        MonkeyTreeExcursion::Clock treeClock;
        bool treeReleasing = false;
        bool treePreparing = false;
        MonkeyBehavior behavior = MonkeyBehavior::Idle;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0U;
    };

    inline MonkeyGroundGait::Point MonkeyVisualCentre(const MonkeyState& monkey)
    {
        if (!monkey.treeExcursion) return {monkey.x,monkey.y+.72f*monkey.scale,monkey.z};
        const auto& plan=*monkey.treeExcursion;
        const auto pose=MonkeyTreeExcursion::Sample(plan,monkey.treeClock.seconds);
        return plan.frame.World(MonkeyTreeAttachment::Transform(
            {0,.72f+plan.attachment.ground.bodyOffset,0},plan.attachment.pivot,pose.body.frame));
    }

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
        void SetCanopy(std::span<const CanopyPlacement> trees);
        void SetClimbingEnabled(bool enabled) noexcept { climbingEnabled_=enabled; }
        [[nodiscard]] static bool IsClosedCanopyTree(
            std::span<const CanopyPlacement> trees, std::size_t index) noexcept;
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ, float windTime = -1.0f) noexcept;

        [[nodiscard]] std::span<const MonkeyState> Monkeys() const noexcept;
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullSimulationCount_; }
        [[nodiscard]] int PreflightSamplesLastUpdate() const noexcept { return preflightSamples_; }
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
            bool treeClimbAttempted = false;
            std::shared_ptr<const MonkeyTreeExcursion::Plan> pendingExcursion;
            int preflightStep = 0;
        };

        void InitializeAgent(std::size_t index) noexcept;
        void UpdateAgent(Agent& agent, float deltaSeconds,
                         float observerX, float observerZ) noexcept;
        void EnterState(Agent& agent, MonkeyBehavior behavior) noexcept;
        void ChooseWanderTarget(Agent& agent) noexcept;
        [[nodiscard]] bool ChooseTreeTarget(Agent& agent) noexcept;
        void ChooseEscapeTarget(Agent& agent, float observerX, float observerZ) noexcept;
        [[nodiscard]] bool TryMove(Agent& agent, float headingRadians,
                                   float distance) noexcept;
        [[nodiscard]] bool IsClearOfObstacles(float x, float z) const noexcept;
        [[nodiscard]] bool IsClearSegment(float fromX, float fromZ, float toX, float toZ) const noexcept;
        [[nodiscard]] bool IsValidGround(float x, float z) const noexcept;

        const IslandTerrain& terrain_;
        std::shared_ptr<const RenderedTerrainSurface> renderedTerrain_;
        float windTime_ = 0;
        bool climbingEnabled_ = true;
        int preflightSamples_ = 0;
        std::vector<CircularObstacle> obstacles_;
        std::vector<CanopyPlacement> trees_;
        std::vector<std::uint8_t> closedCanopyTree_;
        std::vector<Agent> agents_;
        std::vector<MonkeyState> states_;
        std::uint64_t frameIndex_ = 0U;
        std::uint64_t obstacleDeflectionCount_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
