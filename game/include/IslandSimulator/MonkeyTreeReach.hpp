#pragma once
#include "IslandSimulator/MonkeyClimbPose.hpp"
#include <optional>

namespace island::MonkeyTreeReach
{
    using Point = MonkeyGroundGait::Point;
    struct Contact { Point position, normal; }; // Monkey model coordinates.
    inline float StandOff(float treeScale,float monkeyScale) noexcept
    {
        return std::max((CanopyTrunkBaseRadius+CanopyRootFlare)*treeScale+.42f,
                        .48f*treeScale+.80f*monkeyScale);
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
