#pragma once
#include "IslandSimulator/MonkeyTreeAttachment.hpp"
#include "IslandSimulator/JungleGrounding.hpp"
#include <vector>

namespace island::MonkeyAttachmentSite
{
    using Point=MonkeyGroundGait::Point;
    enum class Failure { None, InvalidInput, Heading, Slope, InitialReach, ClimbHeight,
        FinalReach, TrajectoryReach, GroundIntersection, BarkIntersection, Obstacle };
    struct Result { std::optional<MonkeyTreeAttachment::Plan> plan; Failure failure; };
    struct GroundFrame {
        Point origin;
        float heading, scale;
        Point Direction(Point p) const noexcept {
            const float s=std::sin(heading),c=std::cos(heading);
            return {p.x*s+p.z*c,p.y,-p.x*c+p.z*s};
        }
        Point LocalDirection(Point p) const noexcept {
            const float s=std::sin(heading),c=std::cos(heading);
            return {p.x*s-p.z*c,p.y,p.x*c+p.z*s};
        }
        Point World(Point p) const noexcept {
            const auto d=Direction(p);
            return {origin.x+scale*d.x,origin.y+scale*d.y,origin.z+scale*d.z};
        }
        Point Local(Point p) const noexcept {
            return LocalDirection({(p.x-origin.x)/scale,(p.y-origin.y)/scale,(p.z-origin.z)/scale});
        }
    };
    inline constexpr int SampleCount=1001+501;
    // A successful range is only a provisional candidate, not permission to climb.
    // The caller must check every range before activating the returned plan.
    inline Result PrepareRange(const CanopyPlacement& source,GroundFrame frame,
        const RenderedTerrainSurface& terrain,std::span<const CircularObstacle> obstacles,
        float windTime,bool farMesh,int first,int count)
    {
        const auto fail=[](Failure reason) { return Result{{},reason}; };
        if (first<0||first>=SampleCount||count<=0||count>SampleCount-first)
            return fail(Failure::InvalidInput);
        const auto finite=[](Point p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); };
        if (!finite(frame.origin)||!std::isfinite(frame.heading)||!std::isfinite(frame.scale)
            ||frame.scale<=0||!std::isfinite(windTime)||!std::isfinite(source.scale)||source.scale<=0
            ||!std::isfinite(source.yawRadians)||!std::isfinite(source.x)||!std::isfinite(source.z)
            ||source.variant>=CanopyFormCount) return fail(Failure::InvalidInput);
        const float facing=std::atan2(source.z-frame.origin.z,source.x-frame.origin.x);
        if (std::abs(std::remainder(facing-frame.heading,2*std::numbers::pi_v<float>))>.01f)
            return fail(Failure::Heading);
        try {
            const auto tree=GroundJunglePlacement(source,terrain);
            auto base=MonkeyGroundGait::Sample(0,0);
            std::array<Point,4> normals{};
            std::array<float,4> heights{};
            const auto groundContact=[&](Point local) {
                auto world=frame.World(local);
                const auto ground=terrain.Sample(world.x,world.z);
                world.y=ground.height+MonkeyGroundGait::SoleHeight*frame.scale/ground.normal.y;
                return MonkeyTreeReach::Contact{frame.Local(world),
                    frame.LocalDirection({ground.normal.x,ground.normal.y,ground.normal.z})};
            };
            for (std::size_t i=0;i<4;++i) {
                const auto contact=groundContact(base.limbs[i].sole);
                if (contact.normal.y<.85f) return fail(Failure::Slope);
                normals[i]=contact.normal;
                heights[i]=contact.position.y-MonkeyGroundGait::SoleHeight;
            }
            base=MonkeyGroundGait::FitGround(base,heights);
            std::array<MonkeyTreeReach::Contact,2> raised{},staged{};
            for (std::size_t i=0;i<2;++i) {
                const auto palm=MonkeyTreeReach::Target(tree,frame.origin,frame.heading,frame.scale,
                    base.limbs[i].root.y,i==0,windTime,farMesh);
                const auto high=MonkeyTreeReach::Target(tree,frame.origin,frame.heading,frame.scale,
                    .99f+base.bodyOffset,i==0,windTime,farMesh);
                if (!palm||!high) return fail(Failure::InitialReach);
                base.limbs[i]=MonkeyGroundGait::SolveLimb(base.limbs[i].root,palm->position,
                    MonkeyTreeReach::BendHint(1),true,base.reachLimited);
                normals[i]=palm->normal; raised[i]=*high;
                staged[i]=groundContact({base.limbs[i+2].sole.x,0,.18f});
                if (staged[i].normal.y<.85f) return fail(Failure::Slope);
            }
            if (base.reachLimited||!MonkeyTreeReach::RaiseTorso(base,.22f)) return fail(Failure::InitialReach);
            const float height=(frame.origin.y+(.84f+base.bodyOffset)*frame.scale-tree.y)/tree.scale;
            if (height<.5f*frame.scale/tree.scale || height>MakeCanopyTrunkProfile(tree.variant).height-.8f)
                return fail(Failure::ClimbHeight);
            const float angle=std::atan2(frame.origin.z-tree.z,frame.origin.x-tree.x)+tree.yawRadians;
            const auto climb=MonkeyClimbPose::Sample(tree,height,angle,0,frame.scale,windTime,farMesh);
            if (climb.limbs.reachLimited) return fail(Failure::FinalReach);
            std::array<Point,4> contacts{},finalNormals{};
            for (std::size_t i=0;i<4;++i) {
                contacts[i]=frame.Local(climb.contacts[i]);
                finalNormals[i]=frame.LocalDirection(climb.normals[i]);
            }
            const auto root=MonkeyTreeAttachment::Lerp(climb.limbs.limbs[2].root,climb.limbs.limbs[3].root,.5f);
            const Point hip=frame.Local({climb.origin.x+frame.scale*(root.x*climb.right.x+root.y*climb.outward.x),
                climb.origin.y+frame.scale*root.z,
                climb.origin.z+frame.scale*(root.x*climb.right.z+root.y*climb.outward.z)});
            auto plan=MonkeyTreeAttachment::Build(base,normals,raised,staged,contacts,finalNormals,hip);
            const int reachCount=std::min(first+count,1001)-first;
            if (reachCount>0 && !MonkeyTreeAttachment::IsReachableRange(plan,first,reachCount))
                return fail(Failure::TrajectoryReach);
            if (first+count<=1001) return {plan,Failure::None};
            std::vector<CircularObstacle> nearby;
            for (const auto& obstacle:obstacles) {
                if (!std::isfinite(obstacle.x)||!std::isfinite(obstacle.z)||!std::isfinite(obstacle.radius)
                    ||obstacle.radius<0) return fail(Failure::InvalidInput);
                // Exempt exactly the selected canopy's known proxy, not every nearby trunk.
                if (std::hypot(obstacle.x-tree.x,obstacle.z-tree.z)<1e-4f
                    &&std::abs(obstacle.radius-.43f*tree.scale)<1e-4f) continue;
                if (std::hypot(obstacle.x-frame.origin.x,obstacle.z-frame.origin.z)<obstacle.radius+4*frame.scale)
                    nearby.push_back(obstacle);
            }
            const auto clear=[&](Point world,float radius,float obstaclePadding=0.0f) {
                if (world.y-radius<terrain.Sample(world.x,world.z).height+.002f*frame.scale)
                    return Failure::GroundIntersection;
                for (const auto& obstacle:nearby)
                    if (std::hypot(world.x-obstacle.x,world.z-obstacle.z)<radius+obstaclePadding+obstacle.radius)
                        return Failure::Obstacle;
                const float localHeight=std::clamp((world.y-tree.y)/tree.scale,0.0f,
                    MakeCanopyTrunkProfile(tree.variant).height-.001f);
                const float theta=std::atan2(world.z-tree.z,world.x-tree.x)+tree.yawRadians;
                const auto bark=SampleCanopyTrunkSurface(tree.variant,localHeight,theta,farMesh,windTime);
                const float c=std::cos(tree.yawRadians),s=std::sin(tree.yawRadians);
                const Point surface{tree.x+tree.scale*(bark.position[0]*c+bark.position[2]*s),
                    tree.y+tree.scale*bark.position[1],tree.z+tree.scale*(-bark.position[0]*s+bark.position[2]*c)};
                const Point normal{bark.normal[0]*c+bark.normal[2]*s,bark.normal[1],
                    -bark.normal[0]*s+bark.normal[2]*c};
                if ((world.x-surface.x)*normal.x+(world.y-surface.y)*normal.y
                    +(world.z-surface.z)*normal.z<radius) return Failure::BarkIntersection;
                return Failure::None;
            };
            for (int step=std::max(0,first-1001);step<first+count-1001;++step) {
                const auto pose=MonkeyTreeAttachment::Sample(plan,step*.02f);
                for (std::size_t limb=0;limb<pose.contacts.size();++limb) {
                    const auto centre=frame.World(pose.contacts[limb]);
                    const auto reason=clear(centre,0,.14f*frame.scale);
                    if (reason!=Failure::None) return fail(reason);
                    const auto normal=frame.Direction(pose.normals[limb]);
                    auto forward=frame.Direction({0,0,1});
                    const auto dot=[](Point a,Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; };
                    const auto projected=[&](Point axis) {
                        const float d=dot(axis,normal);
                        return Point{axis.x-normal.x*d,axis.y-normal.y*d,axis.z-normal.z*d};
                    };
                    forward=projected(forward);
                    if (dot(forward,forward)<1e-8f) forward=projected({0,1,0});
                    const float length=std::sqrt(dot(forward,forward));
                    forward={forward.x/length,forward.y/length,forward.z/length};
                    const Point right{normal.y*forward.z-normal.z*forward.y,
                        normal.z*forward.x-normal.x*forward.z,normal.x*forward.y-normal.y*forward.x};
                    // Conservative sole/toe footprint in the same normal-aligned frame
                    // as MonkeySoleSkin; a centre above ground alone can hide penetration.
                    for (float x:{-.05f,.05f}) for (float z:{-.05f,.14f}) {
                        const Point corner{centre.x+frame.scale*(right.x*x+forward.x*z-normal.x*.031f),
                            centre.y+frame.scale*(right.y*x+forward.y*z-normal.y*.031f),
                            centre.z+frame.scale*(right.z*x+forward.z*z-normal.z*.031f)};
                        const auto cornerReason=clear(corner,0);
                        if (cornerReason!=Failure::None) return fail(cornerReason);
                    }
                }
                // Conservative torso/head probes, not a claim of full mesh collision.
                for (const auto& probe:std::array<std::array<float,4>,3>{{
                    {0,.62f,-.25f,.20f},{0,.64f,.15f,.20f},{0,.85f,.46f,.16f}}}) {
                    const auto point=MonkeyTreeAttachment::Transform(
                        {probe[0],probe[1]+base.bodyOffset,probe[2]},plan.pivot,pose.frame);
                    const auto reason=clear(frame.World(point),probe[3]*frame.scale);
                    if (reason!=Failure::None) return fail(reason);
                }
            }
            return {plan,Failure::None};
        } catch (const std::invalid_argument&) { return fail(Failure::InvalidInput); }
          catch (const std::out_of_range&) { return fail(Failure::InvalidInput); }
    }
    inline Result Prepare(const CanopyPlacement& source,GroundFrame frame,
        const RenderedTerrainSurface& terrain,std::span<const CircularObstacle> obstacles,
        float windTime,bool farMesh=false)
    {
        return PrepareRange(source,frame,terrain,obstacles,windTime,farMesh,0,SampleCount);
    }
}
