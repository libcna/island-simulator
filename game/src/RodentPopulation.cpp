#include "IslandSimulator/RodentPopulation.hpp"

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
        constexpr float Pi = std::numbers::pi_v<float>;
        constexpr float TwoPi = 2.0f * Pi;
        constexpr std::array<std::array<float, 2>, RodentPopulation::Count> Centres = {{
            {{VegetationLayout::EastClearing.x - 14.0f, VegetationLayout::EastClearing.z + 7.0f}},
            {{VegetationLayout::EastClearing.x + 13.0f, VegetationLayout::EastClearing.z + 5.0f}},
            {{VegetationLayout::WestClearing.x - 12.0f, VegetationLayout::WestClearing.z + 5.0f}},
            {{VegetationLayout::WestClearing.x + 13.0f, VegetationLayout::WestClearing.z - 5.0f}},
            {{-35.0f, -72.0f}}, {{42.0f, -64.0f}}, {{-18.0f, 52.0f}}, {{29.0f, 61.0f}},
            {{-68.0f, 22.0f}}, {{72.0f, 12.0f}}, {{-53.0f, -31.0f}}, {{57.0f, -26.0f}},
            {{IslandEnvironmentField::SeasonalWetlandX - 18.0f,
              IslandEnvironmentField::SeasonalWetlandZ + 8.0f}},
            {{IslandEnvironmentField::SeasonalWetlandX + 17.0f,
              IslandEnvironmentField::SeasonalWetlandZ + 5.0f}},
            {{VegetationLayout::WestClearing.x - 17.0f, VegetationLayout::WestClearing.z - 2.0f}},
            {{VegetationLayout::WestClearing.x + 16.0f, VegetationLayout::WestClearing.z + 4.0f}},
            {{-83.0f, -12.0f}}, {{-76.0f, 34.0f}}, {{63.0f, 38.0f}}, {{78.0f, -18.0f}},
        }};

        float Wrap(const float angle) noexcept { return std::remainder(angle, TwoPi); }
        float Turn(const float current, const float target, const float limit) noexcept
        {
            return Wrap(current + std::clamp(Wrap(target - current), -limit, limit));
        }
    }

    RodentPopulation::RodentPopulation(const IslandTerrain& terrain) : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void RodentPopulation::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        agent.state.species = index < MouseCount ? RodentSpecies::Mouse : RodentSpecies::Rat;
        agent.home.radius = agent.state.species == RodentSpecies::Mouse
            ? 4.2f + .35f * static_cast<float>(index % 3U)
            : 6.0f + .45f * static_cast<float>(index % 3U);
        bool found = false;
        for (int attempt = 0; attempt < 160 && !found; ++attempt) {
            const float angle = static_cast<float>(attempt) * 2.39996323f
                + static_cast<float>(index) * .73f;
            const float radius = .35f + static_cast<float>(attempt / 20) * .72f;
            agent.home.x = Centres[index][0] + std::cos(angle) * radius;
            agent.home.z = Centres[index][1] + std::sin(angle) * radius;
            const TerrainSample sample = terrain_.Sample(agent.home.x, agent.home.z);
            const bool dry = sample.surface == TerrainSurface::InteriorGround
                || sample.surface == TerrainSurface::CoastalGround
                || sample.surface == TerrainSurface::RockyGround;
            found = dry && sample.signedShoreDistance > 30.0f
                && sample.slopeRadians < .55f;
        }
        if (!found) throw std::runtime_error("No safe inland rodent habitat");
        agent.state.x = agent.home.x;agent.state.z = agent.home.z;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .018f;
        agent.state.headingRadians = Wrap(.2f + static_cast<float>(index) * 1.17f);
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 5U == 0U ? RodentBehavior::Forage : RodentBehavior::Rest;
        agent.behaviorSeconds = 2.5f + .65f * static_cast<float>(index % 7U);
        agent.state.gaitPhase = static_cast<float>(index) * .81f;
        agent.state.sniffPhase = static_cast<float>(index) * .57f;
    }

    bool RodentPopulation::IsValidPosition(const std::size_t index, const float x,
                                           const float z) const noexcept
    {
        if (index >= Count || !std::isfinite(x) || !std::isfinite(z)) return false;
        const TerrainSample sample = terrain_.Sample(x, z);
        const bool dry = sample.surface == TerrainSurface::InteriorGround
            || sample.surface == TerrainSurface::CoastalGround
            || sample.surface == TerrainSurface::RockyGround;
        return dry && sample.signedShoreDistance > 28.0f && sample.slopeRadians < .60f
            && std::hypot(x - homes_[index].x, z - homes_[index].z) <= homes_[index].radius;
    }

    void RodentPopulation::Update(const float deltaSeconds, const float observerX,
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
                UpdateAgent(agent, index, std::min(agent.accumulatedSeconds, .5f),
                            observerX, observerZ, observerValid);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }
    }

    void RodentPopulation::UpdateAgent(Agent& agent, const std::size_t index,
                                       const float deltaSeconds, const float observerX,
                                       const float observerZ, const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float distance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        const float fleeDistance = agent.state.species == RodentSpecies::Mouse ? 2.5f : 3.3f;
        if (distance < fleeDistance && agent.state.behavior != RodentBehavior::Hidden) {
            agent.state.behavior = RodentBehavior::Scurry;
            agent.behaviorSeconds = .75f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            if (agent.state.behavior == RodentBehavior::Scurry) {
                agent.state.behavior = RodentBehavior::Hidden;
                agent.behaviorSeconds = 1.8f + .45f * static_cast<float>(agent.transitions % 5U);
            } else if (agent.state.behavior == RodentBehavior::Hidden) {
                agent.state.behavior = RodentBehavior::Rest;
                agent.behaviorSeconds = 2.8f + .55f * static_cast<float>(agent.transitions % 6U);
            } else {
                agent.state.behavior = agent.state.behavior == RodentBehavior::Rest
                    ? RodentBehavior::Forage : RodentBehavior::Rest;
                agent.behaviorSeconds = 2.2f + .5f * static_cast<float>(agent.transitions % 7U);
                agent.state.headingRadians = Wrap(agent.state.headingRadians
                    + .65f + .31f * static_cast<float>(agent.transitions % 5U));
            }
        }
        const float visibilityTarget = agent.state.behavior == RodentBehavior::Hidden ? 0.0f : 1.0f;
        agent.state.visibility += std::clamp(visibilityTarget - agent.state.visibility,
            -4.5f * deltaSeconds, 3.0f * deltaSeconds);
        const float targetSpeed = agent.state.behavior == RodentBehavior::Scurry
            ? (agent.state.species == RodentSpecies::Mouse ? 1.65f : 1.45f)
            : agent.state.behavior == RodentBehavior::Forage ? .28f : 0.0f;
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
            -4.0f * deltaSeconds, 6.0f * deltaSeconds);
        const float homeDistance = std::hypot(agent.state.x - agent.home.x,
                                               agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * .72f) {
            agent.state.headingRadians = Turn(agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x),
                3.2f * deltaSeconds);
        } else if (agent.state.behavior == RodentBehavior::Forage) {
            agent.state.headingRadians = Wrap(agent.state.headingRadians
                + std::sin(agent.state.sniffPhase * .7f) * .45f * deltaSeconds);
        }
        if (agent.state.speed > .001f) {
            const float nextX = agent.state.x
                + std::cos(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
            const float nextZ = agent.state.z
                + std::sin(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
            if (IsValidPosition(index, nextX, nextZ)) {
                agent.state.x = nextX;agent.state.z = nextZ;
            } else {
                agent.state.headingRadians = std::atan2(
                    agent.home.z - agent.state.z, agent.home.x - agent.state.x);
                agent.state.speed = std::min(agent.state.speed, .24f);
            }
        }
        agent.state.gaitPhase = std::fmod(agent.state.gaitPhase
            + deltaSeconds * (2.0f + 11.0f * agent.state.speed), TwoPi);
        agent.state.sniffPhase = std::fmod(agent.state.sniffPhase + deltaSeconds * 2.7f, TwoPi);
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + .018f;
    }
}
