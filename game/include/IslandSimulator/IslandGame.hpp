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
#include "IslandSimulator/SnakeRenderer.hpp"
#include "IslandSimulator/ButterflyMeadow.hpp"
#include "IslandSimulator/ButterflyRenderer.hpp"
#include "IslandSimulator/ContactShadowRenderer.hpp"
#include "IslandSimulator/CrabColony.hpp"
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/BoarRenderer.hpp"
#include "IslandSimulator/GoatRenderer.hpp"
#include "IslandSimulator/SpiderRenderer.hpp"
#include "IslandSimulator/CanopyLighting.hpp"
#include "IslandSimulator/RenderedTerrainHeightMap.hpp"
#include "IslandSimulator/CrabRenderer.hpp"
#ifndef NDEBUG
#include "IslandSimulator/DebugFreeCamera.hpp"
#endif
#include "IslandSimulator/DebugOverlay.hpp"
#include "IslandSimulator/FlightMotion.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/DistantSceneryRenderer.hpp"
#include "IslandSimulator/FishRenderer.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/FirstPersonCameraMotion.hpp"
#include "IslandSimulator/GrassInteractionModel.hpp"
#include "IslandSimulator/ControlTransitionDiagnostic.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/JungleRenderer.hpp"
#include "IslandSimulator/LocomotionPaceToggle.hpp"
#include "IslandSimulator/MonkeyRenderer.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/NaturalPropRenderer.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/GrassRenderer.hpp"
#include "IslandSimulator/OceanRenderer.hpp"
#include "IslandSimulator/PalmRenderer.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/RuntimeQualityProfile.hpp"
#include "IslandSimulator/SeaTurtleGroup.hpp"
#include "IslandSimulator/SeaTurtleRenderer.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/SharkRenderer.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/ShipwreckRenderer.hpp"
#include "IslandSimulator/HutRenderer.hpp"
#include "IslandSimulator/ShorelineRenderer.hpp"
#include "IslandSimulator/SkyRenderer.hpp"
#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/TerrainRenderer.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/TouchOverlay.hpp"
#include "IslandSimulator/UserSettings.hpp"
#include "IslandSimulator/UserSettingsStore.hpp"
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
        BayOverview,
        BayBeachDetail,
        HeadlandDetail,
        BeachPath,
        JungleTrail,
        EastClearingDetail,
        ClearingEdgeDetail,
        CanopyInspection,
        WestClearingDetail,
        StreamDetail,
        GrassDetail,
        GroundPlantDetail,
        GroundBroadleafDetail,
        BushDetail,
        FallenLeafDetail,
        GroundContactDetail,
        MovementTraceDetail,
        PalmDetail,
        WaterlineDetail,
        WetSandDetail,
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
        MonkeyTreeDetail,
        MonkeyClimbDetail,
        ShipwreckDetail,
        HutDetail,
        HutMaterialDetail,
        CrabDetail,
        BoarDetail,
        GoatDetail,
        SpiderDetail,
        SpiderBrightDetail,
        SnakeDetail,
        SnakeHeadDetail,
        ButterflyDetail,
        SeaTurtleDetail,
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
                            std::string startupReportPath = {},
                            bool userSettingsEnabled = true,
                            std::optional<CanopyLodInspection> canopyLodInspection = std::nullopt);

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
        void ReportControlTransition(ControlTransitionEvent event) noexcept;
        void InitializeUserSettings();
        void PersistUserSettings();
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
            int minimumCrabs = 0;
            int maximumCrabs = 0;
            std::uint64_t totalCrabs = 0U;
            int minimumBoars = 0;
            int maximumBoars = 0;
            std::uint64_t totalBoars = 0U;
            int minimumButterflies = 0;
            int maximumButterflies = 0;
            std::uint64_t totalButterflies = 0U;
            int minimumSeaTurtles = 0;
            int maximumSeaTurtles = 0;
            std::uint64_t totalSeaTurtles = 0U;
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
        ButterflyMeadow butterflyMeadow_;
        CrabColony crabColony_;
        BoarHerd boarHerd_;
        std::unique_ptr<SnakePopulation> snakes_;
        std::unique_ptr<GoatHerd> goats_;
        std::unique_ptr<SpiderColony> spiders_;
        FishSchools fishSchools_;
        SeaTurtleGroup seaTurtleGroup_;
        SharkPatrol sharkPatrol_;
        MonkeyTroop monkeyTroop_;
        VegetationLayout vegetationLayout_;
        JungleLayout jungleLayout_;
        // CPU fields shared by grass placement and the uploaded light/cover texture.
        CanopyLightField canopyLightField_;
        GroundCoverField groundCoverField_;
        GrassLayout grassLayout_;
        NaturalPropLayout naturalPropLayout_;
        ShipwreckLayout shipwreckLayout_;
        std::optional<HutPlacement> hutPlacement_;
        WorldCollisionGrid worldCollisionGrid_;
        BenchmarkTraversalRoute benchmarkTraversalRoute_;
        std::unique_ptr<RenderedTerrainSurface> swimTerrainSurface_;
        PlayerMotion playerMotion_;
        FlightMotion flightMotion_;
        bool flightToggleWasDown_ = false;
        FirstPersonCameraMotion cameraMotion_;
        LocomotionPaceToggle locomotionPaceToggle_;
#ifndef NDEBUG
        DebugFreeCamera debugFreeCamera_;
#endif
        GrassInteractionModel grassInteractionModel_;
        TouchControls touchControls_;
        TouchControlState touchControlState_;
        UserSettings userSettings_;
        std::unique_ptr<UserSettingsStore> userSettingsStore_;
        WaterRippleModel waterRippleModel_;
        WaterViewModel waterViewModel_;
#if defined(__ANDROID__)
        RuntimeQualityProfile qualityProfile_ = RuntimeQualityProfile::Mobile();
#else
        RuntimeQualityProfile qualityProfile_ = RuntimeQualityProfile::Full();
#endif
        // Declared first so it outlives all rendering consumers during member destruction.
        std::unique_ptr<CanopyLighting> canopyLighting_;
        std::unique_ptr<RenderedTerrainHeightMap> terrainHeightMap_;
        std::unique_ptr<BirdRenderer> birdRenderer_;
        std::unique_ptr<ButterflyRenderer> butterflyRenderer_;
        std::unique_ptr<ContactShadowRenderer> contactShadowRenderer_;
        std::unique_ptr<CrabRenderer> crabRenderer_;
        std::unique_ptr<BoarRenderer> boarRenderer_;
        std::unique_ptr<GoatRenderer> goatRenderer_;
        std::unique_ptr<SpiderRenderer> spiderRenderer_;
        std::unique_ptr<SnakeRenderer> snakeRenderer_;
        std::unique_ptr<DebugOverlay> debugOverlay_;
        std::unique_ptr<DistantSceneryRenderer> distantSceneryRenderer_;
        std::unique_ptr<FishRenderer> fishRenderer_;
        std::unique_ptr<GrassRenderer> grassRenderer_;
        std::unique_ptr<JungleRenderer> jungleRenderer_;
        std::unique_ptr<MonkeyRenderer> monkeyRenderer_;
        std::unique_ptr<NaturalPropRenderer> naturalPropRenderer_;
        std::unique_ptr<OceanRenderer> oceanRenderer_;
        std::unique_ptr<PalmRenderer> palmRenderer_;
        std::unique_ptr<SeaTurtleRenderer> seaTurtleRenderer_;
        std::unique_ptr<SharkRenderer> sharkRenderer_;
        std::unique_ptr<ShipwreckRenderer> shipwreckRenderer_;
        std::unique_ptr<HutRenderer> hutRenderer_;
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
        std::size_t monkeyQaSubjectIndex_ = 0;
        std::vector<std::vector<MonkeyState>> monkeyTreeQaPoses_;
        std::string capturePath_;
        std::optional<CanopyLodInspection> canopyLodInspection_;
        bool mouseCaptured_ = false;
#if defined(__EMSCRIPTEN__)
        bool browserPointerReleasePending_ = false;
#endif
        bool debugOverlayVisible_ = false;
        bool debugOverlayToggleWasDown_ = false;
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
        bool fullscreenToggleWasDown_ = false;
#endif
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
        float locomotionModeNoticeSeconds_ = 0.0f;
        bool movementKeyWasDown_ = false;
        bool escapeWasDown_ = false;
        bool soundscapeNeedsGesture_ = false;
        bool userSettingsEnabled_ = true;
        bool swimmingEntryDiagnosticReported_ = false;
        std::array<bool, 9> debugTeleportWasDown_{};
        std::size_t monkeyTeleportIndex_ = 0U;
        std::size_t boarTeleportIndex_ = 0U;
        bool inputDiagnostics_ = false;
        bool touchOverlayVisible_ = false;
        float inputDiagnosticsTimer_ = 0.0f;
        std::uint32_t drawnFrames_ = 0;
        std::uint32_t snakeCaptureUpdates_ = 0;
        std::uint32_t goatCaptureUpdates_ = 0;
        std::uint32_t boarCaptureUpdates_ = 0;
        [[nodiscard]] bool UsesBoarCaptureClock() const noexcept {
            return !capturePath_.empty() && viewpoint_==FixedViewpoint::BoarDetail;
        }
        std::uint32_t spiderCaptureUpdates_ = 0;
        std::size_t spiderViewSubject_ = 0;
        [[nodiscard]] bool UsesSpiderViewpoint() const noexcept {
            return viewpoint_==FixedViewpoint::SpiderDetail||viewpoint_==FixedViewpoint::SpiderBrightDetail;
        }
        [[nodiscard]] bool UsesSpiderCaptureClock() const noexcept {
            return !capturePath_.empty() && UsesSpiderViewpoint();
        }
        [[nodiscard]] bool UsesGoatCaptureClock() const noexcept {
            return !capturePath_.empty() && viewpoint_==FixedViewpoint::GoatDetail;
        }
        bool snakeFirstDrawReturnReported_ = false;
        [[nodiscard]] bool UsesSnakeCaptureClock() const noexcept {
            return !capturePath_.empty() && (viewpoint_ == FixedViewpoint::SnakeDetail
                || viewpoint_ == FixedViewpoint::SnakeHeadDetail);
        }
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
