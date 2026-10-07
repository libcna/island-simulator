#pragma once

#include <memory>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class GraphicsDevice;
    class ShaderEffect;
    class VertexBuffer;
}

namespace island
{
    class ShipwreckLayout;

    /** @brief One portable static batch for the grounded 20 m wreck and its debris field. */
    class ShipwreckRenderer final
    {
    public:
        ShipwreckRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          const ShipwreckLayout& layout);
        ~ShipwreckRenderer();

        ShipwreckRenderer(const ShipwreckRenderer&) = delete;
        ShipwreckRenderer& operator=(const ShipwreckRenderer&) = delete;
        ShipwreckRenderer(ShipwreckRenderer&&) = delete;
        ShipwreckRenderer& operator=(ShipwreckRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition);

        [[nodiscard]] int VertexCount() const noexcept { return vertexCount_; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_; }
        [[nodiscard]] int DebrisCount() const noexcept { return debrisCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        [[nodiscard]] bool IsVisible() const noexcept { return visible_; }
        [[nodiscard]] bool UsesAgedWoodShader() const noexcept
        {
            return agedWoodShaderReady_;
        }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> agedWoodEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
        Microsoft::Xna::Framework::BoundingBox bounds_;
        Microsoft::Xna::Framework::Vector3 centre_;
        Microsoft::Xna::Framework::Vector3 woodRight_;
        Microsoft::Xna::Framework::Vector3 woodForward_;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
        int debrisCount_ = 0;
        int drawCallCount_ = 0;
        bool visible_ = false;
        bool agedWoodShaderReady_ = false;
    };
}
