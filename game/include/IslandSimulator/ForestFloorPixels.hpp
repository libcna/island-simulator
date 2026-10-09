#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace island::forest_floor
{
    using Pixel = std::array<std::uint8_t, 4>;
    struct MipLevel { int size; std::vector<Pixel> pixels; };
    inline constexpr int RuntimeTextureSize = 512;
    inline constexpr std::size_t RuntimeFullMipBytes = 1398100;

    /** @brief Validate PNG signature/IHDR dimensions before allowing the decoder to allocate.
     * This is a size guard, not a replacement for the PNG decoder's structural validation.
     */
    [[nodiscard]] int ValidateSourceHeader(std::span<const std::uint8_t> header);

    /** @brief Bounded opaque square RGB-code-value area reduction and complete RGBA8 mip chain.
     * Matches the game's existing non-sRGB SurfaceFormat::Color shading convention. Does not
     * claim physically linear-light filtering or accept alpha requiring premultiplication.
     */
    [[nodiscard]] std::vector<MipLevel> PrepareTexture(std::span<const Pixel> source, int sourceSize,
                                                     int targetSize = RuntimeTextureSize);
}
