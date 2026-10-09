#pragma once

#include <array>
#include <cstdint>

namespace island::forest_floor
{
    // A complete source image spans 1.25 world metres. Each 0.32 m lattice interval overlaps
    // four independently rotated interior windows, never sampling the mismatched source edges.
    inline constexpr float CellMetres = 0.32f;
    inline constexpr float UvPerMetre = 0.80f;
    inline constexpr float MinimumUv = 0.084f;
    inline constexpr float MaximumUv = 0.916f;

    struct SourceWindow final
    {
        float u, v, weight;
        std::int32_t anchorX, anchorZ;
        std::uint32_t quarterTurns;
    };
    using Windows = std::array<SourceWindow, 4>;

    /** @brief Finite island-space source windows, ordered lower-left/right then upper-left/right.
     * Weights sum to one; windows shared by neighboring cells retain identical source coordinates.
     * This addresses sampling boundaries, not baked lighting or the source's artistic quality.
     */
    [[nodiscard]] Windows SampleWindows(float worldX, float worldZ);
    [[nodiscard]] std::uint32_t AnchorHash(std::int32_t x, std::int32_t z) noexcept;
    /** @brief Apply the same quarter-turn to UV positions and explicit texture gradients. */
    [[nodiscard]] std::array<float, 2> Rotate(float x, float z, std::uint32_t quarterTurns) noexcept;
}
