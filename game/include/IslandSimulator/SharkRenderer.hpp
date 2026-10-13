#pragma once

#include <memory>
#include <vector>

#include "IslandSimulator/SharkPatrol.hpp"
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
    /** @brief Portable procedural renderer for the occasional offshore shark. */
    class SharkRenderer final
    {
    public:
        static constexpr int NearVertexCount = 834;
        static constexpr int FarVertexCount = 15;

        explicit SharkRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                               float maximumDistance = 710.0f);
        ~SharkRenderer();

        SharkRenderer(const SharkRenderer&) = delete;
        SharkRenderer& operator=(const SharkRenderer&) = delete;
        SharkRenderer(SharkRenderer&&) = delete;
        SharkRenderer& operator=(SharkRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  const SharkState& shark, float timeSeconds);

        [[nodiscard]] bool IsVisible() const noexcept { return visible_; }
        [[nodiscard]] bool UsesNearLod() const noexcept { return nearLod_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        bool visible_ = false;
        bool nearLod_ = false;
        int drawCallCount_ = 0;
        float maximumDistance_ = 710.0f;
    };
}
