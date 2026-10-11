#pragma once

#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/MonkeyGroundGait.hpp"
#include <stdexcept>

namespace island::MonkeyClimbPose
{
    using Point = MonkeyGroundGait::Point;
    inline constexpr float SupportFraction = .8f;
    inline constexpr float Stride = .22f;
    inline constexpr float CycleDistance = Stride/SupportFraction;
    struct Pose {
        Point origin, right, outward;
        MonkeyGroundGait::Pose limbs;
        std::array<Point,4> contacts, normals; // World-space requested contacts and normals.
    };

    // localHeight and angle address the tree rest mesh; signed phase must advance by
    // world vertical travel / (CycleDistance * monkeyScale) for locked supporting grips.
    // Frame: model +X circumferential, +Y away from bark, +Z upward.
    inline Pose Sample(const CanopyPlacement& tree, float localHeight, float angle,
                       float phase, float monkeyScale, float windTime, bool farMesh = false)
    {
        if (!std::isfinite(monkeyScale) || monkeyScale <= 0 || !std::isfinite(tree.scale)
            || tree.scale <= 0 || !std::isfinite(phase) || !std::isfinite(tree.yawRadians)
            || !std::isfinite(tree.x) || !std::isfinite(tree.y) || !std::isfinite(tree.z))
            throw std::invalid_argument("Invalid monkey climb frame");
        const float yaw = tree.yawRadians, c = std::cos(yaw), s = std::sin(yaw);
        const auto worldPoint = [&](const std::array<float,3>& p) {
            return Point{tree.x+tree.scale*(p[0]*c+p[2]*s),tree.y+tree.scale*p[1],
                         tree.z+tree.scale*(-p[0]*s+p[2]*c)};
        };
        const auto worldNormal = [&](const std::array<float,3>& p) {
            return Point{p[0]*c+p[2]*s,p[1],-p[0]*s+p[2]*c};
        };
        const float worldAngle = angle-yaw;
        Pose result{};
        result.right = {-std::sin(worldAngle),0,std::cos(worldAngle)};
        result.outward = {std::cos(worldAngle),0,std::sin(worldAngle)};
        result.origin = worldPoint(SampleCanopyTrunkSurface(tree.variant,localHeight,angle,farMesh,windTime).position);
        result.limbs = MonkeyGroundGait::Sample(0,0);
        result.limbs.bodyOffset = -.08f;
        for (std::size_t i = 0; i < 4; ++i) {
            auto& limb = result.limbs.limbs[i];
            limb.root.y += result.limbs.bodyOffset;
            const float p = MonkeyGroundGait::WrappedPhase(phase+MonkeyGroundGait::PhaseOffsets[i]);
            const bool supporting = p < SupportFraction;
            const float swing = std::clamp((p-SupportFraction)/(1-SupportFraction),0.0f,1.0f);
            const float ease = swing*swing*(3-2*swing);
            const float travel = supporting ? .5f-p/SupportFraction : -.5f+ease;
            const float h = localHeight+(limb.root.z+travel*Stride)*monkeyScale/tree.scale;
            const float radius = CanopyTrunkRadius(tree.variant,h)*tree.scale;
            const float lateral = limb.root.x*monkeyScale/radius;
            if (std::abs(lateral) >= .9f) throw std::invalid_argument("Trunk too narrow for monkey grips");
            const auto bark = SampleCanopyTrunkSurface(tree.variant,h,angle+std::asin(lateral),farMesh,windTime);
            const Point normal = worldNormal(bark.normal);
            const Point point = worldPoint(bark.position);
            const float lift = supporting ? 0 : .09f*std::pow(std::sin(swing*std::numbers::pi_v<float>),2);
            const float clearance = (MonkeyGroundGait::SoleHeight+lift)*monkeyScale;
            const Point contact{point.x+normal.x*clearance,point.y+normal.y*clearance,point.z+normal.z*clearance};
            result.contacts[i] = contact; result.normals[i] = normal;
            const Point delta{(contact.x-result.origin.x)/monkeyScale,
                              (contact.y-result.origin.y)/monkeyScale,
                              (contact.z-result.origin.z)/monkeyScale};
            const Point target{delta.x*result.right.x+delta.z*result.right.z,
                               delta.x*result.outward.x+delta.z*result.outward.z,delta.y};
            // Fore elbows bend down; hind knees bend up along the trunk.
            limb = MonkeyGroundGait::SolveLimb(limb.root,target,{0,0,i < 2 ? -1.0f : 1.0f},
                                                supporting,result.limbs.reachLimited);
        }
        return result;
    }
}
