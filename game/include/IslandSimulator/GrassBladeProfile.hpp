#pragma once

#include <string>

namespace island
{
    inline constexpr int NearGrassBladeCount = 42;
    inline constexpr int MiddleGrassBladeCount = 10;
    inline constexpr int FarGrassBladeCount = 7;
    inline constexpr int NearGrassTurfBladeCount = 48;
    inline constexpr float GrassMinimumFootprintAxisScale = 0.82f;
    inline constexpr float GrassMaximumFootprintAxisScale = 0.94f;
    inline constexpr float GrassMaximumFootprintStretch =
        1.0f / GrassMinimumFootprintAxisScale;
    enum class GrassMeshTier { Near, Middle, Far };

    struct GrassBladePlacement
    {
        float x;
        float z;
        float facing;
        float windPhase;
        int tuft;
    };

    // Seven unequal local tufts share the existing near-mesh budget and instance transform.
    [[nodiscard]] GrassBladePlacement NearGrassBladePlacement(int blade);

    struct GrassBladeShape
    {
        float height;
        float halfWidth;
        float lean;
        float lateralBow;
        float edgeAsymmetry;
        float sideSign;
        float tipDeflection;
        float tipDrop;
        bool refined;
        bool folded;
    };

    struct GrassTurfBlade
    {
        GrassBladePlacement placement;
        GrassBladeShape shape;
    };

    struct GrassLodBlade
    {
        GrassBladePlacement placement;
        GrassBladeShape shape;
        int sourceBlade;
    };

    // Middle/far proxies represent the same near tufts, with nested source identities. Proxy
    // width retains this fraction of assigned tall-blade upright area; it excludes near turf,
    // seeds, overlap and view angle, so this is not an exact image-coverage guarantee.
    inline constexpr float GrassProxyAreaRetention = 0.75f;
    [[nodiscard]] GrassLodBlade GrassBladeForTier(GrassMeshTier tier, int blade);
    [[nodiscard]] float GrassUprightArea(const GrassBladeShape& shape, int levels);

    // Low, spreading growth between tall tufts. Three cross-sections form each near-only strip.
    [[nodiscard]] GrassTurfBlade NearGrassTurfBlade(int blade);

    // Local coordinates: across the blade, up, and along the direction of lean.
    struct GrassBladePoint
    {
        float across;
        float height;
        float along;
    };

    // t is root-to-tip [0,1]; side is [-1,1]. Side zero is the actual centre/ridge,
    // including terminal bow/drop, and is the attachment point for a seed stem.
    [[nodiscard]] GrassBladePoint SampleGrassBlade(const GrassBladeShape& shape,
                                                   float t, float side);

    struct GrassFootprintPoint
    {
        float x;
        float z;
    };

    /** @brief Applies the shader's area-preserving per-blade footprint variation. */
    [[nodiscard]] GrassFootprintPoint VaryGrassFootprint(
        float x, float z, float rootX, float rootZ,
        float windPhase, float colorVariation);

    /** @brief Emits locale-independent GLSL constants shared by the footprint model. */
    [[nodiscard]] std::string GrassFootprintShaderConstants();
}
