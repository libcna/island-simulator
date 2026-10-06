#include "IslandSimulator/WaterViewModel.hpp"

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

    float WaterViewModel::TargetIntensity(const bool swimming,
                                          const float groundHeight) noexcept
    {
        if (!swimming) return 0.0f;
        const float safeGroundHeight = std::isfinite(groundHeight) ? groundHeight : 0.0f;
        const float waterDepth = std::max(0.0f, -safeGroundHeight);
        const float depthBlend = SmoothStep((waterDepth - 0.35f) / 1.65f);
        return 0.60f + 0.40f * depthBlend;
    }

    void WaterViewModel::Reset(const bool swimming, const float groundHeight) noexcept
    {
        intensity_ = TargetIntensity(swimming, groundHeight);
    }

    float WaterViewModel::Update(const bool swimming, const float groundHeight,
                                 const float deltaSeconds) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, MaximumStepSeconds)
            : 0.0f;
        const float target = TargetIntensity(swimming, groundHeight);
        const float rate = target > intensity_ ? EntryRatePerSecond : ExitRatePerSecond;
        intensity_ = MoveTowards(intensity_, target, rate * safeDelta);
        return intensity_;
    }
}
