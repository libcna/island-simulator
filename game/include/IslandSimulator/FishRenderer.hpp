#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/FishSchools.hpp"
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
    /** @brief One-batch portable renderer for animated shallow-water fish. */
    class FishRenderer final
    {
    public:
        static constexpr int NearVertexCount = 1434;
        static constexpr int FarVertexCount = 12;

        explicit FishRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              float maximumDistance = 215.0f);
        ~FishRenderer();

        FishRenderer(const FishRenderer&) = delete;
        FishRenderer& operator=(const FishRenderer&) = delete;
        FishRenderer(FishRenderer&&) = delete;
        FishRenderer& operator=(FishRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const FishState> fish, float timeSeconds);

        [[nodiscard]] int VisibleFishCount() const noexcept { return visibleFishCount_; }
        [[nodiscard]] int VisibleNearFishCount() const noexcept { return visibleNearFishCount_; }
        [[nodiscard]] int VisibleFarFishCount() const noexcept { return visibleFarFishCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        int visibleFishCount_ = 0;
        int visibleNearFishCount_ = 0;
        int visibleFarFishCount_ = 0;
        int drawCallCount_ = 0;
        float maximumDistance_ = 215.0f;
    };
}
