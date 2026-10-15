#include "IslandSimulator/FrogPopulation.hpp"

#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/StreamRoute.hpp"

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
        struct Habitat { float x, z, radius; };
        const std::array<Habitat, FrogPopulation::Count> Habitats = {{
            {-82.0f, StreamRoute::CentreZ(-82.0f) + 2.6f, 4.8f},
            {-91.0f, StreamRoute::CentreZ(-91.0f) - 2.8f, 5.0f},
            {-101.0f, StreamRoute::CentreZ(-101.0f) + 3.1f, 5.2f},
            {-111.0f, StreamRoute::CentreZ(-111.0f) - 3.2f, 5.4f},
            {-121.0f, StreamRoute::CentreZ(-121.0f) + 3.5f, 5.6f},
            {-132.0f, StreamRoute::CentreZ(-132.0f) - 3.7f, 5.8f},
            {-142.0f, StreamRoute::CentreZ(-142.0f) + 4.0f, 6.0f},
            {-151.0f, StreamRoute::CentreZ(-151.0f) - 4.2f, 6.2f},
            {IslandEnvironmentField::SeasonalWetlandX - 4.0f,
             IslandEnvironmentField::SeasonalWetlandZ + 1.5f, 6.5f},
            {IslandEnvironmentField::SeasonalWetlandX + 4.5f,
             IslandEnvironmentField::SeasonalWetlandZ - 1.0f, 6.5f},
        }};

        float Wrap(float angle) noexcept
        {
            while (angle > Pi) angle -= TwoPi;
            while (angle < -Pi) angle += TwoPi;
            return angle;
        }

        float MoveAngle(float current, float target, float amount) noexcept
        {
            return Wrap(current + std::clamp(Wrap(target - current), -amount, amount));
        }
    }

    FrogPopulation::FrogPopulation(const IslandTerrain& terrain) : terrain_(terrain)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void FrogPopulation::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const Habitat habitat = Habitats[index];
        agent.home = {habitat.x, habitat.z, habitat.radius};
        bool found = false;
        for (int attempt = 0; attempt < 160 && !found; ++attempt) {
            const float angle = static_cast<float>(attempt) * 2.39996323f
                + static_cast<float>(index) * .71f;
            const float radius = .35f + static_cast<float>(attempt / 20) * .62f;
            agent.state.x = habitat.x + std::cos(angle) * radius;
            agent.state.z = habitat.z + std::sin(angle) * radius;
            found = IsValid(agent.state.x, agent.state.z, agent.home);
        }
        if (!found) throw std::runtime_error("No suitable stream or wetland frog habitat");
        agent.groundY = terrain_.Height(agent.state.x, agent.state.z);
        agent.state.y = agent.groundY + .045f;
        agent.state.headingRadians = Wrap(.43f + static_cast<float>(index) * 1.37f);
        agent.state.scale = .32f + static_cast<float>(index % 4U) * .025f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 4U == 0U ? FrogBehavior::Hop : FrogBehavior::Rest;
        agent.state.hopPhase = agent.state.behavior == FrogBehavior::Hop ? .12f : 0.0f;
        agent.state.breathPhase = static_cast<float>(index) * .81f;
        agent.behaviorSeconds = 2.4f + static_cast<float>(index % 5U) * .85f;
    }

    bool FrogPopulation::IsValid(const float x, const float z,
                                 const FrogHome& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        const auto inland = IslandEnvironmentField::SampleInland(
            x, z, sample.signedShoreDistance);
        const bool ground = sample.surface == TerrainSurface::CoastalGround
            || sample.surface == TerrainSurface::InteriorGround
            || sample.surface == TerrainSurface::RockyGround;
        return ground && sample.height > .05f && sample.signedShoreDistance > 18.0f
            && sample.slopeRadians < .48f && inland.moisture > .52f
            && std::hypot(x - home.x, z - home.z) <= home.radius;
    }

    void FrogPopulation::Update(const float deltaSeconds, const float observerX,
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

    void FrogPopulation::UpdateAgent(Agent& agent, const float deltaSeconds,
                                     const float observerX, const float observerZ,
                                     const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 2.8f && agent.state.behavior != FrogBehavior::Flee) {
            agent.state.behavior = FrogBehavior::Flee;
            agent.state.hopPhase = 0.0f;
            agent.behaviorSeconds = 1.5f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else if (agent.state.behavior == FrogBehavior::Rest
                   && (agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            agent.state.behavior = FrogBehavior::Hop;
            agent.state.hopPhase = 0.0f;
            agent.behaviorSeconds = .62f;
        }

        agent.state.breathPhase = std::fmod(agent.state.breathPhase
            + deltaSeconds * 1.45f, TwoPi);
        if (agent.state.behavior == FrogBehavior::Rest) {
            agent.state.y = agent.groundY + .045f;
            return;
        }

        const float hopDuration = agent.state.behavior == FrogBehavior::Flee ? .42f : .62f;
        const float speed = agent.state.behavior == FrogBehavior::Flee ? 1.65f : .78f;
        agent.state.hopPhase = std::min(1.0f,
            agent.state.hopPhase + deltaSeconds / hopDuration);
        const float homeDistance = std::hypot(agent.state.x - agent.home.x,
                                               agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * .70f) {
            agent.state.headingRadians = MoveAngle(agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x),
                deltaSeconds * 3.8f);
        } else if (agent.state.behavior == FrogBehavior::Hop) {
            agent.state.headingRadians = Wrap(agent.state.headingRadians
                + std::sin(agent.state.breathPhase * .63f) * deltaSeconds * .65f);
        }
        const float nextX = agent.state.x
            + std::cos(agent.state.headingRadians) * speed * deltaSeconds;
        const float nextZ = agent.state.z
            + std::sin(agent.state.headingRadians) * speed * deltaSeconds;
        if (IsValid(nextX, nextZ, agent.home)) {
            agent.state.x = nextX;
            agent.state.z = nextZ;
        } else {
            agent.state.headingRadians = std::atan2(
                agent.home.z - agent.state.z, agent.home.x - agent.state.x);
        }
        agent.groundY = terrain_.Height(agent.state.x, agent.state.z);
        const float arc = std::sin(Pi * agent.state.hopPhase);
        agent.state.y = agent.groundY + .045f
            + arc * (agent.state.behavior == FrogBehavior::Flee ? .22f : .14f);
        if (agent.state.hopPhase >= 1.0f) {
            if (agent.state.behavior == FrogBehavior::Flee
                && (agent.behaviorSeconds -= hopDuration) > 0.0f) {
                agent.state.hopPhase = 0.0f;
            } else {
                agent.state.behavior = FrogBehavior::Rest;
                agent.state.hopPhase = 0.0f;
                agent.behaviorSeconds = 3.2f
                    + static_cast<float>(agent.transitions % 5U) * .85f;
                agent.state.y = agent.groundY + .045f;
            }
        }
    }
}
