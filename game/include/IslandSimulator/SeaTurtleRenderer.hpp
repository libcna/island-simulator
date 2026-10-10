#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/SeaTurtleGroup.hpp"
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
    /** @brief One-batch portable renderer for articulated near-shore sea turtles. */
    class SeaTurtleRenderer final
    {
    public:
        static constexpr int NearVertexCount = 1728;
        static constexpr int FarVertexCount = 36;

        explicit SeaTurtleRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            float maximumDistance = 190.0f);
        ~SeaTurtleRenderer();

        SeaTurtleRenderer(const SeaTurtleRenderer&) = delete;
        SeaTurtleRenderer& operator=(const SeaTurtleRenderer&) = delete;
        SeaTurtleRenderer(SeaTurtleRenderer&&) = delete;
        SeaTurtleRenderer& operator=(SeaTurtleRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const SeaTurtleState> turtles, float timeSeconds);

        [[nodiscard]] int VisibleTurtleCount() const noexcept { return visibleTurtleCount_; }
        [[nodiscard]] int VisibleNearTurtleCount() const noexcept {
            return visibleNearTurtleCount_;
        }
        [[nodiscard]] int VisibleFarTurtleCount() const noexcept {
            return visibleFarTurtleCount_;
        }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        int visibleTurtleCount_ = 0;
        int visibleNearTurtleCount_ = 0;
        int visibleFarTurtleCount_ = 0;
        int drawCallCount_ = 0;
        float maximumDistance_ = 190.0f;
    };
}
