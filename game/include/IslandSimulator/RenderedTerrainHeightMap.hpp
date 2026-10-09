#pragma once

#include <memory>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class ShaderEffect;
    class Texture2D;
}

namespace island
{
    class IslandTerrain;
    // Immutable shared RGB24 terrain data. IslandGame owns it before the borrowing
    // grass/jungle renderers and releases it after them, including failed reloads.
    class RenderedTerrainHeightMap final
    {
    public:
        RenderedTerrainHeightMap(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                 const IslandTerrain& terrain);
        ~RenderedTerrainHeightMap();
        RenderedTerrainHeightMap(const RenderedTerrainHeightMap&) = delete;
        RenderedTerrainHeightMap& operator=(const RenderedTerrainHeightMap&) = delete;
        // Slot 0 belongs to shared canopy lighting. Both consumers use slot 1.
        void Bind(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    };
}
