#pragma once
#include "IslandSimulator/CanopyBranchProfile.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <vector>

namespace island {
struct SpiderWebAnchor {
    int branch;
    std::size_t triangle;
    float u,v;
    bool operator==(const SpiderWebAnchor&) const = default;
};
struct SpiderWebSite {
    std::size_t canopyIndex;
    std::array<SpiderWebAnchor,3> anchors;
    bool operator==(const SpiderWebSite&) const = default;
};
// Proposed bounded canopy frames, not live or fully foliage-clear webs.
class SpiderWebLayout final {
public:
    static constexpr std::size_t MaximumSites=6;
    static constexpr float MinimumSeparation=12;
    [[nodiscard]] static std::array<BranchPoint,3> Sample(const SpiderWebSite& site,
        const JungleLayout& jungle,const RenderedTerrainSurface& surface,float time);
    [[nodiscard]] static std::vector<SpiderWebSite> Propose(const JungleLayout& jungle,
        const RenderedTerrainSurface& surface);
};
}
