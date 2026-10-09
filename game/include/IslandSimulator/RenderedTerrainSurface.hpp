#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace island
{
    struct RenderedTerrainSample
    {
        float height;
        TerrainNormal normal;
    };

    // Exact piecewise-planar surface drawn by TerrainRenderer, not the analytic collision field.
    // A fixed 232,324-byte height cache avoids procedural resampling during dense placement audits.
    class RenderedTerrainSurface final
    {
    public:
        explicit RenderedTerrainSurface(const IslandTerrain& terrain);
        // Inclusive mesh extent; nonfinite and out-of-mesh positions are errors, not seabed.
        [[nodiscard]] RenderedTerrainSample Sample(float x, float z) const;
        [[nodiscard]] std::array<float,2> HeightRange(float minX,float minZ,float maxX,float maxZ) const;
        [[nodiscard]] std::span<const float> Heights() const noexcept { return heights_; }
        static constexpr float MinimumEncodedHeight = -64.0f;
        static constexpr float EncodedHeightRange = 128.0f;
        [[nodiscard]] static std::array<std::uint8_t,4> EncodeHeight(float height);
        [[nodiscard]] static float DecodeHeight(std::array<std::uint8_t,4> pixel) noexcept;
        // CPU float emulation of the portable shader, used by the grounding audit and tests.
        [[nodiscard]] float SamplePacked(float x,float z) const;

        static constexpr const char* Shader = R"(
uniform sampler2D uGrassTerrainHeight;
// extent, quads per axis, minimum encoded height, encoded height range.
uniform vec4 uGrassTerrainMap;
float grassTerrainTexel(ivec2 cell)
{
    vec3 bytes = floor(texelFetch(uGrassTerrainHeight, cell, 0).rgb * 255.0 + 0.5);
    return uGrassTerrainMap.z + dot(bytes, vec3(1.0, 256.0, 65536.0))
        * (uGrassTerrainMap.w / 16777215.0);
}
float grassTerrainHeight(vec2 world)
{
    vec2 grid = (world + uGrassTerrainMap.x)
        * (uGrassTerrainMap.y / (2.0 * uGrassTerrainMap.x));
    ivec2 cell = clamp(ivec2(floor(grid)), ivec2(0), ivec2(int(uGrassTerrainMap.y) - 1));
    vec2 t = clamp(grid - vec2(cell), 0.0, 1.0);
    float tr = grassTerrainTexel(cell + ivec2(1,0));
    float bl = grassTerrainTexel(cell + ivec2(0,1));
    if (t.x + t.y <= 1.0) {
        float tl = grassTerrainTexel(cell);
        return tl + (tr - tl) * t.x + (bl - tl) * t.y;
    }
    float br = grassTerrainTexel(cell + ivec2(1,1));
    return br + (bl - br) * (1.0 - t.x) + (tr - br) * (1.0 - t.y);
}
)";

    private:
        std::array<float, TerrainMeshLayout::Resolution * TerrainMeshLayout::Resolution> heights_;
    };
}
