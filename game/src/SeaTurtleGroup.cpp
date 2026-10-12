#include "IslandSimulator/SeaTurtleGroup.hpp"

#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr std::array<float, SeaTurtleGroup::TurtleCount> HomeAngles = {
            -2.54f, -1.34f, -0.18f, 1.16f, 2.48f};

        [[nodiscard]] float WrapAngle(float value) noexcept
        {
            while (value > std::numbers::pi_v<float>) value -= TwoPi;
            while (value < -std::numbers::pi_v<float>) value += TwoPi;
            return value;
        }

        [[nodiscard]] float MoveAngle(const float current, const float target,
                                      const float maximumStep) noexcept
        {
            const float difference = WrapAngle(target - current);
            return WrapAngle(current + std::clamp(difference, -maximumStep, maximumStep));
        }
    }

    SeaTurtleGroup::SeaTurtleGroup(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        agents_.resize(TurtleCount);
        states_.resize(TurtleCount);
        homes_.resize(TurtleCount);
        for (std::size_t index = 0; index < TurtleCount; ++index) {
            InitializeAgent(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void SeaTurtleGroup::InitializeAgent(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const float baseAngle = HomeAngles[index];
        agent.home.radius = 11.5f + static_cast<float>(index % 3U) * 1.8f;
        float angle = baseAngle;
        float bestScore = -1.0f;
        for (int offset = -4; offset <= 4; ++offset) {
            const float candidateAngle = baseAngle + static_cast<float>(offset) * .10f;
            for (const float offshore : {11.0f, 18.0f, 26.0f, 35.0f}) {
                const float radius = terrain_.ShoreRadius(candidateAngle) + offshore;
                const float x = std::cos(candidateAngle) * radius;
                const float z = std::sin(candidateAngle) * radius;
                const auto sample = terrain_.Sample(x, z);
                if (sample.signedShoreDistance < MinimumShoreDistance
                    || sample.signedShoreDistance > MaximumShoreDistance
                    || sample.height > -.72f) continue;
                const float score = IslandEnvironmentField::AquaticHabitatSuitability(
                    x, z, sample.signedShoreDistance, sample.height);
                if (score > bestScore) {
                    bestScore = score;
                    angle = candidateAngle;
                    agent.home.x = x;
                    agent.home.z = z;
                }
            }
        }

        agent.state.x = agent.home.x;
        agent.state.z = agent.home.z;
        agent.state.headingRadians = WrapAngle(
            angle + (index % 2U == 0U ? 1.0f : -1.0f)
                * std::numbers::pi_v<float> * 0.5f);
        agent.state.scale = 0.86f + static_cast<float>(index % 4U) * 0.075f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
        agent.state.behavior = index == 2U
            ? SeaTurtleBehavior::Surface
            : (index % 2U == 0U ? SeaTurtleBehavior::Forage
                                 : SeaTurtleBehavior::Cruise);
        agent.behaviorSeconds = 4.5f + static_cast<float>(index) * 2.1f;
        agent.wanderPhase = std::fmod(static_cast<float>(index) * 1.731f + 0.4f, TwoPi);
        agent.depthPhase = std::fmod(static_cast<float>(index) * 2.219f + 0.8f, TwoPi);
        agent.flipperPhase = std::fmod(static_cast<float>(index) * 1.377f, TwoPi);
        const float seabed = terrain_.Height(agent.state.x, agent.state.z);
        agent.state.y = DesiredDepth(agent, seabed);
        agent.state.speed = agent.state.behavior == SeaTurtleBehavior::Cruise ? 0.72f : 0.28f;
        agent.state.flipperStroke = std::sin(agent.flipperPhase);
    }

    bool SeaTurtleGroup::IsValidHabitat(
        const float x, const float z, const SeaTurtleHomeRange& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        const float homeDistance = std::hypot(x - home.x, z - home.z);
        return sample.signedShoreDistance >= MinimumShoreDistance
            && sample.signedShoreDistance <= MaximumShoreDistance
            && sample.height <= -0.72f && homeDistance <= home.radius
            && IslandEnvironmentField::AquaticHabitatSuitability(
                x, z, sample.signedShoreDistance, sample.height) >= .025f;
    }

    float SeaTurtleGroup::DesiredDepth(const Agent& agent,
                                       const float seabedHeight) const noexcept
    {
        const float lower = seabedHeight + 0.48f * agent.state.scale;
        constexpr float upper = -0.11f;
        if (agent.state.behavior == SeaTurtleBehavior::Surface) {
            return std::max(lower, upper);
        }
        if (agent.state.behavior == SeaTurtleBehavior::Forage) {
            return std::min(upper, lower + 0.18f);
        }
        const float cruiseDepth = -1.12f
            + std::sin(elapsedSeconds_ * 0.22f + agent.depthPhase) * 0.28f;
        return lower <= upper ? std::clamp(cruiseDepth, lower, upper) : upper;
    }

    void SeaTurtleGroup::Update(const float deltaSeconds, const float observerX,
                                const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        elapsedSeconds_ += safeDelta;
        ++frameIndex_;
        fullSimulationCount_ = 0;
        reducedSimulationCount_ = 0;
        dormantSimulationCount_ = 0;

        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? std::hypot(agent.state.y - observerY,
                             std::hypot(agent.state.x - observerX,
                                        agent.state.z - observerZ))
                : std::numeric_limits<float>::infinity();
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
            switch (agent.state.simulationTier) {
            case WildlifeSimulationTier::Full: ++fullSimulationCount_; break;
            case WildlifeSimulationTier::Reduced: ++reducedSimulationCount_; break;
            case WildlifeSimulationTier::Dormant: ++dormantSimulationCount_; break;
            }

            agent.accumulatedSeconds += safeDelta;
            const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(
                agent.state.simulationTier);
            if ((frameIndex_ + index) % divisor == 0U) {
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, 0.5f),
                            observerX, observerZ, observerValid);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }

#ifndef NDEBUG
        assert(fullSimulationCount_ + reducedSimulationCount_ + dormantSimulationCount_
               == static_cast<int>(TurtleCount));
        for (const SeaTurtleState& turtle : states_) {
            assert(std::isfinite(turtle.x) && std::isfinite(turtle.y)
                   && std::isfinite(turtle.z));
            assert(std::isfinite(turtle.headingRadians));
            assert(std::isfinite(turtle.pitchRadians)
                   && std::fabs(turtle.pitchRadians) <= 0.241f);
            assert(std::isfinite(turtle.bankRadians)
                   && std::fabs(turtle.bankRadians) <= 0.221f);
            assert(std::isfinite(turtle.speed) && turtle.speed >= 0.0f);
            assert(std::isfinite(turtle.flipperStroke)
                   && std::fabs(turtle.flipperStroke) <= 1.001f);
            assert(std::isfinite(turtle.scale) && turtle.scale > 0.0f);
        }
#endif
    }

    void SeaTurtleGroup::UpdateAgent(Agent& agent, const float deltaSeconds,
                                     const float observerX, const float observerZ,
                                     const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;

        agent.behaviorSeconds -= deltaSeconds;
        if (agent.behaviorSeconds <= 0.0f) {
            ++agent.transitionCount;
            switch (agent.state.behavior) {
            case SeaTurtleBehavior::Cruise:
                agent.state.behavior = agent.transitionCount % 4U == 0U
                    ? SeaTurtleBehavior::Surface : SeaTurtleBehavior::Forage;
                agent.behaviorSeconds = agent.state.behavior == SeaTurtleBehavior::Surface
                    ? 4.8f + static_cast<float>(agent.transitionCount % 3U) * 0.9f
                    : 7.0f + static_cast<float>(agent.transitionCount % 5U) * 1.2f;
                break;
            case SeaTurtleBehavior::Forage:
                agent.state.behavior = SeaTurtleBehavior::Cruise;
                agent.behaviorSeconds = 13.0f
                    + static_cast<float>(agent.transitionCount % 6U) * 1.7f;
                break;
            case SeaTurtleBehavior::Surface:
                agent.state.behavior = SeaTurtleBehavior::Cruise;
                agent.behaviorSeconds = 15.0f
                    + static_cast<float>(agent.transitionCount % 5U) * 2.0f;
                break;
            }
        }

        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        float desiredHeading = agent.state.headingRadians
            + std::sin(elapsedSeconds_ * 0.19f + agent.wanderPhase) * 0.42f;
        const float homeDistance = std::hypot(
            agent.state.x - agent.home.x, agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * 0.67f) {
            desiredHeading = std::atan2(
                agent.home.z - agent.state.z, agent.home.x - agent.state.x);
        } else if (observerDistance < 4.0f) {
            desiredHeading = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        }
        // A non-circular coast can put the initial tangent against a shallow bank.
        // Steer before reaching it; at the home centre there is no homeward direction,
        // so use the offshore radial direction instead of atan2(0, 0).
        if (!IsValidHabitat(agent.state.x + std::cos(desiredHeading),
                            agent.state.z + std::sin(desiredHeading), agent.home)) {
            desiredHeading = homeDistance > 1.0f
                ? std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x)
                : std::atan2(agent.state.z, agent.state.x);
        }
        const float headingBefore = agent.state.headingRadians;
        agent.state.headingRadians = MoveAngle(
            agent.state.headingRadians, desiredHeading, deltaSeconds * 0.52f);
        const float headingChange = WrapAngle(agent.state.headingRadians - headingBefore);
        const float targetBank = std::clamp(headingChange / deltaSeconds * 0.32f,
                                            -0.22f, 0.22f);
        agent.state.bankRadians += std::clamp(
            targetBank - agent.state.bankRadians,
            -deltaSeconds * 0.42f, deltaSeconds * 0.42f);

        const float targetSpeed = agent.state.behavior == SeaTurtleBehavior::Cruise
            ? 0.72f + static_cast<float>(agent.state.colorVariant) * 0.055f
            : (agent.state.behavior == SeaTurtleBehavior::Surface ? 0.42f : 0.24f);
        agent.state.speed += std::clamp(
            targetSpeed - agent.state.speed, -deltaSeconds * 0.34f, deltaSeconds * 0.34f);
        const float candidateX = agent.state.x
            + std::cos(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        const float candidateZ = agent.state.z
            + std::sin(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        if (IsValidHabitat(candidateX, candidateZ, agent.home)) {
            agent.state.x = candidateX;
            agent.state.z = candidateZ;
        } else {
            agent.state.headingRadians = MoveAngle(
                agent.state.headingRadians,
                desiredHeading,
                deltaSeconds * 1.25f);
            agent.state.speed = std::min(agent.state.speed, 0.34f);
        }

        const float seabed = terrain_.Height(agent.state.x, agent.state.z);
        const float targetDepth = DesiredDepth(agent, seabed);
        const float previousY = agent.state.y;
        const float verticalRate = agent.state.behavior == SeaTurtleBehavior::Surface
            ? 0.42f : 0.26f;
        agent.state.y += std::clamp(
            targetDepth - agent.state.y, -verticalRate * deltaSeconds,
            verticalRate * deltaSeconds);
        const float verticalSpeed = (agent.state.y - previousY) / deltaSeconds;
        const float targetPitch = std::clamp(
            std::atan2(verticalSpeed, std::max(0.18f, agent.state.speed)), -0.24f, 0.24f);
        agent.state.pitchRadians += std::clamp(
            targetPitch - agent.state.pitchRadians,
            -deltaSeconds * 0.36f, deltaSeconds * 0.36f);

        const float flipperRate = agent.state.behavior == SeaTurtleBehavior::Forage
            ? 1.7f : (agent.state.behavior == SeaTurtleBehavior::Surface ? 2.5f : 3.2f);
        agent.flipperPhase = std::fmod(
            agent.flipperPhase + deltaSeconds * flipperRate, TwoPi);
        agent.state.flipperStroke = std::sin(agent.flipperPhase);
    }

    std::span<const SeaTurtleState> SeaTurtleGroup::Turtles() const noexcept
    {
        return states_;
    }

    std::span<const SeaTurtleHomeRange> SeaTurtleGroup::Homes() const noexcept
    {
        return homes_;
    }
}
