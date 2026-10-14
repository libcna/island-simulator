#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/CrocodileGroup.hpp"
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
    /** One-batch portable renderer for the stream-estuary crocodiles. */
    class CrocodileRenderer final
    {
    public:
        explicit CrocodileRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            float maximumDistance = 220.0f);
        ~CrocodileRenderer();

        CrocodileRenderer(const CrocodileRenderer&) = delete;
        CrocodileRenderer& operator=(const CrocodileRenderer&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const CrocodileState> crocodiles);

        [[nodiscard]] int VisibleCount() const noexcept { return visibleCount_; }
        [[nodiscard]] int VisibleNearCount() const noexcept { return visibleNearCount_; }
        [[nodiscard]] int VisibleFarCount() const noexcept { return visibleFarCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        int visibleCount_ = 0;
        int visibleNearCount_ = 0;
        int visibleFarCount_ = 0;
        int drawCallCount_ = 0;
        float maximumDistance_ = 220.0f;
    };
}
