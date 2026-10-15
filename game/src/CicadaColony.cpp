#include "IslandSimulator/CicadaColony.hpp"

#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/JungleLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;

        [[nodiscard]] float Distance(const CicadaState& cicada, const float x,
                                     const float y, const float z) noexcept
        {
            return std::hypot(std::hypot(cicada.x - x, cicada.z - z), cicada.y - y);
        }
    }

    CicadaColony::CicadaColony(const JungleLayout& jungle)
    {
        std::vector<const CanopyPlacement*> eligible;
        eligible.reserve(jungle.Canopy().size());
        for (const auto& tree : jungle.Canopy()) {
            if (tree.variant != MangroveCanopyVariant
                && tree.scale >= JungleLayout::CanopyMinimumScale
                && tree.scale <= JungleLayout::CanopyMaximumScale) {
                eligible.push_back(&tree);
            }
        }
        if (eligible.size() < Count) throw std::runtime_error("Not enough cicada host trees");

        for (std::size_t index = 0; index < Count; ++index) {
            const std::size_t treeIndex = ((index * 37U + 11U) * eligible.size())
                / (Count * 37U + 11U);
            const CanopyPlacement& tree = *eligible[std::min(treeIndex, eligible.size() - 1U)];
            const float localHeight = 0.92f + 0.18f * static_cast<float>(index % 6U);
            const float azimuth = tree.yawRadians + 0.51f
                + static_cast<float>(index) * 2.39996323f;
            const float radius = CanopyTrunkRadius(tree.variant, localHeight)
                * tree.scale * 1.045f;
            homes_[index] = {tree.x, tree.y + localHeight * tree.scale, tree.z,
                             radius, azimuth};

            Agent& agent = agents_[index];
            agent.state.scale = 0.88f + 0.06f * static_cast<float>(index % 5U);
            agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
            agent.state.behavior = index % 5U == 0U
                ? CicadaBehavior::Call : CicadaBehavior::Rest;
            agent.stateSeconds = agent.state.behavior == CicadaBehavior::Call
                ? 0.65f + 0.11f * static_cast<float>(index % 4U)
                : 1.2f + 0.31f * static_cast<float>(index % 7U);
            agent.state.wingPhase = static_cast<float>(index) * 0.73f;
            PlacePerched(index);
            states_[index] = agent.state;
        }
    }

    void CicadaColony::PlacePerched(const std::size_t index) noexcept
    {
        Agent& agent = agents_[index];
        const CicadaHome& home = homes_[index];
        agent.state.normalX = std::cos(home.azimuth);
        agent.state.normalY = 0.0f;
        agent.state.normalZ = std::sin(home.azimuth);
        agent.state.forwardX = 0.0f;
        agent.state.forwardY = 1.0f;
        agent.state.forwardZ = 0.0f;
        agent.state.x = home.x + agent.state.normalX * home.radius;
        agent.state.y = home.y;
        agent.state.z = home.z + agent.state.normalZ * home.radius;
    }

    void CicadaColony::PlaceFlying(const std::size_t index) noexcept
    {
        Agent& agent = agents_[index];
        const CicadaHome& home = homes_[index];
        const float progress = std::clamp(agent.flightProgress, 0.0f, 1.0f);
        const float angle = home.azimuth + progress * TwoPi * 0.72f;
        const float excursion = std::sin(progress * std::numbers::pi_v<float>);
        const float radius = home.radius + excursion * (1.15f + 0.12f * (index % 3U));
        agent.state.x = home.x + std::cos(angle) * radius;
        agent.state.y = home.y + excursion * (0.55f + 0.08f * (index % 4U));
        agent.state.z = home.z + std::sin(angle) * radius;
        agent.state.normalX = 0.0f;
        agent.state.normalY = 1.0f;
        agent.state.normalZ = 0.0f;
        const float tangentX = -std::sin(angle);
        const float tangentZ = std::cos(angle);
        agent.state.forwardX = tangentX;
        agent.state.forwardY = 0.12f * std::cos(progress * std::numbers::pi_v<float>);
        agent.state.forwardZ = tangentZ;
        const float length = std::hypot(std::hypot(agent.state.forwardX,
            agent.state.forwardZ), agent.state.forwardY);
        agent.state.forwardX /= length;
        agent.state.forwardY /= length;
        agent.state.forwardZ /= length;
    }

    void CicadaColony::Update(const float deltaSeconds, const float observerX,
                              const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        ++frameIndex_;
        for (std::size_t index = 0; index < Count; ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? Distance(agent.state, observerX, observerY, observerZ)
                : std::numeric_limits<float>::infinity();
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
            agent.accumulatedSeconds += safeDelta;
            const auto divisor = WildlifeActivationPolicy::UpdateDivisor(
                agent.state.simulationTier);
            if ((frameIndex_ + index) % divisor == 0U) {
                UpdateAgent(index, std::min(agent.accumulatedSeconds, 0.5f));
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }
    }

    void CicadaColony::UpdateAgent(const std::size_t index, const float deltaSeconds) noexcept
    {
        Agent& agent = agents_[index];
        if (deltaSeconds <= 0.0f) return;
        agent.stateSeconds -= deltaSeconds;
        agent.state.wingPhase = std::fmod(agent.state.wingPhase
            + deltaSeconds * (agent.state.behavior == CicadaBehavior::Flight ? 43.0f : 2.1f),
            TwoPi);

        if (agent.state.behavior == CicadaBehavior::Flight) {
            agent.flightProgress += deltaSeconds / (1.35f + 0.07f * (index % 4U));
            if (agent.flightProgress >= 1.0f) {
                agent.flightProgress = 0.0f;
                agent.state.behavior = CicadaBehavior::Rest;
                agent.stateSeconds = 4.0f + 0.37f * static_cast<float>(index % 7U);
                PlacePerched(index);
            } else {
                PlaceFlying(index);
            }
            return;
        }

        if (agent.stateSeconds <= 0.0f) {
            if (agent.state.behavior == CicadaBehavior::Rest) {
                agent.state.behavior = CicadaBehavior::Call;
                agent.stateSeconds = 1.45f + 0.12f * static_cast<float>(index % 4U);
            } else if (index % 3U == 0U) {
                agent.state.behavior = CicadaBehavior::Flight;
                agent.stateSeconds = 1.6f;
                agent.flightProgress = 0.0f;
            } else {
                agent.state.behavior = CicadaBehavior::Rest;
                agent.stateSeconds = 5.0f + 0.41f * static_cast<float>(index % 6U);
            }
        }
        if (agent.state.behavior == CicadaBehavior::Flight) PlaceFlying(index);
        else PlacePerched(index);
    }
}
