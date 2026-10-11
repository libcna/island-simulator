#pragma once
#include "IslandSimulator/MonkeyClimbPose.hpp"
#include <optional>

namespace island::MonkeyTreeReach
{
    using Point = MonkeyGroundGait::Point;
    struct Contact { Point position, normal; }; // Monkey model coordinates.
    inline Point BendHint(float reachWeight) noexcept
    {
        const float t = std::clamp(reachWeight,0.0f,1.0f);
        // Ground elbows bend rearward; reaching elbows hang below the shoulder/palm line.
        // Retaining the rearward pole when the palm moves forward pitches elbows upward.
        return {0,-t,-(1-t)};
    }
    inline float StandOff(float treeScale,float monkeyScale) noexcept
    {
        return std::max((CanopyTrunkBaseRadius+CanopyRootFlare)*treeScale+.42f,
                        .48f*treeScale+.80f*monkeyScale);
    }
    // Begin loading the bark grips without moving planted hind feet or palms.
    // Full vertical attachment must subsequently transfer the hind supports too.
    inline float TorsoAngle(float elapsed, float remaining) noexcept
    {
        const auto ease = [](float t) { t=std::clamp(t/.5f,0.0f,1.0f); return t*t*(3-2*t); };
        return .22f*ease(elapsed-2.5f)*ease(remaining-1.5f);
    }
    inline Point TorsoPoint(Point point, Point pivot, float angle) noexcept
    {
        const float c=std::cos(angle), s=std::sin(angle);
        const float y=point.y-pivot.y, z=point.z-pivot.z;
        return {point.x,pivot.y+y*c+z*s,pivot.z-y*s+z*c};
    }
    inline std::optional<MonkeyGroundGait::Pose> RaiseTorso(
        const MonkeyGroundGait::Pose& planted, float angle)
    {
        if (!std::isfinite(angle) || angle < 0 || angle > .22f) return {};
        auto result=planted;
        const Point pivot{0,planted.limbs[2].root.y,planted.limbs[2].root.z};
        for (std::size_t i=0;i<4;++i) {
            const auto& limb=planted.limbs[i];
            result.limbs[i]=MonkeyGroundGait::SolveLimb(TorsoPoint(limb.root,pivot,angle),
                limb.sole,i<2 ? BendHint(1) : Point{0,0,1},limb.supporting,result.reachLimited);
        }
        if (result.reachLimited) return {};
        return result;
    }
    inline std::array<float,2> Weights(float elapsed,float remaining) noexcept
    {
        const auto ease = [](float t) { t=std::clamp(t/.75f,0.0f,1.0f); return t*t*(3-2*t); };
        // One hand transfers at a time; hind feet never leave the ground in this stage.
        return {ease(elapsed-1)*ease(remaining-.75f),ease(elapsed-1.75f)*ease(remaining)};
    }
    inline std::optional<Contact> Target(const CanopyPlacement& tree,Point origin,
        float heading,float scale,float rootHeight,bool left,float windTime,bool farMesh)
    {
        const float height = (origin.y+(rootHeight+.03f)*scale-tree.y)/tree.scale;
        if (height < .05f || height > MakeCanopyTrunkProfile(tree.variant).height-.1f) return {};
        const float lateral = (left ? -.18f : .18f)*scale/(CanopyTrunkRadius(tree.variant,height)*tree.scale);
        if (std::abs(lateral) >= .9f) return {};
        const float angle = std::atan2(origin.z-tree.z,origin.x-tree.x)+tree.yawRadians+std::asin(lateral);
        const auto bark = SampleCanopyTrunkSurface(tree.variant,height,angle,farMesh,windTime);
        const float c = std::cos(tree.yawRadians), s = std::sin(tree.yawRadians);
        const Point normal{bark.normal[0]*c+bark.normal[2]*s,bark.normal[1],-bark.normal[0]*s+bark.normal[2]*c};
        const Point world{tree.x+tree.scale*(bark.position[0]*c+bark.position[2]*s)+normal.x*MonkeyGroundGait::SoleHeight*scale,
                          tree.y+tree.scale*bark.position[1]+normal.y*MonkeyGroundGait::SoleHeight*scale,
                          tree.z+tree.scale*(-bark.position[0]*s+bark.position[2]*c)+normal.z*MonkeyGroundGait::SoleHeight*scale};
        const auto local = [heading](Point p) {
            return Point{p.x*std::sin(heading)-p.z*std::cos(heading),p.y,
                         p.x*std::cos(heading)+p.z*std::sin(heading)};
        };
        return Contact{local({(world.x-origin.x)/scale,(world.y-origin.y)/scale,(world.z-origin.z)/scale}),local(normal)};
    }
}
