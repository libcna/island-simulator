#pragma once
#include "IslandSimulator/GoatPose.hpp"
#include <vector>

namespace island {
enum class GoatMaterial { Coat,Skin,Horn,Hoof,Eye,Nose };
enum class GoatPart { Body,Neck,Head,Leg,Hoof,Horn,Ear,Eye,Beard,Tail,Nose };
// Body UVs are physical longitudinal/height coordinates before body bob;
// other parts retain longitudinal/circumferential coordinates.
struct GoatVertex { GoatPoint position,normal; float u,v; GoatMaterial material; GoatPart part; };
// Local model-space triangles; the caller applies GoatGroundPose's orthonormal frame.
struct GoatMesh {
    static constexpr std::size_t MaximumVertices=8192;
    std::vector<GoatVertex> vertices;
};
[[nodiscard]] GoatMesh BuildGoatMesh(const GoatGroundPose& pose,float grazing,bool detailed);
}
