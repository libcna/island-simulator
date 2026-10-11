#pragma once
#include <memory>
#include <span>
#include <vector>
#include "IslandSimulator/SnakePopulation.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
namespace Microsoft::Xna::Framework::Graphics {
    class GraphicsDevice; class BasicEffect; class DynamicVertexBuffer; class Texture2D;
    struct VertexPositionColorTexture;
}
namespace island {
    class CanopyLightField;
    class SnakeRenderer final {
    public:
        SnakeRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      const CanopyLightField& canopyLight);
        ~SnakeRenderer();
        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& camera, std::span<const SnakeState> snakes);
        [[nodiscard]] int VisibleCount() const noexcept { return visible_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return draws_; }
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const CanopyLightField& canopy_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColorTexture> vertices_;
        int visible_=0, draws_=0;
    };
}
