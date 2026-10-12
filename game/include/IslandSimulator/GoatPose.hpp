#pragma once
#include <array>
#include <cstddef>

namespace island {
class IslandTerrain;
class RenderedTerrainSurface;
struct GoatPoint { float x,y,z; bool operator==(const GoatPoint&) const = default; };
struct GoatLegPose {
    GoatPoint root,knee,hoof;
    bool stance,reachLimited;
    GoatPoint soleNormal{0,1,0}; // Rendered ground normal expressed in the body frame.
    static constexpr float UpperLength=.39f,LowerLength=.38f,HoofHeight=.055f;
    static constexpr float SoleForward=.025f,StanceFraction=.68f,WalkingSpeed=1.2f;
    bool operator==(const GoatLegPose&) const = default;
};
// Leg order: left rear, left front, right rear, right front.
[[nodiscard]] GoatLegPose EvaluateGoatLeg(float phase,float speed,float bob,
    std::size_t leg,float groundHeight=0);
// Retarget in three dimensions without stretching either bone. Unreachable targets
// are clamped and flagged; callers must not silently accept that as a locked foot.
[[nodiscard]] GoatLegPose RetargetGoatLeg(const GoatLegPose& reference,
    GoatPoint target,std::size_t leg);
// Modulo-cycle constant-speed straight travel calibration; not world-space foot locking.
[[nodiscard]] float GoatPhaseAdvance(float travelledDistance,float speed,float scale);
struct GoatGroundPose {
    GoatPoint origin,right,up,forward;
    float scale,bodyBob;
    std::array<GoatLegPose,4> legs;
    [[nodiscard]] GoatPoint ToWorld(GoatPoint local) const noexcept;
    [[nodiscard]] GoatPoint ToLocal(GoatPoint world) const noexcept;
    bool operator==(const GoatGroundPose&) const = default;
};
[[nodiscard]] GoatGroundPose FitGoatGroundPose(const IslandTerrain& terrain,
    const RenderedTerrainSurface& surface,float x,float z,float heading,
    float phase,float speed,float scale=1);
// Body transform and bob only; caller must supply the four leg solutions.
[[nodiscard]] GoatGroundPose MakeGoatBodyFrame(const IslandTerrain& terrain,
    const RenderedTerrainSurface& surface,float x,float z,float heading,
    float phase,float speed,float scale=1);
}
