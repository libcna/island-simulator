#include "IslandSimulator/StingrayGroup.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr std::array<float, StingrayGroup::Count> HomeAngles{
            -2.95f, -2.69f, 2.87f, 2.48f, -0.34f};

        [[nodiscard]] float Wrap(float value) noexcept
        {
            while (value > std::numbers::pi_v<float>) value -= TwoPi;
            while (value < -std::numbers::pi_v<float>) value += TwoPi;
            return value;
        }

        [[nodiscard]] float MoveAngle(const float current, const float target,
                                      const float maximumStep) noexcept
        {
            return Wrap(current + std::clamp(Wrap(target - current),
                                             -maximumStep, maximumStep));
        }
    }

    StingrayGroup::StingrayGroup(const IslandTerrain& terrain) : terrain_(terrain)
    {
        agents_.resize(Count);
        states_.resize(Count);
        homes_.resize(Count);
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void StingrayGroup::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const float baseAngle = HomeAngles[index];
        float bestScore = -std::numeric_limits<float>::infinity();
        for (int angleOffset = -6; angleOffset <= 6; ++angleOffset) {
            const float angle = baseAngle + static_cast<float>(angleOffset) * .065f;
            for (const float offshore : {8.0f, 12.0f, 17.0f, 23.0f, 30.0f}) {
                const float radius = terrain_.ShoreRadius(angle) + offshore;
                const float x = std::cos(angle) * radius;
                const float z = std::sin(angle) * radius;
                const TerrainSample sample = terrain_.Sample(x, z);
                if (sample.signedShoreDistance < MinimumShoreDistance
                    || sample.signedShoreDistance > MaximumShoreDistance
                    || sample.height > -.72f || sample.height < -6.2f
                    || sample.surface != TerrainSurface::ShallowSeabed) continue;
                const float shelter = 1.0f - std::min(1.0f,
                    std::fabs(Wrap(angle - std::numbers::pi_v<float>)) / 1.45f);
                const float score = sample.beachSuitability * 1.8f
                    + shelter * .55f - sample.coastalRockiness * .8f
                    - std::fabs(sample.height + 2.2f) * .08f;
                if (score > bestScore) {
                    bestScore = score;
                    agent.home.x = x;
                    agent.home.z = z;
                }
            }
        }
        if (!std::isfinite(bestScore)) {
            throw std::runtime_error("Unable to place stingray on shallow sandy seabed");
        }
        agent.home.radius = 8.5f + static_cast<float>(index % 3U) * 1.6f;
        agent.routePhase = .55f + static_cast<float>(index) * 1.213f;
        agent.flapPhase = static_cast<float>(index) * 1.71f;
        agent.state.x = agent.home.x + std::cos(agent.routePhase) * 2.2f;
        agent.state.z = agent.home.z + std::sin(agent.routePhase) * 2.2f;
        if (!IsSandyHabitat(agent.state.x, agent.state.z, agent.home)) {
            agent.state.x = agent.home.x;
            agent.state.z = agent.home.z;
        }
        agent.state.headingRadians = Wrap(agent.routePhase
            + std::numbers::pi_v<float> * .5f);
        agent.state.scale = .83f + static_cast<float>(index % 4U) * .09f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
        agent.state.behavior = index == 2U ? StingrayBehavior::Settle
            : (index % 2U == 0U ? StingrayBehavior::Forage : StingrayBehavior::Glide);
        agent.behaviorSeconds = 5.0f + static_cast<float>(index) * 2.4f;
        agent.state.speed = agent.state.behavior == StingrayBehavior::Glide ? .62f : .22f;
        agent.state.y = DesiredHeight(agent);
        agent.state.wingWave = std::sin(agent.flapPhase);
    }

    bool StingrayGroup::IsSandyHabitat(const float x, const float z,
                                       const StingrayHomeRange& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        return sample.surface == TerrainSurface::ShallowSeabed
            && sample.signedShoreDistance >= MinimumShoreDistance
            && sample.signedShoreDistance <= MaximumShoreDistance
            && sample.height <= -.72f && sample.height >= -6.2f
            && sample.beachSuitability >= .05f && sample.coastalRockiness <= .72f
            && std::hypot(x - home.x, z - home.z) <= home.radius;
    }

    float StingrayGroup::DesiredHeight(const Agent& agent) const noexcept
    {
        const float seabed = terrain_.Height(agent.state.x, agent.state.z);
        const float clearance = agent.state.behavior == StingrayBehavior::Settle
            ? .10f : (agent.state.behavior == StingrayBehavior::Forage ? .20f : .42f);
        return std::min(-.18f, seabed + clearance * agent.state.scale);
    }

    void StingrayGroup::Update(const float deltaSeconds, const float observerX,
                               const float observerY, const float observerZ) noexcept
    {
        const bool validDelta = std::isfinite(deltaSeconds) && deltaSeconds >= 0.0f;
        const float delta = validDelta ? std::min(deltaSeconds, .1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        elapsedSeconds_ += delta;
        ++frameIndex_;
        fullCount_ = reducedCount_ = dormantCount_ = 0;
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? std::hypot(agent.state.y - observerY,
                    std::hypot(agent.state.x - observerX, agent.state.z - observerZ))
                : std::numeric_limits<float>::infinity();
            agent.state.simulationTier = validDelta
                ? WildlifeActivationPolicy::Classify(
                    distance, FullSimulationDistance, ReducedSimulationDistance)
                : WildlifeSimulationTier::Dormant;
            switch (agent.state.simulationTier) {
            case WildlifeSimulationTier::Full: ++fullCount_; break;
            case WildlifeSimulationTier::Reduced: ++reducedCount_; break;
            case WildlifeSimulationTier::Dormant: ++dormantCount_; break;
            }
            agent.accumulatedSeconds += delta;
            const auto divisor = WildlifeActivationPolicy::UpdateDivisor(
                agent.state.simulationTier);
            if (delta > 0.0f && (frameIndex_ + index) % divisor == 0U) {
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, .5f),
                            observerX, observerZ, observerValid);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }
#ifndef NDEBUG
        assert(fullCount_ + reducedCount_ + dormantCount_ == static_cast<int>(Count));
        for (const StingrayState& ray : states_) {
            assert(std::isfinite(ray.x) && std::isfinite(ray.y) && std::isfinite(ray.z));
            assert(std::isfinite(ray.headingRadians) && std::isfinite(ray.bankRadians));
            assert(std::isfinite(ray.speed) && ray.speed >= 0.0f);
            assert(std::isfinite(ray.wingWave) && std::fabs(ray.wingWave) <= 1.001f);
        }
#endif
    }

    void StingrayGroup::UpdateAgent(Agent& agent, const float deltaSeconds,
                                    const float observerX, const float observerZ,
                                    const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        agent.behaviorSeconds -= deltaSeconds;
        if (agent.behaviorSeconds <= 0.0f) {
            ++agent.transitionCount;
            switch (agent.state.behavior) {
            case StingrayBehavior::Glide:
                agent.state.behavior = agent.transitionCount % 3U == 0U
                    ? StingrayBehavior::Settle : StingrayBehavior::Forage;
                break;
            case StingrayBehavior::Forage:
            case StingrayBehavior::Settle:
                agent.state.behavior = StingrayBehavior::Glide;
                break;
            }
            agent.behaviorSeconds = agent.state.behavior == StingrayBehavior::Glide
                ? 12.0f + static_cast<float>(agent.transitionCount % 5U) * 1.7f
                : 5.5f + static_cast<float>(agent.transitionCount % 4U) * 1.1f;
        }

        const float homeDistance = std::hypot(
            agent.state.x - agent.home.x, agent.state.z - agent.home.z);
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        agent.routePhase = std::fmod(agent.routePhase + deltaSeconds * .12f, TwoPi);
        float desiredHeading = std::atan2(
            agent.home.z + std::sin(agent.routePhase) * 4.4f - agent.state.z,
            agent.home.x + std::cos(agent.routePhase) * 4.4f - agent.state.x);
        if (homeDistance > agent.home.radius * .72f) {
            desiredHeading = std::atan2(agent.home.z - agent.state.z,
                                        agent.home.x - agent.state.x);
        } else if (observerDistance < 3.6f) {
            desiredHeading = std::atan2(agent.state.z - observerZ,
                                        agent.state.x - observerX);
        }
        const float before = agent.state.headingRadians;
        agent.state.headingRadians = MoveAngle(before, desiredHeading, .72f * deltaSeconds);
        const float turn = Wrap(agent.state.headingRadians - before) / deltaSeconds;
        const float targetBank = std::clamp(turn * .22f, -.18f, .18f);
        agent.state.bankRadians += std::clamp(targetBank - agent.state.bankRadians,
                                              -.42f * deltaSeconds, .42f * deltaSeconds);
        const float targetSpeed = agent.state.behavior == StingrayBehavior::Glide
            ? .62f : (agent.state.behavior == StingrayBehavior::Forage ? .24f : .03f);
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
                                        -.36f * deltaSeconds, .36f * deltaSeconds);
        const float candidateX = agent.state.x
            + std::cos(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        const float candidateZ = agent.state.z
            + std::sin(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        if (IsSandyHabitat(candidateX, candidateZ, agent.home)) {
            agent.state.x = candidateX;
            agent.state.z = candidateZ;
        } else {
            agent.state.headingRadians = MoveAngle(
                agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x),
                1.25f * deltaSeconds);
            agent.state.speed = std::min(agent.state.speed, .24f);
        }
        const float desiredY = DesiredHeight(agent);
        agent.state.y += std::clamp(desiredY - agent.state.y,
                                    -.22f * deltaSeconds, .22f * deltaSeconds);
        const float flapRate = agent.state.behavior == StingrayBehavior::Glide ? 2.4f : 1.15f;
        agent.flapPhase = std::fmod(agent.flapPhase + flapRate * deltaSeconds, TwoPi);
        agent.state.wingWave = std::sin(agent.flapPhase);
    }
}
