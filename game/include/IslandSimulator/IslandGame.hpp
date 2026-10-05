#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "IslandSimulator/DebugOverlay.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/JungleRenderer.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GrassRenderer.hpp"
#include "IslandSimulator/OceanRenderer.hpp"
#include "IslandSimulator/PalmRenderer.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/TouchOverlay.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"

namespace island
{
    /** @brief Deterministic camera positions used for visual regression review. */
    enum class FixedViewpoint
    {
        None,
        Overview,
        BeachPath,
        JungleTrail
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
                            bool inputDiagnostics = false);

        GetTypeNameHPP()

    protected:
        /** @brief Creates GPU resources after CNA initializes the graphics device. */
        void LoadContent() override;

        /** @brief Updates input and the early first-person camera. */
        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;

        /** @brief Draws terrain, vegetation, and ocean in stable pass order. */
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

    private:
        struct TerrainRenderCell
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices;
            Microsoft::Xna::Framework::BoundingBox bounds;
            int vertexCount = 0;
            int primitiveCount = 0;
        };

        void CreateTerrainMesh();
        void CaptureFrame(const std::string& path);
        void UpdateCamera(float deltaSeconds);
        void ApplyCamera(Microsoft::Xna::Framework::Graphics::BasicEffect& effect);
        void DrawMesh(Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                      Microsoft::Xna::Framework::Graphics::VertexBuffer& vertices,
                      Microsoft::Xna::Framework::Graphics::IndexBuffer& indices,
                      int vertexCount, int primitiveCount);

        Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
        IslandTerrain terrain_;
        GrassLayout grassLayout_;
        VegetationLayout vegetationLayout_;
        JungleLayout jungleLayout_;
        std::vector<CircularObstacle> worldObstacles_;
        PlayerMotion playerMotion_;
        TouchControls touchControls_;
        TouchControlState touchControlState_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> terrainEffect_;
        std::unique_ptr<DebugOverlay> debugOverlay_;
        std::unique_ptr<GrassRenderer> grassRenderer_;
        std::unique_ptr<JungleRenderer> jungleRenderer_;
        std::unique_ptr<OceanRenderer> oceanRenderer_;
        std::unique_ptr<PalmRenderer> palmRenderer_;
        std::unique_ptr<TouchOverlay> touchOverlay_;
        std::vector<TerrainRenderCell> terrainCells_;
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
        std::array<bool, 4> debugTeleportWasDown_{};
        bool inputDiagnostics_ = false;
        bool touchOverlayVisible_ = false;
        float inputDiagnosticsTimer_ = 0.0f;
        std::uint32_t drawnFrames_ = 0;
        int visibleTerrainCellCount_ = 0;
        int terrainVertexCount_ = 0;
        int terrainPrimitiveCount_ = 0;
    };
}
