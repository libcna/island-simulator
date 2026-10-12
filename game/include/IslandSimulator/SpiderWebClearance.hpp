#pragma once
#include "IslandSimulator/SpiderWebLayout.hpp"

namespace island {
// Inclusive surface intersection, including coplanar overlap. No thickness margin.
[[nodiscard]] bool WebSegmentHitsTriangle(BranchPoint from,BranchPoint to,
    const std::array<BranchPoint,3>& triangle);
// Exact shared branch side triangles at the requested instant, tree-local space.
// Trims only 0.1mm at each strand's intentional attachment to its owning branch.
// Does not qualify leaves, trunk, vines, neighbouring plants or continuous wind.
[[nodiscard]] bool WebIntersectsCanopyBranches(const SpiderWebSite& site,int variant,float time);
}
