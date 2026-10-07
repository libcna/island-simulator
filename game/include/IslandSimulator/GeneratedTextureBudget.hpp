#pragma once

#include <cstddef>

namespace island
{
    /** @brief Exact base-level RGBA8 payload of game-generated textures. */
    struct GeneratedTextureBudget final
    {
        static constexpr int BytesPerPixel = 4;

        static constexpr int DebugAtlasWidth = 96;
        static constexpr int DebugAtlasHeight = 32;
        static constexpr int TouchCircleSize = 96;
        static constexpr int TouchCircleTextureCount = 2;
        static constexpr int WaterViewWidth = 160;
        static constexpr int WaterViewHeight = 90;
        static constexpr int SolidWhiteTextureSize = 1;
        static constexpr int SolidWhiteTextureCount = 2;

        static constexpr std::size_t TotalPixelCount =
            static_cast<std::size_t>(DebugAtlasWidth * DebugAtlasHeight)
            + static_cast<std::size_t>(TouchCircleSize * TouchCircleSize
                                       * TouchCircleTextureCount)
            + static_cast<std::size_t>(WaterViewWidth * WaterViewHeight)
            + static_cast<std::size_t>(SolidWhiteTextureSize * SolidWhiteTextureSize
                                       * SolidWhiteTextureCount);
        static constexpr std::size_t TotalRgba8Bytes =
            TotalPixelCount * static_cast<std::size_t>(BytesPerPixel);
    };

    static_assert(GeneratedTextureBudget::TotalRgba8Bytes == 143624U,
                  "Update the generated texture audit when the base-level payload changes");
}
