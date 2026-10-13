#pragma once

#include <array>

namespace island
{
    // Shorter nasal region; identity through the cheeks so ears/limb roots stay put.
    [[nodiscard]] constexpr float BoarMuzzleZ(float authoredZ) noexcept {
        return authoredZ<=.78f?authoredZ:.78f+(authoredZ-.78f)*.55f;
    }
    // Continuous source window, with short fur directed back from the nose.
    [[nodiscard]] constexpr float BoarFurAlong(float z) noexcept {
        constexpr float bridgeU=.1f+.8f*(.78f+.72f)/1.78f;
        return z<=.78f?.1f+.8f*(z+.72f)/1.78f:
            bridgeU+(.14f-bridgeU)*(z-.78f)/(BoarMuzzleZ(1.06f)-.78f);
    }
    struct BoarBodyRing { float z, centreY, radiusX, radiusY; };
    using BoarBodyProfile = std::array<BoarBodyRing, 21>;
    [[nodiscard]] BoarBodyProfile MakeBoarBodyProfile(float rootingDip) noexcept;
    /** @brief Upper surface of the shared profile; coordinates are unscaled model meters. */
    [[nodiscard]] float BoarBackHeight(const BoarBodyProfile& profile, float z, float x) noexcept;

    struct BoarJoint { float x, y, z; };
    using BoarTailCurve=std::array<BoarJoint,5>;
    /** @brief Short raised tail with a lateral terminal curl and bounded gait twitch. */
    [[nodiscard]] BoarTailCurve EvaluateBoarTail(float phase,float bodyBob,int curlSide) noexcept;
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
