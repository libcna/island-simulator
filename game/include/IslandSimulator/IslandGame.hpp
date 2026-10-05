#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/BirdRenderer.hpp"
#include "IslandSimulator/DebugOverlay.hpp"
#include "IslandSimulator/FishRenderer.hpp"
#include "IslandSimulator/FishSchools.hpp"
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
#include "IslandSimulator/SkyRenderer.hpp"
#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/TerrainRenderer.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/TouchOverlay.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
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
        PropDetail,
        DriftwoodDetail,
        BirdDetail,
        FishDetail,
        SharkDetail,
        MonkeyDetail
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
         */
        explicit IslandGame(bool smokeTest = false, std::string capturePath = {},
                            FixedViewpoint viewpoint = FixedViewpoint::None,
                            bool showDebugOverlay = false,
                            bool inputDiagnostics = false,
                            bool showWorldDebug = false);

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
        void UpdateCamera(float deltaSeconds);
        void TryStartSoundscape() noexcept;
        void UpdateCameraMatrices();

        Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
        IslandTerrain terrain_;
        BirdFlock birdFlock_;
        FishSchools fishSchools_;
        SharkPatrol sharkPatrol_;
        MonkeyTroop monkeyTroop_;
        GrassLayout grassLayout_;
        VegetationLayout vegetationLayout_;
        JungleLayout jungleLayout_;
        NaturalPropLayout naturalPropLayout_;
        std::vector<CircularObstacle> worldObstacles_;
        PlayerMotion playerMotion_;
        TouchControls touchControls_;
        TouchControlState touchControlState_;
        std::unique_ptr<BirdRenderer> birdRenderer_;
        std::unique_ptr<DebugOverlay> debugOverlay_;
        std::unique_ptr<FishRenderer> fishRenderer_;
        std::unique_ptr<GrassRenderer> grassRenderer_;
        std::unique_ptr<JungleRenderer> jungleRenderer_;
        std::unique_ptr<MonkeyRenderer> monkeyRenderer_;
        std::unique_ptr<NaturalPropRenderer> naturalPropRenderer_;
        std::unique_ptr<OceanRenderer> oceanRenderer_;
        std::unique_ptr<PalmRenderer> palmRenderer_;
        std::unique_ptr<SharkRenderer> sharkRenderer_;
        std::unique_ptr<SkyRenderer> skyRenderer_;
        std::unique_ptr<Soundscape> soundscape_;
        std::unique_ptr<TerrainRenderer> terrainRenderer_;
        std::unique_ptr<TouchOverlay> touchOverlay_;
        std::unique_ptr<WorldDebugRenderer> worldDebugRenderer_;
        Microsoft::Xna::Framework::Matrix cameraView_;
        Microsoft::Xna::Framework::Matrix cameraProjection_;
        Microsoft::Xna::Framework::Vector3 cameraPosition_;
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
        bool muteToggleWasDown_ = false;
        bool soundscapeNeedsGesture_ = false;
        std::array<bool, 7> debugTeleportWasDown_{};
        std::size_t monkeyTeleportIndex_ = 0U;
        bool inputDiagnostics_ = false;
        bool touchOverlayVisible_ = false;
        float inputDiagnosticsTimer_ = 0.0f;
        std::uint32_t drawnFrames_ = 0;
    };
}
