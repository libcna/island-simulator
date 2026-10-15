#include "IslandSimulator/BirdFlock.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <iterator>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr float GoldenAngle = 2.39996322972865332f;

        [[nodiscard]] constexpr std::size_t FamilyIndex(
            const BirdPathFamily family) noexcept
        {
            switch (family) {
            case BirdPathFamily::Shore: return 0U;
            case BirdPathFamily::Canopy: return 1U;
            case BirdPathFamily::High: return 2U;
            }
            return 0U;
        }

        [[nodiscard]] float WrapAngle(float value) noexcept
        {
            while (value > std::numbers::pi_v<float>) {
                value -= TwoPi;
            }
            while (value < -std::numbers::pi_v<float>) {
                value += TwoPi;
            }
            return value;
        }

        [[nodiscard]] float LerpAngle(const float current, const float target,
                                      const float maximumStep) noexcept
        {
            const float difference = WrapAngle(target - current);
            return WrapAngle(current + std::clamp(difference, -maximumStep, maximumStep));
        }
    }

    BirdFlock::BirdFlock(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        agents_.resize(BirdCount);
        states_.resize(BirdCount);
        routes_.resize(BirdCount);
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            InitializeAgent(index);
            states_[index] = agents_[index].state;
            routes_[index] = agents_[index].route;
        }
        BuildRouteRegions();
    }

    void BirdFlock::InitializeAgent(const std::size_t index)
    {
        Agent& agent = agents_[index];
        agent.index = index;
        agent.nextPerchSeconds = 10.0f + static_cast<float>(index) * 5.75f;
        BirdRouteState& route = agent.route;
        const float sequence = static_cast<float>(index);
        const float angle = std::fmod(0.47f + sequence * GoldenAngle, TwoPi);
        route.direction = index % 3U == 0U ? -1 : 1;

        if (index < 10U) {
            agent.state.family = BirdPathFamily::Shore;
            agent.state.species = BirdSpecies::Coastal;
            route.preferredRadius = 218.0f + static_cast<float>(index % 5U) * 7.5f;
            route.baseAltitude = 28.0f + static_cast<float>(index % 4U) * 5.0f;
            route.minimumClearance = 17.0f;
            agent.state.speed = 10.0f + static_cast<float>(index % 4U) * 0.9f;
        } else if (index < 18U) {
            agent.state.family = BirdPathFamily::Canopy;
            agent.state.species = index < 14U ? BirdSpecies::Parrot : BirdSpecies::FruitDove;
            route.preferredRadius = 70.0f + static_cast<float>(index % 6U) * 16.0f;
            route.baseAltitude = 31.0f + static_cast<float>(index % 4U) * 4.0f;
            route.minimumClearance = 15.0f;
            agent.state.speed = 7.6f + static_cast<float>(index % 3U) * 1.0f;
        } else {
            agent.state.family = BirdPathFamily::High;
            agent.state.species = BirdSpecies::Seabird;
            route.preferredRadius = 125.0f + static_cast<float>(index % 5U) * 27.0f;
            route.baseAltitude = 71.0f + static_cast<float>(index % 4U) * 8.0f;
            route.minimumClearance = 46.0f;
            agent.state.speed = 12.4f + static_cast<float>(index % 4U) * 1.1f;
        }
        route.family = agent.state.family;

        const float groupOffset = static_cast<float>(index % 3U) * 0.055f;
        const float radius = route.preferredRadius
            + (static_cast<float>(index % 4U) - 1.5f) * 3.2f;
        const float spawnAngle = angle + groupOffset;
        agent.state.x = std::cos(spawnAngle) * radius;
        agent.state.z = std::sin(spawnAngle) * radius;
        agent.state.headingRadians = WrapAngle(
            spawnAngle + static_cast<float>(route.direction)
                * std::numbers::pi_v<float> * 0.5f);
        route.verticalAmplitude = 2.1f + static_cast<float>(index % 5U) * 0.55f;
        agent.verticalPhase = std::fmod(sequence * 1.713f, TwoPi);
        agent.verticalFrequency = 0.13f + static_cast<float>(index % 4U) * 0.019f;
        agent.wanderPhaseA = std::fmod(sequence * 0.873f + 0.3f, TwoPi);
        agent.wanderPhaseB = std::fmod(sequence * 1.379f + 1.1f, TwoPi);
        agent.wanderFrequencyA = 0.19f + static_cast<float>(index % 5U) * 0.017f;
        agent.wanderFrequencyB = 0.071f + static_cast<float>(index % 7U) * 0.011f;
        agent.radialDriftPhase = std::fmod(sequence * 1.137f + 0.6f, TwoPi);
        agent.radialDriftFrequency = 0.018f
            + static_cast<float>(index % 6U) * 0.0027f;
        switch (agent.state.family) {
        case BirdPathFamily::Shore:
            agent.radialDriftAmplitude = 9.0f + static_cast<float>(index % 3U) * 1.4f;
            break;
        case BirdPathFamily::Canopy:
            agent.radialDriftAmplitude = 7.0f + static_cast<float>(index % 4U) * 1.2f;
            break;
        case BirdPathFamily::High:
            agent.radialDriftAmplitude = 13.0f + static_cast<float>(index % 3U) * 2.0f;
            break;
        }
        agent.glidePhase = std::fmod(sequence * 1.541f + 0.9f, TwoPi);
        agent.glideFrequency = 0.105f + static_cast<float>(index % 5U) * 0.011f;
        agent.flapPhase = std::fmod(sequence * 1.97f, TwoPi);
        agent.flapSpeed = 5.2f + static_cast<float>(index % 6U) * 0.54f;
        agent.state.wingPose = std::sin(agent.flapPhase);
        agent.state.scale = 0.72f + static_cast<float>(index % 6U) * 0.085f;
        agent.state.colorVariant = static_cast<std::uint8_t>(index % 3U);
        const float terrainHeight = terrain_.Height(agent.state.x, agent.state.z);
        agent.state.y = std::max(route.baseAltitude
            + std::sin(agent.verticalPhase) * route.verticalAmplitude,
            terrainHeight + route.minimumClearance);
        route.spawnX = agent.state.x;
        route.spawnY = agent.state.y;
        route.spawnZ = agent.state.z;
        route.spawnHeadingRadians = agent.state.headingRadians;
    }

    void BirdFlock::SetPerchSites(const std::span<const BirdPerchSite> sites)
    {
        if (sites.size() > std::numeric_limits<std::uint8_t>::max()) {
            throw std::invalid_argument("Bird perch count exceeds the compact state index");
        }
        for (const BirdPerchSite& site : sites) {
            if (!std::isfinite(site.x) || !std::isfinite(site.y)
                || !std::isfinite(site.z) || !std::isfinite(site.headingRadians)
                || site.y < terrain_.Height(site.x, site.z) + 0.35f) {
                throw std::invalid_argument("Bird perch site is invalid or intersects terrain");
            }
        }
        perchSites_.assign(sites.begin(), sites.end());
        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            agent.state.behavior = BirdBehavior::Fly;
            agent.state.perchAmount = 0.0f;
            agent.behaviorSeconds = 0.0f;
            agent.perchCycle = 0U;
            states_[index] = agent.state;
        }
        const auto seedPerchedBird = [this](const std::size_t agentIndex,
                                            const BirdPerchKind kind) {
            const auto site = std::find_if(
                perchSites_.begin(), perchSites_.end(), [kind](const BirdPerchSite& candidate) {
                    return candidate.kind == kind;
                });
            if (site == perchSites_.end() || agentIndex >= agents_.size()) {
                return;
            }
            Agent& bird = agents_[agentIndex];
            const std::size_t siteIndex = static_cast<std::size_t>(
                std::distance(perchSites_.begin(), site));
            bird.state.x = site->x;
            bird.state.y = site->y + bird.state.scale * 0.12f;
            bird.state.z = site->z;
            bird.state.headingRadians = site->headingRadians;
            bird.state.speed = 0.0f;
            bird.state.wingPose = 0.0f;
            bird.state.bankRadians = 0.0f;
            bird.state.glideAmount = 0.0f;
            bird.state.perchAmount = 1.0f;
            bird.state.behavior = BirdBehavior::Perched;
            bird.state.perchSiteIndex = static_cast<std::uint8_t>(siteIndex);
            bird.behaviorSeconds = 0.0f;
            bird.perchCycle = 1U;
            states_[agentIndex] = bird.state;
        };
        seedPerchedBird(0U, BirdPerchKind::Wreck);
        Agent& dove = agents_[14U];
        bool seededGroundDove = false;
        for (std::size_t attempt = 0; attempt < 18U && !seededGroundDove; ++attempt) {
            const float phase = static_cast<float>(attempt) * GoldenAngle;
            const float radius = 2.5f + static_cast<float>(attempt % 5U) * 2.2f;
            const float x = VegetationLayout::EastClearing.x + std::cos(phase) * radius;
            const float z = VegetationLayout::EastClearing.z + std::sin(phase) * radius;
            const TerrainSample sample = terrain_.Sample(x, z);
            if (sample.IsSubmerged() || sample.height <= 0.8f
                || sample.signedShoreDistance <= 22.0f
                || sample.slopeRadians >= 0.24f) {
                continue;
            }
            dove.groundTargetX = x;
            dove.groundTargetZ = z;
            dove.state.x = x;
            dove.state.z = z;
            dove.state.y = sample.height + dove.state.scale * 0.20f;
            dove.state.headingRadians = WrapAngle(phase + 1.2f);
            dove.state.speed = 0.38f;
            dove.state.wingPose = 0.0f;
            dove.state.bankRadians = 0.0f;
            dove.state.glideAmount = 0.0f;
            dove.state.perchAmount = 1.0f;
            dove.state.behavior = BirdBehavior::GroundForage;
            dove.behaviorSeconds = 0.0f;
            dove.perchCycle = 1U;
            states_[14U] = dove.state;
            seededGroundDove = true;
        }
        if (!seededGroundDove) {
            throw std::runtime_error("Unable to seed the deterministic ground-dove habitat");
        }
    }

    void BirdFlock::BuildRouteRegions() noexcept
    {
        routeRegions_[0].family = BirdPathFamily::Shore;
        routeRegions_[1].family = BirdPathFamily::Canopy;
        routeRegions_[2].family = BirdPathFamily::High;
        for (BirdRouteRegion& region : routeRegions_) {
            region.minimumRadius = std::numeric_limits<float>::max();
            region.maximumRadius = std::numeric_limits<float>::lowest();
            region.minimumAltitude = std::numeric_limits<float>::max();
            region.maximumAltitude = std::numeric_limits<float>::lowest();
        }
        for (const BirdRouteState& route : routes_) {
            BirdRouteRegion& region = routeRegions_[FamilyIndex(route.family)];
            region.minimumRadius = std::min(region.minimumRadius, route.preferredRadius);
            region.maximumRadius = std::max(region.maximumRadius, route.preferredRadius);
            region.minimumAltitude = std::min(
                region.minimumAltitude, route.baseAltitude - route.verticalAmplitude);
            region.maximumAltitude = std::max(
                region.maximumAltitude, route.baseAltitude + route.verticalAmplitude);
            ++region.population;
        }
    }

    void BirdFlock::Update(const float deltaSeconds, const float observerX,
                           const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
        elapsedSeconds_ += safeDelta;
        ++frameIndex_;
        fullSimulationCount_ = 0;
        reducedSimulationCount_ = 0;
        dormantSimulationCount_ = 0;
        approachingCount_ = 0;
        perchedCount_ = 0;
        departingCount_ = 0;

        int activePerching = static_cast<int>(std::ranges::count_if(
            agents_, [](const Agent& agent) {
                return agent.state.behavior != BirdBehavior::Fly;
            }));

        for (std::size_t index = 0; index < agents_.size(); ++index) {
            Agent& agent = agents_[index];
            const float deltaX = agent.state.x - observerX;
            const float deltaY = agent.state.y - observerY;
            const float deltaZ = agent.state.z - observerZ;
            const float distance = std::hypot(deltaY, std::hypot(deltaX, deltaZ));
            agent.state.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
            switch (agent.state.simulationTier) {
            case WildlifeSimulationTier::Full:
                ++fullSimulationCount_;
                break;
            case WildlifeSimulationTier::Reduced:
                ++reducedSimulationCount_;
                break;
            case WildlifeSimulationTier::Dormant:
                ++dormantSimulationCount_;
                break;
            }

            agent.accumulatedSeconds += safeDelta;
            const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(
                agent.state.simulationTier);
            if ((frameIndex_ + index) % divisor == 0U) {
                const bool wasFlying = agent.state.behavior == BirdBehavior::Fly;
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, 0.5f),
                            activePerching < 2);
                const bool isFlying = agent.state.behavior == BirdBehavior::Fly;
                if (wasFlying && !isFlying) {
                    ++activePerching;
                } else if (!wasFlying && isFlying) {
                    --activePerching;
                }
                agent.accumulatedSeconds = 0.0f;
            }
            switch (agent.state.behavior) {
            case BirdBehavior::Fly: break;
            case BirdBehavior::Approach: ++approachingCount_; break;
            case BirdBehavior::Perched: ++perchedCount_; break;
            case BirdBehavior::Depart: ++departingCount_; break;
            case BirdBehavior::GroundApproach: ++approachingCount_; break;
            case BirdBehavior::GroundForage: ++perchedCount_; break;
            case BirdBehavior::GroundDepart: ++departingCount_; break;
            }
            states_[index] = agent.state;
        }
#ifndef NDEBUG
        assert(states_.size() == BirdCount);
        assert(fullSimulationCount_ + reducedSimulationCount_ + dormantSimulationCount_
               == static_cast<int>(BirdCount));
        for (const BirdState& state : states_) {
            assert(std::isfinite(state.x) && std::isfinite(state.y)
                   && std::isfinite(state.z) && std::isfinite(state.headingRadians));
            assert(std::isfinite(state.speed) && state.speed >= 0.0f);
            assert(std::isfinite(state.wingPose) && std::fabs(state.wingPose) <= 1.001f);
            assert(std::isfinite(state.bankRadians) && std::fabs(state.bankRadians) <= 0.341f);
            assert(std::isfinite(state.glideAmount)
                   && state.glideAmount >= 0.0f && state.glideAmount <= 1.0f);
            assert(std::isfinite(state.perchAmount)
                   && state.perchAmount >= 0.0f && state.perchAmount <= 1.0f);
            assert(std::isfinite(state.scale) && state.scale > 0.0f);
            assert(state.colorVariant < 3U);
        }
        assert(approachingCount_ + perchedCount_ + departingCount_ <= 2);
#endif
    }

    void BirdFlock::UpdateAgent(Agent& agent, const float deltaSeconds,
                                const bool allowNewPerch) noexcept
    {
        if (deltaSeconds <= 0.0f) {
            return;
        }
        const bool perchEligible = agent.state.family != BirdPathFamily::High;
        if (agent.state.behavior == BirdBehavior::Fly && perchEligible
            && allowNewPerch && !perchSites_.empty()
            && elapsedSeconds_ >= agent.nextPerchSeconds) {
            if (agent.state.species == BirdSpecies::FruitDove
                && agent.perchCycle % 2U == 0U) {
                for (std::size_t attempt = 0; attempt < 18U; ++attempt) {
                    const float side = attempt % 2U == 0U ? -1.0f : 1.0f;
                    const float offset = attempt == 0U ? 0.0f
                        : side * static_cast<float>((attempt + 1U) / 2U) * 0.12f;
                    const float phase = std::fmod(
                        std::atan2(agent.state.z, agent.state.x) + offset + TwoPi, TwoPi);
                    const float radius = std::clamp(
                        std::hypot(agent.state.x, agent.state.z) - 12.0f
                            + std::fmod(static_cast<float>(agent.index * 7U
                                + agent.perchCycle * 5U + attempt * 3U), 18.0f),
                        52.0f, 155.0f);
                    const float candidateX = std::cos(phase) * radius;
                    const float candidateZ = std::sin(phase) * radius;
                    const TerrainSample sample = terrain_.Sample(candidateX, candidateZ);
                    if (!sample.IsSubmerged() && sample.height > 0.8f
                        && sample.signedShoreDistance > 22.0f
                        && sample.slopeRadians < 0.30f) {
                        agent.groundTargetX = candidateX;
                        agent.groundTargetZ = candidateZ;
                        agent.state.behavior = BirdBehavior::GroundApproach;
                        agent.behaviorSeconds = 0.0f;
                        ++agent.perchCycle;
                        break;
                    }
                }
                if (agent.state.behavior == BirdBehavior::GroundApproach) {
                    UpdateGroundDove(agent, deltaSeconds);
                    return;
                }
            }
            float nearestDistanceSquared = std::numeric_limits<float>::max();
            std::size_t nearest = 0U;
            bool found = false;
            for (std::size_t index = 0; index < perchSites_.size(); ++index) {
                const BirdPerchSite& site = perchSites_[index];
                const bool suitable = agent.state.family == BirdPathFamily::Canopy
                    ? site.kind == BirdPerchKind::Branch
                    : site.kind != BirdPerchKind::Branch;
                if (!suitable) {
                    continue;
                }
                const float dx = site.x - agent.state.x;
                const float dy = site.y - agent.state.y;
                const float dz = site.z - agent.state.z;
                const float distanceSquared = dx * dx + dy * dy + dz * dz;
                if (distanceSquared < nearestDistanceSquared) {
                    nearestDistanceSquared = distanceSquared;
                    nearest = index;
                    found = true;
                }
            }
            if (found) {
                agent.state.perchSiteIndex = static_cast<std::uint8_t>(nearest);
                agent.state.behavior = BirdBehavior::Approach;
                agent.behaviorSeconds = 0.0f;
                ++agent.perchCycle;
            }
        }
        if (agent.state.behavior != BirdBehavior::Fly) {
            if (agent.state.behavior == BirdBehavior::GroundApproach
                || agent.state.behavior == BirdBehavior::GroundForage
                || agent.state.behavior == BirdBehavior::GroundDepart) {
                UpdateGroundDove(agent, deltaSeconds);
                return;
            }
            UpdatePerching(agent, deltaSeconds);
            return;
        }
        UpdateFlight(agent, deltaSeconds);
    }

    void BirdFlock::UpdateFlight(Agent& agent, const float deltaSeconds) noexcept
    {
        agent.state.perchAmount = std::max(
            0.0f, agent.state.perchAmount - deltaSeconds * 1.5f);
        const float radius = std::max(1.0f, std::hypot(agent.state.x, agent.state.z));
        const float radialX = agent.state.x / radius;
        const float radialZ = agent.state.z / radius;
        const float tangentX = -radialZ * static_cast<float>(agent.route.direction);
        const float tangentZ = radialX * static_cast<float>(agent.route.direction);
        const float preferredRadius = agent.route.preferredRadius
            + std::sin(elapsedSeconds_ * agent.radialDriftFrequency
                       + agent.radialDriftPhase) * agent.radialDriftAmplitude;
        const float radialCorrection = std::clamp(
            (preferredRadius - radius) * 0.018f, -0.52f, 0.52f);
        const float desiredX = tangentX + radialX * radialCorrection;
        const float desiredZ = tangentZ + radialZ * radialCorrection;
        const float wander = std::sin(elapsedSeconds_ * agent.wanderFrequencyA
                                      + agent.wanderPhaseA) * 0.18f
            + std::sin(elapsedSeconds_ * agent.wanderFrequencyB
                       + agent.wanderPhaseB) * 0.075f;
        const float desiredHeading = std::atan2(desiredZ, desiredX) + wander;
        const float headingError = WrapAngle(desiredHeading - agent.state.headingRadians);
        agent.state.headingRadians = LerpAngle(
            agent.state.headingRadians, desiredHeading, deltaSeconds * 0.82f);
        const float orbitBank = static_cast<float>(agent.route.direction)
            * std::clamp(agent.state.speed / radius * 1.35f, 0.035f, 0.17f);
        const float desiredBank = std::clamp(
            orbitBank + headingError * 0.58f, -0.34f, 0.34f);
        agent.state.bankRadians += std::clamp(
            desiredBank - agent.state.bankRadians,
            -deltaSeconds * 0.62f, deltaSeconds * 0.62f);

        const float speedVariation = 1.0f
            + std::sin(elapsedSeconds_ * 0.21f + agent.wanderPhaseB) * 0.055f;
        agent.state.x += std::cos(agent.state.headingRadians)
            * agent.state.speed * speedVariation * deltaSeconds;
        agent.state.z += std::sin(agent.state.headingRadians)
            * agent.state.speed * speedVariation * deltaSeconds;

        const float terrainHeight = terrain_.Height(agent.state.x, agent.state.z);
        const float lookAhead = agent.state.speed * 2.0f;
        const float upcomingHeight = terrain_.Height(
            agent.state.x + std::cos(agent.state.headingRadians) * lookAhead,
            agent.state.z + std::sin(agent.state.headingRadians) * lookAhead);
        const float verticalWave = std::sin(elapsedSeconds_ * agent.verticalFrequency * TwoPi
                                            + agent.verticalPhase);
        const float desiredAltitude = std::max(
            agent.route.baseAltitude + verticalWave * agent.route.verticalAmplitude,
            std::max(terrainHeight, upcomingHeight) + agent.route.minimumClearance + 2.0f);
        const float maximumVerticalStep = deltaSeconds * 4.5f;
        agent.state.y += std::clamp(desiredAltitude - agent.state.y,
                                    -maximumVerticalStep, maximumVerticalStep);
        agent.state.y = std::max(agent.state.y, terrainHeight + agent.route.minimumClearance);

        agent.flapPhase = std::fmod(agent.flapPhase + agent.flapSpeed * deltaSeconds, TwoPi);
        const float glideSignal = std::sin(
            elapsedSeconds_ * agent.glideFrequency + agent.glidePhase)
            + 0.38f * std::sin(elapsedSeconds_ * agent.glideFrequency * 0.417f
                              + agent.glidePhase * 1.73f);
        const float targetGlide = std::clamp((glideSignal - 0.42f) / 0.42f, 0.0f, 1.0f);
        agent.state.glideAmount += std::clamp(
            targetGlide - agent.state.glideAmount,
            -deltaSeconds * 0.72f, deltaSeconds * 0.72f);
        const float flapAmplitude = 0.72f
            + 0.28f * std::sin(elapsedSeconds_ * 0.31f + agent.verticalPhase);
        const float flappingPose = std::sin(agent.flapPhase) * flapAmplitude;
        const float glidingPose = 0.11f
            + std::sin(elapsedSeconds_ * 0.23f + agent.wanderPhaseA) * 0.035f;
        agent.state.wingPose = std::lerp(
            flappingPose, glidingPose, agent.state.glideAmount);
    }

    void BirdFlock::UpdatePerching(Agent& agent, const float deltaSeconds) noexcept
    {
        if (agent.state.perchSiteIndex >= perchSites_.size()) {
            agent.state.behavior = BirdBehavior::Fly;
            agent.state.perchAmount = 0.0f;
            agent.nextPerchSeconds = elapsedSeconds_ + 60.0f;
            return;
        }
        const BirdPerchSite& site = perchSites_[agent.state.perchSiteIndex];
        agent.behaviorSeconds += deltaSeconds;
        const float restingY = site.y + agent.state.scale * 0.12f;

        if (agent.state.behavior == BirdBehavior::Approach) {
            const float dx = site.x - agent.state.x;
            const float dz = site.z - agent.state.z;
            const float horizontalDistance = std::hypot(dx, dz);
            const float desiredHeading = horizontalDistance > 0.0001f
                ? std::atan2(dz, dx) : site.headingRadians;
            const float headingError = WrapAngle(desiredHeading - agent.state.headingRadians);
            agent.state.headingRadians = LerpAngle(
                agent.state.headingRadians, desiredHeading, deltaSeconds * 1.45f);
            agent.state.bankRadians += std::clamp(
                headingError * 0.32f - agent.state.bankRadians,
                -deltaSeconds * 0.9f, deltaSeconds * 0.9f);
            agent.state.bankRadians = std::clamp(
                agent.state.bankRadians, -0.34f, 0.34f);
            const float step = std::min(horizontalDistance,
                (6.8f + std::min(horizontalDistance, 20.0f) * 0.08f) * deltaSeconds);
            if (horizontalDistance > 0.0001f) {
                agent.state.x += dx / horizontalDistance * step;
                agent.state.z += dz / horizontalDistance * step;
            }
            const float approachLift = std::clamp(horizontalDistance * 0.10f, 0.0f, 3.2f);
            const float targetY = restingY + approachLift;
            const float terrainFloor = terrain_.Height(agent.state.x, agent.state.z) + 0.75f;
            agent.state.y += std::clamp(
                std::max(targetY, terrainFloor) - agent.state.y,
                -deltaSeconds * 4.8f, deltaSeconds * 4.8f);
            agent.state.y = std::max(agent.state.y, terrainFloor);
            agent.state.speed = 6.8f;
            agent.state.glideAmount = std::max(0.0f,
                agent.state.glideAmount - deltaSeconds * 1.2f);
            agent.flapPhase = std::fmod(agent.flapPhase + agent.flapSpeed * deltaSeconds,
                                        TwoPi);
            agent.state.wingPose = std::sin(agent.flapPhase) * 0.72f;
            agent.state.perchAmount = std::clamp(
                1.0f - horizontalDistance / 5.0f, 0.0f, 1.0f);
            if (horizontalDistance < 0.24f
                && std::abs(agent.state.y - restingY) < 0.24f) {
                agent.state.x = site.x;
                agent.state.y = restingY;
                agent.state.z = site.z;
                agent.state.headingRadians = site.headingRadians;
                agent.state.behavior = BirdBehavior::Perched;
                agent.behaviorSeconds = 0.0f;
                agent.state.perchAmount = 1.0f;
            }
            return;
        }

        if (agent.state.behavior == BirdBehavior::Perched) {
            agent.state.x = site.x;
            agent.state.y = restingY;
            agent.state.z = site.z;
            agent.state.headingRadians = site.headingRadians;
            agent.state.speed = 0.0f;
            agent.state.bankRadians = 0.0f;
            agent.state.glideAmount = 0.0f;
            agent.state.perchAmount = 1.0f;
            agent.state.wingPose = 0.035f * std::sin(
                elapsedSeconds_ * 1.7f + static_cast<float>(agent.index));
            const float duration = 8.0f + std::fmod(
                static_cast<float>(agent.index) * 1.7f
                    + static_cast<float>(agent.perchCycle) * 1.3f,
                6.0f);
            if (agent.behaviorSeconds >= duration) {
                agent.state.behavior = BirdBehavior::Depart;
                agent.behaviorSeconds = 0.0f;
            }
            return;
        }

        agent.state.headingRadians = LerpAngle(
            agent.state.headingRadians, site.headingRadians, deltaSeconds * 0.8f);
        agent.state.speed = 8.2f;
        agent.state.x += std::cos(agent.state.headingRadians)
            * agent.state.speed * deltaSeconds;
        agent.state.z += std::sin(agent.state.headingRadians)
            * agent.state.speed * deltaSeconds;
        agent.state.y += deltaSeconds * 2.6f;
        agent.state.y = std::max(agent.state.y,
            terrain_.Height(agent.state.x, agent.state.z) + 1.2f);
        agent.flapPhase = std::fmod(agent.flapPhase + agent.flapSpeed * deltaSeconds, TwoPi);
        agent.state.wingPose = std::sin(agent.flapPhase) * 0.82f;
        agent.state.bankRadians *= std::max(0.0f, 1.0f - deltaSeconds * 2.0f);
        agent.state.glideAmount = 0.0f;
        agent.state.perchAmount = std::max(0.0f, 1.0f - agent.behaviorSeconds / 1.1f);
        if (agent.behaviorSeconds >= 3.2f) {
            agent.state.behavior = BirdBehavior::Fly;
            agent.state.perchAmount = 0.0f;
            agent.state.y = std::max(agent.state.y,
                terrain_.Height(agent.state.x, agent.state.z)
                    + agent.route.minimumClearance);
            agent.nextPerchSeconds = elapsedSeconds_ + 58.0f + std::fmod(
                static_cast<float>(agent.index) * 7.0f
                    + static_cast<float>(agent.perchCycle) * 11.0f,
                52.0f);
        }
    }

    void BirdFlock::UpdateGroundDove(Agent& agent, const float deltaSeconds) noexcept
    {
        agent.behaviorSeconds += deltaSeconds;
        const float bodyClearance = agent.state.scale * 0.20f;
        if (agent.state.behavior == BirdBehavior::GroundApproach) {
            const float dx = agent.groundTargetX - agent.state.x;
            const float dz = agent.groundTargetZ - agent.state.z;
            const float distance = std::hypot(dx, dz);
            const float desiredHeading = distance > 0.0001f
                ? std::atan2(dz, dx) : agent.state.headingRadians;
            const float headingError = WrapAngle(desiredHeading - agent.state.headingRadians);
            agent.state.headingRadians = LerpAngle(
                agent.state.headingRadians, desiredHeading, deltaSeconds * 1.35f);
            agent.state.bankRadians += std::clamp(
                headingError * 0.28f - agent.state.bankRadians,
                -deltaSeconds * 0.8f, deltaSeconds * 0.8f);
            agent.state.bankRadians = std::clamp(
                agent.state.bankRadians, -0.34f, 0.34f);
            const float step = std::min(distance, 6.2f * deltaSeconds);
            if (distance > 0.0001f) {
                agent.state.x += dx / distance * step;
                agent.state.z += dz / distance * step;
            }
            const float ground = terrain_.Height(agent.state.x, agent.state.z);
            const float approachLift = std::clamp(distance * 0.09f, 0.0f, 3.6f);
            const float targetY = ground + bodyClearance + approachLift;
            agent.state.y += std::clamp(targetY - agent.state.y,
                -deltaSeconds * 4.6f, deltaSeconds * 4.6f);
            agent.state.y = std::max(agent.state.y, ground + bodyClearance);
            agent.state.speed = 6.2f;
            agent.flapPhase = std::fmod(agent.flapPhase + agent.flapSpeed * deltaSeconds, TwoPi);
            agent.state.wingPose = std::sin(agent.flapPhase) * 0.68f;
            agent.state.glideAmount = 0.0f;
            agent.state.perchAmount = std::clamp(1.0f - distance / 5.0f, 0.0f, 1.0f);
            if (distance < 0.18f && std::abs(agent.state.y - targetY) < 0.2f) {
                agent.state.x = agent.groundTargetX;
                agent.state.z = agent.groundTargetZ;
                agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + bodyClearance;
                agent.state.behavior = BirdBehavior::GroundForage;
                agent.behaviorSeconds = 0.0f;
                agent.state.bankRadians = 0.0f;
                agent.state.perchAmount = 1.0f;
                agent.state.wingPose = 0.0f;
            }
            return;
        }

        if (agent.state.behavior == BirdBehavior::GroundForage) {
            const float turn = std::sin(elapsedSeconds_ * 0.73f
                + static_cast<float>(agent.index) * 0.91f) * 0.42f;
            agent.state.headingRadians = WrapAngle(
                agent.state.headingRadians + turn * deltaSeconds);
            const float speed = 0.34f + 0.12f * std::sin(
                elapsedSeconds_ * 1.31f + static_cast<float>(agent.index));
            const float candidateX = agent.state.x
                + std::cos(agent.state.headingRadians) * speed * deltaSeconds;
            const float candidateZ = agent.state.z
                + std::sin(agent.state.headingRadians) * speed * deltaSeconds;
            const TerrainSample candidate = terrain_.Sample(candidateX, candidateZ);
            const float targetDistance = std::hypot(
                candidateX - agent.groundTargetX, candidateZ - agent.groundTargetZ);
            if (!candidate.IsSubmerged() && candidate.slopeRadians < 0.48f
                && targetDistance < 2.4f) {
                agent.state.x = candidateX;
                agent.state.z = candidateZ;
            } else {
                agent.state.headingRadians = std::atan2(
                    agent.groundTargetZ - agent.state.z,
                    agent.groundTargetX - agent.state.x);
            }
            agent.state.y = terrain_.Height(agent.state.x, agent.state.z) + bodyClearance;
            agent.state.speed = std::max(0.0f, speed);
            agent.state.bankRadians = 0.0f;
            agent.state.glideAmount = 0.0f;
            agent.state.perchAmount = 1.0f;
            agent.state.wingPose = 0.025f * std::sin(
                elapsedSeconds_ * 2.1f + static_cast<float>(agent.index));
            const float duration = 10.0f + std::fmod(
                static_cast<float>(agent.index) * 1.9f
                    + static_cast<float>(agent.perchCycle) * 1.4f, 7.0f);
            if (agent.behaviorSeconds >= duration) {
                agent.state.behavior = BirdBehavior::GroundDepart;
                agent.behaviorSeconds = 0.0f;
            }
            return;
        }

        agent.state.speed = 7.2f;
        agent.state.x += std::cos(agent.state.headingRadians)
            * agent.state.speed * deltaSeconds;
        agent.state.z += std::sin(agent.state.headingRadians)
            * agent.state.speed * deltaSeconds;
        const float ground = terrain_.Height(agent.state.x, agent.state.z);
        agent.state.y = std::max(agent.state.y + deltaSeconds * 2.9f,
            ground + bodyClearance);
        agent.flapPhase = std::fmod(agent.flapPhase + agent.flapSpeed * deltaSeconds, TwoPi);
        agent.state.wingPose = std::sin(agent.flapPhase) * 0.82f;
        agent.state.bankRadians *= std::max(0.0f, 1.0f - deltaSeconds * 2.0f);
        agent.state.glideAmount = 0.0f;
        agent.state.perchAmount = std::max(0.0f, 1.0f - agent.behaviorSeconds / 1.0f);
        if (agent.behaviorSeconds >= 3.2f) {
            agent.state.behavior = BirdBehavior::Fly;
            agent.state.perchAmount = 0.0f;
            agent.state.y = std::max(agent.state.y,
                terrain_.Height(agent.state.x, agent.state.z)
                    + agent.route.minimumClearance);
            agent.nextPerchSeconds = elapsedSeconds_ + 54.0f + std::fmod(
                static_cast<float>(agent.index) * 5.0f
                    + static_cast<float>(agent.perchCycle) * 9.0f, 43.0f);
        }
    }

    std::span<const BirdState> BirdFlock::Birds() const noexcept
    {
        return states_;
    }

    std::span<const BirdRouteState> BirdFlock::Routes() const noexcept
    {
        return routes_;
    }

    std::span<const BirdRouteRegion> BirdFlock::RouteRegions() const noexcept
    {
        return routeRegions_;
    }
}
