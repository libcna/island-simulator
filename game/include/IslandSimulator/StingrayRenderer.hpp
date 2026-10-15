#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/StingrayGroup.hpp"
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
    /** @brief Portable one-draw renderer for the shallow-water stingray group. */
    class StingrayRenderer final
    {
    public:
        explicit StingrayRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                  float maximumDistance = 220.0f);
        ~StingrayRenderer();

        StingrayRenderer(const StingrayRenderer&) = delete;
        StingrayRenderer& operator=(const StingrayRenderer&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const StingrayState> rays);

        [[nodiscard]] int VisibleCount() const noexcept { return visibleCount_; }
        [[nodiscard]] int NearCount() const noexcept { return nearCount_; }
        [[nodiscard]] int FarCount() const noexcept { return farCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> staging_;
        float maximumDistance_ = 220.0f;
        int visibleCount_ = 0;
        int nearCount_ = 0;
        int farCount_ = 0;
        int drawCallCount_ = 0;
    };
}
