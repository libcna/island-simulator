#include "IslandSimulator/FruitBatColony.hpp"

#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;

        [[nodiscard]] bool Separated(const std::vector<FruitBatRoost>& roosts,
                                     const float x, const float z) noexcept
        {
            return std::ranges::all_of(roosts, [=](const FruitBatRoost& roost) {
                return std::hypot(roost.x - x, roost.z - z) >= 20.0f;
            });
        }
    }

    FruitBatColony::FruitBatColony(const IslandTerrain& terrain,
                                   const std::span<const CanopyPlacement> canopy)
    {
        roosts_.reserve(Count);
        for (std::size_t index = 0; index < canopy.size() && roosts_.size() < Count; ++index) {
            const CanopyPlacement& tree = canopy[index];
            const float radius = std::hypot(tree.x, tree.z);
            if (index % 7U != 2U || tree.scale < .82f || tree.scale > 1.48f
                || radius < 45.0f || radius > 155.0f
                || terrain.SignedShoreDistance(tree.x, tree.z) < 45.0f
                || !Separated(roosts_, tree.x, tree.z)) {
                continue;
            }
            const float crownHeight = CanopyNominalHeight(tree.variant) * tree.scale;
            roosts_.push_back({
                tree.x + std::cos(tree.yawRadians + .7f) * .55f * tree.scale,
                // Hang beneath the lower crown rather than inside its opaque leaf cores.
                // The previous 70% nominal-height point hid both animals and their branch
                // contact from any plausible ground view.
                tree.y + crownHeight * .56f,
                tree.z + std::sin(tree.yawRadians + .7f) * .55f * tree.scale,
                tree.yawRadians + std::numbers::pi_v<float>});
        }
        if (roosts_.size() != Count) {
            throw std::runtime_error("Unable to derive six separated fruit-bat canopy roosts");
        }

        agents_.resize(Count);
        states_.resize(Count);
        for (std::size_t index = 0; index < Count; ++index) {
            Agent& agent = agents_[index];
            agent.roost = roosts_[index];
            agent.orbitRadius = 8.5f + static_cast<float>(index % 3U) * 1.8f;
            agent.orbitPhase = std::fmod(.73f + static_cast<float>(index) * 2.39996323f, TwoPi);
            agent.orbitSpeed = .29f + static_cast<float>(index % 2U) * .035f;
            agent.flapPhase = static_cast<float>(index) * 1.37f;
            agent.state.scale = .84f + static_cast<float>(index % 3U) * .08f;
            agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
            agent.state.behavior = index < MaximumDayFlights
                ? FruitBatBehavior::DayFlight : FruitBatBehavior::Roost;
            if (agent.state.behavior == FruitBatBehavior::Roost) {
                agent.state.x = agent.roost.x;
                agent.state.y = agent.roost.y - .38f * agent.state.scale;
                agent.state.z = agent.roost.z;
                agent.state.headingRadians = agent.roost.headingRadians;
                agent.state.hangAmount = 1.0f;
            } else {
                const float angle = agent.orbitPhase;
                agent.state.x = agent.roost.x + std::cos(angle) * agent.orbitRadius;
                agent.state.z = agent.roost.z + std::sin(angle) * agent.orbitRadius;
                agent.state.y = agent.roost.y + 3.2f + .7f * std::sin(angle * 1.7f);
                agent.state.headingRadians = angle + std::numbers::pi_v<float> * .5f;
                agent.state.speed = agent.orbitRadius * agent.orbitSpeed;
                agent.state.hangAmount = 0.0f;
                agent.state.wingPose = std::sin(agent.flapPhase + .8f);
            }
            states_[index] = agent.state;
        }
    }

    void FruitBatColony::Update(const float deltaSeconds, const float observerX,
                                const float observerY, const float observerZ) noexcept
    {
        const bool valid = std::isfinite(deltaSeconds) && deltaSeconds >= 0.0f
            && std::isfinite(observerX) && std::isfinite(observerY) && std::isfinite(observerZ);
        const float boundedDelta = valid ? std::min(deltaSeconds, .1f) : 0.0f;
        fullSimulationCount_ = reducedSimulationCount_ = dormantSimulationCount_ = 0;
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float dx = agent.state.x - observerX;
            const float dy = agent.state.y - observerY;
            const float dz = agent.state.z - observerZ;
            agent.state.simulationTier = valid
                ? WildlifeActivationPolicy::Classify(
                    std::sqrt(dx * dx + dy * dy + dz * dz),
                    FullSimulationDistance, ReducedSimulationDistance)
                : WildlifeSimulationTier::Dormant;
            switch (agent.state.simulationTier) {
            case WildlifeSimulationTier::Full: ++fullSimulationCount_; break;
            case WildlifeSimulationTier::Reduced: ++reducedSimulationCount_; break;
            case WildlifeSimulationTier::Dormant: ++dormantSimulationCount_; break;
            }
            const auto divisor = WildlifeActivationPolicy::UpdateDivisor(agent.state.simulationTier);
            if (boundedDelta > 0.0f && frameIndex_ % divisor == index % divisor) {
                const float step = boundedDelta * static_cast<float>(divisor);
                agent.accumulatedSeconds += step;
                if (agent.state.behavior == FruitBatBehavior::DayFlight) {
                    const float angle = agent.orbitPhase + agent.accumulatedSeconds * agent.orbitSpeed;
                    agent.state.x = agent.roost.x + std::cos(angle) * agent.orbitRadius;
                    agent.state.z = agent.roost.z + std::sin(angle) * agent.orbitRadius;
                    agent.state.y = agent.roost.y + 3.2f
                        + .7f * std::sin(angle * 1.7f + static_cast<float>(index));
                    agent.state.headingRadians = angle + std::numbers::pi_v<float> * .5f;
                    agent.state.wingPose = std::sin(
                        agent.flapPhase + agent.accumulatedSeconds * 5.8f);
                }
            }
            states_[index] = agent.state;
        }
        ++frameIndex_;
    }
}
