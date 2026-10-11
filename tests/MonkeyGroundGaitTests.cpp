#include "IslandSimulator/MonkeyGroundGait.hpp"
#include "IslandSimulator/MonkeyTailProfile.hpp"
#include "IslandSimulator/MonkeyFaceProfile.hpp"

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
            Require(p.z >= .5499f && p.z <= .705f, "face stays within authored head envelope");
        }
    }
    Require(face::Rows[7].front > face::Rows[4].front, "muzzle projects beyond orbital plane");
    namespace tail = island::MonkeyTailProfile;
    const std::array<tail::Point, 4> anchors{{{0,.62f,-.52f}, {0,.70f,-.91f},
        {0,.86f,-1.25f}, {0,.97f,-1.55f}}};
    const auto length = [](tail::Point p) { return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z); };
    for (int i = 0; i <= 300; ++i) {
        const auto section = tail::Sample(anchors, i/300.0f);
        Require(std::isfinite(section.centre.y) && length(section.tangent) > .1f,
                "tail frame must be finite and nondegenerate");
        Require(section.radius >= .002999f && section.radius <= .036001f,
                "tail continuously tapers within authored radii");
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
