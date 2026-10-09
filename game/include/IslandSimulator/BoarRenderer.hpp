#pragma once

#include <memory>
#include <span>
#include <vector>
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class DynamicVertexBuffer;
    class GraphicsDevice;
    class Texture2D;
    struct VertexPositionColorTexture;
}

namespace island
{
    class CanopyLightField;
    /** @brief Original articulated boars in one bounded, culled portable geometry batch. */
    class BoarRenderer final
    {
    public:
        BoarRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     const IslandTerrain& terrain, const CanopyLightField& canopyLight,
                     float maximumDistance = 135.0f);
        ~BoarRenderer();
        BoarRenderer(const BoarRenderer&) = delete;
        BoarRenderer& operator=(const BoarRenderer&) = delete;
        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& camera,
                  std::span<const BoarState> boars);
        [[nodiscard]] int VisibleCount() const noexcept { return visibleCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCalls_; }
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const IslandTerrain& terrain_;
        const CanopyLightField& canopyLight_;
        RenderedTerrainSurface renderedTerrain_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> coatTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColorTexture> staging_;
        float maximumDistance_;
        int visibleCount_ = 0;
        int drawCalls_ = 0;
        bool reportedContactLimit_ = false;
    };
}
