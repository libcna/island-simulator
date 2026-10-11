#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace island::MonkeyGroundGait
{
    // Consumer-owned quadrupedal pose model. Ground contacts are in model metres;
    // a later terrain-contact stage supplies individual world-space support heights.
    struct Point { float x, y, z; };
    struct Limb {
        Point root, joint, sole;
        bool supporting;
    };
    struct Pose {
        std::array<Limb, 4> limbs; // left fore, right fore, left hind, right hind
    };
    inline constexpr float SupportFraction = 0.68f;
    inline constexpr float UpperLength = 0.31f;
    inline constexpr float LowerLength = 0.33f;
    inline constexpr float RootHeight = 0.57f;
    inline constexpr float StrideLength = 0.32f;
    inline constexpr float SoleHeight = 0.035f;
    inline constexpr std::array<float, 4> PhaseOffsets{0.0f, 0.5f, 0.25f, 0.75f};

    inline float WrappedPhase(float phase) noexcept
    {
        return phase - std::floor(phase);
    }

    inline Limb SampleLimb(float phase, bool front, bool left, float amount) noexcept
    {
        amount = std::clamp(amount, 0.0f, 1.0f);
        phase = WrappedPhase(phase);
        const bool supporting = phase < SupportFraction || amount == 0.0f;
        const float swing = std::clamp((phase - SupportFraction)
                                      / (1.0f - SupportFraction), 0.0f, 1.0f);
        // Linear rearward sole motion in stance cancels forward root travel at the
        // matching stride speed. Smooth return and zero endpoint lift avoid foot pops.
        const float ease = swing * swing * (3.0f - 2.0f * swing);
        const float travel = phase < SupportFraction
            ? 0.5f - phase / SupportFraction : -0.5f + ease;
        const float arc = std::sin(swing * std::numbers::pi_v<float>);
        const float lift = phase < SupportFraction ? 0.0f : 0.11f * arc * arc;
        const Point root{left ? -0.18f : 0.18f, RootHeight, front ? 0.28f : -0.34f};
        const Point sole{root.x, SoleHeight + lift * amount,
                         root.z + travel * StrideLength * amount};
        const float dy = sole.y - root.y;
        const float dz = sole.z - root.z;
        const float distance = std::hypot(dy, dz);
        const float along = (UpperLength * UpperLength - LowerLength * LowerLength
                             + distance * distance) / (2.0f * distance);
        const float perpendicular = std::sqrt(std::max(
            0.0f, UpperLength * UpperLength - along * along));
        const float bend = front ? 1.0f : -1.0f;
        const Point joint{root.x,
            root.y + dy * along / distance - dz * perpendicular * bend / distance,
            root.z + dz * along / distance + dy * perpendicular * bend / distance};
        return {root, joint, sole, supporting};
    }

    inline Pose Sample(float phase, float amount = 1.0f) noexcept
    {
        Pose pose{};
        for (std::size_t index = 0; index < pose.limbs.size(); ++index)
            pose.limbs[index] = SampleLimb(phase + PhaseOffsets[index], index < 2,
                                          index % 2 == 0, amount);
        return pose;
    }
}
