#include "IslandSimulator/IslandGame.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsProfile.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace island
{
    namespace
    {
        constexpr float MouseSensitivity = 0.0021f;
        constexpr float TouchLookSensitivity = 0.0032f;
        constexpr std::uint32_t SmokeTestFrames = 4U;

        [[nodiscard]] Color BlendColor(const Color& from, const Color& to, const float amount)
        {
            const float t = std::clamp(amount, 0.0f, 1.0f);
            const auto channel = [t](const std::uint8_t a, const std::uint8_t b) {
                return static_cast<int>(std::lround(static_cast<float>(a)
                                                    + (static_cast<float>(b) - static_cast<float>(a)) * t));
            };
            return Color(channel(from.getRProperty(), to.getRProperty()),
                         channel(from.getGProperty(), to.getGProperty()),
                         channel(from.getBProperty(), to.getBProperty()), 255);
        }

        [[nodiscard]] Color TerrainColor(const TerrainSample& sample)
        {
            const float height = sample.height;
            Color base;
            if (height < -3.0f) {
                base = Color(38, 130, 145, 255);
            } else if (height < 0.0f) {
                base = BlendColor(Color(38, 130, 145, 255), Color(113, 192, 172, 255),
                                  (height + 3.0f) / 3.0f);
            } else if (height < 1.4f) {
                base = BlendColor(Color(186, 174, 118, 255), Color(226, 201, 139, 255),
                                  height / 1.4f);
            } else if (height < 2.2f) {
                base = Color(220, 196, 131, 255);
            } else if (height < 4.5f) {
                base = BlendColor(Color(204, 186, 123, 255), Color(86, 145, 72, 255),
                                  (height - 2.2f) / 2.3f);
            } else if (height < 9.0f) {
                base = Color(79, 139, 70, 255);
            } else {
                base = BlendColor(Color(79, 139, 70, 255), Color(45, 101, 53, 255),
                                  (height - 9.0f) / 14.0f);
            }

            const Vector3 normal(sample.normal.x, sample.normal.y, sample.normal.z);
            const Vector3 sun = Vector3::Normalize(Vector3(
                environment::SunDirectionX,
                environment::SunDirectionY,
                environment::SunDirectionZ));
            const float light = std::clamp(0.52f + 0.48f * Vector3::Dot(normal, sun), 0.34f, 1.0f);
            return Color(static_cast<std::uint8_t>(static_cast<float>(base.getRProperty()) * light),
                         static_cast<std::uint8_t>(static_cast<float>(base.getGProperty()) * light),
                         static_cast<std::uint8_t>(static_cast<float>(base.getBProperty()) * light),
                         static_cast<std::uint8_t>(255));
        }
    }

    IslandGame::IslandGame(const bool smokeTest, std::string capturePath,
                           FixedViewpoint viewpoint, const bool showDebugOverlay,
                           const bool inputDiagnostics)
        : graphics_(this),
          grassLayout_(terrain_),
          vegetationLayout_(terrain_),
          jungleLayout_(terrain_),
          naturalPropLayout_(terrain_, vegetationLayout_, jungleLayout_),
          cameraPosition_(0.0f, 3.0f, -225.0f),
          smokeTest_(smokeTest),
          viewpoint_(viewpoint),
          capturePath_(std::move(capturePath)),
          debugOverlayVisible_(showDebugOverlay),
          inputDiagnostics_(inputDiagnostics)
    {
        worldObstacles_.reserve(vegetationLayout_.Obstacles().size()
                                + jungleLayout_.Obstacles().size()
                                + naturalPropLayout_.Obstacles().size());
        worldObstacles_.insert(worldObstacles_.end(), vegetationLayout_.Obstacles().begin(),
                               vegetationLayout_.Obstacles().end());
        worldObstacles_.insert(worldObstacles_.end(), jungleLayout_.Obstacles().begin(),
                               jungleLayout_.Obstacles().end());
        worldObstacles_.insert(worldObstacles_.end(), naturalPropLayout_.Obstacles().begin(),
                               naturalPropLayout_.Obstacles().end());
        if (smokeTest_ && viewpoint_ == FixedViewpoint::None) {
            viewpoint_ = FixedViewpoint::Overview;
        }
        if (viewpoint_ == FixedViewpoint::Overview) {
            cameraPosition_ = Vector3(0.0f, 68.0f, -365.0f);
            pitch_ = -0.16f;
        } else if (viewpoint_ == FixedViewpoint::BeachPath) {
            constexpr float pathZ = -190.0f;
            const float pathX = VegetationLayout::PrimaryPathCenterX(pathZ);
            cameraPosition_ = Vector3(pathX, terrain_.Height(pathX, pathZ)
                                                 + PlayerMotion::EyeHeight,
                                      pathZ);
            pitch_ = -0.035f;
        } else if (viewpoint_ == FixedViewpoint::JungleTrail) {
            constexpr float pathZ = 45.0f;
            const float pathX = VegetationLayout::PrimaryPathCenterX(pathZ);
            cameraPosition_ = Vector3(pathX, terrain_.Height(pathX, pathZ)
                                                 + PlayerMotion::EyeHeight,
                                      pathZ);
            yaw_ = std::numbers::pi_v<float> * 0.5f;
            pitch_ = -0.065f;
        } else if (viewpoint_ == FixedViewpoint::PropDetail
                   && !naturalPropLayout_.FallenLogs().empty()) {
            const FallenLogPlacement& log = naturalPropLayout_.FallenLogs().front();
            const float offsetX = -std::sin(log.yawRadians) * 4.8f;
            const float offsetZ = std::cos(log.yawRadians) * 4.8f;
            const float cameraX = log.x + offsetX;
            const float cameraZ = log.z + offsetZ;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            yaw_ = std::atan2(log.x - cameraX, log.z - cameraZ);
            pitch_ = -0.14f;
        } else if (viewpoint_ == FixedViewpoint::DriftwoodDetail
                   && !naturalPropLayout_.Driftwood().empty()) {
            const DriftwoodPlacement& wood = naturalPropLayout_.Driftwood().front();
            const float perpendicularX = -std::sin(wood.yawRadians);
            const float perpendicularZ = std::cos(wood.yawRadians);
            const float firstX = wood.x + perpendicularX * 3.6f;
            const float firstZ = wood.z + perpendicularZ * 3.6f;
            const float secondX = wood.x - perpendicularX * 3.6f;
            const float secondZ = wood.z - perpendicularZ * 3.6f;
            const bool firstIsLandward = terrain_.SignedShoreDistance(firstX, firstZ)
                >= terrain_.SignedShoreDistance(secondX, secondZ);
            const float cameraX = firstIsLandward ? firstX : secondX;
            const float cameraZ = firstIsLandward ? firstZ : secondZ;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            yaw_ = std::atan2(wood.x - cameraX, wood.z - cameraZ);
            pitch_ = -0.16f;
        }
        graphics_.setGraphicsProfileProperty(GraphicsProfile::HiDef);
        graphics_.setPreferredBackBufferWidthProperty(1280);
        graphics_.setPreferredBackBufferHeightProperty(720);
#if defined(__ANDROID__)
        graphics_.setIsFullScreenProperty(true);
        touchOverlayVisible_ = true;
#endif
        getWindowProperty().setTitleProperty("island-simulator");
        setIsMouseVisibleProperty(true);
        setIsFixedTimeStepProperty(true);
#if defined(__EMSCRIPTEN__)
        soundscapeNeedsGesture_ = true;
#endif
    }

    void IslandGame::LoadContent()
    {
        auto& device = getGraphicsDeviceProperty();
        terrainEffect_ = std::make_unique<BasicEffect>(device);
        terrainEffect_->setVertexColorEnabledProperty(true);
        terrainEffect_->setLightingEnabledProperty(false);
        terrainEffect_->setFogEnabledProperty(true);
        terrainEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        terrainEffect_->setFogStartProperty(260.0f);
        terrainEffect_->setFogEndProperty(720.0f);

        CreateTerrainMesh();
        grassRenderer_ = std::make_unique<GrassRenderer>(device, terrain_, grassLayout_);
        jungleRenderer_ = std::make_unique<JungleRenderer>(device, jungleLayout_);
        naturalPropRenderer_ = std::make_unique<NaturalPropRenderer>(device, naturalPropLayout_);
        palmRenderer_ = std::make_unique<PalmRenderer>(device, vegetationLayout_);
        oceanRenderer_ = std::make_unique<OceanRenderer>(device, terrain_);
        skyRenderer_ = std::make_unique<SkyRenderer>(device);
        debugOverlay_ = std::make_unique<DebugOverlay>(device);
        touchOverlay_ = std::make_unique<TouchOverlay>(device);
        playerMotion_.SetPosition(terrain_, cameraPosition_.X, cameraPosition_.Z);
        if (!smokeTest_ && viewpoint_ == FixedViewpoint::None) {
            const PlayerPosition& position = playerMotion_.Position();
            cameraPosition_ = Vector3(position.x, position.y, position.z);
        }
        soundscape_ = std::make_unique<Soundscape>();
        if (!smokeTest_ && !soundscapeNeedsGesture_) {
            TryStartSoundscape();
        }

        std::cout << "island-simulator: renderer " << device.GetGraphicsRendererName() << '\n'
                  << "island-simulator: terrain " << terrainVertexCount_ << " vertices, "
                  << terrainPrimitiveCount_ << " triangles in " << terrainCells_.size()
                  << " cells\n";
        if (debugOverlayVisible_) {
            std::cout << "island-simulator: debug overlay active\n";
        }
        std::cout.flush();
    }

    void IslandGame::CreateTerrainMesh()
    {
        auto& device = getGraphicsDeviceProperty();
        terrainCells_.clear();
        terrainCells_.reserve(TerrainMeshLayout::CellCount);
        terrainVertexCount_ = 0;
        terrainPrimitiveCount_ = 0;

        for (std::size_t cellIndex = 0; cellIndex < TerrainMeshLayout::CellCount; ++cellIndex) {
            const TerrainCellLayout layout = TerrainMeshLayout::Cell(cellIndex);
            std::vector<VertexPositionColor> vertices;
            std::vector<std::uint16_t> indices;
            vertices.reserve(static_cast<std::size_t>(TerrainMeshLayout::VerticesPerCellAxis
                                                       * TerrainMeshLayout::VerticesPerCellAxis));
            indices.reserve(static_cast<std::size_t>(layout.quadColumns * layout.quadRows * 6));
            float minimumHeight = std::numeric_limits<float>::infinity();
            float maximumHeight = -std::numeric_limits<float>::infinity();

            for (int localRow = 0; localRow <= layout.quadRows; ++localRow) {
                const float z = TerrainMeshLayout::Coordinate(layout.firstGridRow + localRow);
                for (int localColumn = 0; localColumn <= layout.quadColumns; ++localColumn) {
                    const float x = TerrainMeshLayout::Coordinate(
                        layout.firstGridColumn + localColumn);
                    const TerrainSample sample = terrain_.Sample(x, z);
                    vertices.emplace_back(Vector3(x, sample.height, z), TerrainColor(sample));
                    minimumHeight = std::min(minimumHeight, sample.height);
                    maximumHeight = std::max(maximumHeight, sample.height);
                }
            }

            for (int row = 0; row < layout.quadRows; ++row) {
                for (int column = 0; column < layout.quadColumns; ++column) {
                    const auto topLeft = static_cast<std::uint16_t>(
                        row * TerrainMeshLayout::VerticesPerCellAxis + column);
                    const auto topRight = static_cast<std::uint16_t>(topLeft + 1U);
                    const auto bottomLeft = static_cast<std::uint16_t>(
                        topLeft + TerrainMeshLayout::VerticesPerCellAxis);
                    const auto bottomRight = static_cast<std::uint16_t>(bottomLeft + 1U);
                    indices.insert(indices.end(), {topLeft, bottomLeft, topRight,
                                                   topRight, bottomLeft, bottomRight});
                }
            }

            TerrainRenderCell cell;
            cell.vertexCount = static_cast<int>(vertices.size());
            cell.primitiveCount = static_cast<int>(indices.size() / 3U);
            cell.bounds = BoundingBox(Vector3(layout.minimumX, minimumHeight - 0.1f, layout.minimumZ),
                                      Vector3(layout.maximumX, maximumHeight + 0.1f, layout.maximumZ));
            cell.vertices = std::make_unique<VertexBuffer>(
                device, VertexPositionColor::getVertexDeclarationStatic(), cell.vertexCount,
                BufferUsage::WriteOnly);
            cell.vertices->SetData(vertices.data(), cell.vertexCount);
            cell.indices = std::make_unique<IndexBuffer>(
                device, IndexElementSize::SixteenBits, static_cast<int>(indices.size()),
                BufferUsage::WriteOnly);
            cell.indices->SetData(indices.data(), static_cast<int>(indices.size()));
            terrainVertexCount_ += cell.vertexCount;
            terrainPrimitiveCount_ += cell.primitiveCount;
            terrainCells_.push_back(std::move(cell));
        }
    }

    void IslandGame::TryStartSoundscape() noexcept
    {
        if (!soundscape_ || soundscape_->WasStartAttempted() || smokeTest_) {
            return;
        }
        const PlayerPosition& position = playerMotion_.Position();
        const float shoreDistance = terrain_.SignedShoreDistance(position.x, position.z);
        const bool swimming = playerMotion_.Mode() == LocomotionMode::Swimming;
        (void)soundscape_->Start(shoreDistance, swimming);
        soundscapeNeedsGesture_ = false;
    }

    void IslandGame::Update(GameTime& gameTime)
    {
        const float deltaSeconds = std::min(
            0.1f, static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty())
                / 1000.0f);
        if (!smokeTest_ && viewpoint_ == FixedViewpoint::None) {
            UpdateCamera(deltaSeconds);
        }
        if (inputDiagnostics_) {
            inputDiagnosticsTimer_ += deltaSeconds;
            if (inputDiagnosticsTimer_ >= 0.25f) {
                inputDiagnosticsTimer_ = 0.0f;
                const PlayerPosition& position = playerMotion_.Position();
                const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
                std::cout << "island-simulator: input sample captured "
                          << (mouseCaptured_ ? 1 : 0) << " relative "
                          << (Mouse::getIsRelativeMouseModeEXTProperty() ? 1 : 0)
                          << " x " << position.x << " z " << position.z
                          << " yaw " << yaw_ << " pitch " << pitch_
                          << " viewport " << viewport.getWidthProperty() << ' '
                          << viewport.getHeightProperty() << '\n';
            }
        }
    }

    void IslandGame::UpdateCamera(const float deltaSeconds)
    {
        const KeyboardState keyboard = Keyboard::GetState();
        const MouseState mouse = Mouse::GetState();
        const auto touches = Touch::TouchPanel::GetState();
        std::vector<TouchContact> touchContacts;
        touchContacts.reserve(static_cast<std::size_t>(touches.getCountProperty()));
        for (const Touch::TouchLocation& touch : touches) {
            TouchPhase phase = TouchPhase::Moved;
            switch (touch.getStateProperty()) {
            case Touch::TouchLocationState::Pressed:
                phase = TouchPhase::Pressed;
                break;
            case Touch::TouchLocationState::Released:
            case Touch::TouchLocationState::Invalid:
                phase = TouchPhase::Released;
                break;
            case Touch::TouchLocationState::Moved:
                phase = TouchPhase::Moved;
                break;
            }
            const Vector2& position = touch.getPositionProperty();
            touchContacts.push_back({touch.getIdProperty(), position.X, position.Y, phase});
        }

        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const float touchWidth = Touch::TouchPanel::getDisplayWidthProperty() > 0
            ? static_cast<float>(Touch::TouchPanel::getDisplayWidthProperty())
            : static_cast<float>(viewport.getWidthProperty());
        const float touchHeight = Touch::TouchPanel::getDisplayHeightProperty() > 0
            ? static_cast<float>(Touch::TouchPanel::getDisplayHeightProperty())
            : static_cast<float>(viewport.getHeightProperty());
        touchDisplayWidth_ = touchWidth;
        touchDisplayHeight_ = touchHeight;
        touchControlState_ = touchControls_.Update(touchContacts, touchWidth, touchHeight);
        if (!touchContacts.empty()) {
            touchOverlayVisible_ = true;
        }
        const bool touchInteracting = touchControlState_.movementActive
            || touchControlState_.lookActive;

        const bool movementKeyDown = keyboard.IsKeyDown(Keys::W) || keyboard.IsKeyDown(Keys::A)
            || keyboard.IsKeyDown(Keys::S) || keyboard.IsKeyDown(Keys::D);
        if (soundscapeNeedsGesture_
            && (mouse.getLeftButtonProperty() == ButtonState::Pressed
                || !touchContacts.empty() || movementKeyDown)) {
            TryStartSoundscape();
        }

        const bool muteToggleDown = keyboard.IsKeyDown(Keys::M);
        if (muteToggleDown && !muteToggleWasDown_ && soundscape_) {
            soundscape_->ToggleMuted();
        }
        muteToggleWasDown_ = muteToggleDown;

        if (touchInteracting && mouseCaptured_) {
            mouseCaptured_ = false;
            Mouse::setIsRelativeMouseModeEXTProperty(false);
            setIsMouseVisibleProperty(true);
        }

        if (mouseCaptured_ && !Mouse::getIsRelativeMouseModeEXTProperty()) {
            mouseCaptured_ = false;
            setIsMouseVisibleProperty(true);
        }

        const bool debugOverlayToggleDown = keyboard.IsKeyDown(Keys::F1);
        if (debugOverlayToggleDown && !debugOverlayToggleWasDown_) {
            debugOverlayVisible_ = !debugOverlayVisible_;
            std::cout << "island-simulator: debug overlay "
                      << (debugOverlayVisible_ ? "enabled" : "disabled") << '\n';
        }
        debugOverlayToggleWasDown_ = debugOverlayToggleDown;

        constexpr std::array<Keys, 4> teleportKeys = {
            Keys::F5, Keys::F6, Keys::F7, Keys::F8};
        constexpr std::array<const char*, 4> teleportNames = {
            "south beach", "jungle trail", "east clearing", "west clearing"};
        for (std::size_t index = 0; index < teleportKeys.size(); ++index) {
            const bool down = keyboard.IsKeyDown(teleportKeys[index]);
            if (debugOverlayVisible_ && down && !debugTeleportWasDown_[index]) {
                float x = 0.0f;
                float z = -225.0f;
                yaw_ = 0.0f;
                pitch_ = -0.03f;
                if (index == 1U) {
                    z = 45.0f;
                    x = VegetationLayout::PrimaryPathCenterX(z);
                    yaw_ = std::numbers::pi_v<float> * 0.5f;
                    pitch_ = -0.065f;
                } else if (index == 2U) {
                    x = 43.0f;
                    z = -18.0f;
                    yaw_ = -std::numbers::pi_v<float> * 0.5f;
                } else if (index == 3U) {
                    x = -82.0f;
                    z = 62.0f;
                    yaw_ = std::numbers::pi_v<float> * 0.25f;
                }
                playerMotion_.SetPosition(terrain_, x, z);
                const PlayerPosition& teleported = playerMotion_.Position();
                cameraPosition_ = Vector3(teleported.x, teleported.y, teleported.z);
                std::cout << "island-simulator: debug teleport "
                          << teleportNames[index] << " x " << x << " z " << z << '\n';
            }
            debugTeleportWasDown_[index] = down;
        }

        if (keyboard.IsKeyDown(Keys::Escape)) {
            if (mouseCaptured_) {
                mouseCaptured_ = false;
                Mouse::setIsRelativeMouseModeEXTProperty(false);
                setIsMouseVisibleProperty(true);
            } else {
                Exit();
            }
            return;
        }

        if (!touchInteracting && !smokeTest_ && !mouseCaptured_
            && mouse.getLeftButtonProperty() == ButtonState::Pressed) {
            mouseCaptured_ = true;
            Mouse::setIsRelativeMouseModeEXTProperty(true);
            setIsMouseVisibleProperty(false);
        }
        if (mouseCaptured_) {
            yaw_ -= static_cast<float>(mouse.getXProperty()) * MouseSensitivity;
            pitch_ = std::clamp(pitch_ - static_cast<float>(mouse.getYProperty()) * MouseSensitivity,
                                -1.48f, 1.48f);
        }
        if (touchControlState_.lookActive) {
            const float resolutionScale = 720.0f / std::max(1.0f, touchHeight);
            yaw_ -= touchControlState_.lookDeltaX * TouchLookSensitivity * resolutionScale;
            pitch_ = std::clamp(
                pitch_ - touchControlState_.lookDeltaY * TouchLookSensitivity * resolutionScale,
                -1.48f, 1.48f);
        }

        const Vector3 forward(std::sin(yaw_), 0.0f, std::cos(yaw_));
        const Vector3 right(forward.Z, 0.0f, -forward.X);
        Vector3 wish = Vector3::Zero;
        if (keyboard.IsKeyDown(Keys::W)) wish = wish + forward;
        if (keyboard.IsKeyDown(Keys::S)) wish = wish - forward;
        if (keyboard.IsKeyDown(Keys::D)) wish = wish + right;
        if (keyboard.IsKeyDown(Keys::A)) wish = wish - right;
        wish = wish + forward * touchControlState_.moveForward
            + right * touchControlState_.moveRight;

        playerMotion_.Update(terrain_, wish.X, wish.Z, deltaSeconds, worldObstacles_);
        const PlayerPosition& position = playerMotion_.Position();
        cameraPosition_ = Vector3(position.x, position.y, position.z);
        if (soundscape_) {
            soundscape_->Update(terrain_.SignedShoreDistance(position.x, position.z),
                                playerMotion_.Mode() == LocomotionMode::Swimming, deltaSeconds);
        }
    }

    void IslandGame::ApplyCamera(BasicEffect& effect)
    {
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const float aspect = static_cast<float>(viewport.getWidthProperty())
            / static_cast<float>(std::max(1, viewport.getHeightProperty()));
        const float cosPitch = std::cos(pitch_);
        const Vector3 forward(std::sin(yaw_) * cosPitch, std::sin(pitch_),
                              std::cos(yaw_) * cosPitch);
        effect.setWorldProperty(Matrix::getIdentityProperty());
        effect.setViewProperty(Matrix::CreateLookAt(cameraPosition_, cameraPosition_ + forward,
                                                     Vector3::Up));
        effect.setProjectionProperty(Matrix::CreatePerspectiveFieldOfView(
            std::numbers::pi_v<float> / 3.0f, aspect, 0.08f, 1200.0f));
    }

    void IslandGame::DrawMesh(BasicEffect& effect, VertexBuffer& vertices, IndexBuffer& indices,
                              const int vertexCount, const int primitiveCount)
    {
        auto& device = getGraphicsDeviceProperty();
        device.SetVertexBuffer(&vertices);
        device.setIndicesProperty(&indices);
        for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount, 0,
                                         primitiveCount);
        }
    }

    void IslandGame::CaptureFrame(const std::string& path)
    {
        auto& device = getGraphicsDeviceProperty();
        const auto& viewport = device.getViewportProperty();
        const int width = viewport.getWidthProperty();
        const int height = viewport.getHeightProperty();
        std::vector<Color> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        device.GetBackBufferData(pixels.data(), static_cast<int>(pixels.size()));

        std::ofstream output(path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Cannot create frame capture at '" + path + "'");
        }
        output << "P6\n" << width << ' ' << height << "\n255\n";
        for (const Color& pixel : pixels) {
            const std::array<char, 3> rgb = {
                static_cast<char>(pixel.getRProperty()),
                static_cast<char>(pixel.getGProperty()),
                static_cast<char>(pixel.getBProperty())};
            output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        }
        if (!output) {
            throw std::runtime_error("Failed while writing frame capture at '" + path + "'");
        }
        std::cout << "island-simulator: captured frame to " << path << '\n';
    }

    void IslandGame::Draw(const GameTime& gameTime)
    {
        auto& device = getGraphicsDeviceProperty();
        debugOverlay_->UpdateFrameTiming();
        device.Clear(Color(136, 202, 224, 255), 1.0f);
        ApplyCamera(*terrainEffect_);
        skyRenderer_->Draw(terrainEffect_->getViewProperty(),
                           terrainEffect_->getProjectionProperty(), cameraPosition_);
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        const BoundingFrustum terrainFrustum(terrainEffect_->getViewProperty()
                                             * terrainEffect_->getProjectionProperty());
        visibleTerrainCellCount_ = 0;
        for (TerrainRenderCell& cell : terrainCells_) {
            if (!terrainFrustum.Intersects(cell.bounds)) {
                continue;
            }
            DrawMesh(*terrainEffect_, *cell.vertices, *cell.indices, cell.vertexCount,
                     cell.primitiveCount);
            ++visibleTerrainCellCount_;
        }

        const float timeSeconds = static_cast<float>(gameTime.getTotalGameTimeProperty()
                                                         .getTotalSecondsProperty());
        naturalPropRenderer_->Draw(terrainEffect_->getViewProperty(),
                                   terrainEffect_->getProjectionProperty(), cameraPosition_);

        jungleRenderer_->Draw(terrainEffect_->getViewProperty(),
                              terrainEffect_->getProjectionProperty(), cameraPosition_,
                              timeSeconds);

        grassRenderer_->Draw(terrainEffect_->getViewProperty(),
                             terrainEffect_->getProjectionProperty(), cameraPosition_,
                             timeSeconds);

        palmRenderer_->Draw(terrainEffect_->getViewProperty(),
                            terrainEffect_->getProjectionProperty(), cameraPosition_,
                            timeSeconds);

        oceanRenderer_->Draw(terrainEffect_->getViewProperty(),
                             terrainEffect_->getProjectionProperty(), cameraPosition_,
                             timeSeconds);

        if (touchOverlayVisible_) {
            const auto& viewport = device.getViewportProperty();
            touchOverlay_->Draw(touchControlState_, touchDisplayWidth_, touchDisplayHeight_,
                                viewport.getWidthProperty(), viewport.getHeightProperty(), true);
        }

        if (debugOverlayVisible_) {
            const PlayerPosition& player = playerMotion_.Position();
            DebugOverlayState overlayState;
            overlayState.playerPosition = Vector3(player.x, player.y, player.z);
            overlayState.renderer = device.GetGraphicsRendererName();
            overlayState.swimming = playerMotion_.Mode() == LocomotionMode::Swimming;
            overlayState.groundHeight = playerMotion_.GroundHeight();
            overlayState.groundSlopeDegrees = playerMotion_.GroundSlopeRadians()
                * 180.0f / std::numbers::pi_v<float>;
            overlayState.offshoreDistance = playerMotion_.OffshoreDistance(terrain_);
            overlayState.oceanCurrentStrength = playerMotion_.OceanCurrentStrength(terrain_);
            overlayState.visibleTerrainCells = visibleTerrainCellCount_;
            overlayState.totalTerrainCells = static_cast<int>(terrainCells_.size());
            overlayState.visiblePalms = palmRenderer_->VisiblePalmCount();
            overlayState.totalPalms = palmRenderer_->TotalPalmCount();
            overlayState.nearPalms = palmRenderer_->VisibleNearPalmCount();
            overlayState.farPalms = palmRenderer_->VisibleFarPalmCount();
            overlayState.palmCells = palmRenderer_->VisibleCellCount();
            overlayState.palmDraws = palmRenderer_->DrawCallCount();
            overlayState.visibleCanopy = jungleRenderer_->VisibleCanopyCount();
            overlayState.totalCanopy = jungleRenderer_->TotalCanopyCount();
            overlayState.visibleUnderstory = jungleRenderer_->VisibleUnderstoryCount();
            overlayState.totalUnderstory = jungleRenderer_->TotalUnderstoryCount();
            overlayState.visibleBushes = jungleRenderer_->VisibleBushCount();
            overlayState.totalBushes = jungleRenderer_->TotalBushCount();
            overlayState.visibleGroundPlants = jungleRenderer_->VisibleGroundPlantCount();
            overlayState.totalGroundPlants = jungleRenderer_->TotalGroundPlantCount();
            overlayState.jungleCells = jungleRenderer_->VisibleCellCount();
            overlayState.jungleDraws = jungleRenderer_->DrawCallCount();
            overlayState.visibleRocks = naturalPropRenderer_->VisibleRockCount();
            overlayState.totalRocks = naturalPropRenderer_->TotalRockCount();
            overlayState.visibleLogs = naturalPropRenderer_->VisibleLogCount();
            overlayState.totalLogs = naturalPropRenderer_->TotalLogCount();
            overlayState.visibleDriftwood = naturalPropRenderer_->VisibleDriftwoodCount();
            overlayState.totalDriftwood = naturalPropRenderer_->TotalDriftwoodCount();
            overlayState.propCells = naturalPropRenderer_->VisibleCellCount();
            overlayState.propDraws = naturalPropRenderer_->DrawCallCount();
            overlayState.visibleGrassPatches = grassRenderer_->VisiblePatchCount();
            overlayState.totalGrassPatches = grassRenderer_->TotalPatchCount();
            overlayState.grassCells = grassRenderer_->VisibleCellCount();
            overlayState.grassDraws = grassRenderer_->DrawCallCount();
            overlayState.audioAvailable = soundscape_ && soundscape_->IsAvailable();
            overlayState.audioMuted = soundscape_ && soundscape_->IsMuted();
            if (soundscape_) {
                overlayState.surfVolume = soundscape_->Mix().surfVolume;
                overlayState.breezeVolume = soundscape_->Mix().breezeVolume;
                overlayState.playedWaveAccents = soundscape_->PlayedWaveAccentCount();
            }
            debugOverlay_->Draw(overlayState);
        }

        if (drawnFrames_ == 0U) {
            std::cout << "island-simulator: visible terrain cells " << visibleTerrainCellCount_
                      << '/' << terrainCells_.size() << ", palms "
                      << palmRenderer_->VisiblePalmCount() << '/'
                      << palmRenderer_->TotalPalmCount() << " (near "
                      << palmRenderer_->VisibleNearPalmCount() << ", far "
                      << palmRenderer_->VisibleFarPalmCount() << ") in "
                      << palmRenderer_->VisibleCellCount() << " cells, "
                      << palmRenderer_->DrawCallCount() << " draws; jungle "
                      << jungleRenderer_->VisibleCanopyCount() << '/'
                      << jungleRenderer_->TotalCanopyCount() << " canopy, "
                      << jungleRenderer_->VisibleUnderstoryCount() << '/'
                      << jungleRenderer_->TotalUnderstoryCount() << " understory, "
                      << jungleRenderer_->VisibleBushCount() << '/'
                      << jungleRenderer_->TotalBushCount() << " bushes, "
                      << jungleRenderer_->VisibleGroundPlantCount() << '/'
                      << jungleRenderer_->TotalGroundPlantCount() << " ground plants, "
                      << jungleRenderer_->DrawCallCount() << " draws; props "
                      << naturalPropRenderer_->VisibleRockCount() << '/'
                      << naturalPropRenderer_->TotalRockCount() << " rocks, "
                      << naturalPropRenderer_->VisibleLogCount() << '/'
                      << naturalPropRenderer_->TotalLogCount() << " logs, "
                      << naturalPropRenderer_->VisibleDriftwoodCount() << '/'
                      << naturalPropRenderer_->TotalDriftwoodCount() << " driftwood, "
                      << naturalPropRenderer_->DrawCallCount() << " draws; grass "
                      << grassRenderer_->VisiblePatchCount() << '/'
                      << grassRenderer_->TotalPatchCount() << " patches in "
                      << grassRenderer_->VisibleCellCount() << " cells, "
                      << grassRenderer_->DrawCallCount() << " draws\n";
        }
        ++drawnFrames_;
        if (!capturePath_.empty() && drawnFrames_ == SmokeTestFrames) {
            CaptureFrame(capturePath_);
        }
        if (smokeTest_ && drawnFrames_ >= SmokeTestFrames) {
            std::cout << "island-simulator: smoke test rendered " << drawnFrames_ << " frames\n";
            Exit();
        }
    }

    GetTypeNameCPP(IslandGame, "IslandSimulator.IslandGame")
}
