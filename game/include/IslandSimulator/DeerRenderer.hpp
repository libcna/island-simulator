#pragma once

#include "IslandSimulator/DeerHerd.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <memory>
#include <vector>

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class DynamicVertexBuffer;
    class GraphicsDevice;
    struct VertexPositionColor;
}

namespace island
{
    class CanopyLightField;

    /** One-draw portable procedural renderer for the small inland deer herd. */
    class DeerRenderer final
    {
    public:
        DeerRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     const CanopyLightField& canopy, float maximumDistance);
        ~DeerRenderer();
        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& camera,
                  std::span<const DeerState> deer);
        [[nodiscard]] int VisibleCount() const noexcept { return visibleCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const CanopyLightField& canopy_;
        float maximumDistance_ = 0.0f;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> staging_;
        int visibleCount_ = 0;
        int drawCallCount_ = 0;
    };
}
