#include "IslandSimulator/SkinkPopulation.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
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
        constexpr float Pi = std::numbers::pi_v<float>;
        constexpr float TwoPi = Pi * 2.0f;
        constexpr std::array<std::array<float, 2>, SkinkPopulation::Count> Centres = {{
            {{VegetationLayout::EastClearing.x - 8.0f, VegetationLayout::EastClearing.z + 8.0f}},
            {{VegetationLayout::EastClearing.x + 9.0f, VegetationLayout::EastClearing.z}},
            {{VegetationLayout::WestClearing.x - 6.0f, VegetationLayout::WestClearing.z + 7.0f}},
            {{VegetationLayout::WestClearing.x + 8.0f, VegetationLayout::WestClearing.z - 2.0f}},
            {{IslandEnvironmentField::SeasonalWetlandX - 14.0f,
              IslandEnvironmentField::SeasonalWetlandZ + 8.0f}},
            {{IslandEnvironmentField::SeasonalWetlandX + 13.0f,
              IslandEnvironmentField::SeasonalWetlandZ + 1.0f}},
            {{-37.0f, -71.0f}}, {{43.0f, -55.0f}},
        }};

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

    SkinkPopulation::SkinkPopulation(const IslandTerrain& terrain) : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void SkinkPopulation::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        agent.home.radius = 6.4f + static_cast<float>(index % 3U) * .55f;
        bool found = false;
        for (int attempt = 0; attempt < 128 && !found; ++attempt) {
            const float angle = static_cast<float>(attempt) * 2.39996323f
                + static_cast<float>(index) * .79f;
            const float radius = .5f + static_cast<float>(attempt / 16) * .78f;
            agent.home.x = Centres[index][0] + std::cos(angle) * radius;
            agent.home.z = Centres[index][1] + std::sin(angle) * radius;
            found = IsValid(agent.home.x, agent.home.z, agent.home);
        }
        if (!found) throw std::runtime_error("No open inland skink habitat");
        agent.state.x = agent.home.x;
        agent.state.z = agent.home.z;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .034f;
        agent.state.headingRadians = Wrap(.31f + static_cast<float>(index) * 1.51f);
        agent.state.scale = .38f + static_cast<float>(index % 4U) * .026f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 3U == 0U ? SkinkBehavior::Forage : SkinkBehavior::Bask;
        agent.behaviorSeconds = 3.8f + static_cast<float>(index % 5U) * 1.2f;
        agent.state.gaitPhase = static_cast<float>(index) * .93f;
        agent.state.breathPhase = static_cast<float>(index) * .67f;
    }

    bool SkinkPopulation::IsValid(const float x, const float z,
                                  const SkinkHome& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        const bool ground = sample.surface == TerrainSurface::CoastalGround
            || sample.surface == TerrainSurface::InteriorGround
            || sample.surface == TerrainSurface::RockyGround;
        return ground && sample.height > .15f && sample.signedShoreDistance >= 34.0f
            && sample.slopeRadians < .52f
            && std::hypot(x - home.x, z - home.z) <= home.radius;
    }

    void SkinkPopulation::Update(const float deltaSeconds, const float observerX,
                                 const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, .1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
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

    void SkinkPopulation::UpdateAgent(Agent& agent, const float deltaSeconds,
                                      const float observerX, const float observerZ,
                                      const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 3.1f && agent.state.behavior != SkinkBehavior::Flee) {
            agent.state.behavior = SkinkBehavior::Flee;
            agent.behaviorSeconds = 1.1f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            if (agent.state.behavior == SkinkBehavior::Bask) {
                agent.state.behavior = SkinkBehavior::Forage;
                agent.behaviorSeconds = 2.8f + static_cast<float>(agent.transitions % 4U) * .7f;
            } else {
                agent.state.behavior = SkinkBehavior::Bask;
                agent.behaviorSeconds = 4.0f + static_cast<float>(agent.transitions % 5U) * 1.0f;
            }
        }
        const float targetSpeed = agent.state.behavior == SkinkBehavior::Bask
            ? 0.0f : (agent.state.behavior == SkinkBehavior::Flee ? 1.65f : .42f);
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
                                        -deltaSeconds * 4.5f, deltaSeconds * 7.0f);
        const float homeDistance = std::hypot(agent.state.x - agent.home.x,
                                               agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * .72f) {
            agent.state.headingRadians = MoveAngle(agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x),
                deltaSeconds * 3.3f);
        } else if (agent.state.behavior == SkinkBehavior::Forage) {
            agent.state.headingRadians = Wrap(agent.state.headingRadians
                + std::sin(agent.state.breathPhase * .61f) * deltaSeconds * .48f);
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
            agent.state.speed = std::min(agent.state.speed, .5f);
        }
        agent.state.gaitPhase = std::fmod(agent.state.gaitPhase
            + deltaSeconds * (2.0f + agent.state.speed * 8.5f), TwoPi);
        agent.state.breathPhase = std::fmod(agent.state.breathPhase + deltaSeconds * 1.3f, TwoPi);
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .034f;
    }
}
