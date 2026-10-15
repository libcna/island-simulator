#include "IslandSimulator/CoconutCrabGroup.hpp"

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
        constexpr std::array<float, CoconutCrabGroup::Count> Angles = {{
            -2.55f, -1.62f, -.64f, .44f, 1.48f, 2.53f}};
        float Wrap(float angle) noexcept
        {
            while (angle > Pi) angle -= TwoPi;
            while (angle < -Pi) angle += TwoPi;
            return angle;
        }
        float MoveAngle(float current, float target, float step) noexcept
        {
            return Wrap(current + std::clamp(Wrap(target - current), -step, step));
        }
    }

    CoconutCrabGroup::CoconutCrabGroup(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void CoconutCrabGroup::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        agent.home.radius = 7.5f + static_cast<float>(index % 3U) * .8f;
        bool found = false;
        float angle = Angles[index];
        for (int attempt = 0; attempt < 120 && !found; ++attempt) {
            const float side = attempt % 2 == 0 ? 1.0f : -1.0f;
            angle = Angles[index] + side * static_cast<float>((attempt + 1) / 2) * .018f;
            const float inset = 31.0f + static_cast<float>(index % 3U) * 5.0f;
            const float radius = terrain_.ShoreRadius(angle) - inset;
            agent.home.x = std::cos(angle) * radius;
            agent.home.z = std::sin(angle) * radius;
            found = IsValid(agent.home.x, agent.home.z, agent.home);
        }
        if (!found) throw std::runtime_error("No coastal palm-zone habitat for coconut crab");
        agent.state.x = agent.home.x;
        agent.state.z = agent.home.z;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .080f;
        agent.state.headingRadians = Wrap(angle + Pi * .5f);
        agent.state.scale = .92f + static_cast<float>(index % 3U) * .06f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 2U == 0U
            ? CoconutCrabBehavior::Rest : CoconutCrabBehavior::Forage;
        agent.behaviorSeconds = 2.0f + static_cast<float>(index) * .61f;
        agent.steeringPhase = .5f + static_cast<float>(index) * 1.23f;
        agent.state.clawPose = .25f + static_cast<float>(index % 3U) * .10f;
    }

    bool CoconutCrabGroup::IsValid(const float x, const float z,
                                   const CoconutCrabHome& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        const bool ground = sample.surface == TerrainSurface::CoastalGround
            || sample.surface == TerrainSurface::InteriorGround
            || sample.surface == TerrainSurface::RockyGround;
        return ground && sample.height > .15f
            && sample.signedShoreDistance >= MinimumShoreDistance
            && sample.signedShoreDistance <= MaximumShoreDistance
            && sample.slopeRadians < .42f
            && std::hypot(x - home.x, z - home.z) <= home.radius;
    }

    void CoconutCrabGroup::Update(const float deltaSeconds,
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
            const auto divisor = WildlifeActivationPolicy::UpdateDivisor(
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

    void CoconutCrabGroup::UpdateAgent(Agent& agent, const float deltaSeconds,
                                       const float observerX, const float observerZ,
                                       const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 5.5f
            && agent.state.behavior != CoconutCrabBehavior::Retreat) {
            agent.state.behavior = CoconutCrabBehavior::Retreat;
            agent.behaviorSeconds = 1.8f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            if (agent.state.behavior == CoconutCrabBehavior::Rest) {
                agent.state.behavior = CoconutCrabBehavior::Forage;
                agent.behaviorSeconds = 4.0f + static_cast<float>(agent.transitions % 4U) * .75f;
            } else {
                agent.state.behavior = CoconutCrabBehavior::Rest;
                agent.behaviorSeconds = 1.8f + static_cast<float>(agent.transitions % 3U) * .60f;
            }
        }
        agent.steeringPhase = std::fmod(agent.steeringPhase + deltaSeconds * .52f, TwoPi);
        const float targetSpeed = agent.state.behavior == CoconutCrabBehavior::Rest
            ? 0.0f : (agent.state.behavior == CoconutCrabBehavior::Retreat ? .95f : .30f);
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
                                        -deltaSeconds * 1.5f, deltaSeconds * 2.2f);
        if (std::hypot(agent.state.x - agent.home.x, agent.state.z - agent.home.z)
                > agent.home.radius * .72f) {
            agent.state.headingRadians = MoveAngle(agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z,
                           agent.home.x - agent.state.x), deltaSeconds * 2.3f);
        } else if (agent.state.behavior == CoconutCrabBehavior::Forage) {
            agent.state.headingRadians = Wrap(agent.state.headingRadians
                + std::sin(agent.steeringPhase) * deltaSeconds * .55f);
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
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .080f;
        agent.state.legPhase = std::sin(agent.steeringPhase * 3.0f)
            * std::clamp(agent.state.speed / .30f, 0.0f, 1.0f);
        agent.state.clawPose = .32f + .18f * std::sin(agent.steeringPhase * .7f);
    }
}
