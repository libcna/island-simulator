#include "IslandSimulator/AntTrails.hpp"

#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
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

        [[nodiscard]] float Length(const float x, const float y, const float z) noexcept
        {
            return std::sqrt(x*x+y*y+z*z);
        }
    }

    AntTrails::AntTrails(const IslandTerrain& terrain, const JungleLayout& jungle)
        : terrain_(terrain)
    {
        constexpr float PathTrailZ=-70.0f;
        const float pathTrailX=VegetationLayout::PrimaryPathCenterX(PathTrailZ);
        trails_[0] = {pathTrailX,terrain.Height(pathTrailX,PathTrailZ),PathTrailZ,2.8f,false};
        trails_[1] = {VegetationLayout::WestClearing.x + 4.0f,
                      terrain.Height(VegetationLayout::WestClearing.x + 4.0f,
                                     VegetationLayout::WestClearing.z - 2.0f),
                      VegetationLayout::WestClearing.z - 2.0f, 2.5f, false};
        trails_[2] = {-4.0f, terrain.Height(-4.0f, -137.0f), -137.0f, 2.2f, false};
        trailAngles_ = {.38f, 2.18f, -0.74f, 0.0f};

        const CanopyPlacement* trunk = nullptr;
        float bestDistance = std::numeric_limits<float>::infinity();
        for (const auto& candidate : jungle.Canopy()) {
            if (candidate.variant == MangroveCanopyVariant
                || candidate.scale < JungleLayout::CanopyMinimumScale
                || candidate.scale > JungleLayout::CanopyMaximumScale) continue;
            const float distance = std::hypot(candidate.x - VegetationLayout::EastClearing.x,
                                              candidate.z - VegetationLayout::EastClearing.z);
            if (distance < bestDistance) { bestDistance = distance; trunk = &candidate; }
        }
        if (!trunk) throw std::runtime_error("No suitable ant trail trunk");
        trails_[3] = {trunk->x, trunk->y, trunk->z, 1.85f*trunk->scale, true};
        trunkVariants_[3] = trunk->variant;
        trunkScales_[3] = trunk->scale;
        trailAngles_[3] = trunk->yawRadians + .73f;

        for (std::size_t index = 0; index < Count; ++index) {
            Initialize(index);
            states_[index] = agents_[index].state;
        }
    }

    void AntTrails::Initialize(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const std::size_t member = index % PerTrail;
        agent.state.trailIndex = static_cast<std::uint8_t>(index / PerTrail);
        agent.progress = std::fmod((static_cast<float>(member) + .31f * agent.state.trailIndex)
            / static_cast<float>(PerTrail), 1.0f);
        agent.outward = member % 2U == 0U;
        agent.speed = .075f + .009f * static_cast<float>(index % 5U);
        agent.state.scale = .86f + .07f * static_cast<float>(index % 4U);
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
        agent.state.behavior = member % 11U == 0U ? AntBehavior::Pause
            : agent.outward ? AntBehavior::Walk : AntBehavior::Carry;
        agent.pauseSeconds = agent.state.behavior == AntBehavior::Pause
            ? .35f + .08f * static_cast<float>(member % 4U) : 0.0f;
        agent.state.gaitPhase = static_cast<float>(index) * .79f;
        Place(agent);
    }

    void AntTrails::Place(Agent& agent) noexcept
    {
        const std::size_t trailIndex = agent.state.trailIndex;
        const auto& trail = trails_[trailIndex];
        const float path = agent.outward ? agent.progress : 1.0f-agent.progress;
        if (trail.onTrunk) {
            const float spiral = trailAngles_[trailIndex] + path * .42f;
            const float localHeight=.10f+path*1.85f;
            const float radius = CanopyTrunkRadius(trunkVariants_[trailIndex],localHeight)
                * trunkScales_[trailIndex] * 1.035f;
            agent.state.normalX = std::cos(spiral);
            agent.state.normalY = 0.0f;
            agent.state.normalZ = std::sin(spiral);
            agent.state.x = trail.x + agent.state.normalX * radius;
            agent.state.y = trail.y + localHeight * trunkScales_[trailIndex];
            agent.state.z = trail.z + agent.state.normalZ * radius;
            const float direction = agent.outward ? 1.0f : -1.0f;
            agent.state.forwardX = -.10f * std::sin(spiral) * direction;
            agent.state.forwardY = .995f * direction;
            agent.state.forwardZ = .10f * std::cos(spiral) * direction;
        } else {
            const float angle = trailAngles_[trailIndex];
            const float along = (path * 2.0f - 1.0f) * trail.halfLength;
            const float side = std::sin(path * TwoPi + trailIndex * .81f) * .34f;
            const float c = std::cos(angle), s = std::sin(angle);
            agent.state.x = trail.x + c * along - s * side;
            agent.state.z = trail.z + s * along + c * side;
            const TerrainSample sample = terrain_.Sample(agent.state.x, agent.state.z);
            agent.state.y = sample.height + .014f * agent.state.scale;
            agent.state.normalX = sample.normal.x;
            agent.state.normalY = sample.normal.y;
            agent.state.normalZ = sample.normal.z;
            const float lateralDerivative = std::cos(path * TwoPi + trailIndex * .81f)
                * .34f * TwoPi / (2.0f * trail.halfLength);
            const float direction = agent.outward ? 1.0f : -1.0f;
            float fx = (c - s * lateralDerivative) * direction;
            float fz = (s + c * lateralDerivative) * direction;
            const float fy = -(fx * sample.normal.x + fz * sample.normal.z)
                / std::max(.2f, sample.normal.y);
            const float length = Length(fx, fy, fz);
            agent.state.forwardX = fx / length;
            agent.state.forwardY = fy / length;
            agent.state.forwardZ = fz / length;
        }
    }

    void AntTrails::Update(const float deltaSeconds, const float observerX,
                           const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds,0.0f,.1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        ++frameIndex_;
        for (std::size_t index = 0; index < Count; ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? Length(agent.state.x-observerX,agent.state.y-observerY,agent.state.z-observerZ)
                : std::numeric_limits<float>::infinity();
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance,FullSimulationDistance,ReducedSimulationDistance);
            agent.accumulatedSeconds += safeDelta;
            const auto divisor = WildlifeActivationPolicy::UpdateDivisor(agent.state.simulationTier);
            if ((frameIndex_+index)%divisor==0U) {
                UpdateAgent(agent,std::min(agent.accumulatedSeconds,.5f));
                agent.accumulatedSeconds=0.0f;
            }
            states_[index]=agent.state;
        }
    }

    void AntTrails::UpdateAgent(Agent& agent, const float deltaSeconds) noexcept
    {
        if (deltaSeconds <= 0.0f) return;
        if (agent.pauseSeconds > 0.0f) {
            agent.pauseSeconds = std::max(0.0f,agent.pauseSeconds-deltaSeconds);
            agent.state.behavior = AntBehavior::Pause;
            return;
        }
        const auto& trail = trails_[agent.state.trailIndex];
        agent.progress += agent.speed * deltaSeconds / (trail.halfLength*2.0f);
        if (agent.progress >= 1.0f) {
            agent.progress -= 1.0f;
            agent.outward = !agent.outward;
            if ((static_cast<unsigned>(agent.state.gaitPhase*10.0f)
                    + agent.state.trailIndex)%7U==0U) agent.pauseSeconds=.28f;
        }
        agent.state.behavior = agent.outward ? AntBehavior::Walk : AntBehavior::Carry;
        agent.state.gaitPhase = std::fmod(agent.state.gaitPhase
            + deltaSeconds * 17.0f,TwoPi);
        Place(agent);
    }
}
