#include "IslandSimulator/FirstPersonCameraMotion.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float MaximumStepSeconds = 0.1f;

        [[nodiscard]] float SmoothTowards(const float current, const float target,
                                          const float response, const float deltaSeconds) noexcept
        {
            const float blend = 1.0f - std::exp(-response * deltaSeconds);
            return current + (target - current) * blend;
        }
    }

    CameraMotionOffset FirstPersonCameraMotion::Update(
        const float horizontalSpeed, const LocomotionMode mode,
        const float deltaSeconds) noexcept
    {
        const float dt = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, MaximumStepSeconds)
            : 0.0f;
        const float speed = std::isfinite(horizontalSpeed)
            ? std::max(0.0f, horizontalSpeed)
            : 0.0f;
        const float referenceSpeed = mode == LocomotionMode::Walking
            ? PlayerMotion::WalkSpeed
            : PlayerMotion::SwimSpeed;
        const float normalizedSpeed = std::clamp(speed / referenceSpeed, 0.0f, 1.0f);
        const float runBlend = mode == LocomotionMode::Walking
            ? std::clamp((speed - PlayerMotion::WalkSpeed)
                             / (PlayerMotion::RunSpeed - PlayerMotion::WalkSpeed),
                         0.0f, 1.0f)
            : 0.0f;
        const float targetStrength = enabled_ ? normalizedSpeed : 0.0f;
        const float strengthResponse = targetStrength > motionStrength_ ? 7.5f : 5.0f;
        motionStrength_ = SmoothTowards(motionStrength_, targetStrength, strengthResponse, dt);
        swimBlend_ = SmoothTowards(swimBlend_, mode == LocomotionMode::Swimming ? 1.0f : 0.0f,
                                   3.5f, dt);

        if (motionStrength_ > 0.0001f) {
            const float walkingCyclesPerSecond = 0.78f + normalizedSpeed * 0.38f
                + runBlend * 0.48f;
            const float swimmingCyclesPerSecond = 0.45f + normalizedSpeed * 0.22f;
            const float cyclesPerSecond = std::lerp(walkingCyclesPerSecond,
                                                    swimmingCyclesPerSecond, swimBlend_);
            const float phaseSpeedScale = std::max(0.22f, normalizedSpeed);
            phaseRadians_ += std::numbers::pi_v<float> * 2.0f
                * cyclesPerSecond * phaseSpeedScale * dt;
            phaseRadians_ = std::fmod(phaseRadians_, std::numbers::pi_v<float> * 2.0f);
        }

        const float walkingLateral = std::sin(phaseRadians_)
            * std::lerp(0.007f, MaximumLateralOffset, runBlend);
        const float walkingVertical = std::sin(phaseRadians_ * 2.0f)
            * std::lerp(0.012f, MaximumVerticalOffset, runBlend);
        const float swimmingLateral = std::sin(phaseRadians_ + 0.55f) * MaximumLateralOffset;
        const float swimmingVertical = std::sin(phaseRadians_) * MaximumVerticalOffset;
        offset_.lateralMeters = std::lerp(walkingLateral, swimmingLateral, swimBlend_)
            * motionStrength_;
        offset_.verticalMeters = std::lerp(walkingVertical, swimmingVertical, swimBlend_)
            * motionStrength_;
        return offset_;
    }

    void FirstPersonCameraMotion::Reset() noexcept
    {
        phaseRadians_ = 0.0f;
        motionStrength_ = 0.0f;
        swimBlend_ = 0.0f;
        offset_ = {};
    }
}
