#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace island
{
    using BushPoint = std::array<float,3>;

    struct BushShoot
    {
        float angle, reach, height, bend, phase;
    };

    struct BushLeaf
    {
        int source;
        BushPoint root;
        float angle, pitch, roll, length, width, phase, pigment, rootWind;
        bool operator==(const BushLeaf&) const = default;
    };

    inline constexpr std::array<float,6> BushShootKnots{0,0.3f,0.5f,0.7f,0.9f,1};
    inline constexpr int BushLeavesPerShoot = 9;
    [[nodiscard]] int BushShootCount(int variant);
    [[nodiscard]] BushShoot MakeBushShoot(int variant, int shoot);
    [[nodiscard]] BushPoint SampleBushShoot(const BushShoot& shoot, float t);
    [[nodiscard]] BushLeaf MakeBushLeaf(int variant, int shoot, int leaf);
    [[nodiscard]] BushPoint SampleBushLeaf(const BushLeaf& leaf, float t, float side);
    [[nodiscard]] BushPoint BushLeafNormal(const BushLeaf& leaf, float t, float side);
    // Keep the complete oval outline at either distance; far only removes the inner rib.
    [[nodiscard]] constexpr int BushLeafRows() { return 7; }
    [[nodiscard]] constexpr int BushLeafColumns(bool far) { return far ? 2 : 3; }
    [[nodiscard]] std::vector<std::array<float,2>> BushLeafCoordinates(bool far);
    [[nodiscard]] std::vector<std::uint16_t> BushLeafIndices(bool far);
    [[nodiscard]] float BushShootWind(float t);
    [[nodiscard]] float BushLeafWind(const BushLeaf& leaf, float t);
}
