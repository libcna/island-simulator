#include "IslandSimulator/MonkeyGroundGait.hpp"

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
