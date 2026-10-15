#include "IslandSimulator/HermitCrabGroup.hpp"

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
        constexpr float Pi = std::numbers::pi_v<float>;
        constexpr float TwoPi = 2.0f * Pi;
        constexpr std::array<float, HermitCrabGroup::Count> HomeAngles = {{
            -2.82f, -2.30f, -1.76f, -1.18f, -.52f,
             .10f,   .73f,   1.38f,  2.04f, 2.66f}};

        [[nodiscard]] float WrapAngle(float angle) noexcept
        {
            while (angle > Pi) angle -= TwoPi;
            while (angle < -Pi) angle += TwoPi;
            return angle;
        }

        [[nodiscard]] float MoveAngle(const float current, const float target,
                                      const float step) noexcept
        {
            return WrapAngle(current
                + std::clamp(WrapAngle(target - current), -step, step));
        }
    }

    HermitCrabGroup::HermitCrabGroup(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void HermitCrabGroup::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        agent.home.radius = 3.8f + static_cast<float>(index % 3U) * .45f;
        bool found = false;
        float angle = HomeAngles[index];
        for (int attempt = 0; attempt < 96 && !found; ++attempt) {
            const float side = attempt % 2 == 0 ? 1.0f : -1.0f;
            angle = HomeAngles[index] + side
                * static_cast<float>((attempt + 1) / 2) * .022f;
            const float radius = terrain_.ShoreRadius(angle)
                - (4.5f + static_cast<float>(index % 4U) * .72f);
            agent.home.x = std::cos(angle) * radius;
            agent.home.z = std::sin(angle) * radius;
            found = IsValid(agent.home.x, agent.home.z, agent.home);
        }
        if (!found) throw std::runtime_error("No sandy habitat for hermit crab");

        const float offsetAngle = static_cast<float>(index) * 2.39996323f;
        const float offsetRadius = .35f + static_cast<float>(index % 4U) * .22f;
        agent.state.x = agent.home.x + std::cos(offsetAngle) * offsetRadius;
        agent.state.z = agent.home.z + std::sin(offsetAngle) * offsetRadius;
        if (!IsValid(agent.state.x, agent.state.z, agent.home)) {
            agent.state.x = agent.home.x;
            agent.state.z = agent.home.z;
        }
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .038f;
        agent.state.headingRadians = WrapAngle(angle + Pi * .5f
            + static_cast<float>(static_cast<int>(index % 3U) - 1) * .18f);
        agent.state.scale = .42f + static_cast<float>(index % 4U) * .025f;
        agent.state.shellVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 3U == 0U
            ? HermitCrabBehavior::Pause : HermitCrabBehavior::Forage;
        agent.behaviorSeconds = 1.4f + static_cast<float>(index % 6U) * .53f;
        agent.steeringPhase = .3f + static_cast<float>(index) * 1.47f;
        agent.state.antennaPhase = static_cast<float>(index) * .71f;
    }

    bool HermitCrabGroup::IsValid(const float x, const float z,
                                  const HermitCrabHome& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        const bool sandy = sample.surface == TerrainSurface::DrySand
            || sample.surface == TerrainSurface::WetSand
            || sample.surface == TerrainSurface::CoastalGround;
        return sandy && sample.height >= -.01f
            && sample.signedShoreDistance >= MinimumShoreDistance
            && sample.signedShoreDistance <= MaximumShoreDistance
            && sample.slopeRadians < .34f
            && std::hypot(x - home.x, z - home.z) <= home.radius;
    }

    void HermitCrabGroup::Update(const float deltaSeconds,
                                 const float observerX, const float observerY,
                                 const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, .1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX)
            && std::isfinite(observerY) && std::isfinite(observerZ);
        ++frameIndex_;
        fullCount_ = reducedCount_ = dormantCount_ = 0;
        for (std::size_t index = 0; index < Count; ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? std::hypot(agent.state.y - observerY,
                    std::hypot(agent.state.x - observerX, agent.state.z - observerZ))
                : std::numeric_limits<float>::infinity();
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
            switch (agent.state.simulationTier) {
            case WildlifeSimulationTier::Full: ++fullCount_; break;
            case WildlifeSimulationTier::Reduced: ++reducedCount_; break;
            case WildlifeSimulationTier::Dormant: ++dormantCount_; break;
            }
            agent.accumulatedSeconds += safeDelta;
            const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(
                agent.state.simulationTier);
            if ((frameIndex_ + index) % divisor == 0U) {
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, .5f),
                            observerX, observerZ, observerValid);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }
        assert(fullCount_ + reducedCount_ + dormantCount_ == static_cast<int>(Count));
    }

    void HermitCrabGroup::UpdateAgent(Agent& agent, const float deltaSeconds,
                                      const float observerX, const float observerZ,
                                      const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 3.5f
            && agent.state.behavior != HermitCrabBehavior::Retreat) {
            agent.state.behavior = HermitCrabBehavior::Retreat;
            agent.behaviorSeconds = 1.25f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitionCount;
            if (agent.state.behavior == HermitCrabBehavior::Pause) {
                agent.state.behavior = HermitCrabBehavior::Forage;
                agent.behaviorSeconds = 3.2f
                    + static_cast<float>(agent.transitionCount % 5U) * .58f;
            } else {
                agent.state.behavior = HermitCrabBehavior::Pause;
                agent.behaviorSeconds = 1.1f
                    + static_cast<float>(agent.transitionCount % 4U) * .44f;
            }
        }

        agent.steeringPhase = std::fmod(
            agent.steeringPhase + deltaSeconds * .67f, TwoPi);
        const float targetSpeed = agent.state.behavior == HermitCrabBehavior::Pause
            ? 0.0f : (agent.state.behavior == HermitCrabBehavior::Retreat ? .82f : .22f);
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
                                        -deltaSeconds * 1.7f,
                                        deltaSeconds * 2.4f);
        const float homeDistance = std::hypot(
            agent.state.x - agent.home.x, agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * .70f) {
            agent.state.headingRadians = MoveAngle(agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z,
                           agent.home.x - agent.state.x), deltaSeconds * 2.5f);
        } else if (agent.state.behavior == HermitCrabBehavior::Forage) {
            agent.state.headingRadians = WrapAngle(agent.state.headingRadians
                + std::sin(agent.steeringPhase) * deltaSeconds * .72f);
        }
        const float nextX = agent.state.x
            + std::cos(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        const float nextZ = agent.state.z
            + std::sin(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        if (IsValid(nextX, nextZ, agent.home)) {
            agent.state.x = nextX;
            agent.state.z = nextZ;
        } else {
            agent.state.headingRadians = std::atan2(
                agent.home.z - agent.state.z, agent.home.x - agent.state.x);
        }
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .038f;
        agent.state.legPhase = std::sin(agent.steeringPhase * 3.2f)
            * std::clamp(agent.state.speed / .22f, 0.0f, 1.0f);
        agent.state.antennaPhase = std::fmod(
            agent.state.antennaPhase + deltaSeconds * 2.1f, TwoPi);
    }
}
