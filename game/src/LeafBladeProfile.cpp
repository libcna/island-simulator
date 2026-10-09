#include "IslandSimulator/LeafBladeProfile.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    LeafBladePoint SampleLeafBlade(const float along, const float across, const float length,
                                   const float halfWidth, const float lift, const float phase)
    {
        if (!std::isfinite(along) || !std::isfinite(across) || !std::isfinite(length)
            || !std::isfinite(halfWidth) || !std::isfinite(lift) || !std::isfinite(phase)
            || along < 0.0f || along > 1.0f || std::abs(across) > 1.0f
            || length <= 0.0f || halfWidth <= 0.0f || lift <= 0.0f) {
            throw std::invalid_argument("Leaf blade parameters must be finite and in their supported ranges");
        }
        constexpr float Pi = std::numbers::pi_v<float>;
        const float arch = std::max(0.0f, std::sin(along * Pi));
        const float taper = std::pow(arch, 0.70f);
        const float phaseAngle = std::remainder(phase, 1.0f) * 2.0f * Pi;
        const float margin = 1.0f + 0.045f * across * std::sin(along * 9.0f + phaseAngle);
        const float side = across * halfWidth * taper * margin;
        const float centreHeight = lift * (0.62f * along + 0.75f * arch)
            - length * 0.12f * along * along;
        const float ridge = (1.0f - std::abs(across)) * halfWidth * taper * 0.18f;
        const float twist = across * halfWidth * taper * 0.14f
            * std::sin(phaseAngle + along * 4.1f);
        const LeafBladePoint point{side, centreHeight + ridge + twist, length * along};
        if (!std::isfinite(point.across) || !std::isfinite(point.height) || !std::isfinite(point.along)) {
            throw std::invalid_argument("Leaf blade dimensions overflow the supported floating-point geometry");
        }
        return point;
    }

    LeafBladePoint SampleLeafBladeNormal(const float along, const float across, const float length,
                                         const float halfWidth, const float lift, const float phase)
    {
        // Validate even endpoint/out-of-range calls before clamping the derivative stencil.
        (void)SampleLeafBlade(along, across, length, halfWidth, lift, phase);
        const float t = std::clamp(along, 0.002f, 0.998f);
        const auto a = SampleLeafBlade(t - 0.001f, across, length, halfWidth, lift, phase);
        const auto b = SampleLeafBlade(t + 0.001f, across, length, halfWidth, lift, phase);
        const auto c = SampleLeafBlade(t, std::max(-1.0f, across - 0.002f), length, halfWidth, lift, phase);
        const auto d = SampleLeafBlade(t, std::min(1.0f, across + 0.002f), length, halfWidth, lift, phase);
        const float tx = b.across - a.across, ty = b.height - a.height, tz = b.along - a.along;
        const float sx = d.across - c.across, sy = d.height - c.height, sz = d.along - c.along;
        const float nx = ty * sz - tz * sy, ny = tz * sx - tx * sz, nz = tx * sy - ty * sx;
        const float magnitude = std::hypot(nx, ny, nz);
        if (!std::isfinite(magnitude) || magnitude <= 0.0f) {
            throw std::invalid_argument("Leaf blade normal cannot be represented at these dimensions");
        }
        return {nx / magnitude, ny / magnitude, nz / magnitude};
    }
}
