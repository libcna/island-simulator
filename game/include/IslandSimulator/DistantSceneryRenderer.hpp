#pragma once

#include <memory>

#include "Microsoft/Xna/Framework/Matrix.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class GraphicsDevice;
    class VertexBuffer;
}

namespace island
{
    class DistantSceneryLayout;

    /** @brief One portable fogged batch for unreachable low horizon islets. */
    class DistantSceneryRenderer final
    {
    public:
        DistantSceneryRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            const DistantSceneryLayout& layout);
        ~DistantSceneryRenderer();

        DistantSceneryRenderer(const DistantSceneryRenderer&) = delete;
        DistantSceneryRenderer& operator=(const DistantSceneryRenderer&) = delete;
        DistantSceneryRenderer(DistantSceneryRenderer&&) = delete;
        DistantSceneryRenderer& operator=(DistantSceneryRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection);

        [[nodiscard]] int IsletCount() const noexcept { return isletCount_; }
        [[nodiscard]] int VertexCount() const noexcept { return vertexCount_; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
        int isletCount_ = 0;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
        int drawCallCount_ = 0;
    };
}
