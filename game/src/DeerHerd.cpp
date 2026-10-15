#include "IslandSimulator/DeerHerd.hpp"

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
        constexpr std::array<std::array<float, 2>, DeerHerd::Count> Centres = {{
            {{VegetationLayout::EastClearing.x - 5.0f, VegetationLayout::EastClearing.z + 2.0f}},
            {{VegetationLayout::EastClearing.x + 7.0f, VegetationLayout::EastClearing.z - 3.0f}},
            {{VegetationLayout::WestClearing.x - 4.0f, VegetationLayout::WestClearing.z + 2.0f}},
            {{VegetationLayout::WestClearing.x + 7.0f, VegetationLayout::WestClearing.z - 4.0f}},
        }};

        float Wrap(const float angle) noexcept { return std::remainder(angle, TwoPi); }
        float Turn(const float current, const float target, const float limit) noexcept
        {
            return Wrap(current + std::clamp(Wrap(target - current), -limit, limit));
        }
    }

    DeerHerd::DeerHerd(const IslandTerrain& terrain,
                       const std::span<const CircularObstacle> obstacles)
        : terrain_(terrain), obstacles_(obstacles)
    {
        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
    }

    void DeerHerd::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        agent.home.radius = 13.0f + static_cast<float>(index % 2U) * 2.0f;
        bool found = false;
        for (int attempt = 0; attempt < 192 && !found; ++attempt) {
            const float angle = static_cast<float>(attempt) * 2.39996323f
                + static_cast<float>(index) * 1.13f;
            const float radius = .6f + static_cast<float>(attempt / 24) * 1.15f;
            agent.home.x = Centres[index][0] + std::cos(angle) * radius;
            agent.home.z = Centres[index][1] + std::sin(angle) * radius;
            const TerrainSample sample = terrain_.Sample(agent.home.x, agent.home.z);
            found = (sample.surface == TerrainSurface::InteriorGround
                     || sample.surface == TerrainSurface::CoastalGround)
                && sample.signedShoreDistance > 44.0f && sample.slopeRadians < .42f
                && !obstacles_.IntersectsCircle(agent.home.x, agent.home.z, .72f);
        }
        if (!found) throw std::runtime_error("No safe open inland deer habitat");
        agent.state.x = agent.home.x;
        agent.state.z = agent.home.z;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z);
        agent.state.headingRadians = Wrap(.4f + static_cast<float>(index) * 1.45f);
        agent.state.scale = index == 3U ? .84f : .94f + .035f * static_cast<float>(index);
        agent.state.coatVariant = static_cast<std::uint8_t>(index % 3U);
        agent.state.antlered = index == 0U;
        agent.state.behavior = index == 0U ? DeerBehavior::Walk
            : index % 3U == 2U ? DeerBehavior::Rest : DeerBehavior::Graze;
        agent.state.grazeAmount = agent.state.behavior == DeerBehavior::Graze ? 1.0f : 0.0f;
        agent.behaviorSeconds = 3.5f + static_cast<float>(index) * 1.2f;
        agent.state.gaitPhase = static_cast<float>(index) * 1.4f;
    }

    bool DeerHerd::IsValidPosition(const std::size_t index, const float x,
                                   const float z) const noexcept
    {
        if (index >= Count || !std::isfinite(x) || !std::isfinite(z)) return false;
        const TerrainSample sample = terrain_.Sample(x, z);
        const bool ground = sample.surface == TerrainSurface::InteriorGround
            || sample.surface == TerrainSurface::CoastalGround;
        return ground && sample.signedShoreDistance > 40.0f && sample.slopeRadians < .48f
            && std::hypot(x - homes_[index].x, z - homes_[index].z) <= homes_[index].radius
            && !obstacles_.IntersectsCircle(x, z, .72f);
    }

    void DeerHerd::Update(const float deltaSeconds, const float observerX,
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
                ? std::hypot(agent.state.y + 1.0f - observerY,
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

    void DeerHerd::UpdateAgent(Agent& agent, const float deltaSeconds,
                               const float observerX, const float observerZ,
                               const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        const std::size_t index = static_cast<std::size_t>(&agent - agents_.data());
        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 9.0f) {
            agent.state.behavior = DeerBehavior::Retreat;
            agent.behaviorSeconds = 1.8f;
            agent.state.headingRadians = std::atan2(
                agent.state.z - observerZ, agent.state.x - observerX);
        } else if ((agent.behaviorSeconds -= deltaSeconds) <= 0.0f) {
            ++agent.transitions;
            const auto phase = (agent.transitions + static_cast<std::uint32_t>(index) * 3U) % 7U;
            agent.state.behavior = phase < 3U ? DeerBehavior::Graze
                : phase < 5U ? DeerBehavior::Walk : DeerBehavior::Rest;
            agent.behaviorSeconds = 3.5f + static_cast<float>((agent.transitions * 7U
                + static_cast<std::uint32_t>(index) * 5U) % 45U) * .12f;
            const float turn = -.85f + static_cast<float>((agent.transitions * 37U
                + static_cast<std::uint32_t>(index) * 19U) % 171U) * .01f;
            agent.state.headingRadians = Wrap(agent.state.headingRadians + turn);
        }
        const float targetSpeed = agent.state.behavior == DeerBehavior::Retreat ? 3.1f
            : agent.state.behavior == DeerBehavior::Walk ? .78f : 0.0f;
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
            -2.8f * deltaSeconds, 3.8f * deltaSeconds);
        const float targetGraze = agent.state.behavior == DeerBehavior::Graze ? 1.0f : 0.0f;
        agent.state.grazeAmount += std::clamp(targetGraze - agent.state.grazeAmount,
            -1.8f * deltaSeconds, 1.3f * deltaSeconds);
        const float homeDistance = std::hypot(agent.state.x - agent.home.x,
                                               agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * .68f) {
            agent.state.headingRadians = Turn(agent.state.headingRadians,
                std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x),
                2.0f * deltaSeconds);
        }
        const float nextX = agent.state.x
            + std::cos(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        const float nextZ = agent.state.z
            + std::sin(agent.state.headingRadians) * agent.state.speed * deltaSeconds;
        if (IsValidPosition(index, nextX, nextZ)) {
            agent.state.x = nextX;
            agent.state.z = nextZ;
        } else {
            agent.state.headingRadians = std::atan2(
                agent.home.z - agent.state.z, agent.home.x - agent.state.x);
            agent.state.speed = std::min(agent.state.speed, .45f);
            agent.behaviorSeconds = std::min(agent.behaviorSeconds, .8f);
        }
        agent.state.gaitPhase = std::fmod(agent.state.gaitPhase
            + deltaSeconds * (1.4f + agent.state.speed * 4.2f), TwoPi);
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z);
    }
}
