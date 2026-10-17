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
    static constexpr float CoxaLength=.0234f,UpperLength=.065f,LowerLength=.0845f;
};
// Metres, +z forward. Four pairs attach to the front body, not the abdomen.
// Order: left front to rear, then right front to rear. No time/stride-dependent
// target here: the simulation must retain world-space stance anchors itself.
class SpiderPose final {
public:
    static constexpr std::size_t LegCount=8;
    static constexpr float VisualScale=1.30f;
    static constexpr SpiderPoint ThoraxCentre{0,.0468f,.0234f};
    static constexpr SpiderPoint ThoraxRadii{.0234f,.0182f,.0312f};
    static constexpr SpiderPoint AbdomenCentre{0,.0507f,-.0325f};
    static constexpr SpiderPoint AbdomenRadii{.0325f,.0273f,.0442f};
    [[nodiscard]] static SpiderPoint NeutralFoot(std::size_t leg);
    // Fixed-length coxa followed by a two-link solve in the root/target plane.
    // Too close/far targets are clamped and flagged, never silently stretched.
    [[nodiscard]] static SpiderLegPose Solve(std::size_t leg,SpiderPoint target);
    [[nodiscard]] static std::array<SpiderLegPose,LegCount> Rest();
};
}
