#pragma once

#include <cstddef>
#include "IslandSimulator/ForestFloorPixels.hpp"
#include "IslandSimulator/LeafSurfaceModel.hpp"

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
        static constexpr int CanopyLightSize = 1024;
        static constexpr int BoarCoatSize = 256;
        static constexpr int SnakeScaleSize = 64;
        static constexpr int ResidentMaterialSize = 128;
        static constexpr std::size_t ResidentMaterialMipBytes = 87380U;
        static constexpr int BearFurSize = 128;
        static constexpr std::size_t BearFurMipBytes = 87380U;
        static constexpr std::size_t MonkeyFurMipBytes = 1398100U;
        static constexpr std::size_t HutThatchMipBytes = 349524U;
        static constexpr int GoatFurSize = 256;
        static constexpr std::size_t GoatFurMipBytes = 349524U;
        static constexpr int SpiderSetaeSize = 256;
        static constexpr std::size_t SpiderSetaeMipBytes = 349524U;
        static constexpr std::size_t SnakeScaleRgba8Bytes = SnakeScaleSize * SnakeScaleSize * BytesPerPixel;
        // One shared map for grass roots and jungle litter; retain established budget names.
        static constexpr int GrassHeightSize = 241;
        static constexpr std::size_t GrassHeightRgba8Bytes =
            static_cast<std::size_t>(GrassHeightSize) * GrassHeightSize * BytesPerPixel;
        static constexpr std::size_t BoarCoatRgba8Bytes = 349524U; // File-backed full mip chain.
        static constexpr std::size_t CanopyLightRgba8Bytes =
            static_cast<std::size_t>(CanopyLightSize) * CanopyLightSize * BytesPerPixel;

        static constexpr std::size_t TotalPixelCount =
            static_cast<std::size_t>(DebugAtlasWidth * DebugAtlasHeight)
            + static_cast<std::size_t>(TouchCircleSize * TouchCircleSize
                                       * TouchCircleTextureCount)
            + static_cast<std::size_t>(WaterViewWidth * WaterViewHeight)
            + static_cast<std::size_t>(SolidWhiteTextureSize * SolidWhiteTextureSize
                                       * SolidWhiteTextureCount)
            + static_cast<std::size_t>(CanopyLightSize * CanopyLightSize)
            + static_cast<std::size_t>(SnakeScaleSize * SnakeScaleSize)
            + static_cast<std::size_t>(ResidentMaterialSize * ResidentMaterialSize)
            + static_cast<std::size_t>(BearFurSize * BearFurSize)
            + static_cast<std::size_t>(GrassHeightSize * GrassHeightSize);
        static constexpr std::size_t TotalRgba8Bytes =
            TotalPixelCount * static_cast<std::size_t>(BytesPerPixel);
        // All game texture payloads, including the file-backed floor and leaf mip chains.
        // Excludes decoder temporaries, CNA CPU shadows, driver padding and framebuffer storage.
        static constexpr std::size_t TotalRuntimeRgba8Bytes =
            TotalRgba8Bytes + forest_floor::RuntimeFullMipBytes + leaf_surface::FullMipBytes
            + MonkeyFurMipBytes + HutThatchMipBytes + GoatFurMipBytes + SpiderSetaeMipBytes
            + BoarCoatRgba8Bytes + ResidentMaterialMipBytes + BearFurMipBytes
            - static_cast<std::size_t>((ResidentMaterialSize * ResidentMaterialSize
                + BearFurSize * BearFurSize) * BytesPerPixel);
    };

    static_assert(GeneratedTextureBudget::TotalRgba8Bytes == 4717708U,
                  "Update the generated texture audit when the base-level payload changes");
}
