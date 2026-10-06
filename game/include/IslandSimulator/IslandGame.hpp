#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "IslandSimulator/BenchmarkTraversalRoute.hpp"
#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/BirdRenderer.hpp"
#include "IslandSimulator/ContactShadowRenderer.hpp"
#ifndef NDEBUG
#include "IslandSimulator/DebugFreeCamera.hpp"
#endif
#include "IslandSimulator/DebugOverlay.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/DistantSceneryRenderer.hpp"
#include "IslandSimulator/FishRenderer.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/FirstPersonCameraMotion.hpp"
#include "IslandSimulator/GrassInteractionModel.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/JungleRenderer.hpp"
#include "IslandSimulator/MonkeyRenderer.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/NaturalPropRenderer.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GrassRenderer.hpp"
#include "IslandSimulator/OceanRenderer.hpp"
#include "IslandSimulator/PalmRenderer.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/SharkRenderer.hpp"
#include "IslandSimulator/ShorelineRenderer.hpp"
#include "IslandSimulator/SkyRenderer.hpp"
#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/TerrainRenderer.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/TouchOverlay.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/WaterRippleModel.hpp"
#include "IslandSimulator/WaterViewModel.hpp"
#include "IslandSimulator/WaterViewOverlay.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/WorldDebugRenderer.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace island
{
    /** @brief Deterministic camera positions used for visual regression review. */
    enum class FixedViewpoint
    {
        None,
        Overview,
        BeachPath,
        JungleTrail,
        EastClearingDetail,
        WestClearingDetail,
        GrassDetail,
        GroundPlantDetail,
        GroundContactDetail,
        MovementTraceDetail,
        PalmDetail,
        WaterlineDetail,
        CurrentBoundaryDetail,
        DistantSceneryDetail,
        PropDetail,
        DriftwoodDetail,
        ReefDetail,
        FallenFrondDetail,
        BirdDetail,
        FishDetail,
        SharkDetail,
        MonkeyDetail,
        BenchmarkTraversal
    };

    /** @brief Main application and first portable island rendering slice. */
    class IslandGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        /**
         * @brief Creates the game.
         * @param smokeTest Whether to exit automatically after a few rendered frames.
         * @param capturePath Optional PPM path for a deterministic rendered-frame capture.
         * @param viewpoint Optional fixed QA camera without player movement.
         * @param benchmarkPath Optional JSON destination for an uncapped fixed-view benchmark.
         * @param startupReportPath Optional JSON destination for first-draw phase timing.
         */
        explicit IslandGame(bool smokeTest = false, std::string capturePath = {},
                            FixedViewpoint viewpoint = FixedViewpoint::None,
                            bool showDebugOverlay = false,
                            bool inputDiagnostics = false,
                            bool showWorldDebug = false,
                            std::string benchmarkPath = {},
                            std::string startupReportPath = {});

        GetTypeNameHPP()

    protected:
        /** @brief Creates GPU resources after CNA initializes the graphics device. */
        void LoadContent() override;

        /** @brief Updates input and the early first-person camera. */
        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;

        /** @brief Draws terrain, vegetation, and ocean in stable pass order. */
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

    private:
        void CaptureFrame(const std::string& path);
        void RecordBenchmarkFrame();
        void RecordBenchmarkSceneSample();
        void RecordStartupTiming(std::chrono::steady_clock::time_point drawStarted);
        void WriteBenchmarkReport();
        void WriteStartupReport();
        void UpdateBenchmarkTraversal();
        void UpdateCamera(float deltaSeconds);
        void TryStartSoundscape() noexcept;
        void UpdateCameraMatrices();
        [[nodiscard]] int TrackedSceneDrawCount() const noexcept;
        [[nodiscard]] bool IsDebugFreeCameraActive() const noexcept;

        struct BenchmarkSceneRanges
        {
            std::size_t sampleCount = 0U;
            int minimumTrackedDraws = 0;
            int maximumTrackedDraws = 0;
            std::uint64_t totalTrackedDraws = 0U;
            int minimumTerrainCells = 0;
            int maximumTerrainCells = 0;
            std::uint64_t totalTerrainCells = 0U;
            int minimumTerrainDraws = 0;
            int maximumTerrainDraws = 0;
            std::uint64_t totalTerrainDraws = 0U;
            int minimumContactShadows = 0;
            int maximumContactShadows = 0;
            std::uint64_t totalContactShadows = 0U;
            int minimumPalms = 0;
            int maximumPalms = 0;
            std::uint64_t totalPalms = 0U;
            int minimumCanopy = 0;
            int maximumCanopy = 0;
            std::uint64_t totalCanopy = 0U;
            int minimumGrass = 0;
            int maximumGrass = 0;
            std::uint64_t totalGrass = 0U;
            float minimumShoreDistance = 0.0f;
            float maximumShoreDistance = 0.0f;
            Microsoft::Xna::Framework::Vector3 firstCameraPosition{0.0f, 0.0f, 0.0f};
            Microsoft::Xna::Framework::Vector3 lastCameraPosition{0.0f, 0.0f, 0.0f};
        };

        std::chrono::steady_clock::time_point constructionStarted_ =
            std::chrono::steady_clock::now();
        Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
        IslandTerrain terrain_;
        DistantSceneryLayout distantSceneryLayout_;
        BirdFlock birdFlock_;
        FishSchools fishSchools_;
        SharkPatrol sharkPatrol_;
        MonkeyTroop monkeyTroop_;
        GrassLayout grassLayout_;
        VegetationLayout vegetationLayout_;
        JungleLayout jungleLayout_;
        NaturalPropLayout naturalPropLayout_;
        WorldCollisionGrid worldCollisionGrid_;
        BenchmarkTraversalRoute benchmarkTraversalRoute_;
        PlayerMotion playerMotion_;
        FirstPersonCameraMotion cameraMotion_;
#ifndef NDEBUG
        DebugFreeCamera debugFreeCamera_;
#endif
        GrassInteractionModel grassInteractionModel_;
        TouchControls touchControls_;
        TouchControlState touchControlState_;
        WaterRippleModel waterRippleModel_;
        WaterViewModel waterViewModel_;
        std::unique_ptr<BirdRenderer> birdRenderer_;
        std::unique_ptr<ContactShadowRenderer> contactShadowRenderer_;
        std::unique_ptr<DebugOverlay> debugOverlay_;
        std::unique_ptr<DistantSceneryRenderer> distantSceneryRenderer_;
        std::unique_ptr<FishRenderer> fishRenderer_;
        std::unique_ptr<GrassRenderer> grassRenderer_;
        std::unique_ptr<JungleRenderer> jungleRenderer_;
        std::unique_ptr<MonkeyRenderer> monkeyRenderer_;
        std::unique_ptr<NaturalPropRenderer> naturalPropRenderer_;
        std::unique_ptr<OceanRenderer> oceanRenderer_;
        std::unique_ptr<PalmRenderer> palmRenderer_;
        std::unique_ptr<SharkRenderer> sharkRenderer_;
        std::unique_ptr<ShorelineRenderer> shorelineRenderer_;
        std::unique_ptr<SkyRenderer> skyRenderer_;
        std::unique_ptr<Soundscape> soundscape_;
        std::unique_ptr<TerrainRenderer> terrainRenderer_;
        std::unique_ptr<TouchOverlay> touchOverlay_;
        std::unique_ptr<WaterViewOverlay> waterViewOverlay_;
        std::unique_ptr<WorldDebugRenderer> worldDebugRenderer_;
        Microsoft::Xna::Framework::Matrix cameraView_;
        Microsoft::Xna::Framework::Matrix cameraProjection_;
        Microsoft::Xna::Framework::Vector3 cameraPosition_;
        Microsoft::Xna::Framework::Vector3 renderCameraPosition_;
        float yaw_ = 0.0f;
        float pitch_ = -0.03f;
        float touchDisplayWidth_ = 1.0f;
        float touchDisplayHeight_ = 1.0f;
        bool smokeTest_ = false;
        FixedViewpoint viewpoint_ = FixedViewpoint::None;
        std::string capturePath_;
        bool mouseCaptured_ = false;
        bool debugOverlayVisible_ = false;
        bool debugOverlayToggleWasDown_ = false;
        bool worldDebugVisible_ = false;
        bool worldDebugToggleWasDown_ = false;
        bool worldSeedCopyWasDown_ = false;
        bool worldSeedCopySucceeded_ = false;
        float worldSeedCopyFeedbackSeconds_ = 0.0f;
#ifndef NDEBUG
        bool debugFreeCameraToggleWasDown_ = false;
#endif
        bool muteToggleWasDown_ = false;
        bool cameraMotionToggleWasDown_ = false;
        bool runToggleWasDown_ = false;
        bool runModeEnabled_ = false;
        bool movementKeyWasDown_ = false;
        bool escapeWasDown_ = false;
        bool soundscapeNeedsGesture_ = false;
        std::array<bool, 8> debugTeleportWasDown_{};
        std::size_t monkeyTeleportIndex_ = 0U;
        bool inputDiagnostics_ = false;
        bool touchOverlayVisible_ = false;
        float inputDiagnosticsTimer_ = 0.0f;
        std::uint32_t drawnFrames_ = 0;
        std::string benchmarkPath_;
        std::string startupReportPath_;
        std::chrono::steady_clock::time_point constructionFinished_;
        std::chrono::steady_clock::time_point contentFinished_;
        double worldGenerationMilliseconds_ = 0.0;
        double frameworkInitializationMilliseconds_ = 0.0;
        double contentCreationMilliseconds_ = 0.0;
        double postContentMilliseconds_ = 0.0;
        double firstDrawMilliseconds_ = 0.0;
        double totalStartupMilliseconds_ = 0.0;
        bool startupTimingRecorded_ = false;
        std::chrono::steady_clock::time_point benchmarkPreviousFrameTime_;
        std::vector<double> benchmarkFrameMilliseconds_;
        std::size_t benchmarkWarmupIntervals_ = 0U;
        bool benchmarkHasPreviousFrame_ = false;
        bool benchmarkSceneSamplePending_ = false;
        bool benchmarkReportPending_ = false;
        float benchmarkTraversalPresentationSpeed_ = 0.0f;
        BenchmarkSceneRanges benchmarkSceneRanges_;
    };
}
