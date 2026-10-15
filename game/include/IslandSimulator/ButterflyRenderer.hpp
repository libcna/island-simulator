#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/ButterflyMeadow.hpp"
#include "IslandSimulator/GeckoPopulation.hpp"
#include "IslandSimulator/SkinkPopulation.hpp"
#include "IslandSimulator/DragonflySwarm.hpp"
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
    /** @brief One-batch portable renderer for low-altitude clearing butterflies. */
    class ButterflyRenderer final
    {
    public:
        explicit ButterflyRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            float maximumDistance = 72.0f);
        ~ButterflyRenderer();

        ButterflyRenderer(const ButterflyRenderer&) = delete;
        ButterflyRenderer& operator=(const ButterflyRenderer&) = delete;
        ButterflyRenderer(ButterflyRenderer&&) = delete;
        ButterflyRenderer& operator=(ButterflyRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const ButterflyState> butterflies,
                  std::span<const ButterflyFlowerPatch> flowerPatches,
                  std::span<const GeckoState> geckos,
                  std::span<const SkinkState> skinks,
                  std::span<const DragonflyState> dragonflies);

        [[nodiscard]] int VisibleButterflyCount() const noexcept {
            return visibleButterflyCount_;
        }
        [[nodiscard]] int VisibleNearButterflyCount() const noexcept {
            return visibleNearButterflyCount_;
        }
        [[nodiscard]] int VisibleFarButterflyCount() const noexcept {
            return visibleFarButterflyCount_;
        }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        [[nodiscard]] int VisibleGeckoCount() const noexcept { return visibleGeckoCount_; }
        [[nodiscard]] int VisibleSkinkCount() const noexcept { return visibleSkinkCount_; }
        [[nodiscard]] int VisibleDragonflyCount() const noexcept { return visibleDragonflyCount_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        int visibleButterflyCount_ = 0;
        int visibleNearButterflyCount_ = 0;
        int visibleFarButterflyCount_ = 0;
        int visibleGeckoCount_ = 0;
        int visibleSkinkCount_ = 0;
        int visibleDragonflyCount_ = 0;
        int drawCallCount_ = 0;
        float maximumDistance_ = 72.0f;
    };
}
