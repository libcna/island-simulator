#pragma once

#include "IslandSimulator/LeafBladeProfile.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace island
{
    struct SplitGroundLeaf
    {
        int source;
        float angle, length, width, lift, phase, pigment;
        bool operator==(const SplitGroundLeaf&) const = default;
    };

    inline constexpr std::array<float,9> SplitGroundLeafKnots{0,0.22f,0.34f,0.45f,0.56f,0.67f,0.78f,0.88f,1};
    inline constexpr float SplitGroundPetioleHalfWidth = 0.017f;
    // Endpoint/midpoint-only sampling makes a smoothstep outline piecewise straight.
    // Quarter intervals retain the rounded lobe shoulders in the actual near mesh.
    inline constexpr int SplitGroundLeafNearSubdivisions = 4;
    [[nodiscard]] constexpr int SplitGroundLeafRows(bool far)
    {
        return far ? 9 : 8*SplitGroundLeafNearSubdivisions+1;
    }
    [[nodiscard]] constexpr int SplitGroundLeafColumns(bool far) { return far ? 3 : 5; }
    [[nodiscard]] float SplitGroundLeafRow(int row, bool far);
    [[nodiscard]] SplitGroundLeaf SplitGroundLeafForTier(int index, bool far);
    [[nodiscard]] LeafBladePoint SampleSplitGroundLeaf(const SplitGroundLeaf& leaf, float t, float side);
    [[nodiscard]] LeafBladePoint SplitGroundLeafNormal(const SplitGroundLeaf& leaf, float t, float side);
    [[nodiscard]] std::vector<std::uint16_t> SplitGroundLeafIndices(bool far);
}
