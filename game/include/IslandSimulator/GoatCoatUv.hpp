#pragma once
#include "IslandSimulator/GoatMesh.hpp"
#include <array>
#include <cmath>
#include <numbers>
namespace island {
// Interior mirror window: no source-edge tiling claim. Periodic circumference
// joins continuously; strand direction follows authored longitudinal coordinates.
[[nodiscard]] inline std::array<float,2> GoatCoatUv(const GoatVertex& vertex) {
    const float across=.5f-.5f*std::cos(2*std::numbers::pi_v<float>*vertex.v);
    const float along=vertex.part==GoatPart::Body?1-vertex.u:vertex.u;
    return {.06f+.88f*across,.06f+.88f*along};
}
}
