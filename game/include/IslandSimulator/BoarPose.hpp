#pragma once

#include <array>

namespace island
{
    struct BoarBodyRing { float z, centreY, radiusX, radiusY; };
    using BoarBodyProfile = std::array<BoarBodyRing, 11>;
    [[nodiscard]] BoarBodyProfile MakeBoarBodyProfile(float rootingDip) noexcept;
    /** @brief Upper surface of the shared profile; coordinates are unscaled model meters. */
    [[nodiscard]] float BoarBackHeight(const BoarBodyProfile& profile, float z, float x) noexcept;

    struct BoarJoint { float x, y, z; };
    struct BoarLegPose
    {
        BoarJoint root, knee, hoof;
        bool stance;
        bool reachLimited = false;
        static constexpr float UpperLength = 0.35f;
        static constexpr float LowerLength = 0.32f;
        static constexpr float HoofHeight = 0.065f;
        static constexpr float StanceFraction = 0.64f;
    };
    /** @brief Fixed-length two-segment pose above a supplied local ground height. */
    [[nodiscard]] BoarLegPose EvaluateBoarLeg(float phase, float movement, float bodyBob,
                                            bool front, bool left, float groundHeight = 0.0f) noexcept;
}
