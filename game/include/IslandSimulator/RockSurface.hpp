#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace island
{
    inline constexpr float RockSurfaceMaximumHeight = 1.23f;
    struct RockSurfaceVertex
    {
        std::array<float, 3> position;
        std::array<float, 3> normal;
    };

    struct RockSurface
    {
        std::vector<RockSurfaceVertex> vertices;
        std::vector<std::uint16_t> indices;
    };

    // Four shared authored profiles, with higher angular resolution only at close distance.
    // Normals are area weighted over shared topology; the basal rim remains open and buried.
    [[nodiscard]] RockSurface BuildRockSurface(int variant, bool farMesh);
}
