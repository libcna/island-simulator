#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace island::MonkeyGroundGait
{
    // Consumer-owned quadrupedal pose model. Ground contacts are in model metres;
    // FitGround takes individually sampled rendered heights transformed to this space.
    struct Point { float x, y, z; };
    struct Limb {
        Point root, joint, sole;
        bool supporting;
    };
    struct Pose {
        std::array<Limb, 4> limbs; // left fore, right fore, left hind, right hind
        float bodyOffset = 0.0f;
        bool reachLimited = false;
    };
    inline constexpr float SupportFraction = 0.68f;
    inline constexpr float UpperLength = 0.31f;
    inline constexpr float LowerLength = 0.33f;
    inline constexpr float RootHeight = 0.57f;
    inline constexpr float StrideLength = 0.32f;
    inline constexpr float SoleHeight = 0.035f;
    inline constexpr std::array<float, 4> PhaseOffsets{0.0f, 0.5f, 0.25f, 0.75f};

    // General spatial two-link solver for ground and bark targets. The bend hint chooses
    // the elbow/knee side without constraining the target to the old sagittal plane.
    inline Limb SolveLimb(Point root, Point target, Point bendHint, bool supporting,
                          bool& reachLimited) noexcept
    {
        const auto dot = [](Point a, Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; };
        const auto scale = [](Point a,float s) { return Point{a.x*s,a.y*s,a.z*s}; };
        const auto add = [](Point a,Point b) { return Point{a.x+b.x,a.y+b.y,a.z+b.z}; };
        Point direction{target.x-root.x,target.y-root.y,target.z-root.z};
        const float distance = std::sqrt(dot(direction,direction));
        direction = distance > 1e-7f ? scale(direction,1/distance) : Point{0,-1,0};
        const float reach = std::clamp(distance,std::abs(UpperLength-LowerLength)+.001f,
                                      UpperLength+LowerLength-.001f);
        reachLimited = reachLimited || std::abs(reach-distance) > 1e-6f;
        target = add(root,scale(direction,reach));
        Point bend = add(bendHint,scale(direction,-dot(bendHint,direction)));
        if (dot(bend,bend) < 1e-8f) {
            const Point fallback = std::abs(direction.x) < .8f ? Point{1,0,0} : Point{0,0,1};
            bend = add(fallback,scale(direction,-dot(fallback,direction)));
        }
        bend = scale(bend,1/std::sqrt(dot(bend,bend)));
        const float along = (UpperLength*UpperLength-LowerLength*LowerLength+reach*reach)/(2*reach);
        const float offset = std::sqrt(std::max(0.0f,UpperLength*UpperLength-along*along));
        return {root,add(root,add(scale(direction,along),scale(bend,offset))),target,supporting};
    }

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

    // Heights are the rendered ground under each sampled sole, in model coordinates.
    // Keep the horizontal support targets and swing clearance; move the trunk only as
    // far as required to keep all four two-link chains inside their reachable interval.
    inline Pose FitGround(Pose pose, const std::array<float, 4>& ground) noexcept
    {
        float minimumRoot = -1e6f, maximumRoot = 1e6f, mean = 0.0f;
        constexpr float maximumReach = UpperLength + LowerLength - 0.001f;
        constexpr float minimumDrop = 0.08f;
        for (std::size_t i = 0; i < pose.limbs.size(); ++i) {
            auto& limb = pose.limbs[i];
            limb.sole.y += ground[i];
            const float dz = limb.sole.z - limb.root.z;
            const float maximumDrop = std::sqrt(std::max(0.0f,
                maximumReach * maximumReach - dz * dz));
            minimumRoot = std::max(minimumRoot, limb.sole.y + minimumDrop);
            maximumRoot = std::min(maximumRoot, limb.sole.y + maximumDrop);
            mean += ground[i] * 0.25f;
        }
        pose.reachLimited = minimumRoot > maximumRoot;
        const float rootHeight = pose.reachLimited ? minimumRoot
            : std::clamp(RootHeight + mean, minimumRoot, maximumRoot);
        pose.bodyOffset = rootHeight - RootHeight;
        for (std::size_t i = 0; i < pose.limbs.size(); ++i) {
            auto& limb = pose.limbs[i];
            limb.root.y = rootHeight;
            const float dz = limb.sole.z - limb.root.z;
            const float maximumDrop = std::sqrt(std::max(0.0f,
                maximumReach * maximumReach - dz * dz));
            // On impossible cliffs preserve bone lengths and diagnose lost contact;
            // never stretch the mesh or silently pretend that a floating foot is planted.
            limb.sole.y = std::max(limb.sole.y, rootHeight - maximumDrop);
            limb = SolveLimb(limb.root,limb.sole,{0,0,i < 2 ? -1.0f : 1.0f},
                              limb.supporting,pose.reachLimited);
        }
        return pose;
    }
}
