#include "IslandSimulator/WaterRippleModel.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    namespace
    {
        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] float MoveTowards(const float current, const float target,
                                        const float maximumDelta) noexcept
        {
            if (current < target) return std::min(current + maximumDelta, target);
            return std::max(current - maximumDelta, target);
        }
    }

    float WaterRippleModel::TargetIntensity(const bool swimming, const float groundHeight,
                                            const float horizontalSpeed) noexcept
    {
        if (!std::isfinite(groundHeight) || !std::isfinite(horizontalSpeed)) return 0.0f;

        const float waterDepth = std::max(0.0f, -groundHeight);
        float depth = SmoothStep(waterDepth / 0.55f);
        if (swimming) depth = std::max(depth, 0.72f);
        const float motion = SmoothStep((std::max(0.0f, horizontalSpeed) - 0.10f) / 1.35f);
        return std::clamp(depth * motion, 0.0f, 1.0f);
    }

    void WaterRippleModel::Reset(const bool swimming, const float groundHeight,
                                 const float horizontalSpeed) noexcept
    {
        intensity_ = TargetIntensity(swimming, groundHeight, horizontalSpeed);
        phase_ = 0.0f;
    }

    float WaterRippleModel::Update(const bool swimming, const float groundHeight,
                                   const float horizontalSpeed,
                                   const float deltaSeconds) noexcept
    {
        if (!std::isfinite(deltaSeconds)) return intensity_;
        const float dt = std::clamp(deltaSeconds, 0.0f, MaximumStepSeconds);
        const float target = TargetIntensity(swimming, groundHeight, horizontalSpeed);
        const float rate = target > intensity_ ? EntryRatePerSecond : ExitRatePerSecond;
        intensity_ = MoveTowards(intensity_, target, rate * dt);

        if (intensity_ > 0.001f || target > 0.001f) {
            const float safeSpeed = std::isfinite(horizontalSpeed)
                ? std::clamp(horizontalSpeed, 0.0f, 4.5f) : 0.0f;
            phase_ = std::fmod(phase_ + dt * (0.42f + safeSpeed * 0.16f), 1.0f);
            if (phase_ < 0.0f) phase_ += 1.0f;
        }
        return intensity_;
    }
}
