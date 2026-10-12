#pragma once
#include "IslandSimulator/GoatMesh.hpp"
#include <array>
#include <cmath>
#include <numbers>
namespace island {
// Interior source window, not a tileability claim. Body uses physical height;
// other parts mirror circumference. Direction follows longitudinal coordinates.
[[nodiscard]] inline std::array<float,2> GoatCoatUv(const GoatVertex& vertex) {
    const float angle=2*std::numbers::pi_v<float>*vertex.v;
    // Head rings use x=rx*cos(angle), y=cy+ry*sin(angle); reflect at back/belly,
    // not visible flank centres. Body height was authored before pose deformation.
    const float across=vertex.part==GoatPart::Body?vertex.v:
        vertex.part==GoatPart::Head?.5f+.5f*std::sin(angle):.5f-.5f*std::cos(angle);
    const float along=vertex.part==GoatPart::Body?1-vertex.u:vertex.u;
    return {.06f+.88f*across,.06f+.88f*along};
}
}
