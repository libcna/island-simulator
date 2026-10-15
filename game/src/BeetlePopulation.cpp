#include "IslandSimulator/BeetlePopulation.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr std::array<std::array<float, 3>, BeetlePopulation::ColonyCount> AuthoredHomes = {{
            {{VegetationLayout::EastClearing.x - 10.0f,
              VegetationLayout::EastClearing.z + 8.0f, 3.2f}},
            {{VegetationLayout::WestClearing.x + 8.0f,
              VegetationLayout::WestClearing.z - 7.0f, 3.5f}},
            {{-35.0f, -72.0f, 3.0f}},
        }};
    }

    BeetlePopulation::BeetlePopulation(const IslandTerrain& terrain) : terrain_(terrain)
    {
        for (std::size_t colony = 0; colony < ColonyCount; ++colony) {
            homes_[colony] = {AuthoredHomes[colony][0], AuthoredHomes[colony][1],
                              AuthoredHomes[colony][2]};
            const TerrainSample sample = terrain_.Sample(homes_[colony].x, homes_[colony].z);
            if (sample.height <= 0.0f || sample.signedShoreDistance < 22.0f
                || sample.slopeRadians > 0.62f) {
                throw std::runtime_error("No suitable inland beetle leaf-litter habitat");
            }
        }
        for (std::size_t index = 0; index < Count; ++index) {
            Agent& agent = agents_[index];
            agent.colony = static_cast<std::uint8_t>(index / PerColony);
            agent.angle = static_cast<float>(index) * 2.39996323f;
            agent.radius = .55f + .36f * static_cast<float>(index % PerColony);
            agent.state.headingRadians = agent.angle + .4f;
            agent.state.gaitPhase = static_cast<float>(index) * .77f;
            agent.state.scale = .86f + .08f * static_cast<float>(index % 4U);
            agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
            agent.state.behavior = index % 3U == 0U
                ? BeetleBehavior::Rest : BeetleBehavior::Forage;
            agent.behaviorSeconds = 1.2f + .37f * static_cast<float>(index % 7U);
            Place(agent);
            states_[index] = agent.state;
        }
    }

    void BeetlePopulation::Place(Agent& agent) noexcept
    {
        const BeetleHome& home = homes_[agent.colony];
        agent.state.x = home.x + std::cos(agent.angle) * agent.radius;
        agent.state.z = home.z + std::sin(agent.angle) * agent.radius;
        const TerrainSample sample = terrain_.Sample(agent.state.x, agent.state.z);
        agent.state.y = sample.height + .018f;
        agent.state.normalX = sample.normal.x;
        agent.state.normalY = sample.normal.y;
        agent.state.normalZ = sample.normal.z;
    }

    void BeetlePopulation::Update(const float deltaSeconds, const float observerX,
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

    void BeetlePopulation::UpdateAgent(Agent& agent, const float deltaSeconds,
                                       const float observerX, const float observerZ,
                                       const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float playerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (playerDistance < 1.5f && agent.state.behavior != BeetleBehavior::Hide) {
            agent.state.behavior = BeetleBehavior::Hide;
            agent.behaviorSeconds = 2.4f + .22f * static_cast<float>(agent.colony);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            if (agent.state.behavior == BeetleBehavior::Hide) {
                agent.state.behavior = BeetleBehavior::Rest;
                agent.behaviorSeconds = 1.1f;
            } else {
                agent.state.behavior = agent.state.behavior == BeetleBehavior::Rest
                    ? BeetleBehavior::Forage : BeetleBehavior::Rest;
                agent.behaviorSeconds = agent.state.behavior == BeetleBehavior::Forage
                    ? 3.0f + .31f * static_cast<float>(agent.transitions % 5U)
                    : .8f + .19f * static_cast<float>(agent.transitions % 4U);
            }
        }

        const float targetVisibility = agent.state.behavior == BeetleBehavior::Hide ? 0.0f : 1.0f;
        const float visibilityStep = deltaSeconds * (targetVisibility > agent.state.visibility
            ? 1.6f : 2.6f);
        agent.state.visibility += std::clamp(targetVisibility - agent.state.visibility,
                                             -visibilityStep, visibilityStep);
        if (agent.state.behavior != BeetleBehavior::Forage) return;

        const BeetleHome& home = homes_[agent.colony];
        const float turn = (.24f + .025f * static_cast<float>(agent.transitions % 5U))
            * deltaSeconds;
        agent.angle = std::fmod(agent.angle + turn, TwoPi);
        agent.radius = std::clamp(agent.radius
            + std::sin(agent.state.gaitPhase * .23f) * .035f * deltaSeconds,
            .35f, home.radius);
        agent.state.headingRadians = agent.angle + std::numbers::pi_v<float> * .5f;
        agent.state.gaitPhase = std::fmod(agent.state.gaitPhase + deltaSeconds * 8.5f, TwoPi);
        Place(agent);
    }
}
