#include "IslandSimulator/GrassInteractionModel.hpp"

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

    float GrassInteractionModel::TargetStrength(const float horizontalSpeed) noexcept
    {
        if (!std::isfinite(horizontalSpeed)) return 0.0f;
        return SmoothStep((std::max(0.0f, horizontalSpeed) - 0.12f) / 2.80f);
    }

    void GrassInteractionModel::Reset(const float horizontalSpeed) noexcept
    {
        strength_ = TargetStrength(horizontalSpeed);
    }

    float GrassInteractionModel::Update(const float horizontalSpeed,
                                        const float deltaSeconds) noexcept
    {
        if (!std::isfinite(deltaSeconds)) return strength_;
        const float dt = std::clamp(deltaSeconds, 0.0f, MaximumStepSeconds);
        const float target = TargetStrength(horizontalSpeed);
        const float rate = target > strength_ ? EntryRatePerSecond : ExitRatePerSecond;
        strength_ = MoveTowards(strength_, target, rate * dt);
        return strength_;
    }
}
