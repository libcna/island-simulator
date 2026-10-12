#pragma once
#include <array>
#include <cstddef>
#include <optional>

namespace island {
using BranchPoint=std::array<float,3>;
struct CanopyBranchVertex {
    BranchPoint position, outward;
    float weight, phase, angle, along;
};
struct CanopyBranchProfile {
    std::array<CanopyBranchVertex,18> vertices;
    std::array<std::array<std::size_t,3>,24> triangles;
};
// Same three rings and six-sided support in both canopy mesh tiers.
[[nodiscard]] CanopyBranchProfile MakeCanopyBranchProfile(int variant,int cluster);
[[nodiscard]] BranchPoint CanopyBranchPosition(const CanopyBranchVertex& vertex,
    std::optional<float> windTime=std::nullopt);
// Barycentric attachment to an actual triangle, after per-vertex wind deformation.
// Coordinates remain tree-local; the caller must apply the real instance transform.
[[nodiscard]] BranchPoint SampleCanopyBranchAnchor(const CanopyBranchProfile& branch,
    std::size_t triangle,float u,float v,std::optional<float> windTime=std::nullopt);
}
