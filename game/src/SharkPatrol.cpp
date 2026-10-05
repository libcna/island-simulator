#include "IslandSimulator/SharkPatrol.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
        constexpr float MinimumShoreDistance = -88.0f;
        constexpr float MaximumShoreDistance = -34.0f;

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

        [[nodiscard]] float SignedAngleDifference(const float from, const float to) noexcept
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
    }

    SharkPatrol::SharkPatrol(const IslandTerrain& terrain)
        : terrain_(terrain),
          routeAngle_(WrapAngle(-std::numbers::pi_v<float> * 0.5f + 0.34f)),
          presenceSecondsRemaining_(52.0f),
          bodyPhase_(0.7f)
    {
        ResolveRouteTarget();
        const float radius = std::max(1.0f, std::hypot(targetX_, targetZ_));
        const float radialX = targetX_ / radius;
        const float radialZ = targetZ_ / radius;
        const float tangentX = -radialZ * static_cast<float>(direction_);
        const float tangentZ = radialX * static_cast<float>(direction_);
        state_.x = targetX_ - tangentX * 2.4f;
        state_.z = targetZ_ - tangentZ * 2.4f;
        state_.headingRadians = std::atan2(tangentZ, tangentX);
        const float seabed = terrain_.Height(state_.x, state_.z);
        state_.y = std::max(seabed + 1.2f, -2.4f);
        state_.bodyWave = std::sin(bodyPhase_);
    }

    float SharkPatrol::NextPresentDuration() const noexcept
    {
        return 42.0f + static_cast<float>((presenceCycleCount_ * 17U + 11U) % 25U);
    }

    float SharkPatrol::NextHiddenDuration() const noexcept
    {
        return 92.0f + static_cast<float>((presenceCycleCount_ * 29U + 7U) % 69U);
    }

    void SharkPatrol::ResolveRouteTarget() noexcept
    {
        const float offshoreVariation = std::sin(elapsedSeconds_ * 0.037f + 0.8f) * 5.5f
            + std::sin(elapsedSeconds_ * 0.013f + 2.1f) * 2.5f;
        float radius = terrain_.ShoreRadius(routeAngle_) + 52.0f + offshoreVariation;
        targetX_ = std::cos(routeAngle_) * radius;
        targetZ_ = std::sin(routeAngle_) * radius;
        TerrainSample sample = terrain_.Sample(targetX_, targetZ_);
        for (int attempt = 0; attempt < 10 && sample.height > -3.5f; ++attempt) {
            radius += 2.5f;
            targetX_ = std::cos(routeAngle_) * radius;
            targetZ_ = std::sin(routeAngle_) * radius;
            sample = terrain_.Sample(targetX_, targetZ_);
        }
    }

    void SharkPatrol::Update(const float deltaSeconds, const float observerX,
                             const float observerY, const float observerZ) noexcept
    {
        const float safeDelta = std::clamp(deltaSeconds, 0.0f, 0.1f);
        elapsedSeconds_ += safeDelta;
        ++frameIndex_;

        presenceSecondsRemaining_ -= safeDelta;
        if (presenceSecondsRemaining_ <= 0.0f) {
            state_.present = !state_.present;
            ++presenceCycleCount_;
            presenceSecondsRemaining_ += state_.present
                ? NextPresentDuration() : NextHiddenDuration();
        }

        const float targetActivity = state_.present ? 1.0f : 0.0f;
        const float activityRate = state_.present ? 0.22f : 0.16f;
        state_.activity += std::clamp(targetActivity - state_.activity,
                                      -activityRate * safeDelta, activityRate * safeDelta);
        state_.activity = std::clamp(state_.activity, 0.0f, 1.0f);
        state_.visible = state_.activity > 0.035f;

        const float deltaX = state_.x - observerX;
        const float deltaY = state_.y - observerY;
        const float deltaZ = state_.z - observerZ;
        const float distance = std::hypot(deltaY, std::hypot(deltaX, deltaZ));
        state_.simulationTier = state_.visible
            ? WildlifeActivationPolicy::Classify(
                distance, FullSimulationDistance, ReducedSimulationDistance)
            : WildlifeSimulationTier::Dormant;

        accumulatedSeconds_ += safeDelta;
        const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(
            state_.simulationTier);
        if (frameIndex_ % divisor == 0U) {
            UpdateMotion(std::min(accumulatedSeconds_, 0.5f));
            accumulatedSeconds_ = 0.0f;
        }
    }

    void SharkPatrol::UpdateMotion(const float deltaSeconds) noexcept
    {
        if (deltaSeconds <= 0.0f) {
            return;
        }
        const float routeRadius = std::max(1.0f, std::hypot(targetX_, targetZ_));
        routeAngle_ = WrapAngle(routeAngle_ + static_cast<float>(direction_)
            * 1.05f / routeRadius * deltaSeconds);
        ResolveRouteTarget();

        const float toTargetX = targetX_ - state_.x;
        const float toTargetZ = targetZ_ - state_.z;
        const float desiredHeading = std::atan2(toTargetZ, toTargetX)
            + std::sin(elapsedSeconds_ * 0.11f + 1.7f) * 0.055f;
        state_.headingRadians += std::clamp(
            SignedAngleDifference(state_.headingRadians, desiredHeading),
            -0.62f * deltaSeconds, 0.62f * deltaSeconds);

        const float targetDistance = std::hypot(toTargetX, toTargetZ);
        const float speed = std::clamp(1.65f + targetDistance * 0.06f, 1.65f, 2.45f);
        const float proposedX = state_.x + std::cos(state_.headingRadians) * speed * deltaSeconds;
        const float proposedZ = state_.z + std::sin(state_.headingRadians) * speed * deltaSeconds;
        const TerrainSample proposedSample = terrain_.Sample(proposedX, proposedZ);
        if (proposedSample.signedShoreDistance <= MaximumShoreDistance
            && proposedSample.signedShoreDistance >= MinimumShoreDistance
            && proposedSample.height <= -3.0f) {
            state_.x = proposedX;
            state_.z = proposedZ;
        } else {
            state_.headingRadians += std::clamp(
                SignedAngleDifference(state_.headingRadians,
                                      std::atan2(toTargetZ, toTargetX)),
                -1.25f * deltaSeconds, 1.25f * deltaSeconds);
        }

        const float seabed = terrain_.Height(state_.x, state_.z);
        const float surfacePatrolDepth = -2.35f
            + std::sin(elapsedSeconds_ * 0.19f + 0.6f) * 0.42f;
        const float hiddenDepth = std::max(seabed + 0.85f, -8.0f);
        const float desiredDepth = hiddenDepth
            + (std::max(seabed + 1.15f, surfacePatrolDepth) - hiddenDepth) * state_.activity;
        state_.y += std::clamp(desiredDepth - state_.y,
                               -0.72f * deltaSeconds, 0.72f * deltaSeconds);
        bodyPhase_ = std::fmod(bodyPhase_ + speed * 1.48f * deltaSeconds, TwoPi);
        state_.bodyWave = std::sin(bodyPhase_);
    }
}
