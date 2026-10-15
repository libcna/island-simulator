#include "IslandSimulator/GeckoPopulation.hpp"

#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

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
        constexpr std::array<std::array<float, 2>, GeckoPopulation::Count> HabitatCentres = {{
            {{VegetationLayout::EastClearing.x - 12.0f, VegetationLayout::EastClearing.z + 4.0f}},
            {{VegetationLayout::EastClearing.x + 14.0f, VegetationLayout::EastClearing.z - 5.0f}},
            {{VegetationLayout::WestClearing.x - 10.0f, VegetationLayout::WestClearing.z + 3.0f}},
            {{VegetationLayout::WestClearing.x + 12.0f, VegetationLayout::WestClearing.z - 7.0f}},
            {{IslandEnvironmentField::SeasonalWetlandX - 18.0f,
              IslandEnvironmentField::SeasonalWetlandZ + 4.0f}},
            {{IslandEnvironmentField::SeasonalWetlandX + 18.0f,
              IslandEnvironmentField::SeasonalWetlandZ - 3.0f}},
            {{-42.0f, -74.0f}},
            {{48.0f, -58.0f}},
        }};

        [[nodiscard]] float WrapAngle(float value) noexcept
        {
            while (value > std::numbers::pi_v<float>) value -= TwoPi;
            while (value < -std::numbers::pi_v<float>) value += TwoPi;
            return value;
        }

        [[nodiscard]] float MoveAngle(float current, float target, float maximumStep) noexcept
        {
            const float difference = WrapAngle(target - current);
            return WrapAngle(current + std::clamp(difference, -maximumStep, maximumStep));
        }
    }

    GeckoPopulation::GeckoPopulation(const IslandTerrain& terrain) : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void GeckoPopulation::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const auto centre = HabitatCentres[index];
        agent.home.radius = 5.2f + static_cast<float>(index % 3U) * .45f;
        bool found = false;
        for (int attempt = 0; attempt < 96 && !found; ++attempt) {
            const float angle = static_cast<float>(attempt) * 2.39996323f
                + static_cast<float>(index) * .71f;
            const float radius = .4f + static_cast<float>(attempt / 12) * .72f;
            agent.home.x = centre[0] + std::cos(angle) * radius;
            agent.home.z = centre[1] + std::sin(angle) * radius;
            found = ValidGround(agent.home.x, agent.home.z, agent.home);
        }
        if (!found) throw std::runtime_error("No suitable inland gecko habitat");
        agent.state.x = agent.home.x;
        agent.state.z = agent.home.z;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .038f;
        agent.state.headingRadians = WrapAngle(.43f + static_cast<float>(index) * 1.37f);
        // Tokay-sized rather than monitor-lizard-sized: roughly 37--47 cm including tail.
        agent.state.scale = .34f + static_cast<float>(index % 4U) * .025f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 4U == 0U ? GeckoBehavior::Scurry : GeckoBehavior::Rest;
        agent.behaviorSeconds = 3.2f + static_cast<float>(index % 5U) * 1.15f;
        agent.state.gaitPhase = static_cast<float>(index) * .83f;
    }

    bool GeckoPopulation::ValidGround(const float x, const float z,
                                      const GeckoHome& home) const noexcept
    {
        const auto sample = terrain_.Sample(x, z);
        const bool surface = sample.surface == TerrainSurface::CoastalGround
            || sample.surface == TerrainSurface::InteriorGround
            || sample.surface == TerrainSurface::RockyGround;
        return surface && sample.signedShoreDistance >= 34.0f
            && sample.slopeRadians < .52f
            && std::hypot(x - home.x, z - home.z) <= home.radius;
    }

    void GeckoPopulation::Update(const float deltaSeconds, const float observerX,
                                 const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, .1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        ++frameIndex_;
        fullSimulationCount_ = reducedSimulationCount_ = dormantSimulationCount_ = 0;
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? std::hypot(agent.state.y - observerY,
                    std::hypot(agent.state.x - observerX, agent.state.z - observerZ))
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
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, .5f),
                            observerX, observerZ, observerValid);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }
#ifndef NDEBUG
        assert(fullSimulationCount_ + reducedSimulationCount_ + dormantSimulationCount_
               == static_cast<int>(Count));
#endif
    }

    void GeckoPopulation::UpdateAgent(Agent& agent, const float deltaSeconds,
                                      const float observerX, const float observerZ,
                                      const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 2.7f && agent.state.behavior == GeckoBehavior::Rest) {
            agent.state.behavior = GeckoBehavior::Scurry;
            agent.behaviorSeconds = .75f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else {
            agent.behaviorSeconds -= deltaSeconds;
            if (agent.behaviorSeconds <= 0.0f) {
                ++agent.transitionCount;
                if (agent.state.behavior == GeckoBehavior::Scurry) {
                    agent.state.behavior = GeckoBehavior::Hidden;
                    agent.behaviorSeconds = 2.5f
                        + static_cast<float>(agent.transitionCount % 5U) * .75f;
                } else if (agent.state.behavior == GeckoBehavior::Hidden) {
                    agent.state.behavior = GeckoBehavior::Rest;
                    agent.behaviorSeconds = 5.0f
                        + static_cast<float>(agent.transitionCount % 6U) * 1.1f;
                } else {
                    agent.state.behavior = GeckoBehavior::Scurry;
                    agent.behaviorSeconds = 1.1f
                        + static_cast<float>(agent.transitionCount % 4U) * .22f;
                    agent.state.headingRadians = WrapAngle(agent.state.headingRadians
                        + 1.17f + static_cast<float>(agent.transitionCount % 3U) * .71f);
                }
            }
        }

        const float visibilityTarget = agent.state.behavior == GeckoBehavior::Hidden ? 0.0f : 1.0f;
        agent.state.visibility += std::clamp(visibilityTarget - agent.state.visibility,
            -deltaSeconds * 3.8f, deltaSeconds * 2.6f);
        agent.state.visibility = std::clamp(agent.state.visibility, 0.0f, 1.0f);
        const float targetSpeed = agent.state.behavior == GeckoBehavior::Scurry ? 1.35f : 0.0f;
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
            -deltaSeconds * 4.0f, deltaSeconds * 6.0f);

        if (agent.state.behavior == GeckoBehavior::Scurry) {
            const float homeDistance = std::hypot(
                agent.state.x - agent.home.x, agent.state.z - agent.home.z);
            if (homeDistance > agent.home.radius * .72f) {
                const float homeHeading = std::atan2(
                    agent.home.z - agent.state.z, agent.home.x - agent.state.x);
                agent.state.headingRadians = MoveAngle(
                    agent.state.headingRadians, homeHeading, deltaSeconds * 3.2f);
            }
            const float candidateX = agent.state.x
                + std::cos(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
            const float candidateZ = agent.state.z
                + std::sin(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
            if (ValidGround(candidateX, candidateZ, agent.home)) {
                agent.state.x = candidateX;
                agent.state.z = candidateZ;
            } else {
                agent.state.headingRadians = std::atan2(
                    agent.home.z - agent.state.z, agent.home.x - agent.state.x);
                agent.state.speed = std::min(agent.state.speed, .45f);
            }
        }
        agent.state.gaitPhase = std::fmod(agent.state.gaitPhase
            + deltaSeconds * (2.2f + agent.state.speed * 8.0f), TwoPi);
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .038f;
    }
}
