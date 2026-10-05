#include "IslandSimulator/FishSchools.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr float MinimumShoreDistance = -48.0f;
        constexpr float MaximumShoreDistance = -3.0f;
        constexpr float MinimumSeabedDepth = 0.85f;

        [[nodiscard]] float WrapAngle(float angle) noexcept
        {
            while (angle >= TwoPi) {
                angle -= TwoPi;
            }
            while (angle < 0.0f) {
                angle += TwoPi;
            }
            return angle;
        }

        [[nodiscard]] float SignedAngleDifference(float from, float to) noexcept
        {
            float difference = to - from;
            while (difference > std::numbers::pi_v<float>) {
                difference -= TwoPi;
            }
            while (difference < -std::numbers::pi_v<float>) {
                difference += TwoPi;
            }
            return difference;
        }

        [[nodiscard]] float FishDepth(const float seabedHeight, const float depthPhase,
                                      const float elapsedSeconds) noexcept
        {
            const float desired = -0.72f
                + std::sin(elapsedSeconds * 0.61f + depthPhase) * 0.22f;
            const float lower = seabedHeight + 0.34f;
            const float upper = -0.24f;
            return lower <= upper ? std::clamp(desired, lower, upper) : upper;
        }
    }

    FishSchools::FishSchools(const IslandTerrain& terrain)
        : terrain_(terrain)
    {
        schools_.resize(SchoolCount);
        agents_.resize(FishCount);
        states_.resize(FishCount);
        for (std::size_t schoolIndex = 0; schoolIndex < schools_.size(); ++schoolIndex) {
            InitializeSchool(schoolIndex);
        }
        for (std::size_t fishIndex = 0; fishIndex < agents_.size(); ++fishIndex) {
            InitializeAgent(fishIndex);
            states_[fishIndex] = agents_[fishIndex].state;
        }
    }

    void FishSchools::InitializeSchool(const std::size_t schoolIndex)
    {
        School& school = schools_[schoolIndex];
        school.routeAngle = WrapAngle(-std::numbers::pi_v<float> * 0.5f
            + static_cast<float>(schoolIndex) * TwoPi / static_cast<float>(SchoolCount)
            + static_cast<float>(schoolIndex % 3U) * 0.037f);
        school.offshoreDistance = 17.0f + static_cast<float>(schoolIndex % 4U) * 3.4f;
        school.routeSpeed = 0.30f + static_cast<float>(schoolIndex % 3U) * 0.075f;
        school.direction = schoolIndex % 2U == 0U ? 1 : -1;
        ResolveSchoolCentre(school);
    }

    void FishSchools::InitializeAgent(const std::size_t fishIndex)
    {
        Agent& agent = agents_[fishIndex];
        const std::size_t schoolIndex = fishIndex / FishPerSchool;
        const std::size_t memberIndex = fishIndex % FishPerSchool;
        const School& school = schools_[schoolIndex];
        const float radialLength = std::max(1.0f, std::hypot(school.centreX, school.centreZ));
        const float radialX = school.centreX / radialLength;
        const float radialZ = school.centreZ / radialLength;
        const float tangentX = -radialZ * static_cast<float>(school.direction);
        const float tangentZ = radialX * static_cast<float>(school.direction);
        agent.alongOffset = (static_cast<float>(memberIndex % 3U) - 1.0f) * 1.7f;
        agent.radialOffset = (memberIndex < 3U ? -1.0f : 1.0f)
            * (0.65f + static_cast<float>(memberIndex % 3U) * 0.18f);
        agent.wanderPhase = std::fmod(static_cast<float>(fishIndex) * 1.271f + 0.4f, TwoPi);
        agent.wanderFrequency = 0.43f + static_cast<float>(fishIndex % 5U) * 0.041f;
        agent.depthPhase = std::fmod(static_cast<float>(fishIndex) * 0.917f, TwoPi);
        agent.bodyPhase = std::fmod(static_cast<float>(fishIndex) * 1.733f, TwoPi);
        agent.bodySpeed = 6.1f + static_cast<float>(fishIndex % 6U) * 0.43f;
        agent.state.x = school.centreX + tangentX * agent.alongOffset
            + radialX * agent.radialOffset;
        agent.state.z = school.centreZ + tangentZ * agent.alongOffset
            + radialZ * agent.radialOffset;
        const TerrainSample sample = terrain_.Sample(agent.state.x, agent.state.z);
        agent.state.y = FishDepth(sample.height, agent.depthPhase, elapsedSeconds_);
        agent.state.headingRadians = std::atan2(tangentZ, tangentX)
            + std::sin(agent.wanderPhase) * 0.12f;
        agent.state.speed = 1.05f + static_cast<float>(fishIndex % 5U) * 0.11f;
        agent.state.bodyWave = std::sin(agent.bodyPhase);
        agent.state.scale = 0.76f + static_cast<float>(fishIndex % 6U) * 0.065f;
        agent.state.schoolIndex = static_cast<std::uint8_t>(schoolIndex);
        agent.state.colorVariant = static_cast<std::uint8_t>((schoolIndex + memberIndex) % 4U);
    }

    void FishSchools::ResolveSchoolCentre(School& school) const noexcept
    {
        float radius = terrain_.ShoreRadius(school.routeAngle) + school.offshoreDistance;
        float x = std::cos(school.routeAngle) * radius;
        float z = std::sin(school.routeAngle) * radius;
        TerrainSample sample = terrain_.Sample(x, z);
        for (int attempt = 0; attempt < 8 && sample.height > -MinimumSeabedDepth; ++attempt) {
            radius += 2.0f;
            x = std::cos(school.routeAngle) * radius;
            z = std::sin(school.routeAngle) * radius;
            sample = terrain_.Sample(x, z);
        }
        school.centreX = x;
        school.centreZ = z;
        school.centreY = FishDepth(sample.height, school.routeAngle, elapsedSeconds_);
    }

    void FishSchools::Update(const float deltaSeconds, const float observerX,
                             const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::clamp(deltaSeconds, 0.0f, 0.1f);
        elapsedSeconds_ += safeDelta;
        ++frameIndex_;
        fullSimulationCount_ = 0;
        reducedSimulationCount_ = 0;
        dormantSimulationCount_ = 0;

        for (std::size_t schoolIndex = 0; schoolIndex < schools_.size(); ++schoolIndex) {
            School& school = schools_[schoolIndex];
            const float deltaX = school.centreX - observerX;
            const float deltaY = school.centreY - observerY;
            const float deltaZ = school.centreZ - observerZ;
            const float distance = std::hypot(deltaY, std::hypot(deltaX, deltaZ));
            school.simulationTier = WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance);
            const int population = static_cast<int>(FishPerSchool);
            switch (school.simulationTier) {
            case WildlifeSimulationTier::Full:
                fullSimulationCount_ += population;
                break;
            case WildlifeSimulationTier::Reduced:
                reducedSimulationCount_ += population;
                break;
            case WildlifeSimulationTier::Dormant:
                dormantSimulationCount_ += population;
                break;
            }
            for (std::size_t memberIndex = 0; memberIndex < FishPerSchool; ++memberIndex) {
                agents_[schoolIndex * FishPerSchool + memberIndex].state.simulationTier =
                    school.simulationTier;
            }

            school.accumulatedSeconds += safeDelta;
            const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(
                school.simulationTier);
            if ((frameIndex_ + schoolIndex) % divisor == 0U) {
                UpdateSchool(schoolIndex, std::min(school.accumulatedSeconds, 0.5f));
                school.accumulatedSeconds = 0.0f;
            }
        }

        for (std::size_t fishIndex = 0; fishIndex < agents_.size(); ++fishIndex) {
            states_[fishIndex] = agents_[fishIndex].state;
        }
    }

    void FishSchools::UpdateSchool(const std::size_t schoolIndex,
                                   const float deltaSeconds) noexcept
    {
        if (deltaSeconds <= 0.0f) {
            return;
        }
        School& school = schools_[schoolIndex];
        const float routeRadius = std::max(1.0f, std::hypot(school.centreX, school.centreZ));
        school.routeAngle = WrapAngle(school.routeAngle
            + static_cast<float>(school.direction) * school.routeSpeed
                / routeRadius * deltaSeconds);
        ResolveSchoolCentre(school);
        const float radialLength = std::max(1.0f, std::hypot(school.centreX, school.centreZ));
        const float radialX = school.centreX / radialLength;
        const float radialZ = school.centreZ / radialLength;
        const float tangentX = -radialZ * static_cast<float>(school.direction);
        const float tangentZ = radialX * static_cast<float>(school.direction);

        for (std::size_t memberIndex = 0; memberIndex < FishPerSchool; ++memberIndex) {
            Agent& agent = agents_[schoolIndex * FishPerSchool + memberIndex];
            const float localDrift = std::sin(elapsedSeconds_ * agent.wanderFrequency
                                              + agent.wanderPhase);
            const float targetX = school.centreX
                + tangentX * (agent.alongOffset + localDrift * 0.75f)
                + radialX * (agent.radialOffset + localDrift * 0.32f);
            const float targetZ = school.centreZ
                + tangentZ * (agent.alongOffset + localDrift * 0.75f)
                + radialZ * (agent.radialOffset + localDrift * 0.32f);
            const float toTargetX = targetX - agent.state.x;
            const float toTargetZ = targetZ - agent.state.z;
            const float targetDistance = std::hypot(toTargetX, toTargetZ);
            float desiredHeading = std::atan2(tangentZ, tangentX)
                + localDrift * 0.20f;
            if (targetDistance > 0.55f) {
                desiredHeading = std::atan2(toTargetZ, toTargetX);
            }
            const float headingDifference = SignedAngleDifference(
                agent.state.headingRadians, desiredHeading);
            agent.state.headingRadians += std::clamp(
                headingDifference, -1.55f * deltaSeconds, 1.55f * deltaSeconds);

            const float speedScale = std::clamp(0.78f + targetDistance * 0.22f, 0.78f, 1.32f);
            const float proposedX = agent.state.x + std::cos(agent.state.headingRadians)
                * agent.state.speed * speedScale * deltaSeconds;
            const float proposedZ = agent.state.z + std::sin(agent.state.headingRadians)
                * agent.state.speed * speedScale * deltaSeconds;
            const TerrainSample proposedSample = terrain_.Sample(proposedX, proposedZ);
            if (proposedSample.signedShoreDistance <= MaximumShoreDistance
                && proposedSample.signedShoreDistance >= MinimumShoreDistance
                && proposedSample.height <= -0.55f) {
                agent.state.x = proposedX;
                agent.state.z = proposedZ;
            } else {
                const float correctionHeading = proposedSample.signedShoreDistance
                        > MaximumShoreDistance
                    ? std::atan2(radialZ, radialX)
                    : std::atan2(-radialZ, -radialX);
                agent.state.headingRadians += std::clamp(
                    SignedAngleDifference(agent.state.headingRadians, correctionHeading),
                    -2.4f * deltaSeconds, 2.4f * deltaSeconds);
            }

            const TerrainSample currentSample = terrain_.Sample(agent.state.x, agent.state.z);
            const float desiredDepth = FishDepth(currentSample.height, agent.depthPhase,
                                                 elapsedSeconds_);
            agent.state.y += std::clamp(desiredDepth - agent.state.y,
                                        -0.9f * deltaSeconds, 0.9f * deltaSeconds);
            agent.bodyPhase = std::fmod(agent.bodyPhase
                                        + agent.bodySpeed * speedScale * deltaSeconds, TwoPi);
            agent.state.bodyWave = std::sin(agent.bodyPhase);
        }
    }

    std::span<const FishState> FishSchools::Fish() const noexcept
    {
        return states_;
    }
}
