#include "IslandSimulator/BirdFlock.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

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
        BirdRouteState& route = agent.route;
        const float sequence = static_cast<float>(index);
        const float angle = std::fmod(0.47f + sequence * GoldenAngle, TwoPi);
        route.direction = index % 3U == 0U ? -1 : 1;

        if (index < 10U) {
            agent.state.family = BirdPathFamily::Shore;
            route.preferredRadius = 218.0f + static_cast<float>(index % 5U) * 7.5f;
            route.baseAltitude = 28.0f + static_cast<float>(index % 4U) * 5.0f;
            route.minimumClearance = 17.0f;
            agent.state.speed = 10.0f + static_cast<float>(index % 4U) * 0.9f;
        } else if (index < 18U) {
            agent.state.family = BirdPathFamily::Canopy;
            route.preferredRadius = 70.0f + static_cast<float>(index % 6U) * 16.0f;
            route.baseAltitude = 31.0f + static_cast<float>(index % 4U) * 4.0f;
            route.minimumClearance = 15.0f;
            agent.state.speed = 7.6f + static_cast<float>(index % 3U) * 1.0f;
        } else {
            agent.state.family = BirdPathFamily::High;
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
                UpdateAgent(agent, std::min(agent.accumulatedSeconds, 0.5f));
                agent.accumulatedSeconds = 0.0f;
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
            assert(std::isfinite(state.scale) && state.scale > 0.0f);
            assert(state.colorVariant < 3U);
        }
#endif
    }

    void BirdFlock::UpdateAgent(Agent& agent, const float deltaSeconds) noexcept
    {
        if (deltaSeconds <= 0.0f) {
            return;
        }
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
