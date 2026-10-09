#include "IslandSimulator/RenderedTerrainHeightMap.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include <vector>

namespace island
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;

    RenderedTerrainHeightMap::RenderedTerrainHeightMap(GraphicsDevice& device,
                                                       const IslandTerrain& terrain) : device_(device)
    {
        static_assert(GeneratedTextureBudget::GrassHeightSize == TerrainMeshLayout::Resolution);
        const auto surface = std::make_unique<RenderedTerrainSurface>(terrain);
        std::vector<Color> pixels;
        pixels.reserve(surface->Heights().size());
        for (const float height : surface->Heights()) {
            const auto p = RenderedTerrainSurface::EncodeHeight(height);
            pixels.emplace_back(p[0],p[1],p[2],p[3]);
        }
        texture_ = std::make_unique<Texture2D>(device_,TerrainMeshLayout::Resolution,TerrainMeshLayout::Resolution);
        texture_->SetData(pixels.data(),static_cast<int>(pixels.size()));
    }
    RenderedTerrainHeightMap::~RenderedTerrainHeightMap() = default;

    void RenderedTerrainHeightMap::Bind(ShaderEffect& effect)
    {
        device_.getSamplerStatesProperty()[1] = SamplerState::PointClamp;
        effect.SetTexture(1,*texture_);
        // Keep the established shared shader uniform names and exact decoding path.
        effect.SetUniformInt("uGrassTerrainHeight",1);
        effect.SetUniformVec4("uGrassTerrainMap",TerrainMeshLayout::Extent,
            TerrainMeshLayout::QuadsPerAxis,RenderedTerrainSurface::MinimumEncodedHeight,
            RenderedTerrainSurface::EncodedHeightRange);
    }
}
