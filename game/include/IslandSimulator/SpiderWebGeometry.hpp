#pragma once
#include "IslandSimulator/CanopyBranchProfile.hpp"
#include <vector>

namespace island {
enum class WebStrandKind { Frame, Radial, Spiral };
struct WebStrand {
    std::size_t from, to;
    WebStrandKind kind;
};
// A connected strand graph, not a GPU mesh. Barycentric nodes preserve attachment
// while the three real branch anchors move; topology is constructed only once.
class SpiderWebGeometry final {
public:
    static constexpr std::size_t Spokes=24, Turns=8, MaximumNodes=220, MaximumStrands=434;
    explicit SpiderWebGeometry(const std::array<BranchPoint,3>& restAnchors);
    [[nodiscard]] const std::vector<BranchPoint>& Weights() const { return weights_; }
    [[nodiscard]] const std::vector<WebStrand>& Strands() const { return strands_; }
    [[nodiscard]] std::vector<BranchPoint> Sample(const std::array<BranchPoint,3>& anchors) const;
private:
    std::vector<BranchPoint> weights_;
    std::vector<WebStrand> strands_;
};
}
