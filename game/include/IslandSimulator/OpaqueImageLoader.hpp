#pragma once
#include "IslandSimulator/ForestFloorPixels.hpp"

namespace Microsoft::Xna::Framework::Graphics { class GraphicsDevice; }
namespace island
{
    // Shared bounded PNG loading through CNA, followed by the existing CPU area/mip filter.
    // Historical forest_floor pixel types are reused, not a second image-processing pipeline.
    [[nodiscard]] std::vector<forest_floor::MipLevel> LoadOpaqueImageMips(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const char* path, int targetSize);
}
