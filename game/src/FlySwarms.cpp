#include "IslandSimulator/FlySwarms.hpp"

#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr std::array<std::array<float, 2>, FlySwarms::SwarmCount> Centres = {{
            {{IslandEnvironmentField::SeasonalWetlandX,
              IslandEnvironmentField::SeasonalWetlandZ}},
            {{VegetationLayout::EastClearing.x - 7.0f,
              VegetationLayout::EastClearing.z + 4.0f}},
            {{VegetationLayout::WestClearing.x + 6.0f,
              VegetationLayout::WestClearing.z - 3.0f}},
            {{-35.0f, -72.0f}},
        }};
    }

    FlySwarms::FlySwarms(const IslandTerrain& terrain) : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void FlySwarms::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const std::size_t swarm = index / PerSwarm;
        agent.home = {Centres[swarm][0], Centres[swarm][1], 2.8f + .25f * swarm};
        agent.angle = static_cast<float>(index) * 2.39996323f;
        agent.radius = .35f + .16f * static_cast<float>(index % PerSwarm);
        agent.altitude = .48f + .12f * static_cast<float>(index % 4U);
        agent.state.x = agent.home.x + std::cos(agent.angle) * agent.radius;
        agent.state.z = agent.home.z + std::sin(agent.angle) * agent.radius;
        const TerrainSample sample = terrain_.Sample(agent.state.x, agent.state.z);
        if (sample.height <= 0.0f || sample.signedShoreDistance < 20.0f)
            throw std::runtime_error("No suitable fly swarm habitat");
        agent.state.y = sample.height + agent.altitude;
        agent.state.headingRadians = agent.angle + std::numbers::pi_v<float> * .5f;
        agent.state.wingPhase = static_cast<float>(index) * .73f;
        agent.state.scale = .78f + .08f * static_cast<float>(index % 3U);
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
        agent.state.behavior = index % 3U == 0U ? FlyBehavior::Hover : FlyBehavior::Orbit;
        agent.behaviorSeconds = 1.4f + .42f * static_cast<float>(index % 7U);
    }

    void FlySwarms::Update(const float deltaSeconds, const float observerX,
                           const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, .1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        ++frameIndex_;
        for (std::size_t index = 0; index < Count; ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? std::hypot(agent.state.y - observerY,
                    std::hypot(agent.state.x - observerX, agent.state.z - observerZ))
                : std::numeric_limits<float>::infinity();
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
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
    }

    void FlySwarms::UpdateAgent(Agent& agent, const float deltaSeconds,
                                const float observerX, const float observerZ,
                                const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float distance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (distance < 1.2f && agent.state.behavior != FlyBehavior::Dart) {
            agent.state.behavior = FlyBehavior::Dart;
            agent.behaviorSeconds = .55f;
            agent.angle = std::atan2(agent.state.z - observerZ,
                                     agent.state.x - observerX);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            agent.state.behavior = agent.state.behavior == FlyBehavior::Hover
                ? FlyBehavior::Orbit : agent.state.behavior == FlyBehavior::Orbit
                    ? FlyBehavior::Dart : FlyBehavior::Hover;
            agent.behaviorSeconds = agent.state.behavior == FlyBehavior::Dart
                ? .48f : 1.1f + .35f * static_cast<float>(agent.transitions % 6U);
        }
        const float angularSpeed = agent.state.behavior == FlyBehavior::Hover ? .35f
            : agent.state.behavior == FlyBehavior::Dart ? 4.7f : 1.55f;
        agent.angle = std::fmod(agent.angle + angularSpeed * deltaSeconds, TwoPi);
        const float pulse = .12f * std::sin(agent.state.wingPhase * .11f);
        const float radius = std::clamp(agent.radius + pulse, .22f, 1.35f);
        const float targetX = agent.home.x + std::cos(agent.angle) * radius;
        const float targetZ = agent.home.z + std::sin(agent.angle) * radius;
        const float response = agent.state.behavior == FlyBehavior::Dart ? 8.0f : 3.4f;
        agent.state.x += std::clamp(targetX - agent.state.x,
            -response * deltaSeconds, response * deltaSeconds);
        agent.state.z += std::clamp(targetZ - agent.state.z,
            -response * deltaSeconds, response * deltaSeconds);
        agent.state.headingRadians = agent.angle + std::numbers::pi_v<float> * .5f;
        const float ground = terrain_.Height(agent.state.x, agent.state.z);
        const float targetY = ground + agent.altitude
            + std::sin(agent.state.wingPhase * .17f) * .09f;
        agent.state.y += std::clamp(targetY - agent.state.y,
            -1.8f * deltaSeconds, 1.8f * deltaSeconds);
        agent.state.wingPhase = std::fmod(agent.state.wingPhase
            + 74.0f * deltaSeconds, TwoPi);
    }
}
