#pragma once
#include "IslandSimulator/MonkeyAttachmentSite.hpp"

namespace island::MonkeyTreeExcursion
{
    using Point=MonkeyGroundGait::Point;
    enum class Stage { Attach, Ascend, Rest, Descend, Detach, Finished };
    inline constexpr float AttachSeconds=10, TravelSeconds=8, RestSeconds=3;
    inline constexpr float DescendStart=AttachSeconds+TravelSeconds+RestSeconds;
    inline constexpr float DetachStart=DescendStart+TravelSeconds;
    inline constexpr float Duration=DetachStart+AttachSeconds;
    inline constexpr float Cycles=8;
    struct Plan {
        MonkeyTreeAttachment::Plan attachment;
        MonkeyAttachmentSite::GroundFrame frame;
        CanopyPlacement tree;
        float baseHeight, angle, windTime;
        bool farMesh;
    };
    struct Pose {
        MonkeyTreeAttachment::Pose body;
        Stage stage;
        float phase;
    };
    struct Clock {
        float seconds=0;
        bool returning=false;
        bool finished=false;
        // Latch an interruption: during attachment/ascent, retrace to the original
        // planted pose; once descending, finish the normal return. Never reset a
        // tree-attached animal directly to walking. The caller releases ground grips
        // only after finished, using the ordinary inspection-release animation.
        void Advance(float deltaSeconds,bool requestReturn=false) noexcept {
            if (!std::isfinite(deltaSeconds)||deltaSeconds<=0||finished) return;
            returning=returning||requestReturn;
            const float delta=std::min(deltaSeconds,.5f);
            if (returning && seconds<AttachSeconds+TravelSeconds) {
                seconds=std::max(0.0f,seconds-delta);
                finished=seconds==0;
            } else {
                // The rest interval is one identical four-grip pose, so skipping its
                // remainder changes neither position nor velocity at descent start.
                if (returning && seconds<DescendStart) seconds=DescendStart;
                seconds=std::min(Duration,seconds+delta);
                finished=seconds==Duration;
            }
        }
    };
    inline Plan Build(const MonkeyTreeAttachment::Plan& attachment,
        MonkeyAttachmentSite::GroundFrame frame,const CanopyPlacement& groundedTree,
        float windTime,bool farMesh=false)
    {
        return {attachment,frame,groundedTree,
            (frame.origin.y+(.84f+attachment.ground.bodyOffset)*frame.scale-groundedTree.y)/groundedTree.scale,
            std::atan2(frame.origin.z-groundedTree.z,frame.origin.x-groundedTree.x)+groundedTree.yawRadians,
            windTime,farMesh};
    }
    // Coordinates stay in the original ground yaw/scale frame, including body and soles.
    // Signed phase is derived from height; descending exactly retraces the same grip path.
    // Wind/LOD are held at the preflight values here. Runtime wind retargeting is separate.
    inline Pose Sample(const Plan& plan,float seconds)
    {
        if (!std::isfinite(seconds)) throw std::invalid_argument("Invalid monkey excursion time");
        const float time=std::clamp(seconds,0.0f,Duration);
        if (time<=AttachSeconds)
            return {MonkeyTreeAttachment::Sample(plan.attachment,time),Stage::Attach,0};
        if (time>=DetachStart)
            return {MonkeyTreeAttachment::Sample(plan.attachment,Duration-time),
                time==Duration ? Stage::Finished : Stage::Detach,0};
        const Stage stage=time<AttachSeconds+TravelSeconds ? Stage::Ascend
            : time<DescendStart ? Stage::Rest : Stage::Descend;
        const float t=stage==Stage::Ascend ? (time-AttachSeconds)/TravelSeconds
            : stage==Stage::Descend ? (DetachStart-time)/TravelSeconds : 1.0f;
        // Zero vertical speed at start, summit and end; integer summit phase has four supports.
        const float phase=Cycles*t*t*(3-2*t);
        const float height=plan.baseHeight+phase*MonkeyClimbPose::CycleDistance*plan.frame.scale/plan.tree.scale;
        const auto climb=MonkeyClimbPose::Sample(plan.tree,height,plan.angle,phase,
            plan.frame.scale,plan.windTime,plan.farMesh);
        const auto localPoint=[&](Point p) {
            return plan.frame.Local({climb.origin.x+plan.frame.scale*(p.x*climb.right.x+p.y*climb.outward.x),
                climb.origin.y+plan.frame.scale*p.z,
                climb.origin.z+plan.frame.scale*(p.x*climb.right.z+p.y*climb.outward.z)});
        };
        MonkeyTreeAttachment::Pose body{};
        body.limbs=plan.attachment.ground;
        body.limbs.reachLimited=climb.limbs.reachLimited;
        body.movingLimb=-1;
        for (std::size_t i=0;i<4;++i) {
            auto& limb=body.limbs.limbs[i];
            limb=climb.limbs.limbs[i];
            limb.root=localPoint(limb.root);
            limb.joint=localPoint(limb.joint);
            limb.sole=localPoint(limb.sole);
            body.contacts[i]=plan.frame.Local(climb.contacts[i]);
            body.normals[i]=plan.frame.LocalDirection(climb.normals[i]);
            if (!limb.supporting) body.movingLimb=static_cast<int>(i);
        }
        body.frame={MonkeyTreeAttachment::Lerp(body.limbs.limbs[2].root,body.limbs.limbs[3].root,.5f),
            std::numbers::pi_v<float>*.5f};
        return {body,stage,phase};
    }
    // Check the whole climb, not only the attachment endpoint. Reject upper trunk sections
    // that become too narrow or too bent. This is reach validation, not mesh collision proof.
    inline bool IsReachable(const Plan& plan)
    {
        try {
            for (int step=0;step<=static_cast<int>(Duration*100);++step) {
                const auto pose=Sample(plan,step*.01f);
                if (pose.body.limbs.reachLimited) return false;
                for (const auto& limb:pose.body.limbs.limbs)
                    for (const auto p:{limb.root,limb.joint,limb.sole})
                        if (!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)) return false;
            }
        } catch (const std::invalid_argument&) { return false; }
        return true;
    }
}
