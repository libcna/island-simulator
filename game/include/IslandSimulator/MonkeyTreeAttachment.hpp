#pragma once
#include "IslandSimulator/MonkeyTreeReach.hpp"

namespace island::MonkeyTreeAttachment
{
    using Point = MonkeyGroundGait::Point;
    struct Frame { Point hip; float pitch; };
    struct Key {
        Frame frame;
        std::array<Point,4> contacts, normals;
        int movingLimb = -1; // Contact transferred on the incoming segment.
    };
    struct Plan {
        MonkeyGroundGait::Pose ground;
        Point pivot;
        std::array<Key,11> keys;
    };
    struct Pose {
        Frame frame;
        MonkeyGroundGait::Pose limbs;
        std::array<Point,4> contacts, normals;
        int movingLimb;
    };
    inline Point Lerp(Point a,Point b,float t) noexcept
    { return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t}; }
    inline Point Transform(Point p,Point pivot,Frame frame) noexcept
    {
        const auto rotated=MonkeyTreeReach::TorsoPoint(p,pivot,frame.pitch);
        return {rotated.x+frame.hip.x-pivot.x,rotated.y+frame.hip.y-pivot.y,
            rotated.z+frame.hip.z-pivot.z};
    }
    // All coordinates are in the original ground monkey's yaw/scale frame.
    // The caller supplies actual bark queries and grounded, collision-clear staging feet.
    // This planner never invents a flat terrain height or a cylinder contact surface.
    inline Plan Build(const MonkeyGroundGait::Pose& planted,
        const std::array<Point,4>& normals,
        const std::array<MonkeyTreeReach::Contact,2>& raisedHands,
        const std::array<MonkeyTreeReach::Contact,2>& stagedFeet,
        const std::array<Point,4>& finalContacts,const std::array<Point,4>& finalNormals,
        Point finalHip)
    {
        Plan plan{};
        plan.ground=planted;
        plan.pivot=Lerp(planted.limbs[2].root,planted.limbs[3].root,.5f);
        auto& first=plan.keys[0];
        first.frame={plan.pivot,.22f};
        first.normals=normals;
        for (std::size_t i=0;i<4;++i) first.contacts[i]=planted.limbs[i].sole;
        const auto transfer=[&](std::size_t key,std::size_t limb,Point contact,Point normal) {
            plan.keys[key].contacts[limb]=contact;
            plan.keys[key].normals[limb]=normal;
            plan.keys[key].movingLimb=static_cast<int>(limb);
        };
        for (std::size_t k=1;k<plan.keys.size();++k) {
            plan.keys[k]=plan.keys[k-1];
            plan.keys[k].movingLimb=-1;
            // Move the torso nearer before reaching higher. At the conservative
            // approach stand-off, raising a palm first can exceed full arm reach.
            if (k==1) plan.keys[k].frame={Lerp(plan.pivot,finalHip,.45f),.65f};
            else if (k<=3) transfer(k,k-2,raisedHands[k-2].position,raisedHands[k-2].normal);
            else if (k<=5) transfer(k,k-2,stagedFeet[k-4].position,stagedFeet[k-4].normal);
            else if (k==6) plan.keys[k].frame={finalHip,std::numbers::pi_v<float>*.5f};
            else {
                const std::size_t limb=k<=8 ? k-5 : k-9;
                transfer(k,limb,finalContacts[limb],finalNormals[limb]);
            }
        }
        return plan;
    }
    // Ten one-second, zero-end-velocity segments. Reversing seconds provides the
    // same contacts/support ordering for dismount, rather than a teleport to ground.
    inline Pose Sample(const Plan& plan,float seconds)
    {
        if (!std::isfinite(seconds)) throw std::invalid_argument("Invalid monkey attachment time");
        const float time=std::clamp(seconds,0.0f,10.0f);
        const auto segment=std::min(static_cast<std::size_t>(time),std::size_t{9});
        const float t=time-static_cast<float>(segment), w=t*t*(3-2*t);
        const auto& a=plan.keys[segment]; const auto& b=plan.keys[segment+1];
        Pose result{};
        result.frame={Lerp(a.frame.hip,b.frame.hip,w),a.frame.pitch+(b.frame.pitch-a.frame.pitch)*w};
        result.limbs=plan.ground;
        result.movingLimb=t>0 && t<1 ? b.movingLimb : -1;
        for (std::size_t i=0;i<4;++i) {
            auto normal=Lerp(a.normals[i],b.normals[i],w);
            const float length=std::sqrt(normal.x*normal.x+normal.y*normal.y+normal.z*normal.z);
            if (!(length>1e-5f)) throw std::invalid_argument("Invalid monkey attachment normal");
            normal={normal.x/length,normal.y/length,normal.z/length};
            Point contact=Lerp(a.contacts[i],b.contacts[i],w);
            const bool moving=result.movingLimb==static_cast<int>(i);
            const float lift=moving ? .08f*std::pow(std::sin(t*std::numbers::pi_v<float>),2) : 0;
            contact={contact.x+normal.x*lift,contact.y+normal.y*lift,contact.z+normal.z*lift};
            result.contacts[i]=contact; result.normals[i]=normal;
            const Point bend=i<2 ? Point{0,-1,0}
                : Point{0,std::sin(result.frame.pitch),std::cos(result.frame.pitch)};
            result.limbs.limbs[i]=MonkeyGroundGait::SolveLimb(
                Transform(plan.ground.limbs[i].root,plan.pivot,result.frame),contact,bend,!moving,
                result.limbs.reachLimited);
        }
        return result;
    }
    // Bounded preflight for a candidate, before lifting any ground supports.
    // This samples reach only; terrain/obstacle/body-clearance and changing wind
    // still require caller checks. It is not a general continuous collision proof.
    inline bool IsReachable(const Plan& plan)
    {
        const auto finite=[](Point p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); };
        try {
            for (int step=0;step<=1000;++step) {
                const auto pose=Sample(plan,step*.01f);
                if (pose.limbs.reachLimited) return false;
                for (const auto& limb:pose.limbs.limbs)
                    if (!finite(limb.root)||!finite(limb.joint)||!finite(limb.sole)) return false;
            }
        } catch (const std::invalid_argument&) { return false; }
        return true;
    }
}
