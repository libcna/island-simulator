#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class SpriteBatch;
    class SpriteFont;
    class Texture2D;
}

namespace island
{
    /** @brief Per-frame diagnostics presented by the opt-in developer overlay. */
    struct DebugOverlayState
    {
        Microsoft::Xna::Framework::Vector3 playerPosition;
        std::string renderer;
        bool swimming = false;
        float groundHeight = 0.0f;
        float groundSlopeDegrees = 0.0f;
        float offshoreDistance = 0.0f;
        float oceanCurrentStrength = 0.0f;
        int visibleTerrainCells = 0;
        int totalTerrainCells = 0;
        int visiblePalms = 0;
        int totalPalms = 0;
        int nearPalms = 0;
        int farPalms = 0;
        int palmCells = 0;
        int palmDraws = 0;
        int visibleCanopy = 0;
        int totalCanopy = 0;
        int visibleUnderstory = 0;
        int totalUnderstory = 0;
        int visibleBushes = 0;
        int totalBushes = 0;
        int visibleGroundPlants = 0;
        int totalGroundPlants = 0;
        int jungleCells = 0;
        int jungleDraws = 0;
        int visibleRocks = 0;
        int totalRocks = 0;
        int visibleLogs = 0;
        int totalLogs = 0;
        int visibleDriftwood = 0;
        int totalDriftwood = 0;
        int propCells = 0;
        int propDraws = 0;
        int visibleGrassPatches = 0;
        int totalGrassPatches = 0;
        int grassCells = 0;
        int grassDraws = 0;
        bool audioAvailable = false;
        bool audioMuted = false;
        float surfVolume = 0.0f;
        float breezeVolume = 0.0f;
        float jungleVolume = 0.0f;
        float waterMovementVolume = 0.0f;
        int playedWaveAccents = 0;
        int playedWaterTransitions = 0;
    };

    /** @brief Small asset-free bitmap-font overlay for renderer and gameplay diagnostics. */
    class DebugOverlay final
    {
    public:
        explicit DebugOverlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~DebugOverlay();

        DebugOverlay(const DebugOverlay&) = delete;
        DebugOverlay& operator=(const DebugOverlay&) = delete;
        DebugOverlay(DebugOverlay&&) = delete;
        DebugOverlay& operator=(DebugOverlay&&) = delete;

        /** @brief Records actual wall-clock spacing between presented draw frames. */
        void UpdateFrameTiming() noexcept;

        /** @brief Draws the diagnostics over an already-complete 3D scene. */
        void Draw(const DebugOverlayState& state);

        [[nodiscard]] float FramesPerSecond() const noexcept { return framesPerSecond_; }
        [[nodiscard]] float FrameMilliseconds() const noexcept { return frameMilliseconds_; }

    private:
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteFont> font_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> whitePixel_;
        std::chrono::steady_clock::time_point previousFrameTime_;
        float timingWindowSeconds_ = 0.0f;
        float framesPerSecond_ = 0.0f;
        float frameMilliseconds_ = 0.0f;
        int timingWindowFrames_ = 0;
        bool hasPreviousFrameTime_ = false;
    };
}
