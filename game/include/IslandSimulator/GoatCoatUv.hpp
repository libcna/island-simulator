#pragma once
#include "IslandSimulator/GoatMesh.hpp"
#include <array>
#include <cmath>
#include <numbers>
namespace island {
// Interior mirror window: no source-edge tiling claim. Periodic circumference
// joins continuously; strand direction follows authored longitudinal coordinates.
[[nodiscard]] inline std::array<float,2> GoatCoatUv(const GoatVertex& vertex) {
    const float angle=2*std::numbers::pi_v<float>*vertex.v;
    // Loft rings use x=rx*cos(angle), y=cy+ry*sin(angle). Mirroring cos folded
    // the image at the centre of both visible flanks, producing a rump whorl.
    // Use ring height there: the reflection now lies at the back/belly ridges.
    const bool longitudinalLoft=vertex.part==GoatPart::Body||vertex.part==GoatPart::Head;
    const float across=longitudinalLoft?.5f+.5f*std::sin(angle):.5f-.5f*std::cos(angle);
    const float along=vertex.part==GoatPart::Body?1-vertex.u:vertex.u;
    return {.06f+.88f*across,.06f+.88f*along};
}
}
