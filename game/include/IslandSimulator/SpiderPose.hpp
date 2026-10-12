#pragma once
#include <array>
#include <cstddef>

namespace island {
struct SpiderPoint {
    float x,y,z;
    bool operator==(const SpiderPoint&) const = default;
};
struct SpiderLegPose {
    SpiderPoint root,hip,knee,foot;
    bool reachLimited;
    bool operator==(const SpiderLegPose&) const = default;
    static constexpr float CoxaLength=.018f,UpperLength=.050f,LowerLength=.065f;
};
// Metres, +z forward. Four pairs attach to the front body, not the abdomen.
// Order: left front to rear, then right front to rear. No time/stride-dependent
// target here: the simulation must retain world-space stance anchors itself.
class SpiderPose final {
public:
    static constexpr std::size_t LegCount=8;
    static constexpr SpiderPoint ThoraxCentre{0,.036f,.018f};
    static constexpr SpiderPoint ThoraxRadii{.018f,.014f,.024f};
    static constexpr SpiderPoint AbdomenCentre{0,.039f,-.025f};
    static constexpr SpiderPoint AbdomenRadii{.025f,.021f,.034f};
    [[nodiscard]] static SpiderPoint NeutralFoot(std::size_t leg);
    // Fixed-length coxa followed by a two-link solve in the root/target plane.
    // Too close/far targets are clamped and flagged, never silently stretched.
    [[nodiscard]] static SpiderLegPose Solve(std::size_t leg,SpiderPoint target);
    [[nodiscard]] static std::array<SpiderLegPose,LegCount> Rest();
};
}
