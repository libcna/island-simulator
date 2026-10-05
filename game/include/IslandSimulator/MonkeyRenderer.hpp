#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "IslandSimulator/MonkeyTroop.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class IndexBuffer;
    class SkinnedEffect;
    struct SkinningData;
    class Texture2D;
    class VertexBuffer;
}

namespace island
{
    /** @brief Portable GPU-skinned renderer for the procedural monkey troop. */
    class MonkeyRenderer final
    {
    public:
        static constexpr std::size_t SilhouetteCount = 3U;

        explicit MonkeyRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~MonkeyRenderer();

        MonkeyRenderer(const MonkeyRenderer&) = delete;
        MonkeyRenderer& operator=(const MonkeyRenderer&) = delete;
        MonkeyRenderer(MonkeyRenderer&&) = delete;
        MonkeyRenderer& operator=(MonkeyRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const MonkeyState> monkeys);

        [[nodiscard]] int VisibleMonkeyCount() const noexcept { return visibleMonkeyCount_; }
        [[nodiscard]] int VisibleNearMonkeyCount() const noexcept {
            return visibleNearMonkeyCount_;
        }
        [[nodiscard]] int VisibleFarMonkeyCount() const noexcept {
            return visibleFarMonkeyCount_;
        }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        [[nodiscard]] int ActiveCrossfadeCount() const noexcept {
            return activeCrossfadeCount_;
        }
        [[nodiscard]] std::uint64_t CompletedCrossfadeCount() const noexcept {
            return completedCrossfadeCount_;
        }

    private:
        struct PlaybackState;

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> whiteTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SkinnedEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SkinningData> skinningData_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer>,
                   SilhouetteCount> nearVertices_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer>,
                   SilhouetteCount> nearIndices_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer>,
                   SilhouetteCount> farVertices_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer>,
                   SilhouetteCount> farIndices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> faceVertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> faceIndices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> eyeVertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> eyeIndices_;
        std::vector<std::unique_ptr<PlaybackState>> playbackStates_;
        std::array<int, SilhouetteCount> nearVertexCounts_{};
        std::array<int, SilhouetteCount> nearPrimitiveCounts_{};
        std::array<int, SilhouetteCount> farVertexCounts_{};
        std::array<int, SilhouetteCount> farPrimitiveCounts_{};
        int faceVertexCount_ = 0;
        int facePrimitiveCount_ = 0;
        int eyeVertexCount_ = 0;
        int eyePrimitiveCount_ = 0;
        int visibleMonkeyCount_ = 0;
        int visibleNearMonkeyCount_ = 0;
        int visibleFarMonkeyCount_ = 0;
        int drawCallCount_ = 0;
        int activeCrossfadeCount_ = 0;
        std::uint64_t completedCrossfadeCount_ = 0U;
    };
}
