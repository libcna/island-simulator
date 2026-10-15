#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/CrabColony.hpp"
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
    /** @brief One-batch portable renderer for animated beach crabs. */
    class CrabRenderer final
    {
    public:
        explicit CrabRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              float maximumDistance = 92.0f);
        ~CrabRenderer();

        CrabRenderer(const CrabRenderer&) = delete;
        CrabRenderer& operator=(const CrabRenderer&) = delete;
        CrabRenderer(CrabRenderer&&) = delete;
        CrabRenderer& operator=(CrabRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const CrabState> crabs,
                  std::span<const CrabBurrow> burrows);

        [[nodiscard]] int VisibleCrabCount() const noexcept { return visibleCrabCount_; }
        [[nodiscard]] int VisibleNearCrabCount() const noexcept { return visibleNearCrabCount_; }
        [[nodiscard]] int VisibleFarCrabCount() const noexcept { return visibleFarCrabCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        int visibleCrabCount_ = 0;
        int visibleNearCrabCount_ = 0;
        int visibleFarCrabCount_ = 0;
        int drawCallCount_ = 0;
        float maximumDistance_ = 92.0f;
    };
}
