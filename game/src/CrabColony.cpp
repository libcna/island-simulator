#include "IslandSimulator/CrabColony.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr std::array<float, 6> ColonyAngles = {
            -2.72f, -1.78f, -0.18f, 0.68f, 1.58f, 2.52f};

        [[nodiscard]] float WrapAngle(float value) noexcept
        {
            while (value > std::numbers::pi_v<float>) value -= TwoPi;
            while (value < -std::numbers::pi_v<float>) value += TwoPi;
            return value;
        }

        [[nodiscard]] float MoveAngle(const float current, const float target,
                                      const float maximumStep) noexcept
        {
            const float difference = WrapAngle(target - current);
            return WrapAngle(current + std::clamp(difference, -maximumStep, maximumStep));
        }
    }

    CrabColony::CrabColony(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        agents_.resize(CrabCount);
        states_.resize(CrabCount);
        homes_.resize(CrabCount);
        for (std::size_t index = 0; index < CrabCount; ++index) {
            InitializeAgent(index);
            states_[index] = agents_[index].state;
            homes_[index] = agents_[index].home;
        }
        RebuildBurrows();
    }

    bool CrabColony::IsNearDebris(const CrabHomeRange& home,
        const std::span<const CrabHabitatCue> cues) noexcept
    {
        return std::ranges::any_of(cues, [&](const CrabHabitatCue cue) {
            return std::isfinite(cue.x) && std::isfinite(cue.z)
                && std::hypot(home.x - cue.x, home.z - cue.z) <= 3.25f;
        });
    }

    void CrabColony::SetDebrisHabitats(const std::span<const CrabHabitatCue> cues)
    {
        if (cues.empty()) throw std::invalid_argument("Crab debris habitat requires cues");
        CrabColony replacement(terrain_);
        std::array<bool, 6> selected{};
        for (std::size_t colony = 0; colony < ColonyAngles.size(); ++colony) {
            float best = std::numeric_limits<float>::infinity();
            for (std::size_t index = 0; index < cues.size(); ++index) {
                const auto cue = cues[index];
                if (!std::isfinite(cue.x) || !std::isfinite(cue.z))
                    throw std::invalid_argument("Crab debris cue must be finite");
                const auto sample = terrain_.Sample(cue.x, cue.z);
                const CrabHomeRange probe{cue.x, cue.z, 1.0f};
                if (sample.coastalRockiness >= .35f
                    || !replacement.IsValidHabitat(cue.x, cue.z, probe)) continue;
                bool separated = true;
                for (std::size_t previous = 0; previous < colony; ++previous)
                    separated = separated && std::hypot(cue.x - replacement.debrisAnchors_[previous].x,
                        cue.z - replacement.debrisAnchors_[previous].z) >= 28.0f;
                if (!separated) continue;
                const float difference = std::abs(WrapAngle(
                    std::atan2(cue.z, cue.x) - ColonyAngles[colony]));
                if (difference < best) {
                    best = difference;
                    replacement.debrisAnchors_[colony] = cue;
                    selected[colony] = true;
                }
            }
            if (!selected[colony]) throw std::runtime_error(
                "No separated sandy debris anchor for crab colony");
        }
        replacement.debrisAnchored_ = true;
        for (std::size_t index = 0; index < CrabCount; ++index) {
            replacement.InitializeAgent(index);
            const auto anchor = replacement.debrisAnchors_[index / 5U];
            if (std::hypot(replacement.agents_[index].home.x - anchor.x,
                           replacement.agents_[index].home.z - anchor.z) > 3.25f)
                throw std::runtime_error("Crab could not remain beside selected beach debris");
            replacement.states_[index] = replacement.agents_[index].state;
            replacement.homes_[index] = replacement.agents_[index].home;
        }
        replacement.RebuildBurrows();
        agents_.swap(replacement.agents_);
        states_.swap(replacement.states_);
        homes_.swap(replacement.homes_);
        debrisAnchors_ = replacement.debrisAnchors_;
        burrows_ = replacement.burrows_;
        debrisAnchored_ = true;
        frameIndex_ = 0;
    }

    void CrabColony::RebuildBurrows() noexcept
    {
        for (std::size_t colony = 0; colony < burrows_.size(); ++colony) {
            float x = 0.0f;
            float z = 0.0f;
            for (std::size_t member = 0; member < 5U; ++member) {
                const CrabHomeRange& home = homes_[colony * 5U + member];
                x += home.x;
                z += home.z;
            }
            x /= 5.0f;
            z /= 5.0f;
            burrows_[colony] = CrabBurrow{
                x, terrain_.Height(x, z) + 0.012f, z,
                0.24f + static_cast<float>(colony % 3U) * 0.035f};
        }
    }

    void CrabColony::InitializeAgent(const std::size_t index)
    {
        Agent& agent = agents_[index];
        const float sequence = static_cast<float>(index);
        const std::size_t colonyIndex = index / 5U;
        const float memberOffset = (static_cast<float>(index % 5U) - 2.0f) * 0.012f;
        const float authoredAngle = ColonyAngles[colonyIndex] + memberOffset;
        float angle = authoredAngle;
        const float landwardOffset = 3.4f + static_cast<float>(index % 6U) * 1.55f;
        const float tangentOffset = (static_cast<float>((index * 7U) % 9U) - 4.0f) * 0.42f;
        agent.home.radius = 5.8f + static_cast<float>(index % 4U) * 0.7f;
        const auto separatedFromEarlierColonyMembers=[&](const float x,const float z) {
            const std::size_t first=colonyIndex*5U;
            for(std::size_t previous=first;previous<index;++previous)
                if(std::hypot(x-agents_[previous].home.x,z-agents_[previous].home.z)<1.12f)
                    return false;
            return true;
        };
        bool foundHabitat = false;
        if (debrisAnchored_) {
            const auto anchor = debrisAnchors_[colonyIndex];
            angle = std::atan2(anchor.z, anchor.x);
            for (int attempt = 0; attempt < 64 && !foundHabitat; ++attempt) {
                const float radius = .58f + static_cast<float>(attempt) * .038f;
                const float theta = static_cast<float>(index) * 1.173f
                    + static_cast<float>(attempt) * 2.399963f;
                agent.home.x = anchor.x + std::cos(theta) * radius;
                agent.home.z = anchor.z + std::sin(theta) * radius;
                foundHabitat = IsValidHabitat(agent.home.x, agent.home.z, agent.home)
                    && separatedFromEarlierColonyMembers(agent.home.x,agent.home.z);
            }
        }
        for (int attempt = 0; attempt < 97 && !foundHabitat; ++attempt) {
            const float offset = static_cast<float>((attempt + 1) / 2) * 0.025f
                * (attempt % 2 == 0 ? -1.0f : 1.0f);
            angle = authoredAngle + offset;
            const float radius = terrain_.ShoreRadius(angle) - landwardOffset;
            agent.home.x = std::cos(angle) * radius - std::sin(angle) * tangentOffset;
            agent.home.z = std::sin(angle) * radius + std::cos(angle) * tangentOffset;
            const auto sample = terrain_.Sample(agent.home.x, agent.home.z);
            if (sample.coastalRockiness < 0.35f
                && IsValidHabitat(agent.home.x, agent.home.z, agent.home)
                && separatedFromEarlierColonyMembers(agent.home.x,agent.home.z)) {
                foundHabitat = true;
                break;
            }
        }
        if (!foundHabitat) throw std::runtime_error("No sandy shoreline habitat for beach crab");
        agent.state.x = agent.home.x;
        agent.state.z = agent.home.z;
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + 0.075f;
        agent.state.bodyYawRadians = WrapAngle(angle + std::numbers::pi_v<float>);
        agent.preferredDirection = index % 2U == 0U ? 1 : -1;
        agent.state.travelHeadingRadians = WrapAngle(
            angle + static_cast<float>(agent.preferredDirection)
                * std::numbers::pi_v<float> * 0.5f);
        agent.state.scale = 0.60f + static_cast<float>(index % 7U) * 0.038f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 4U);
        agent.state.behavior = index % 4U == 0U ? CrabBehavior::Pause : CrabBehavior::Forage;
        agent.behaviorSeconds = 1.2f + static_cast<float>(index % 8U) * 0.47f;
        agent.movementPhase = std::fmod(sequence * 1.173f + 0.4f, TwoPi);
        agent.animationPhase = std::fmod(sequence * 2.071f, TwoPi);
        agent.state.clawPose = 0.2f + static_cast<float>(index % 5U) * 0.08f;
    }

    bool CrabColony::IsValidHabitat(const float x, const float z,
                                     const CrabHomeRange& home) const noexcept
    {
        const TerrainSample sample = terrain_.Sample(x, z);
        const float homeDistance = std::hypot(x - home.x, z - home.z);
        const bool beachSurface = sample.surface == TerrainSurface::WetSand
            || sample.surface == TerrainSurface::DrySand
            || sample.surface == TerrainSurface::CoastalGround;
        return beachSurface && sample.height >= -0.02f
            && sample.signedShoreDistance >= MinimumShoreDistance
            && sample.signedShoreDistance <= MaximumShoreDistance
            && sample.slopeRadians < 0.38f && homeDistance <= home.radius;
    }

    void CrabColony::Update(const float deltaSeconds, const float observerX,
                            const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY)
            && std::isfinite(observerZ);
        ++frameIndex_;
        fullSimulationCount_ = 0;
        reducedSimulationCount_ = 0;
        dormantSimulationCount_ = 0;

        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float distance = observerValid
                ? std::hypot(agent.state.y - observerY,
                             std::hypot(agent.state.x - observerX,
                                        agent.state.z - observerZ))
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
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, 0.5f),
                            observerX, observerZ, observerValid);
                agent.accumulatedSeconds = 0.0f;
            }
            states_[index] = agent.state;
        }

#ifndef NDEBUG
        assert(fullSimulationCount_ + reducedSimulationCount_ + dormantSimulationCount_
               == static_cast<int>(CrabCount));
        for (const CrabState& crab : states_) {
            assert(std::isfinite(crab.x) && std::isfinite(crab.y) && std::isfinite(crab.z));
            assert(std::isfinite(crab.bodyYawRadians)
                   && std::isfinite(crab.travelHeadingRadians));
            assert(std::isfinite(crab.speed) && crab.speed >= 0.0f);
            assert(std::isfinite(crab.legPhase) && std::fabs(crab.legPhase) <= 1.001f);
            assert(std::isfinite(crab.clawPose)
                   && crab.clawPose >= 0.0f && crab.clawPose <= 1.0f);
        }
#endif
    }

    void CrabColony::UpdateAgent(Agent& agent, const float deltaSeconds,
                                 const float observerX, const float observerZ,
                                 const bool observerValid) noexcept
    {
        if (deltaSeconds <= 0.0f) return;

        const float observerDistance = observerValid
            ? std::hypot(agent.state.x - observerX, agent.state.z - observerZ)
            : std::numeric_limits<float>::infinity();
        if (observerDistance < 5.2f) {
            agent.state.behavior = CrabBehavior::Scuttle;
            agent.behaviorSeconds = 1.5f;
            float awayX = agent.state.x - observerX;
            float awayZ = agent.state.z - observerZ;
            const float awayLength = std::hypot(awayX, awayZ);
            if (awayLength > 0.001f) {
                awayX /= awayLength;
                awayZ /= awayLength;
                agent.state.travelHeadingRadians = std::atan2(awayZ, awayX);
            } else {
                agent.state.travelHeadingRadians += static_cast<float>(agent.preferredDirection)
                    * std::numbers::pi_v<float> * 0.5f;
            }
        } else {
            agent.behaviorSeconds -= deltaSeconds;
            if (agent.behaviorSeconds <= 0.0f) {
                ++agent.transitionCount;
                if (agent.state.behavior == CrabBehavior::Pause) {
                    agent.state.behavior = CrabBehavior::Forage;
                    agent.behaviorSeconds = 3.0f
                        + static_cast<float>((agent.transitionCount + agent.state.colorVariant)
                                             % 5U) * 0.63f;
                    agent.preferredDirection *= -1;
                } else {
                    agent.state.behavior = CrabBehavior::Pause;
                    agent.behaviorSeconds = 1.2f
                        + static_cast<float>((agent.transitionCount * 3U
                                             + agent.state.colorVariant) % 6U) * 0.48f;
                }
            }
        }

        float targetSpeed = 0.0f;
        if (agent.state.behavior == CrabBehavior::Forage) {
            targetSpeed = 0.42f + agent.state.scale * 0.16f;
            const float homeAngle = std::atan2(agent.home.z, agent.home.x);
            const float tangentHeading = homeAngle
                + static_cast<float>(agent.preferredDirection)
                    * std::numbers::pi_v<float> * 0.5f;
            agent.state.travelHeadingRadians = MoveAngle(
                agent.state.travelHeadingRadians,
                tangentHeading + std::sin(agent.movementPhase + agent.transitionCount * 0.7f)
                    * 0.42f,
                deltaSeconds * 1.2f);
        } else if (agent.state.behavior == CrabBehavior::Scuttle) {
            targetSpeed = 1.85f + agent.state.scale * 0.35f;
        }
        agent.state.speed += std::clamp(targetSpeed - agent.state.speed,
                                        -deltaSeconds * 2.8f, deltaSeconds * 4.8f);

        const float homeDistance = std::hypot(
            agent.state.x - agent.home.x, agent.state.z - agent.home.z);
        if (homeDistance > agent.home.radius * 0.72f) {
            const float homeHeading = std::atan2(
                agent.home.z - agent.state.z, agent.home.x - agent.state.x);
            agent.state.travelHeadingRadians = MoveAngle(
                agent.state.travelHeadingRadians, homeHeading, deltaSeconds * 2.4f);
        }

        const float candidateX = agent.state.x
            + std::cos(agent.state.travelHeadingRadians) * agent.state.speed * deltaSeconds;
        const float candidateZ = agent.state.z
            + std::sin(agent.state.travelHeadingRadians) * agent.state.speed * deltaSeconds;
        if (IsValidHabitat(candidateX, candidateZ, agent.home)) {
            agent.state.x = candidateX;
            agent.state.z = candidateZ;
        } else {
            agent.state.travelHeadingRadians = std::atan2(
                agent.home.z - agent.state.z, agent.home.x - agent.state.x);
            agent.state.speed = std::min(agent.state.speed, 0.65f);
        }
        agent.state.bodyYawRadians = MoveAngle(
            agent.state.bodyYawRadians,
            agent.state.travelHeadingRadians
                - static_cast<float>(agent.preferredDirection)
                    * std::numbers::pi_v<float> * 0.5f,
            deltaSeconds * 3.5f);
        agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + 0.075f;
        agent.movementPhase = std::fmod(agent.movementPhase + deltaSeconds * 0.73f, TwoPi);
        agent.animationPhase = std::fmod(
            agent.animationPhase + deltaSeconds * (4.0f + agent.state.speed * 8.0f), TwoPi);
        agent.state.legPhase = agent.state.speed > 0.08f
            ? std::sin(agent.animationPhase) : agent.state.legPhase * 0.82f;
        const float clawTarget = agent.state.behavior == CrabBehavior::Pause
            ? 0.60f + 0.16f * std::sin(agent.movementPhase * 1.7f)
            : 0.24f + 0.08f * std::sin(agent.movementPhase);
        agent.state.clawPose += std::clamp(
            clawTarget - agent.state.clawPose, -deltaSeconds * 1.8f, deltaSeconds * 1.8f);
    }

    std::span<const CrabState> CrabColony::Crabs() const noexcept
    {
        return states_;
    }

    std::span<const CrabHomeRange> CrabColony::Homes() const noexcept
    {
        return homes_;
    }
}
