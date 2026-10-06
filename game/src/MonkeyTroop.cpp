#include "IslandSimulator/MonkeyTroop.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr float GoldenAngle = 2.39996322972865332f;
        constexpr float MinimumLandFromShore = 48.0f;
        constexpr float MaximumSlope = 0.46f;

        [[nodiscard]] float WrapAngle(float value) noexcept
        {
            while (value > std::numbers::pi_v<float>) {
                value -= TwoPi;
            }
            while (value < -std::numbers::pi_v<float>) {
                value += TwoPi;
            }
            return value;
        }

        [[nodiscard]] float TurnToward(const float current, const float target,
                                       const float maximumStep) noexcept
        {
            return WrapAngle(current + std::clamp(
                WrapAngle(target - current), -maximumStep, maximumStep));
        }

        [[nodiscard]] float UnitSequence(const std::uint32_t value) noexcept
        {
            std::uint32_t mixed = value * 0x9e3779b9U + 0x85ebca6bU;
            mixed ^= mixed >> 16U;
            mixed *= 0x7feb352dU;
            mixed ^= mixed >> 15U;
            return static_cast<float>(mixed & 0x00ffffffU)
                / static_cast<float>(0x01000000U);
        }
    }

    MonkeyTroop::MonkeyTroop(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        agents_.resize(MonkeyCount);
        states_.resize(MonkeyCount);
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            InitializeAgent(index);
            states_[index] = agents_[index].state;
        }
    }

    void MonkeyTroop::SetObstacles(const std::span<const CircularObstacle> obstacles)
    {
        obstacles_.assign(obstacles.begin(), obstacles.end());
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            if (!IsValidGround(agents_[index].state.x, agents_[index].state.z)) {
                agents_[index] = Agent{};
                InitializeAgent(index);
            }
            states_[index] = agents_[index].state;
        }
    }

    bool MonkeyTroop::IsClearOfObstacles(const float x, const float z) const noexcept
    {
        for (const CircularObstacle& obstacle : obstacles_) {
            const float clearance = CollisionRadius + std::max(0.0f, obstacle.radius);
            const float separationX = x - obstacle.x;
            const float separationZ = z - obstacle.z;
            if (separationX * separationX + separationZ * separationZ
                < clearance * clearance) {
                return false;
            }
        }
        return true;
    }

    bool MonkeyTroop::IsValidGround(const float x, const float z) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        return sample.signedShoreDistance >= MinimumLandFromShore
            && sample.height >= 2.8f && sample.slopeRadians <= MaximumSlope
            && !VegetationLayout::IsReservedRoute(x, z)
            && IsClearOfObstacles(x, z);
    }

    void MonkeyTroop::InitializeAgent(const std::size_t index) noexcept
    {
        Agent& agent = agents_[index];
        const float baseAngle = 0.31f + static_cast<float>(index) * GoldenAngle;
        const float baseRadius = 72.0f + static_cast<float>(index % 3U) * 29.0f;
        bool found = false;
        for (int attempt = 0; attempt < 80; ++attempt) {
            const float angle = baseAngle + static_cast<float>(attempt) * 0.43f;
            const float radius = baseRadius
                + static_cast<float>((attempt % 9) - 4) * 3.5f;
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            if (IsValidGround(x, z)) {
                agent.state.homeX = x;
                agent.state.homeZ = z;
                found = true;
                break;
            }
        }
        if (!found) {
            agent.state.homeX = std::cos(baseAngle) * 58.0f;
            agent.state.homeZ = std::sin(baseAngle) * 58.0f;
        }

        agent.state.x = agent.state.homeX;
        agent.state.z = agent.state.homeZ;
        agent.state.targetX = agent.state.homeX;
        agent.state.targetZ = agent.state.homeZ;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z);
        agent.state.headingRadians = WrapAngle(baseAngle + std::numbers::pi_v<float> * 0.5f);
        agent.state.scale = 0.86f + static_cast<float>(index % 5U) * 0.055f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.decisionCounter = static_cast<std::uint32_t>(index * 7U + 3U);

        constexpr std::array<MonkeyBehavior, 5> InitialBehaviors = {
            MonkeyBehavior::Idle, MonkeyBehavior::Observe, MonkeyBehavior::Wander,
            MonkeyBehavior::Walk, MonkeyBehavior::Idle};
        EnterState(agent, InitialBehaviors[index % InitialBehaviors.size()]);
        if (agent.state.behavior == MonkeyBehavior::Wander
            || agent.state.behavior == MonkeyBehavior::Walk) {
            ChooseWanderTarget(agent);
            agent.state.behavior = MonkeyBehavior::Walk;
            agent.stateSecondsRemaining = 14.0f;
        }
    }

    void MonkeyTroop::EnterState(Agent& agent, const MonkeyBehavior behavior) noexcept
    {
        agent.state.behavior = behavior;
        ++agent.decisionCounter;
        const float variation = UnitSequence(agent.decisionCounter) * 2.4f;
        switch (behavior) {
        case MonkeyBehavior::Idle:
            agent.stateSecondsRemaining = 3.8f + variation;
            agent.state.speed = 0.0f;
            break;
        case MonkeyBehavior::Observe:
            agent.stateSecondsRemaining = 2.8f + variation;
            agent.state.speed = 0.0f;
            break;
        case MonkeyBehavior::Wander:
            agent.stateSecondsRemaining = 0.18f;
            agent.state.speed = 0.0f;
            break;
        case MonkeyBehavior::Walk:
            agent.stateSecondsRemaining = 10.0f + variation * 2.0f;
            agent.state.speed = 0.92f + UnitSequence(agent.decisionCounter + 13U) * 0.30f;
            break;
        case MonkeyBehavior::RunAway:
            agent.stateSecondsRemaining = 4.2f + variation * 0.55f;
            agent.state.speed = 2.25f + UnitSequence(agent.decisionCounter + 29U) * 0.38f;
            break;
        }
    }

    void MonkeyTroop::ChooseWanderTarget(Agent& agent) noexcept
    {
        for (int attempt = 0; attempt < 24; ++attempt) {
            const std::uint32_t sequence = agent.decisionCounter * 31U
                + static_cast<std::uint32_t>(attempt) * 17U;
            const float angle = UnitSequence(sequence) * TwoPi;
            const float distance = 8.0f + UnitSequence(sequence + 5U) * 20.0f;
            const float x = agent.state.homeX + std::cos(angle) * distance;
            const float z = agent.state.homeZ + std::sin(angle) * distance;
            if (IsValidGround(x, z)) {
                agent.state.targetX = x;
                agent.state.targetZ = z;
                return;
            }
        }
        agent.state.targetX = agent.state.homeX;
        agent.state.targetZ = agent.state.homeZ;
    }

    void MonkeyTroop::ChooseEscapeTarget(Agent& agent, const float observerX,
                                         const float observerZ) noexcept
    {
        float awayX = agent.state.x - observerX;
        float awayZ = agent.state.z - observerZ;
        const float length = std::hypot(awayX, awayZ);
        if (length > 0.001f) {
            awayX /= length;
            awayZ /= length;
        } else {
            const float angle = UnitSequence(agent.decisionCounter + 71U) * TwoPi;
            awayX = std::cos(angle);
            awayZ = std::sin(angle);
        }
        for (int attempt = 0; attempt < 18; ++attempt) {
            const float turn = (static_cast<float>(attempt) - 8.5f) * 0.10f;
            const float candidateX = awayX * std::cos(turn) - awayZ * std::sin(turn);
            const float candidateZ = awayX * std::sin(turn) + awayZ * std::cos(turn);
            const float distance = 16.0f + static_cast<float>(attempt % 4) * 2.5f;
            const float x = agent.state.x + candidateX * distance;
            const float z = agent.state.z + candidateZ * distance;
            if (std::hypot(x - agent.state.homeX, z - agent.state.homeZ)
                    <= MaximumHomeDistance
                && IsValidGround(x, z)) {
                agent.state.targetX = x;
                agent.state.targetZ = z;
                return;
            }
        }
        agent.state.targetX = agent.state.homeX;
        agent.state.targetZ = agent.state.homeZ;
    }

    bool MonkeyTroop::TryMove(Agent& agent, const float headingRadians,
                              const float distance) noexcept
    {
        const float proposedX = agent.state.x + std::cos(headingRadians) * distance;
        const float proposedZ = agent.state.z + std::sin(headingRadians) * distance;
        if (std::hypot(proposedX - agent.state.homeX, proposedZ - agent.state.homeZ)
                > MaximumHomeDistance
            || !IsValidGround(proposedX, proposedZ)) {
            return false;
        }
        agent.state.x = proposedX;
        agent.state.z = proposedZ;
        agent.state.headingRadians = WrapAngle(headingRadians);
        return true;
    }

    void MonkeyTroop::Update(const float deltaSeconds, const float observerX,
                             const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
        ++frameIndex_;
        fullSimulationCount_ = 0;
        reducedSimulationCount_ = 0;
        dormantSimulationCount_ = 0;

        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float distance = std::hypot(agent.state.y - observerY,
                std::hypot(agent.state.x - observerX, agent.state.z - observerZ));
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
            switch (agent.state.simulationTier) {
            case WildlifeSimulationTier::Full:
                ++fullSimulationCount_;
                break;
            case WildlifeSimulationTier::Reduced:
                ++reducedSimulationCount_;
                break;
            case WildlifeSimulationTier::Dormant:
                ++dormantSimulationCount_;
                break;
            }

            agent.accumulatedSeconds += safeDelta;
            const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(
                agent.state.simulationTier);
            if ((frameIndex_ + index) % divisor == 0U) {
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, 0.5f),
                            observerX, observerZ);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }
#ifndef NDEBUG
        assert(states_.size() == MonkeyCount);
        assert(fullSimulationCount_ + reducedSimulationCount_ + dormantSimulationCount_
               == static_cast<int>(MonkeyCount));
        for (const MonkeyState& state : states_) {
            assert(std::isfinite(state.x) && std::isfinite(state.y)
                   && std::isfinite(state.z) && std::isfinite(state.headingRadians));
            assert(std::isfinite(state.headLookRadians)
                   && std::fabs(state.headLookRadians) <= MaximumHeadLookRadians + 0.001f);
            assert(std::isfinite(state.speed) && state.speed >= 0.0f);
            assert(std::isfinite(state.animationSeconds) && state.animationSeconds >= 0.0f);
            assert(std::isfinite(state.scale) && state.scale > 0.0f);
            assert(std::isfinite(state.homeX) && std::isfinite(state.homeZ)
                   && std::isfinite(state.targetX) && std::isfinite(state.targetZ));
            assert(std::hypot(state.x - state.homeX, state.z - state.homeZ)
                   <= MaximumHomeDistance + 0.001f);
            assert(std::hypot(state.targetX - state.homeX, state.targetZ - state.homeZ)
                   <= MaximumHomeDistance + 0.001f);
            assert(state.colorVariant < 4U);
        }
#endif
    }

    void MonkeyTroop::UpdateAgent(Agent& agent, const float deltaSeconds,
                                  const float observerX, const float observerZ) noexcept
    {
        if (deltaSeconds <= 0.0f) {
            return;
        }
        agent.stateSecondsRemaining -= deltaSeconds;
        agent.state.animationSeconds += deltaSeconds;

        const float observerDistance = std::hypot(
            agent.state.x - observerX, agent.state.z - observerZ);
        if (observerDistance < 7.5f && agent.state.behavior != MonkeyBehavior::RunAway) {
            EnterState(agent, MonkeyBehavior::RunAway);
            ChooseEscapeTarget(agent, observerX, observerZ);
        }

        float desiredHeadLook = 0.0f;
        if (observerDistance >= 7.5f && observerDistance < 30.0f
            && (agent.state.behavior == MonkeyBehavior::Idle
                || agent.state.behavior == MonkeyBehavior::Observe)) {
            const float observerHeading = std::atan2(
                observerZ - agent.state.z, observerX - agent.state.x);
            const float proximity = std::clamp(
                (30.0f - observerDistance) / 14.0f, 0.0f, 1.0f);
            desiredHeadLook = std::clamp(
                WrapAngle(observerHeading - agent.state.headingRadians),
                -MaximumHeadLookRadians, MaximumHeadLookRadians) * proximity;
        }
        agent.state.headLookRadians += std::clamp(
            desiredHeadLook - agent.state.headLookRadians,
            -0.75f * deltaSeconds, 0.75f * deltaSeconds);

        if (agent.state.behavior == MonkeyBehavior::Wander) {
            ChooseWanderTarget(agent);
            EnterState(agent, MonkeyBehavior::Walk);
        } else if (agent.state.behavior == MonkeyBehavior::Walk
                   || agent.state.behavior == MonkeyBehavior::RunAway) {
            const float toTargetX = agent.state.targetX - agent.state.x;
            const float toTargetZ = agent.state.targetZ - agent.state.z;
            const float targetDistance = std::hypot(toTargetX, toTargetZ);
            if (targetDistance > 0.05f) {
                const float desiredHeading = std::atan2(toTargetZ, toTargetX);
                const float turnRate = agent.state.behavior == MonkeyBehavior::RunAway
                    ? 3.0f : 1.9f;
                agent.state.headingRadians = TurnToward(
                    agent.state.headingRadians, desiredHeading, turnRate * deltaSeconds);
                const float stepDistance = agent.state.speed * deltaSeconds;
                const float directX = agent.state.x
                    + std::cos(agent.state.headingRadians) * stepDistance;
                const float directZ = agent.state.z
                    + std::sin(agent.state.headingRadians) * stepDistance;
                const bool directObstacleBlocked = !IsClearOfObstacles(directX, directZ);
                if (!TryMove(agent, agent.state.headingRadians, stepDistance)) {
                    const float preferredSide = (agent.decisionCounter & 1U) == 0U ? 1.0f : -1.0f;
                    constexpr std::array<float, 4> DeflectionAngles = {
                        0.48f, -0.48f, 0.92f, -0.92f};
                    bool moved = false;
                    for (const float deflection : DeflectionAngles) {
                        const float candidateHeading = agent.state.headingRadians
                            + deflection * preferredSide;
                        if (TryMove(agent, candidateHeading, stepDistance)) {
                            if (directObstacleBlocked) {
                                ++obstacleDeflectionCount_;
                            }
                            moved = true;
                            break;
                        }
                    }
                    if (!moved) {
                        ++agent.decisionCounter;
                        agent.stateSecondsRemaining = 0.0f;
                    }
                }
            }
            if (targetDistance < 0.85f || agent.stateSecondsRemaining <= 0.0f) {
                EnterState(agent, agent.state.behavior == MonkeyBehavior::RunAway
                    ? MonkeyBehavior::Observe : MonkeyBehavior::Idle);
            }
        } else if (agent.stateSecondsRemaining <= 0.0f) {
            if (agent.state.behavior == MonkeyBehavior::Idle) {
                EnterState(agent, (agent.decisionCounter % 3U) == 0U
                    ? MonkeyBehavior::Observe : MonkeyBehavior::Wander);
            } else {
                EnterState(agent, MonkeyBehavior::Wander);
            }
        }

        agent.state.y = terrain_.Height(agent.state.x, agent.state.z);
    }

    std::span<const MonkeyState> MonkeyTroop::Monkeys() const noexcept
    {
        return states_;
    }
}
