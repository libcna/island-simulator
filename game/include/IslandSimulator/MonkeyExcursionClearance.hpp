#pragma once
#include "IslandSimulator/MonkeyTreeExcursion.hpp"

namespace island::MonkeyExcursionClearance
{
    using Point=MonkeyGroundGait::Point;
    enum class Failure { None, InvalidInput, Reach, Terrain, Bark, Obstacle };
    struct Result { Failure failure; float seconds; int probe; };
    inline constexpr int SampleCount=static_cast<int>(MonkeyTreeExcursion::Duration*50)+1;
    // Bounded sampled collision preflight, not continuous or exact skinned-mesh collision.
    // The supporting tree uses its real wind-deformed triangle surface; other solid
    // objects use the world's conservative vertical circular proxies.
    inline Result ValidateRange(const MonkeyTreeExcursion::Plan& plan,
        const RenderedTerrainSurface& terrain,std::span<const CircularObstacle> obstacles,
        float initialWindTime,int first,int count)
    {
        if (!std::isfinite(initialWindTime)||first<0||count<=0||first>=SampleCount
            ||count>SampleCount-first) return {Failure::InvalidInput,0,-1};
        const auto& tree=plan.tree;
        const auto& frame=plan.frame;
        if (!std::isfinite(frame.scale)||frame.scale<=0||!std::isfinite(frame.heading)
            ||!std::isfinite(frame.origin.x)||!std::isfinite(frame.origin.y)||!std::isfinite(frame.origin.z)
            ||!std::isfinite(tree.scale)||tree.scale<=0||!std::isfinite(tree.yawRadians)
            ||!std::isfinite(tree.x)||!std::isfinite(tree.y)||!std::isfinite(tree.z)
            ||tree.variant>=CanopyFormCount) return {Failure::InvalidInput,0,-1};
        const float yawC=std::cos(tree.yawRadians),yawS=std::sin(tree.yawRadians);
        std::vector<CircularObstacle> nearby;
        for (const auto& obstacle:obstacles) {
            if (!std::isfinite(obstacle.x)||!std::isfinite(obstacle.z)||!std::isfinite(obstacle.radius)
                ||obstacle.radius<0) return {Failure::InvalidInput,0,-1};
            if (std::hypot(obstacle.x-tree.x,obstacle.z-tree.z)<1e-4f
                &&std::abs(obstacle.radius-CanopyTrunkBaseRadius*tree.scale)<1e-4f) continue;
            const float extent=4*frame.scale+MakeCanopyTrunkProfile(tree.variant).lean*tree.scale;
            if (std::hypot(obstacle.x-frame.origin.x,obstacle.z-frame.origin.z)<obstacle.radius+extent)
                nearby.push_back(obstacle);
        }
        float seconds=0;
        int probe=0;
        try {
            for (int step=first;step<first+count;++step) {
                seconds=step*.02f; probe=0;
                const float wind=initialWindTime+seconds;
                const auto pose=MonkeyTreeExcursion::Sample(plan,seconds,wind,plan.farMesh);
                if (pose.body.limbs.reachLimited) return {Failure::Reach,seconds,-1};
                const auto check=[&](Point local,float modelRadius,float padding=0.0f) {
                    ++probe;
                    const auto world=frame.World(local);
                    const float radius=modelRadius*frame.scale;
                    if (!std::isfinite(world.x)||!std::isfinite(world.y)||!std::isfinite(world.z))
                        return Failure::InvalidInput;
                    if (world.y-radius<terrain.Sample(world.x,world.z).height+.002f*frame.scale)
                        return Failure::Terrain;
                    for (const auto& obstacle:nearby)
                        if (std::hypot(world.x-obstacle.x,world.z-obstacle.z)<radius+padding*frame.scale+obstacle.radius)
                            return Failure::Obstacle;
                    const float height=std::clamp((world.y-tree.y)/tree.scale,0.0f,
                        MakeCanopyTrunkProfile(tree.variant).height-.001f);
                    const auto centre=SampleCanopyTrunk(tree.variant,height/MakeCanopyTrunkProfile(tree.variant).height);
                    const float dx=(world.x-tree.x)/tree.scale,dz=(world.z-tree.z)/tree.scale;
                    const float angle=std::atan2(dx*yawS+dz*yawC-centre[2],dx*yawC-dz*yawS-centre[0]);
                    const auto bark=SampleCanopyTrunkSurface(tree.variant,height,angle,plan.farMesh,wind);
                    const Point surface{tree.x+tree.scale*(bark.position[0]*yawC+bark.position[2]*yawS),
                        tree.y+tree.scale*bark.position[1],tree.z+tree.scale*(-bark.position[0]*yawS+bark.position[2]*yawC)};
                    const Point normal{bark.normal[0]*yawC+bark.normal[2]*yawS,bark.normal[1],
                        -bark.normal[0]*yawS+bark.normal[2]*yawC};
                    if ((world.x-surface.x)*normal.x+(world.y-surface.y)*normal.y+(world.z-surface.z)*normal.z<radius)
                        return Failure::Bark;
                    return Failure::None;
                };
                for (std::size_t i=0;i<4;++i) {
                    const auto& limb=pose.body.limbs.limbs[i];
                    auto reason=check(limb.sole,0,.14f);
                    if (reason!=Failure::None) return {reason,seconds,probe};
                    const auto normal=pose.body.normals[i];
                    const auto dot=[](Point a,Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; };
                    const auto project=[&](Point a) {
                        const float d=dot(a,normal);
                        return Point{a.x-normal.x*d,a.y-normal.y*d,a.z-normal.z*d};
                    };
                    auto forward=project({0,0,1});
                    if (dot(forward,forward)<1e-8f) forward=project({0,1,0});
                    const float length=std::sqrt(dot(forward,forward));
                    forward={forward.x/length,forward.y/length,forward.z/length};
                    const Point right{normal.y*forward.z-normal.z*forward.y,
                        normal.z*forward.x-normal.x*forward.z,normal.x*forward.y-normal.y*forward.x};
                    for (float x:{-.05f,.05f}) for (float z:{-.05f,.14f}) {
                        reason=check({limb.sole.x+right.x*x+forward.x*z-normal.x*.031f,
                            limb.sole.y+right.y*x+forward.y*z-normal.y*.031f,
                            limb.sole.z+right.z*x+forward.z*z-normal.z*.031f},0);
                        if (reason!=Failure::None) return {reason,seconds,probe};
                    }
                    for (float t:{.2f,.4f,.6f,.8f}) {
                        reason=check(MonkeyTreeAttachment::Lerp(limb.root,limb.joint,t),.055f);
                        if (reason!=Failure::None) return {reason,seconds,probe};
                        reason=check(MonkeyTreeAttachment::Lerp(limb.joint,limb.sole,t),.035f);
                        if (reason!=Failure::None) return {reason,seconds,probe};
                    }
                }
                for (const auto& sphere:std::array<std::array<float,4>,3>{{
                    {0,.62f,-.25f,.20f},{0,.64f,.15f,.20f},{0,.85f,.46f,.16f}}}) {
                    const auto local=MonkeyTreeAttachment::Transform(
                        {sphere[0],sphere[1]+plan.attachment.ground.bodyOffset,sphere[2]},
                        plan.attachment.pivot,pose.body.frame);
                    const auto reason=check(local,sphere[3]);
                    if (reason!=Failure::None) return {reason,seconds,probe};
                }
            }
        } catch (const std::invalid_argument&) { return {Failure::Reach,seconds,probe}; }
          catch (const std::out_of_range&) { return {Failure::Terrain,seconds,probe}; }
        return {Failure::None,(first+count-1)*.02f,-1};
    }
    inline Result Validate(const MonkeyTreeExcursion::Plan& plan,
        const RenderedTerrainSurface& terrain,std::span<const CircularObstacle> obstacles,float windTime)
    {
        return ValidateRange(plan,terrain,obstacles,windTime,0,SampleCount);
    }
}
