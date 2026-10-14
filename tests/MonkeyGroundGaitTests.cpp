#include "IslandSimulator/MonkeyGroundGait.hpp"
#include "IslandSimulator/MonkeyTailProfile.hpp"
#include "IslandSimulator/MonkeyFaceProfile.hpp"
#include "IslandSimulator/MonkeyFurUv.hpp"
#include "IslandSimulator/MonkeyLimbProfile.hpp"

#include <iostream>
#include <stdexcept>

namespace gait = island::MonkeyGroundGait;

static void Require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static float Distance(gait::Point a, gait::Point b)
{
    return std::sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y)
                     + (a.z-b.z)*(a.z-b.z));
}

int main()
try {
    namespace limbProfile = island::MonkeyLimbProfile;
    for (const auto mantle : {limbProfile::ShoulderMantle,limbProfile::HipMantle}) {
        Require(mantle.centre.x>0&&mantle.radii.x>=.10f&&mantle.radii.y>=.09f
            &&mantle.radii.z>=.12f,"monkey joint mantle keeps a broad anatomical volume");
        Require(mantle.limbWeight>.1f&&mantle.limbWeight<.2f,
            "monkey joint mantle remains body-rooted with bounded limb following");
    }
    const auto rest=gait::Sample(0,0);
    for (const auto& limb:rest.limbs) for (float thickness:{.9f,1.0f,1.1f}) {
        for (std::size_t row=0;row<limbProfile::Fractions.size();++row) {
            const auto section=limbProfile::Sample(limb,row,thickness);
            Require(std::isfinite(section.centre.y) && section.radius>0,"finite positive limb sleeve");
            Require(section.bodyWeight>=0 && section.bodyWeight<=1 && section.lowerWeight>=0
                && section.lowerWeight<=1,"limb skin weights bounded");
            Require(!(section.bodyWeight>0 && section.lowerWeight>0),"limb skin uses at most two influences");
            if (row==0) Require(section.bodyWeight==1 && std::abs(section.centre.x)<std::abs(limb.root.x)
                && section.centre.y>limb.root.y,"proximal sleeve stays embedded and attached to torso");
            if (row==0) Require(std::abs(section.centre.x)+section.radius<.14f,
                "whole proximal ring must fit inside even the narrow shoulder torso");
            if (row+1==limbProfile::Fractions.size()) Require(section.lowerWeight==1
                && Distance(section.centre,limb.sole)<1e-6f,"wrist stays attached to lower limb endpoint");
            if (row>0) Require(Distance(section.centre,limbProfile::Sample(limb,row-1,thickness).centre)>1e-4f,
                "limb rings cannot collapse");
        }
    }
    namespace fur = island::MonkeyFurUv;
    constexpr float pi = std::numbers::pi_v<float>;
    for (int i=0;i<720;++i) {
        const float angle=i*pi/180.0f;
        Require(fur::Around(angle)>=.09999f && fur::Around(angle)<=.90001f,
            "fur UV stays inside source image");
        Require(std::abs(fur::Around(angle)-fur::Around(angle+2*pi))<1e-5f,
            "circumference closure has no texture jump");
    }
    for (float centre : {0.0f,pi}) {
        const float density=std::abs(fur::Around(centre+.01f)-fur::Around(centre-.01f))/.02f;
        Require(std::abs(density-.8f/pi)<1e-4f,"side-facing fur must not collapse into a stretched stripe");
    }
    Require(fur::Along(0)==.1f && std::abs(fur::Along(1)-.9f)<1e-6f,"longitudinal UV interior endpoints");
    for (float x : {-.2f,0.0f,.2f})
        for (float y : {-.55f,-.2f,.2f})
            for (float z : {-.15f,0.0f,.15f}) {
                bool limited = false;
                const auto limb = gait::SolveLimb({0,0,0},{x,y,z},{0,0,-1},true,limited);
                Require(!limited && Distance(limb.sole,{x,y,z}) < 1e-5f,"spatial IK reaches feasible targets");
                Require(std::abs(Distance(limb.root,limb.joint)-gait::UpperLength) < 1e-5f
                    && std::abs(Distance(limb.joint,limb.sole)-gait::LowerLength) < 1e-5f,
                    "spatial IK preserves both segment lengths");
            }
    for (auto target : {gait::Point{0,0,0},gait::Point{0,2,0},gait::Point{0,0,-.4f}}) {
        bool limited = false;
        const auto limb = gait::SolveLimb({0,0,0},target,{0,0,-1},true,limited);
        Require(std::abs(Distance(limb.root,limb.joint)-gait::UpperLength) < 1e-5f
            && std::abs(Distance(limb.joint,limb.sole)-gait::LowerLength) < 1e-5f,
            "degenerate bend directions and unreachable targets preserve finite bone lengths");
        Require(limited == (target.y == 2 || Distance(target,{0,0,0}) == 0),
                "unreachable IK contacts must be diagnosed");
    }
    namespace face = island::MonkeyFaceProfile;
    for (std::size_t row = 0; row < face::Rows.size(); ++row) {
        if (row > 0) Require(face::Rows[row].y < face::Rows[row-1].y,
                             "face rows are ordered without folds");
        Require(face::Rows[row].halfWidth > 0, "face rows do not collapse to degenerate strips");
        for (int column = 0; column <= 16; ++column) {
            const float u = column/8.0f-1.0f;
            const auto p = face::Sample(row,u), mirror = face::Sample(row,-u);
            Require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z), "finite face profile");
            Require(std::fabs(p.x+mirror.x) < 1e-6f && std::fabs(p.z-mirror.z) < 1e-6f,
                    "continuous bilateral face profile");
            Require(p.z >= .5649f && p.z <= .690f, "face stays within authored head envelope");
            const float ambient=face::AmbientRetention(p.x,p.y);
            Require(ambient>=.8f&&ambient<=1,"socket ambient retention outside bounded range");
            Require(std::abs(ambient-face::AmbientRetention(-p.x,p.y))<1e-6f,
                    "socket ambient retention must be bilateral");
        }
    }
    Require(face::Rows[4].halfWidth>=.140f&&face::Rows[8].halfWidth>=.108f,
        "short monkey face lost its broad brow and muzzle");
    Require(face::CraniumRadii.x>=.18f&&face::CraniumRadii.y>=.20f
        &&face::EarCentre.x+face::EarRadii.x>face::CraniumRadii.x,
        "monkey cranium or visible outer ear became too small");
    Require(face::Relief(face::EyeCentreX,face::EyeCentreY)<-.004f,"orbit must be recessed");
    Require(face::Relief(face::EyeCentreX,.907f)>.007f,"brow must project beyond socket");
    Require(face::Relief(0,.850f)>.007f,"nose bridge must have relief");
    Require(face::AmbientRetention(face::EyeCentreX,face::EyeCentreY)<.81f
        &&face::AmbientRetention(0,.780f)==1,"ambient occlusion restricted to sockets");
    const auto eyeSurface=[](std::size_t row) {
        return face::Sample(row,std::asin(face::EyeCentreX/face::Rows[row].halfWidth)/1.57079632679f).z;
    };
    const float orbitalSurface=std::lerp(eyeSurface(4),eyeSurface(3),
        (face::EyeCentreY-face::Rows[4].y)/(face::Rows[3].y-face::Rows[4].y));
    Require(face::EyeCentreZ-face::EyeRadiusZ<orbitalSurface
        &&face::EyeCentreZ+face::EyeRadiusZ>orbitalSurface,
        "eye cornea must straddle the recessed orbital surface, not float outside");
    Require(face::Rows[7].front > face::Rows[4].front, "muzzle projects beyond orbital plane");
    Require(face::EyeCentreZ>.640f&&face::EyeRadiusX>=.024f,
        "close monkey corneas remain readable above the face surface");
    Require(face::NoseRadii.x>=.032f&&face::NoseRadii.y>=.018f
            &&face::NoseCentre.z+face::NoseRadii.z>.680f,
        "fitted nose pad remains broad and visible on the short muzzle");
    Require(face::MuzzlePadRadii.x>=.050f&&face::MuzzlePadRadii.y>=.040f
            &&face::MuzzlePadCentre.z+face::MuzzlePadRadii.z>.682f,
        "paired muzzle pads remain broad and project beyond the facial skin");
    for(float side:{-1.0f,1.0f}) for(bool upper:{false,true}) for(int i=0;i<=10;++i) {
        const float t=-.92f+1.84f*i/10.0f;
        const auto outer=face::Eyelid(side,t,upper,true);
        const auto inner=face::Eyelid(side,t,upper,false);
        Require(std::isfinite(outer.x)&&std::isfinite(outer.y)&&std::isfinite(outer.z),
            "eyelid profile must stay finite");
        Require(outer.z>=.6505f&&inner.z<=.654f,"eyelid leaves the facial relief envelope");
        Require(upper ? outer.y>inner.y : outer.y<inner.y,
            "upper/lower eyelid bands must retain nonzero ordered width");
        const auto mirror=face::Eyelid(-side,-t,upper,true);
        Require(std::abs(outer.x+mirror.x)<1e-6f&&std::abs(outer.y-mirror.y)<1e-6f
            &&std::abs(outer.z-mirror.z)<1e-6f,"eyelid profile must mirror bilaterally");
    }
    namespace tail = island::MonkeyTailProfile;
    const std::array<tail::Point, 4> anchors{{{0,.62f,-.48f}, {0,.70f,-.65f},
        {0,.80f,-.79f}, {0,.99f,-.65f}}};
    const auto length = [](tail::Point p) { return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z); };
    float previousRadius=.106f;
    for (int i = 0; i <= 300; ++i) {
        const auto section = tail::Sample(anchors, i/300.0f);
        Require(std::isfinite(section.centre.y) && length(section.tangent) > .1f,
                "tail frame must be finite and nondegenerate");
        Require(section.radius >= 0 && section.radius <= .105001f,
                "fur-bearing tail stays within authored radii");
        Require(section.radius<=previousRadius+1e-6f,"tail radius must taper toward the hook");
        if(i<=240)Require(section.radius>.056f,"tail becomes needle-thin before rounded tip");
        if(i<=270)Require(section.radius>.046f,"tail loses its fur-bearing terminal taper");
        if(i==300)Require(section.radius==0,"tail tip must close");
        previousRadius=section.radius;
        Require(section.secondWeight >= 0 && section.secondWeight <= 1,
                "tail weights remain normalized and nonnegative");
        Require(section.firstBone >= 0 && section.secondBone <= 2,
                "tail stays within existing three-bone palette");
    }
    for (int joint = 1; joint <= 2; ++joint) {
        const float t = joint/3.0f;
        const auto before = tail::Sample(anchors, t-1e-5f);
        const auto after = tail::Sample(anchors, t+1e-5f);
        Require(length(before.centre-after.centre) < 1e-4f, "tail centreline has no joint gap");
        Require(length(before.tangent-after.tangent) < 1e-3f, "tail tangent is continuous at anchors");
        Require(before.secondBone == after.firstBone && before.secondWeight > .999f
                && after.secondWeight < .001f, "tail joint shares the same dominant transform");
        Require(length(tail::Sample(anchors,t).centre-anchors[joint]) < 1e-5f,
                "tail centreline passes through rig anchors");
    }
    Require(anchors.back().z>anchors[2].z&&anchors.back().y>anchors[2].y,
        "tail tip must curl forward and upward instead of ending as a straight rod");
    Require(tail::Sample(anchors,1).tangent.z>0,
        "terminal tail tangent must preserve the visible hook");
    for (int sample = -200; sample <= 400; ++sample) {
        const float phase = static_cast<float>(sample) / 200.0f;
        for (const float amount : {0.0f, 0.25f, 1.0f}) {
            const auto pose = gait::Sample(phase, amount);
            const auto repeated = gait::Sample(phase + 1.0f, amount);
            unsigned supports = 0;
            for (std::size_t i = 0; i < pose.limbs.size(); ++i) {
                const auto& limb = pose.limbs[i];
                Require(std::isfinite(limb.joint.y) && std::isfinite(limb.joint.z),
                        "finite two-link joints");
                Require(std::fabs(Distance(limb.root, limb.joint) - gait::UpperLength) < 1e-5f,
                        "upper limb retains authored length");
                Require(std::fabs(Distance(limb.joint, limb.sole) - gait::LowerLength) < 1e-5f,
                        "lower limb retains authored length");
                Require(limb.sole.y >= gait::SoleHeight, "soles never penetrate support plane");
                Require(Distance(limb.sole, repeated.limbs[i].sole) < 1e-5f,
                        "periodic ground gait");
                if (limb.supporting) {
                    ++supports;
                    Require(std::fabs(limb.sole.y - gait::SoleHeight) < 1e-6f,
                            "supporting sole stays planted vertically");
                }
            }
            Require(supports >= 2, "walking retains at least two supporting limbs");
        }
    }
    const auto a = gait::SampleLimb(0.1f, true, true, 1.0f);
    for (int frame = 0; frame < 200; ++frame) {
        const auto flat = gait::Sample(frame / 200.0f);
        for (const float slope : {-0.3f, 0.0f, 0.3f}) {
            std::array<float, 4> ground{};
            for (std::size_t i = 0; i < ground.size(); ++i)
                ground[i] = slope * flat.limbs[i].sole.z + 0.15f * flat.limbs[i].sole.x + 0.23f;
            const auto fitted = gait::FitGround(flat, ground);
            Require(!fitted.reachLimited, "ordinary slopes must be reachable");
            for (std::size_t i = 0; i < ground.size(); ++i) {
                const auto& limb = fitted.limbs[i];
                Require(std::fabs(limb.sole.y - ground[i] - flat.limbs[i].sole.y) < 1e-5f,
                        "individual terrain height and swing clearance preserved");
                Require(std::fabs(Distance(limb.root, limb.joint) - gait::UpperLength) < 1e-5f,
                        "terrain fitting preserves upper length");
                Require(std::fabs(Distance(limb.joint, limb.sole) - gait::LowerLength) < 1e-5f,
                        "terrain fitting preserves lower length");
                Require(std::fabs(limb.root.y - gait::RootHeight - fitted.bodyOffset) < 1e-5f,
                        "limb root stays connected to adjusted trunk");
            }
        }
    }
    const auto cliff = gait::FitGround(gait::Sample(0.0f), {0.0f, 2.0f, 0.0f, 0.0f});
    Require(cliff.reachLimited, "impossible cliff must report lost contact");
    for (const auto& limb : cliff.limbs) {
        Require(std::fabs(Distance(limb.root, limb.joint) - gait::UpperLength) < 1e-5f,
                "cliff cannot stretch upper bone");
        Require(std::fabs(Distance(limb.joint, limb.sole) - gait::LowerLength) < 1e-5f,
                "cliff cannot stretch lower bone");
    }
    const auto b = gait::SampleLimb(0.3f, true, true, 1.0f);
    const float rootTravel = 0.2f * gait::StrideLength / gait::SupportFraction;
    Require(std::fabs(b.sole.z + rootTravel - a.sole.z) < 1e-6f,
            "stance cancels matching forward root travel");
    std::cout << "Monkey ground gait: four contacts, periodicity and fixed limb lengths pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
