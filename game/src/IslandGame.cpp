#include "IslandSimulator/IslandGame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsProfile.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
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
        constexpr std::uint32_t MonkeyCaptureFrames = 24U;

    }

    IslandGame::IslandGame(const bool smokeTest, std::string capturePath,
                           FixedViewpoint viewpoint, const bool showDebugOverlay,
                           const bool inputDiagnostics, const bool showWorldDebug)
        : graphics_(this),
          birdFlock_(terrain_),
          fishSchools_(terrain_),
          sharkPatrol_(terrain_),
          monkeyTroop_(terrain_),
          grassLayout_(terrain_),
          vegetationLayout_(terrain_),
          jungleLayout_(terrain_),
          naturalPropLayout_(terrain_, vegetationLayout_, jungleLayout_),
          cameraPosition_(0.0f, 3.0f, -225.0f),
          smokeTest_(smokeTest),
          viewpoint_(viewpoint),
          capturePath_(std::move(capturePath)),
          debugOverlayVisible_(showDebugOverlay),
          worldDebugVisible_(showWorldDebug),
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
        monkeyTroop_.SetObstacles(worldObstacles_);
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
        } else if (viewpoint_ == FixedViewpoint::BirdDetail
                   && birdFlock_.Birds().size() > 10U) {
            const BirdState& bird = birdFlock_.Birds()[10U];
            const Vector3 flightForward(std::cos(bird.headingRadians), 0.0f,
                                        std::sin(bird.headingRadians));
            const Vector3 flightRight(-flightForward.Z, 0.0f, flightForward.X);
            cameraPosition_ = Vector3(bird.x, bird.y, bird.z)
                - flightForward * 17.0f + flightRight * 4.5f - Vector3::Up * 3.5f;
            const Vector3 toBird = Vector3(bird.x, bird.y, bird.z) - cameraPosition_;
            yaw_ = std::atan2(toBird.X, toBird.Z);
            pitch_ = std::atan2(toBird.Y, std::hypot(toBird.X, toBird.Z));
        } else if (viewpoint_ == FixedViewpoint::FishDetail
                   && !fishSchools_.Fish().empty()) {
            const FishState& fish = fishSchools_.Fish().front();
            const Vector3 radial = Vector3::Normalize(Vector3(fish.x, 0.0f, fish.z));
            const Vector3 tangent(-radial.Z, 0.0f, radial.X);
            cameraPosition_ = Vector3(fish.x, fish.y, fish.z)
                + radial * 8.5f + tangent * 1.5f + Vector3::Up * 3.4f;
            const Vector3 toFish = Vector3(fish.x, fish.y, fish.z) - cameraPosition_;
            yaw_ = std::atan2(toFish.X, toFish.Z);
            pitch_ = std::atan2(toFish.Y, std::hypot(toFish.X, toFish.Z));
        } else if (viewpoint_ == FixedViewpoint::SharkDetail) {
            const SharkState& shark = sharkPatrol_.State();
            const Vector3 radial = Vector3::Normalize(Vector3(shark.x, 0.0f, shark.z));
            const Vector3 tangent(-radial.Z, 0.0f, radial.X);
            cameraPosition_ = Vector3(shark.x, shark.y, shark.z)
                + radial * 14.0f + tangent * 3.5f + Vector3::Up * 5.8f;
            const Vector3 toShark = Vector3(shark.x, shark.y, shark.z) - cameraPosition_;
            yaw_ = std::atan2(toShark.X, toShark.Z);
            pitch_ = std::atan2(toShark.Y, std::hypot(toShark.X, toShark.Z));
        } else if (viewpoint_ == FixedViewpoint::MonkeyDetail) {
            const MonkeyState& monkey = monkeyTroop_.Monkeys().front();
            const Vector3 forward(std::cos(monkey.headingRadians), 0.0f,
                                  std::sin(monkey.headingRadians));
            const Vector3 side(-forward.Z, 0.0f, forward.X);
            cameraPosition_ = Vector3(monkey.x, monkey.y, monkey.z)
                + forward * 4.2f + side * 1.0f + Vector3::Up * 1.40f;
            const Vector3 toMonkey = Vector3(monkey.x, monkey.y + 0.72f, monkey.z)
                - cameraPosition_;
            yaw_ = std::atan2(toMonkey.X, toMonkey.Z);
            pitch_ = std::atan2(toMonkey.Y, std::hypot(toMonkey.X, toMonkey.Z));
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
        terrainRenderer_ = std::make_unique<TerrainRenderer>(device, terrain_);
        birdRenderer_ = std::make_unique<BirdRenderer>(device);
        fishRenderer_ = std::make_unique<FishRenderer>(device);
        sharkRenderer_ = std::make_unique<SharkRenderer>(device);
        monkeyRenderer_ = std::make_unique<MonkeyRenderer>(device);
        grassRenderer_ = std::make_unique<GrassRenderer>(device, terrain_, grassLayout_);
        jungleRenderer_ = std::make_unique<JungleRenderer>(device, jungleLayout_);
        naturalPropRenderer_ = std::make_unique<NaturalPropRenderer>(device, naturalPropLayout_);
        palmRenderer_ = std::make_unique<PalmRenderer>(device, vegetationLayout_);
        oceanRenderer_ = std::make_unique<OceanRenderer>(device, terrain_);
        skyRenderer_ = std::make_unique<SkyRenderer>(device);
        debugOverlay_ = std::make_unique<DebugOverlay>(device);
        touchOverlay_ = std::make_unique<TouchOverlay>(device);
        worldDebugRenderer_ = std::make_unique<WorldDebugRenderer>(device, terrain_);
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
                  << "island-simulator: terrain " << terrainRenderer_->VertexCount()
                  << " vertices, " << terrainRenderer_->PrimitiveCount() << " triangles in "
                  << terrainRenderer_->TotalCellCount()
                  << " cells\n"
                  << "island-simulator: birds " << birdFlock_.Birds().size()
                  << " agents in a distance-throttled one-batch simulation\n"
                  << "island-simulator: fish " << fishSchools_.Fish().size()
                  << " agents in " << FishSchools::SchoolCount
                  << " distance-throttled shallow-water schools\n"
                  << "island-simulator: one occasional non-hostile offshore shark patrol\n"
                  << "island-simulator: monkeys " << monkeyTroop_.Monkeys().size()
                  << " agents with portable GPU skeletal animation, "
                  << MonkeyRenderer::SilhouetteCount << " silhouettes, 4 palettes\n";
        if (debugOverlayVisible_) {
            std::cout << "island-simulator: debug overlay active\n";
        }
        if (worldDebugVisible_) {
            std::cout << "island-simulator: world debug visualization active\n";
        }
        std::cout.flush();
    }

    void IslandGame::TryStartSoundscape() noexcept
    {
        if (!soundscape_ || soundscape_->WasStartAttempted() || smokeTest_) {
            return;
        }
        const PlayerPosition& position = playerMotion_.Position();
        const float shoreDistance = terrain_.SignedShoreDistance(position.x, position.z);
        const bool swimming = playerMotion_.Mode() == LocomotionMode::Swimming;
        const float waterDepth = std::max(0.0f, -playerMotion_.GroundHeight());
        const float horizontalSpeed = std::hypot(playerMotion_.VelocityX(),
                                                 playerMotion_.VelocityZ());
        (void)soundscape_->Start(shoreDistance, swimming, waterDepth, horizontalSpeed);
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
        birdFlock_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        fishSchools_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        sharkPatrol_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        monkeyTroop_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
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

        const bool worldDebugToggleDown = keyboard.IsKeyDown(Keys::F2);
        if (worldDebugToggleDown && !worldDebugToggleWasDown_) {
            worldDebugVisible_ = !worldDebugVisible_;
            std::cout << "island-simulator: world debug visualization "
                      << (worldDebugVisible_ ? "enabled" : "disabled") << '\n';
        }
        worldDebugToggleWasDown_ = worldDebugToggleDown;

        constexpr std::array<Keys, 7> teleportKeys = {
            Keys::F5, Keys::F6, Keys::F7, Keys::F8, Keys::F9, Keys::F10, Keys::F11};
        constexpr std::array<const char*, 7> teleportNames = {
            "south beach", "jungle trail", "east clearing", "west clearing",
            "fish shallows", "offshore shark", "monkey troop"};
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
                } else if (index == 4U && !fishSchools_.Fish().empty()) {
                    const FishState& fish = fishSchools_.Fish().front();
                    const float angle = std::atan2(fish.z, fish.x);
                    const float radius = terrain_.ShoreRadius(angle) - 2.5f;
                    x = std::cos(angle) * radius;
                    z = std::sin(angle) * radius;
                    yaw_ = std::atan2(fish.x - x, fish.z - z);
                    pitch_ = -0.16f;
                } else if (index == 5U) {
                    const SharkState& shark = sharkPatrol_.State();
                    const float angle = std::atan2(shark.z, shark.x);
                    const float radius = terrain_.ShoreRadius(angle) - 2.5f;
                    x = std::cos(angle) * radius;
                    z = std::sin(angle) * radius;
                    yaw_ = std::atan2(shark.x - x, shark.z - z);
                    pitch_ = -0.095f;
                } else if (index == 6U) {
                    const std::span<const MonkeyState> monkeys = monkeyTroop_.Monkeys();
                    const MonkeyState& monkey = monkeys[monkeyTeleportIndex_ % monkeys.size()];
                    monkeyTeleportIndex_ = (monkeyTeleportIndex_ + 1U) % monkeys.size();
                    x = monkey.x + std::cos(monkey.headingRadians) * 5.0f;
                    z = monkey.z + std::sin(monkey.headingRadians) * 5.0f;
                    yaw_ = std::atan2(monkey.x - x, monkey.z - z);
                    pitch_ = -0.08f;
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
            const SoundscapeListenerState listener{
                cameraPosition_.X,
                cameraPosition_.Y,
                cameraPosition_.Z,
                forward.X * std::cos(pitch_),
                std::sin(pitch_),
                forward.Z * std::cos(pitch_),
                playerMotion_.VelocityX(),
                playerMotion_.VelocityZ(),
            };
            soundscape_->Update(terrain_.SignedShoreDistance(position.x, position.z),
                                playerMotion_.Mode() == LocomotionMode::Swimming,
                                std::max(0.0f, -playerMotion_.GroundHeight()),
                                std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()),
                                deltaSeconds, listener,
                                birdFlock_.Birds(), monkeyTroop_.Monkeys());
        }
    }

    void IslandGame::UpdateCameraMatrices()
    {
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const float aspect = static_cast<float>(viewport.getWidthProperty())
            / static_cast<float>(std::max(1, viewport.getHeightProperty()));
        const float cosPitch = std::cos(pitch_);
        const Vector3 forward(std::sin(yaw_) * cosPitch, std::sin(pitch_),
                              std::cos(yaw_) * cosPitch);
        cameraView_ = Matrix::CreateLookAt(cameraPosition_, cameraPosition_ + forward,
                                           Vector3::Up);
        cameraProjection_ = Matrix::CreatePerspectiveFieldOfView(
            std::numbers::pi_v<float> / 3.0f, aspect, 0.08f, 1200.0f);
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
        UpdateCameraMatrices();
        skyRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_);
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        terrainRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_);

        const float timeSeconds = static_cast<float>(gameTime.getTotalGameTimeProperty()
                                                         .getTotalSecondsProperty());
        naturalPropRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_);

        jungleRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                              timeSeconds);

        grassRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                             timeSeconds);

        palmRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                            timeSeconds);

        birdRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                            birdFlock_.Birds());

        fishRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                            fishSchools_.Fish());

        sharkRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                             sharkPatrol_.State());

        monkeyRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                              monkeyTroop_.Monkeys());

        oceanRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                             timeSeconds);

        if (debugOverlayVisible_ && worldDebugVisible_) {
            worldDebugRenderer_->Draw(cameraView_, cameraProjection_, cameraPosition_,
                                      playerMotion_.Position(), playerMotion_.GroundHeight(),
                                      playerMotion_.Mode() == LocomotionMode::Swimming,
                                      worldObstacles_, birdFlock_.Birds(), fishSchools_.Fish(),
                                      sharkPatrol_.State(), monkeyTroop_.Monkeys());
        }

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
            overlayState.visibleTerrainCells = terrainRenderer_->VisibleCellCount();
            overlayState.totalTerrainCells = terrainRenderer_->TotalCellCount();
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
            overlayState.visibleBirds = birdRenderer_->VisibleBirdCount();
            overlayState.totalBirds = static_cast<int>(birdFlock_.Birds().size());
            overlayState.nearBirds = birdRenderer_->VisibleNearBirdCount();
            overlayState.farBirds = birdRenderer_->VisibleFarBirdCount();
            overlayState.fullSimulationBirds = birdFlock_.FullSimulationCount();
            overlayState.reducedSimulationBirds = birdFlock_.ReducedSimulationCount();
            overlayState.dormantSimulationBirds = birdFlock_.DormantSimulationCount();
            overlayState.birdDraws = birdRenderer_->DrawCallCount();
            overlayState.visibleFish = fishRenderer_->VisibleFishCount();
            overlayState.totalFish = static_cast<int>(fishSchools_.Fish().size());
            overlayState.nearFish = fishRenderer_->VisibleNearFishCount();
            overlayState.farFish = fishRenderer_->VisibleFarFishCount();
            overlayState.fullSimulationFish = fishSchools_.FullSimulationCount();
            overlayState.reducedSimulationFish = fishSchools_.ReducedSimulationCount();
            overlayState.dormantSimulationFish = fishSchools_.DormantSimulationCount();
            overlayState.fishDraws = fishRenderer_->DrawCallCount();
            overlayState.sharkVisible = sharkRenderer_->IsVisible();
            overlayState.sharkPresent = sharkPatrol_.State().present;
            overlayState.sharkNear = sharkRenderer_->UsesNearLod();
            overlayState.sharkActivity = sharkPatrol_.State().activity;
            overlayState.sharkSimulationTier = static_cast<int>(
                sharkPatrol_.State().simulationTier);
            overlayState.sharkDraws = sharkRenderer_->DrawCallCount();
            overlayState.sharkPresenceCycles = static_cast<int>(
                sharkPatrol_.PresenceCycleCount());
            overlayState.visibleMonkeys = monkeyRenderer_->VisibleMonkeyCount();
            overlayState.totalMonkeys = static_cast<int>(monkeyTroop_.Monkeys().size());
            overlayState.nearMonkeys = monkeyRenderer_->VisibleNearMonkeyCount();
            overlayState.farMonkeys = monkeyRenderer_->VisibleFarMonkeyCount();
            overlayState.fullSimulationMonkeys = monkeyTroop_.FullSimulationCount();
            overlayState.reducedSimulationMonkeys = monkeyTroop_.ReducedSimulationCount();
            overlayState.dormantSimulationMonkeys = monkeyTroop_.DormantSimulationCount();
            for (const MonkeyState& monkey : monkeyTroop_.Monkeys()) {
                switch (monkey.behavior) {
                case MonkeyBehavior::Idle:
                    ++overlayState.idleMonkeys;
                    break;
                case MonkeyBehavior::Observe:
                    ++overlayState.observeMonkeys;
                    break;
                case MonkeyBehavior::Wander:
                case MonkeyBehavior::Walk:
                    ++overlayState.walkingMonkeys;
                    break;
                case MonkeyBehavior::RunAway:
                    ++overlayState.runningMonkeys;
                    break;
                }
            }
            overlayState.monkeyDraws = monkeyRenderer_->DrawCallCount();
            overlayState.monkeyObstacleDeflections = monkeyTroop_.ObstacleDeflectionCount();
            overlayState.activeMonkeyCrossfades = monkeyRenderer_->ActiveCrossfadeCount();
            overlayState.completedMonkeyCrossfades = monkeyRenderer_->CompletedCrossfadeCount();
            overlayState.audioAvailable = soundscape_ && soundscape_->IsAvailable();
            overlayState.audioMuted = soundscape_ && soundscape_->IsMuted();
            if (soundscape_) {
                overlayState.surfVolume = soundscape_->Mix().surfVolume;
                overlayState.breezeVolume = soundscape_->Mix().breezeVolume;
                overlayState.jungleVolume = soundscape_->Mix().jungleVolume;
                overlayState.waterMovementVolume = soundscape_->WaterMix().volume;
                overlayState.playedWaveAccents = soundscape_->PlayedWaveAccentCount();
                overlayState.playedWaterTransitions =
                    soundscape_->PlayedWaterTransitionCount();
                overlayState.playedBirdCalls = soundscape_->PlayedBirdCallCount();
                overlayState.playedMonkeyCalls = soundscape_->PlayedMonkeyCallCount();
                overlayState.wildlifeCallActive = soundscape_->IsWildlifeCallActive();
            }
            overlayState.worldDebugVisible = worldDebugVisible_;
            if (worldDebugVisible_) {
                overlayState.worldDebugObstacles = worldDebugRenderer_->VisibleObstacleCount();
                overlayState.worldDebugCells = worldDebugRenderer_->VisibleCellCount();
                overlayState.worldDebugWildlife = worldDebugRenderer_->WildlifeMarkerCount();
                overlayState.worldDebugLines = worldDebugRenderer_->LineCount();
                overlayState.worldDebugDraws = worldDebugRenderer_->DrawCallCount();
            }
            debugOverlay_->Draw(overlayState);
        }

        if (drawnFrames_ == 0U) {
            std::cout << "island-simulator: visible terrain cells "
                      << terrainRenderer_->VisibleCellCount() << '/'
                      << terrainRenderer_->TotalCellCount() << ", palms "
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
                      << grassRenderer_->DrawCallCount() << " draws; birds "
                      << birdRenderer_->VisibleBirdCount() << '/'
                      << birdFlock_.Birds().size() << " visible (near "
                      << birdRenderer_->VisibleNearBirdCount() << ", far "
                      << birdRenderer_->VisibleFarBirdCount() << "), "
                      << birdRenderer_->DrawCallCount() << " draw; fish "
                      << fishRenderer_->VisibleFishCount() << '/'
                      << fishSchools_.Fish().size() << " visible (near "
                      << fishRenderer_->VisibleNearFishCount() << ", far "
                      << fishRenderer_->VisibleFarFishCount() << "), "
                      << fishRenderer_->DrawCallCount() << " draw; shark "
                      << (sharkRenderer_->IsVisible() ? "visible" : "culled")
                      << " activity " << sharkPatrol_.State().activity << ", "
                      << sharkRenderer_->DrawCallCount() << " draw; monkeys "
                      << monkeyRenderer_->VisibleMonkeyCount() << '/'
                      << monkeyTroop_.Monkeys().size() << " visible, "
                      << monkeyRenderer_->DrawCallCount() << " draws, blends "
                      << monkeyRenderer_->ActiveCrossfadeCount() << '/'
                      << monkeyRenderer_->CompletedCrossfadeCount() << " active/completed\n";
        }
        ++drawnFrames_;
        const std::uint32_t targetFrames = !capturePath_.empty()
                && viewpoint_ == FixedViewpoint::MonkeyDetail
            ? MonkeyCaptureFrames : SmokeTestFrames;
        if (!capturePath_.empty() && drawnFrames_ == targetFrames) {
            CaptureFrame(capturePath_);
            if (viewpoint_ == FixedViewpoint::MonkeyDetail) {
                std::cout << "island-simulator: monkey crossfades "
                          << monkeyRenderer_->ActiveCrossfadeCount() << '/'
                          << monkeyRenderer_->CompletedCrossfadeCount()
                          << " active/completed\n";
            }
        }
        if (smokeTest_ && drawnFrames_ >= targetFrames) {
            std::cout << "island-simulator: smoke test rendered " << drawnFrames_ << " frames\n";
            Exit();
        }
    }

    GetTypeNameCPP(IslandGame, "IslandSimulator.IslandGame")
}
