#pragma once
#include "IslandSimulator/SpiderPose.hpp"
#include <vector>

namespace island {
enum class SpiderPart { FrontBody,Abdomen,Leg,Palp,Eye,Hair };
enum class SpiderMaterial { Cuticle,Marking,Eye,Hair };
struct SpiderVertex {
    SpiderPoint position,normal;
    SpiderPart part;
    SpiderMaterial material;
};
struct SpiderMesh {
    static constexpr std::size_t MaximumVertices=10000;
    std::vector<SpiderVertex> vertices;
};
// Local metres; the caller owns ground fitting and world-space stance anchors.
[[nodiscard]] SpiderMesh BuildSpiderMesh(
    const std::array<SpiderLegPose,SpiderPose::LegCount>& legs,bool detailed);
}
