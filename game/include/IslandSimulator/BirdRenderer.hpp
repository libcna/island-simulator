#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/BirdFlock.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class DynamicVertexBuffer;
    class GraphicsDevice;
    struct VertexPositionColor;
}

namespace island
{
    /** @brief One-batch portable procedural renderer for the island's animated birds. */
    class BirdRenderer final
    {
    public:
        explicit BirdRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~BirdRenderer();

        BirdRenderer(const BirdRenderer&) = delete;
        BirdRenderer& operator=(const BirdRenderer&) = delete;
        BirdRenderer(BirdRenderer&&) = delete;
        BirdRenderer& operator=(BirdRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const BirdState> birds);

        [[nodiscard]] int VisibleBirdCount() const noexcept { return visibleBirdCount_; }
        [[nodiscard]] int VisibleNearBirdCount() const noexcept { return visibleNearBirdCount_; }
        [[nodiscard]] int VisibleFarBirdCount() const noexcept { return visibleFarBirdCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        int visibleBirdCount_ = 0;
        int visibleNearBirdCount_ = 0;
        int visibleFarBirdCount_ = 0;
        int drawCallCount_ = 0;
    };
}
