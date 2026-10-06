#pragma once

#include <chrono>
#include <cstdint>
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
        Microsoft::Xna::Framework::Vector3 cameraPosition;
        std::string renderer;
        bool freeCameraActive = false;
        bool swimming = false;
        float groundHeight = 0.0f;
        float groundSlopeDegrees = 0.0f;
        float offshoreDistance = 0.0f;
        float oceanCurrentStrength = 0.0f;
        float waterRippleIntensity = 0.0f;
        float waterRipplePhase = 0.0f;
        float waterViewIntensity = 0.0f;
        int waterViewDraws = 0;
        int visibleTerrainCells = 0;
        int totalTerrainCells = 0;
        int distantIslets = 0;
        int distantSceneryDraws = 0;
        int visibleContactShadows = 0;
        int visiblePalmShadows = 0;
        int visibleCanopyShadows = 0;
        int visibleRockShadows = 0;
        int visibleWoodShadows = 0;
        int visibleMonkeyShadows = 0;
        int contactShadowDraws = 0;
        int visiblePalms = 0;
        int totalPalms = 0;
        int nearPalms = 0;
        int farPalms = 0;
        int palmCells = 0;
        int palmDraws = 0;
        int visibleCanopy = 0;
        int totalCanopy = 0;
        int nearCanopy = 0;
        int farCanopy = 0;
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
        int visibleReefRocks = 0;
        int totalReefRocks = 0;
        int visibleLogs = 0;
        int totalLogs = 0;
        int visibleDriftwood = 0;
        int totalDriftwood = 0;
        int visibleBeachDetails = 0;
        int totalBeachDetails = 0;
        int visibleFallenFronds = 0;
        int totalFallenFronds = 0;
        int propCells = 0;
        int propDraws = 0;
        int visibleGrassPatches = 0;
        int totalGrassPatches = 0;
        int grassCells = 0;
        int grassDraws = 0;
        float grassInteractionStrength = 0.0f;
        int visibleBirds = 0;
        int totalBirds = 0;
        int nearBirds = 0;
        int farBirds = 0;
        int fullSimulationBirds = 0;
        int reducedSimulationBirds = 0;
        int dormantSimulationBirds = 0;
        int birdDraws = 0;
        int visibleFish = 0;
        int totalFish = 0;
        int nearFish = 0;
        int farFish = 0;
        int fullSimulationFish = 0;
        int reducedSimulationFish = 0;
        int dormantSimulationFish = 0;
        int fishDraws = 0;
        bool sharkVisible = false;
        bool sharkPresent = false;
        bool sharkNear = false;
        float sharkActivity = 0.0f;
        int sharkSimulationTier = 0;
        int sharkDraws = 0;
        int sharkPresenceCycles = 0;
        int visibleMonkeys = 0;
        int totalMonkeys = 0;
        int nearMonkeys = 0;
        int farMonkeys = 0;
        int fullSimulationMonkeys = 0;
        int reducedSimulationMonkeys = 0;
        int dormantSimulationMonkeys = 0;
        int idleMonkeys = 0;
        int observeMonkeys = 0;
        int walkingMonkeys = 0;
        int runningMonkeys = 0;
        int monkeyDraws = 0;
        std::uint64_t monkeyObstacleDeflections = 0U;
        int activeMonkeyCrossfades = 0;
        std::uint64_t completedMonkeyCrossfades = 0U;
        bool audioAvailable = false;
        bool audioMuted = false;
        float surfVolume = 0.0f;
        float breezeVolume = 0.0f;
        float jungleVolume = 0.0f;
        float waterMovementVolume = 0.0f;
        float oceanCurrentAudioCue = 0.0f;
        int playedWaveAccents = 0;
        int playedWaterTransitions = 0;
        int playedBirdCalls = 0;
        int playedMonkeyCalls = 0;
        bool wildlifeCallActive = false;
        int playingAudioSources = 0;
        int audioSourceBudget = 0;
        bool worldDebugVisible = false;
        int worldDebugObstacles = 0;
        int collisionCandidates = 0;
        int collisionObstacles = 0;
        int worldDebugCells = 0;
        int worldDebugWildlife = 0;
        int worldDebugWildlifeTargets = 0;
        int worldDebugMonkeyHomes = 0;
        int worldDebugAudioZones = 0;
        int worldDebugAudioCandidates = 0;
        int worldDebugActiveAudioEmitters = 0;
        int worldDebugGroundSamples = 0;
        int worldDebugGroundSupports = 0;
        int worldDebugSteepSamples = 0;
        int movementObstacleRejections = 0;
        int movementTerrainRejections = 0;
        int movementSlideSubsteps = 0;
        bool movementSafetyClamped = false;
        int worldDebugLines = 0;
        int worldDebugDraws = 0;
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
