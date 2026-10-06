#include "IslandSimulator/IslandGame.hpp"

#include "IslandSimulator/PerformanceStatistics.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <limits>
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
        constexpr std::size_t BenchmarkWarmupIntervals = 120U;
        constexpr std::size_t BenchmarkSampleIntervals = 600U;

        [[nodiscard]] const char* ViewpointName(const FixedViewpoint viewpoint) noexcept
        {
            switch (viewpoint) {
            case FixedViewpoint::Overview: return "overview";
            case FixedViewpoint::BeachPath: return "beach";
            case FixedViewpoint::JungleTrail: return "jungle";
            case FixedViewpoint::GrassDetail: return "grass";
            case FixedViewpoint::GroundPlantDetail: return "ground";
            case FixedViewpoint::GroundContactDetail: return "contact";
            case FixedViewpoint::MovementTraceDetail: return "movement";
            case FixedViewpoint::PalmDetail: return "palms";
            case FixedViewpoint::WaterlineDetail: return "water";
            case FixedViewpoint::CurrentBoundaryDetail: return "current";
            case FixedViewpoint::DistantSceneryDetail: return "distant";
            case FixedViewpoint::PropDetail: return "props";
            case FixedViewpoint::DriftwoodDetail: return "driftwood";
            case FixedViewpoint::ReefDetail: return "reef";
            case FixedViewpoint::FallenFrondDetail: return "fronds";
            case FixedViewpoint::BirdDetail: return "birds";
            case FixedViewpoint::FishDetail: return "fish";
            case FixedViewpoint::SharkDetail: return "shark";
            case FixedViewpoint::MonkeyDetail: return "monkeys";
            case FixedViewpoint::BenchmarkTraversal: return "traversal";
            case FixedViewpoint::None: return "interactive";
            }
            return "unknown";
        }
    }

    IslandGame::IslandGame(const bool smokeTest, std::string capturePath,
                           FixedViewpoint viewpoint, const bool showDebugOverlay,
                           const bool inputDiagnostics, const bool showWorldDebug,
                           std::string benchmarkPath)
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
          renderCameraPosition_(cameraPosition_),
          smokeTest_(smokeTest),
          viewpoint_(viewpoint),
          capturePath_(std::move(capturePath)),
          debugOverlayVisible_(showDebugOverlay),
          worldDebugVisible_(showWorldDebug),
          inputDiagnostics_(inputDiagnostics),
          benchmarkPath_(std::move(benchmarkPath))
    {
        std::vector<CircularObstacle> worldObstacles;
        worldObstacles.reserve(vegetationLayout_.Obstacles().size()
                               + jungleLayout_.Obstacles().size()
                               + naturalPropLayout_.Obstacles().size());
        worldObstacles.insert(worldObstacles.end(), vegetationLayout_.Obstacles().begin(),
                              vegetationLayout_.Obstacles().end());
        worldObstacles.insert(worldObstacles.end(), jungleLayout_.Obstacles().begin(),
                              jungleLayout_.Obstacles().end());
        worldObstacles.insert(worldObstacles.end(), naturalPropLayout_.Obstacles().begin(),
                              naturalPropLayout_.Obstacles().end());
        worldCollisionGrid_.Reset(worldObstacles);
        monkeyTroop_.SetObstacles(worldCollisionGrid_.Obstacles());
        if (smokeTest_ && viewpoint_ == FixedViewpoint::None) {
            viewpoint_ = FixedViewpoint::Overview;
        }
        if (!benchmarkPath_.empty() && viewpoint_ == FixedViewpoint::None) {
            viewpoint_ = FixedViewpoint::Overview;
        }
        if (viewpoint_ == FixedViewpoint::BenchmarkTraversal) {
            const BenchmarkRouteSample start = benchmarkTraversalRoute_.Sample(0.0f);
            cameraPosition_ = Vector3(start.x, terrain_.Height(start.x, start.z)
                                                  + PlayerMotion::EyeHeight,
                                      start.z);
            yaw_ = std::atan2(start.forwardX, start.forwardZ);
            pitch_ = -0.025f;
        } else if (viewpoint_ == FixedViewpoint::Overview) {
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
        } else if (viewpoint_ == FixedViewpoint::GroundContactDetail) {
            constexpr float contactZ = 45.0f;
            const float contactX = VegetationLayout::PrimaryPathCenterX(contactZ);
            const Vector3 target(contactX, terrain_.Height(contactX, contactZ) + 0.42f,
                                 contactZ);
            const float cameraX = contactX - 4.2f;
            const float cameraZ = contactZ - 5.0f;
            cameraPosition_ = Vector3(
                cameraX, terrain_.Height(cameraX, cameraZ) + 3.2f, cameraZ);
            const Vector3 toContact = target - cameraPosition_;
            yaw_ = std::atan2(toContact.X, toContact.Z);
            pitch_ = std::atan2(toContact.Y, std::hypot(toContact.X, toContact.Z));
        } else if (viewpoint_ == FixedViewpoint::MovementTraceDetail
                   && !vegetationLayout_.Obstacles().empty()) {
            const CircularObstacle& obstacle = vegetationLayout_.Obstacles().front();
            const float cameraX = obstacle.x - 2.2f;
            const float cameraZ = obstacle.z + 4.8f;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ) + 2.5f,
                                      cameraZ);
            const Vector3 target(obstacle.x, terrain_.Height(obstacle.x, obstacle.z) + 0.7f,
                                 obstacle.z);
            const Vector3 toTarget = target - cameraPosition_;
            yaw_ = std::atan2(toTarget.X, toTarget.Z);
            pitch_ = std::atan2(toTarget.Y, std::hypot(toTarget.X, toTarget.Z));
        } else if (viewpoint_ == FixedViewpoint::GroundPlantDetail
                   && !jungleLayout_.GroundPlants().empty()) {
            const GroundPlantPlacement* selected = nullptr;
            float bestScore = std::numeric_limits<float>::max();
            for (const GroundPlantPlacement& plant : jungleLayout_.GroundPlants()) {
                if (plant.variant != 1U) continue;
                const float pathX = VegetationLayout::PrimaryPathCenterX(plant.z);
                const float pathDistance = std::fabs(plant.x - pathX);
                if (pathDistance < 3.2f || pathDistance > 6.5f) continue;
                const auto hasNearby = [&plant](const auto& placements,
                                                const float radius) {
                    const float radiusSquared = radius * radius;
                    for (const auto& placement : placements) {
                        const float deltaX = plant.x - placement.x;
                        const float deltaZ = plant.z - placement.z;
                        const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
                        if (distanceSquared > 0.01f && distanceSquared < radiusSquared) {
                            return true;
                        }
                    }
                    return false;
                };
                if (hasNearby(jungleLayout_.Understory(), 4.5f)
                    || hasNearby(jungleLayout_.Bushes(), 3.2f)
                    || hasNearby(jungleLayout_.GroundPlants(), 1.5f)) {
                    continue;
                }
                bool obstructed = false;
                for (const CircularObstacle& obstacle : worldCollisionGrid_.Obstacles()) {
                    const float deltaX = plant.x - obstacle.x;
                    const float deltaZ = plant.z - obstacle.z;
                    const float clearance = obstacle.radius + 3.0f;
                    if (deltaX * deltaX + deltaZ * deltaZ < clearance * clearance) {
                        obstructed = true;
                        break;
                    }
                }
                if (obstructed) continue;
                const float shoreDistance = terrain_.SignedShoreDistance(plant.x, plant.z);
                const float score = std::fabs(shoreDistance - 58.0f) * 1.4f
                    + std::fabs(pathDistance - 4.2f) * 5.0f
                    - plant.scale * 2.0f;
                if (score < bestScore) {
                    bestScore = score;
                    selected = &plant;
                }
            }
            if (selected == nullptr) {
                for (const GroundPlantPlacement& plant : jungleLayout_.GroundPlants()) {
                    if (plant.variant == 1U) {
                        selected = &plant;
                        break;
                    }
                }
            }
            if (selected == nullptr) selected = &jungleLayout_.GroundPlants().front();
            const float pathX = VegetationLayout::PrimaryPathCenterX(selected->z);
            const float cameraOffset = pathX < selected->x ? -2.8f : 2.8f;
            const float cameraX = selected->x + cameraOffset;
            const float cameraZ = selected->z;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            const Vector3 target(selected->x, selected->y + 0.42f * selected->scale,
                                 selected->z);
            const Vector3 toPlant = target - cameraPosition_;
            yaw_ = std::atan2(toPlant.X, toPlant.Z);
            pitch_ = std::atan2(toPlant.Y, std::hypot(toPlant.X, toPlant.Z));
            std::cout << "island-simulator: ground QA target variant "
                      << static_cast<int>(selected->variant)
                      << " at player-eye detail distance\n";
        } else if (viewpoint_ == FixedViewpoint::PalmDetail
                   && !vegetationLayout_.Palms().empty()) {
            constexpr float targetX = -120.0f;
            constexpr float targetZ = -135.0f;
            const PalmPlacement* selected = &vegetationLayout_.Palms().front();
            float bestScore = std::numeric_limits<float>::max();
            for (const PalmPlacement& palm : vegetationLayout_.Palms()) {
                const float shoreDistance = terrain_.SignedShoreDistance(palm.x, palm.z);
                if (shoreDistance < 8.0f || shoreDistance > 24.0f) continue;
                const float deltaX = palm.x - targetX;
                const float deltaZ = palm.z - targetZ;
                const float score = deltaX * deltaX + deltaZ * deltaZ
                    - palm.scale * 120.0f
                    - ((palm.variant == 1U || palm.variant == 3U) ? 600.0f : 0.0f);
                if (score < bestScore) {
                    bestScore = score;
                    selected = &palm;
                }
            }
            const Vector3 radial = Vector3::Normalize(
                Vector3(selected->x, 0.0f, selected->z));
            const Vector3 tangent(-radial.Z, 0.0f, radial.X);
            const float cameraX = selected->x + radial.X * 14.0f + tangent.X * 2.0f;
            const float cameraZ = selected->z + radial.Z * 14.0f + tangent.Z * 2.0f;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            const Vector3 target(selected->x, selected->y + 5.2f * selected->scale,
                                 selected->z);
            const Vector3 toPalm = target - cameraPosition_;
            yaw_ = std::atan2(toPalm.X, toPalm.Z);
            pitch_ = std::atan2(toPalm.Y, std::hypot(toPalm.X, toPalm.Z));
        } else if (viewpoint_ == FixedViewpoint::GrassDetail
                   && !grassLayout_.Patches().empty()) {
            constexpr float targetX = 35.0f;
            constexpr float targetZ = -170.0f;
            const GrassPlacement* selected = &grassLayout_.Patches().front();
            float bestScore = std::numeric_limits<float>::max();
            for (const GrassPlacement& patch : grassLayout_.Patches()) {
                if (patch.pathScale) continue;
                bool clear = true;
                for (const CircularObstacle& obstacle : worldCollisionGrid_.Obstacles()) {
                    const float deltaX = patch.x - obstacle.x;
                    const float deltaZ = patch.z - obstacle.z;
                    const float clearance = obstacle.radius + 5.0f;
                    if (deltaX * deltaX + deltaZ * deltaZ < clearance * clearance) {
                        clear = false;
                        break;
                    }
                }
                if (!clear) continue;
                const auto foliageIsClear = [&patch](const auto& placements,
                                                     const float radius) {
                    for (const auto& placement : placements) {
                        const float deltaX = patch.x - placement.x;
                        const float deltaZ = patch.z - placement.z;
                        if (deltaX * deltaX + deltaZ * deltaZ < radius * radius) {
                            return false;
                        }
                    }
                    return true;
                };
                if (!foliageIsClear(jungleLayout_.Understory(), 4.0f)
                    || !foliageIsClear(jungleLayout_.Bushes(), 4.0f)
                    || !foliageIsClear(jungleLayout_.GroundPlants(), 3.5f)) {
                    continue;
                }
                const float deltaX = patch.x - targetX;
                const float deltaZ = patch.z - targetZ;
                const float score = deltaX * deltaX + deltaZ * deltaZ;
                if (score < bestScore) {
                    bestScore = score;
                    selected = &patch;
                }
            }
            cameraPosition_ = Vector3(selected->x, selected->y + PlayerMotion::EyeHeight,
                                      selected->z);
            yaw_ = std::atan2(-selected->x, -selected->z);
            pitch_ = -0.38f;
        } else if (viewpoint_ == FixedViewpoint::WaterlineDetail) {
            constexpr float angle = -std::numbers::pi_v<float> * 0.5f;
            const float radius = terrain_.ShoreRadius(angle) + 18.0f;
            cameraPosition_ = Vector3(std::cos(angle) * radius, 0.58f,
                                      std::sin(angle) * radius);
            yaw_ = std::atan2(-cameraPosition_.X, -cameraPosition_.Z);
            pitch_ = -0.025f;
        } else if (viewpoint_ == FixedViewpoint::CurrentBoundaryDetail) {
            constexpr float angle = -std::numbers::pi_v<float> * 0.5f;
            const float radius = terrain_.ShoreRadius(angle) + 62.0f;
            cameraPosition_ = Vector3(std::cos(angle) * radius, 0.58f,
                                      std::sin(angle) * radius);
            yaw_ = std::atan2(-cameraPosition_.X, -cameraPosition_.Z);
            pitch_ = -0.015f;
        } else if (viewpoint_ == FixedViewpoint::DistantSceneryDetail) {
            constexpr float cameraZ = -214.0f;
            cameraPosition_ = Vector3(0.0f, terrain_.Height(0.0f, cameraZ)
                                                + PlayerMotion::EyeHeight,
                                      cameraZ);
            yaw_ = std::numbers::pi_v<float>;
            pitch_ = -0.005f;
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
        } else if (viewpoint_ == FixedViewpoint::ReefDetail) {
            const auto reef = std::max_element(
                naturalPropLayout_.Rocks().begin(), naturalPropLayout_.Rocks().end(),
                [](const RockPlacement& first, const RockPlacement& second) {
                    if (first.offshore != second.offshore) return !first.offshore;
                    return first.scaleY < second.scaleY;
                });
            if (reef != naturalPropLayout_.Rocks().end() && reef->offshore) {
                const Vector3 radial = Vector3::Normalize(Vector3(reef->x, 0.0f, reef->z));
                const Vector3 tangent(-radial.Z, 0.0f, radial.X);
                const float cameraX = reef->x - radial.X * 7.5f + tangent.X * 1.4f;
                const float cameraZ = reef->z - radial.Z * 7.5f + tangent.Z * 1.4f;
                cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                       + PlayerMotion::EyeHeight,
                                          cameraZ);
                const Vector3 toReef = Vector3(reef->x, reef->y + reef->scaleY * 0.56f,
                                               reef->z) - cameraPosition_;
                yaw_ = std::atan2(toReef.X, toReef.Z);
                pitch_ = std::atan2(toReef.Y, std::hypot(toReef.X, toReef.Z));
            }
        } else if (viewpoint_ == FixedViewpoint::FallenFrondDetail
                   && !naturalPropLayout_.FallenFronds().empty()) {
            const auto closestToShore = std::min_element(
                naturalPropLayout_.FallenFronds().begin(),
                naturalPropLayout_.FallenFronds().end(),
                [this](const FallenFrondPlacement& first,
                       const FallenFrondPlacement& second) {
                    return terrain_.SignedShoreDistance(first.x, first.z)
                        < terrain_.SignedShoreDistance(second.x, second.z);
                });
            const FallenFrondPlacement& frond = *closestToShore;
            const Vector3 direction(std::cos(frond.yawRadians), 0.0f,
                                    std::sin(frond.yawRadians));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const float cameraX = frond.x - direction.X * 0.8f + side.X * 3.1f;
            const float cameraZ = frond.z - direction.Z * 0.8f + side.Z * 3.1f;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            const Vector3 toFrond = Vector3(frond.x, frond.y + 0.10f, frond.z)
                - cameraPosition_;
            yaw_ = std::atan2(toFrond.X, toFrond.Z);
            pitch_ = std::atan2(toFrond.Y, std::hypot(toFrond.X, toFrond.Z));
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
        if (!benchmarkPath_.empty()) {
            graphics_.setSynchronizeWithVerticalRetraceProperty(false);
        }
#if defined(__ANDROID__)
        graphics_.setIsFullScreenProperty(true);
        touchOverlayVisible_ = true;
#endif
        getWindowProperty().setTitleProperty("island-simulator");
        setIsMouseVisibleProperty(true);
        setIsFixedTimeStepProperty(benchmarkPath_.empty());
#if defined(__EMSCRIPTEN__)
        soundscapeNeedsGesture_ = true;
#endif
    }

    void IslandGame::LoadContent()
    {
        auto& device = getGraphicsDeviceProperty();
        terrainRenderer_ = std::make_unique<TerrainRenderer>(device, terrain_);
        distantSceneryRenderer_ = std::make_unique<DistantSceneryRenderer>(
            device, distantSceneryLayout_);
        contactShadowRenderer_ = std::make_unique<ContactShadowRenderer>(
            device, terrain_, vegetationLayout_, jungleLayout_, naturalPropLayout_);
        birdRenderer_ = std::make_unique<BirdRenderer>(device);
        fishRenderer_ = std::make_unique<FishRenderer>(device);
        sharkRenderer_ = std::make_unique<SharkRenderer>(device);
        monkeyRenderer_ = std::make_unique<MonkeyRenderer>(device);
        grassRenderer_ = std::make_unique<GrassRenderer>(device, terrain_, grassLayout_);
        jungleRenderer_ = std::make_unique<JungleRenderer>(device, jungleLayout_);
        naturalPropRenderer_ = std::make_unique<NaturalPropRenderer>(device, naturalPropLayout_);
        palmRenderer_ = std::make_unique<PalmRenderer>(device, vegetationLayout_);
        oceanRenderer_ = std::make_unique<OceanRenderer>(device, terrain_);
        shorelineRenderer_ = std::make_unique<ShorelineRenderer>(device, terrain_);
        skyRenderer_ = std::make_unique<SkyRenderer>(device);
        debugOverlay_ = std::make_unique<DebugOverlay>(device);
        touchOverlay_ = std::make_unique<TouchOverlay>(device);
        waterViewOverlay_ = std::make_unique<WaterViewOverlay>(device);
        worldDebugRenderer_ = std::make_unique<WorldDebugRenderer>(device, terrain_);
        if (viewpoint_ == FixedViewpoint::GroundContactDetail) {
            constexpr float contactZ = 45.0f;
            playerMotion_.SetPosition(
                terrain_, VegetationLayout::PrimaryPathCenterX(contactZ), contactZ);
        } else if (viewpoint_ == FixedViewpoint::MovementTraceDetail
                   && !vegetationLayout_.Obstacles().empty()) {
            const CircularObstacle& obstacle = vegetationLayout_.Obstacles().front();
            playerMotion_.SetPosition(terrain_, obstacle.x - 1.8f, obstacle.z);
            for (int step = 0; step < 90; ++step) {
                playerMotion_.Update(terrain_, 1.0f, 0.0f, 1.0f / 60.0f,
                                     worldCollisionGrid_);
            }
        } else {
            playerMotion_.SetPosition(terrain_, cameraPosition_.X, cameraPosition_.Z);
        }
        const bool waterViewAllowed = viewpoint_ == FixedViewpoint::None
            || viewpoint_ == FixedViewpoint::WaterlineDetail
            || viewpoint_ == FixedViewpoint::CurrentBoundaryDetail
            || viewpoint_ == FixedViewpoint::BenchmarkTraversal;
        waterViewModel_.Reset(waterViewAllowed
                                  && playerMotion_.Mode() == LocomotionMode::Swimming,
                              playerMotion_.GroundHeight(),
                              playerMotion_.OceanCurrentStrength(terrain_));
        const float initialRippleSpeed = viewpoint_ == FixedViewpoint::WaterlineDetail
            ? 1.35f
            : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ());
        waterRippleModel_.Reset(
            playerMotion_.Mode() == LocomotionMode::Swimming,
            playerMotion_.GroundHeight(), initialRippleSpeed);
        const float initialGrassSpeed = viewpoint_ == FixedViewpoint::GrassDetail
            ? 3.2f
            : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ());
        grassInteractionModel_.Reset(initialGrassSpeed);
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
                  << "island-simulator: collision grid "
                  << worldCollisionGrid_.Obstacles().size() << " static obstacles in "
                  << worldCollisionGrid_.CellCount() << " occupied cells\n"
                  << "island-simulator: birds " << birdFlock_.Birds().size()
                  << " agents in a distance-throttled one-batch simulation\n"
                  << "island-simulator: fish " << fishSchools_.Fish().size()
                  << " agents in " << FishSchools::SchoolCount
                  << " distance-throttled shallow-water schools\n"
                  << "island-simulator: one occasional non-hostile offshore shark patrol\n"
                  << "island-simulator: monkeys " << monkeyTroop_.Monkeys().size()
                  << " agents with portable GPU skeletal animation, "
                  << MonkeyRenderer::SilhouetteCount << " silhouettes, 4 palettes\n"
                  << "island-simulator: terrain-conforming contact shadows in one portable "
                     "near-field batch\n"
                  << "island-simulator: near-surface swimming view procedural edge veil\n"
                  << "island-simulator: movement-driven player ripples integrated into the "
                     "portable ocean shader\n";
        if (debugOverlayVisible_) {
            std::cout << "island-simulator: debug overlay active\n";
        }
        if (worldDebugVisible_) {
            std::cout << "island-simulator: world debug visualization active with 9 ground-contact "
                         "samples, resolved movement trace, wildlife navigation guides, 6 "
                         "audio-zone contours, and positional-emitter markers\n";
        }
        if (!benchmarkPath_.empty()) {
            std::cout << "island-simulator: benchmark " << ViewpointName(viewpoint_)
                      << " warming " << BenchmarkWarmupIntervals << " intervals, sampling "
                      << BenchmarkSampleIntervals << " intervals with VSync disabled\n";
            if (viewpoint_ == FixedViewpoint::BenchmarkTraversal) {
                std::cout << "island-simulator: benchmark traversal rail "
                          << benchmarkTraversalRoute_.ApproximateLength() << " m across "
                          << benchmarkTraversalRoute_.Waypoints().size()
                          << " authored waypoints\n";
            }
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
        (void)soundscape_->Start(
            shoreDistance, swimming, waterDepth, horizontalSpeed,
            playerMotion_.OceanCurrentStrength(terrain_));
        soundscapeNeedsGesture_ = false;
    }

    void IslandGame::Update(GameTime& gameTime)
    {
        const float deltaSeconds = std::min(
            0.1f, static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty())
                / 1000.0f);
        if (!benchmarkPath_.empty()
            && viewpoint_ == FixedViewpoint::BenchmarkTraversal) {
            UpdateBenchmarkTraversal();
        }
        if (!smokeTest_ && viewpoint_ == FixedViewpoint::None) {
            UpdateCamera(deltaSeconds);
        }
        const bool waterViewAllowed = viewpoint_ == FixedViewpoint::None
            || viewpoint_ == FixedViewpoint::WaterlineDetail
            || viewpoint_ == FixedViewpoint::CurrentBoundaryDetail
            || viewpoint_ == FixedViewpoint::BenchmarkTraversal;
        (void)waterViewModel_.Update(
            waterViewAllowed && playerMotion_.Mode() == LocomotionMode::Swimming,
            playerMotion_.GroundHeight(), playerMotion_.OceanCurrentStrength(terrain_),
            deltaSeconds);
        const float rippleSpeed = viewpoint_ == FixedViewpoint::WaterlineDetail
            ? 1.35f
            : (viewpoint_ == FixedViewpoint::BenchmarkTraversal
                   ? benchmarkTraversalPresentationSpeed_
                   : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()));
        (void)waterRippleModel_.Update(
            playerMotion_.Mode() == LocomotionMode::Swimming,
            playerMotion_.GroundHeight(), rippleSpeed, deltaSeconds);
        const float grassInteractionSpeed = viewpoint_ == FixedViewpoint::GrassDetail
            ? 3.2f
            : (viewpoint_ == FixedViewpoint::BenchmarkTraversal
                   ? benchmarkTraversalPresentationSpeed_
                   : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()));
        (void)grassInteractionModel_.Update(grassInteractionSpeed, deltaSeconds);
        birdFlock_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        fishSchools_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        sharkPatrol_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        monkeyTroop_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        if (soundscape_ && worldDebugVisible_) {
            soundscape_->RefreshDebugSources(
                cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z,
                birdFlock_.Birds(), monkeyTroop_.Monkeys());
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
                          << " player " << position.x << ' ' << position.z
                          << " camera " << cameraPosition_.X << ' '
                          << cameraPosition_.Y << ' ' << cameraPosition_.Z
                          << " free " << (IsDebugFreeCameraActive() ? 1 : 0)
                          << " yaw " << yaw_ << " pitch " << pitch_
                          << " viewport " << viewport.getWidthProperty() << ' '
                          << viewport.getHeightProperty() << '\n' << std::flush;
            }
        }
    }

    void IslandGame::UpdateBenchmarkTraversal()
    {
        float progress = 0.0f;
        if (benchmarkWarmupIntervals_ >= BenchmarkWarmupIntervals) {
            progress = static_cast<float>(benchmarkFrameMilliseconds_.size())
                / static_cast<float>(BenchmarkSampleIntervals - 1U);
        }
        const BenchmarkRouteSample sample = benchmarkTraversalRoute_.Sample(progress);
        playerMotion_.SetPosition(terrain_, sample.x, sample.z);
        const PlayerPosition& player = playerMotion_.Position();
        cameraPosition_ = Vector3(player.x, player.y, player.z);
        yaw_ = std::atan2(sample.forwardX, sample.forwardZ);

        constexpr float lookAheadDistance = 5.0f;
        const float targetX = sample.x + sample.forwardX * lookAheadDistance;
        const float targetZ = sample.z + sample.forwardZ * lookAheadDistance;
        const bool targetIsWater = terrain_.Height(targetX, targetZ) < -0.16f;
        const float targetY = targetIsWater
            ? PlayerMotion::SwimmingEyeHeight
            : terrain_.Height(targetX, targetZ) + PlayerMotion::EyeHeight;
        pitch_ = std::clamp(
            std::atan2(targetY - cameraPosition_.Y, lookAheadDistance) - 0.025f,
            -0.18f, 0.14f);
        benchmarkTraversalPresentationSpeed_ = playerMotion_.Mode()
            == LocomotionMode::Swimming ? PlayerMotion::SwimSpeed : PlayerMotion::WalkSpeed;
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

        const bool cameraMotionToggleDown = keyboard.IsKeyDown(Keys::B);
        if (cameraMotionToggleDown && !cameraMotionToggleWasDown_) {
            cameraMotion_.SetEnabled(!cameraMotion_.IsEnabled());
            std::cout << "island-simulator: camera motion "
                      << (cameraMotion_.IsEnabled() ? "enabled" : "disabled") << '\n';
        }
        cameraMotionToggleWasDown_ = cameraMotionToggleDown;

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
                      << (worldDebugVisible_
                              ? "enabled with 9 ground-contact samples, resolved movement trace, "
                                "wildlife navigation guides, 6 audio-zone contours, and "
                                "positional-emitter markers"
                              : "disabled")
                      << '\n';
        }
        worldDebugToggleWasDown_ = worldDebugToggleDown;

#ifndef NDEBUG
        const bool debugFreeCameraToggleDown = keyboard.IsKeyDown(Keys::F3);
        if (debugOverlayVisible_ && debugFreeCameraToggleDown
            && !debugFreeCameraToggleWasDown_) {
            if (debugFreeCamera_.IsActive()) {
                debugFreeCamera_.Deactivate();
                const PlayerPosition& player = playerMotion_.Position();
                cameraPosition_ = Vector3(player.x, player.y, player.z);
            } else {
                debugFreeCamera_.Activate(
                    cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
                cameraMotion_.Reset();
            }
            std::cout << "island-simulator: debug free camera "
                      << (debugFreeCamera_.IsActive() ? "enabled" : "disabled")
                      << '\n' << std::flush;
        }
        debugFreeCameraToggleWasDown_ = debugFreeCameraToggleDown;
        if (!debugOverlayVisible_ && debugFreeCamera_.IsActive()) {
            debugFreeCamera_.Deactivate();
            const PlayerPosition& player = playerMotion_.Position();
            cameraPosition_ = Vector3(player.x, player.y, player.z);
            std::cout << "island-simulator: debug free camera disabled with overlay\n";
        }
#endif

        constexpr std::array<Keys, 8> teleportKeys = {
            Keys::F5, Keys::F6, Keys::F7, Keys::F8, Keys::F9, Keys::F10, Keys::F11,
            Keys::F12};
        constexpr std::array<const char*, 8> teleportNames = {
            "south beach", "jungle trail", "east clearing", "west clearing",
            "fish shallows", "offshore shark", "monkey troop", "offshore current"};
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
                } else if (index == 7U) {
                    constexpr float angle = -std::numbers::pi_v<float> * 0.5f;
                    const float radius = terrain_.ShoreRadius(angle) + 62.0f;
                    x = std::cos(angle) * radius;
                    z = std::sin(angle) * radius;
                    yaw_ = 0.0f;
                    pitch_ = -0.015f;
                }
                playerMotion_.SetPosition(terrain_, x, z);
                cameraMotion_.Reset();
                const PlayerPosition& teleported = playerMotion_.Position();
                cameraPosition_ = Vector3(teleported.x, teleported.y, teleported.z);
#ifndef NDEBUG
                if (debugFreeCamera_.IsActive()) {
                    debugFreeCamera_.Activate(
                        cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
                }
#endif
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

#ifndef NDEBUG
        if (debugFreeCamera_.IsActive()) {
            const float vertical = (keyboard.IsKeyDown(Keys::E) ? 1.0f : 0.0f)
                - (keyboard.IsKeyDown(Keys::Q) ? 1.0f : 0.0f);
            const bool fast = keyboard.IsKeyDown(Keys::LeftShift)
                || keyboard.IsKeyDown(Keys::RightShift);
            debugFreeCamera_.Update(
                yaw_, DebugFreeCameraInput{Vector3::Dot(wish, forward),
                                            Vector3::Dot(wish, right), vertical, fast},
                deltaSeconds);
            const DebugFreeCameraPosition& freePosition = debugFreeCamera_.Position();
            cameraPosition_ = Vector3(freePosition.x, freePosition.y, freePosition.z);
            cameraMotion_.Reset();
        } else {
#endif
            const PlayerPosition positionBeforeMovement = playerMotion_.Position();
            playerMotion_.Update(terrain_, wish.X, wish.Z, deltaSeconds, worldCollisionGrid_);
            const PlayerPosition& position = playerMotion_.Position();
            cameraPosition_ = Vector3(position.x, position.y, position.z);
            const float travelledDistance = std::hypot(position.x - positionBeforeMovement.x,
                                                       position.z - positionBeforeMovement.z);
            const float physicalSpeed = deltaSeconds > 0.000001f
                ? travelledDistance / deltaSeconds
                : 0.0f;
            (void)cameraMotion_.Update(physicalSpeed,
                                       playerMotion_.Mode(), deltaSeconds);
#ifndef NDEBUG
        }
#endif
        if (soundscape_) {
            const PlayerPosition& position = playerMotion_.Position();
            const SoundscapeListenerState listener{
                cameraPosition_.X,
                cameraPosition_.Y,
                cameraPosition_.Z,
                forward.X * std::cos(pitch_),
                std::sin(pitch_),
                forward.Z * std::cos(pitch_),
                IsDebugFreeCameraActive() ? 0.0f : playerMotion_.VelocityX(),
                IsDebugFreeCameraActive() ? 0.0f : playerMotion_.VelocityZ(),
            };
            soundscape_->Update(terrain_.SignedShoreDistance(position.x, position.z),
                                playerMotion_.Mode() == LocomotionMode::Swimming,
                                std::max(0.0f, -playerMotion_.GroundHeight()),
                                std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()),
                                playerMotion_.OceanCurrentStrength(terrain_),
                                deltaSeconds, listener,
                                birdFlock_.Birds(), monkeyTroop_.Monkeys());
        }
    }

    bool IslandGame::IsDebugFreeCameraActive() const noexcept
    {
#ifndef NDEBUG
        return debugFreeCamera_.IsActive();
#else
        return false;
#endif
    }

    void IslandGame::UpdateCameraMatrices()
    {
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const float aspect = static_cast<float>(viewport.getWidthProperty())
            / static_cast<float>(std::max(1, viewport.getHeightProperty()));
        const float cosPitch = std::cos(pitch_);
        const Vector3 forward(std::sin(yaw_) * cosPitch, std::sin(pitch_),
                              std::cos(yaw_) * cosPitch);
        renderCameraPosition_ = cameraPosition_;
        if (viewpoint_ == FixedViewpoint::None && !smokeTest_
            && !IsDebugFreeCameraActive()) {
            const CameraMotionOffset& motion = cameraMotion_.Offset();
            const Vector3 right(std::cos(yaw_), 0.0f, -std::sin(yaw_));
            renderCameraPosition_ = renderCameraPosition_
                + right * motion.lateralMeters + Vector3::Up * motion.verticalMeters;
        }
        cameraView_ = Matrix::CreateLookAt(renderCameraPosition_,
                                           renderCameraPosition_ + forward,
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

    int IslandGame::TrackedSceneDrawCount() const noexcept
    {
        return 2 + terrainRenderer_->VisibleCellCount()
            + palmRenderer_->DrawCallCount() + jungleRenderer_->DrawCallCount()
            + naturalPropRenderer_->DrawCallCount() + grassRenderer_->DrawCallCount()
            + distantSceneryRenderer_->DrawCallCount()
            + contactShadowRenderer_->DrawCallCount()
            + birdRenderer_->DrawCallCount() + fishRenderer_->DrawCallCount()
            + sharkRenderer_->DrawCallCount() + monkeyRenderer_->DrawCallCount()
            + shorelineRenderer_->DrawCallCount() + waterViewOverlay_->DrawCallCount();
    }

    void IslandGame::RecordBenchmarkSceneSample()
    {
        const int trackedDraws = TrackedSceneDrawCount();
        const int terrainCells = terrainRenderer_->VisibleCellCount();
        const int contactShadows = contactShadowRenderer_->VisibleShadowCount();
        const int palms = palmRenderer_->VisiblePalmCount();
        const int canopy = jungleRenderer_->VisibleCanopyCount();
        const int grass = grassRenderer_->VisiblePatchCount();
        const float shoreDistance = terrain_.SignedShoreDistance(
            cameraPosition_.X, cameraPosition_.Z);
        BenchmarkSceneRanges& ranges = benchmarkSceneRanges_;
        if (ranges.sampleCount == 0U) {
            ranges.minimumTrackedDraws = ranges.maximumTrackedDraws = trackedDraws;
            ranges.minimumTerrainCells = ranges.maximumTerrainCells = terrainCells;
            ranges.minimumContactShadows = ranges.maximumContactShadows = contactShadows;
            ranges.minimumPalms = ranges.maximumPalms = palms;
            ranges.minimumCanopy = ranges.maximumCanopy = canopy;
            ranges.minimumGrass = ranges.maximumGrass = grass;
            ranges.minimumShoreDistance = ranges.maximumShoreDistance = shoreDistance;
            ranges.firstCameraPosition = cameraPosition_;
        } else {
            ranges.minimumTrackedDraws = std::min(ranges.minimumTrackedDraws, trackedDraws);
            ranges.maximumTrackedDraws = std::max(ranges.maximumTrackedDraws, trackedDraws);
            ranges.minimumTerrainCells = std::min(ranges.minimumTerrainCells, terrainCells);
            ranges.maximumTerrainCells = std::max(ranges.maximumTerrainCells, terrainCells);
            ranges.minimumContactShadows = std::min(
                ranges.minimumContactShadows, contactShadows);
            ranges.maximumContactShadows = std::max(
                ranges.maximumContactShadows, contactShadows);
            ranges.minimumPalms = std::min(ranges.minimumPalms, palms);
            ranges.maximumPalms = std::max(ranges.maximumPalms, palms);
            ranges.minimumCanopy = std::min(ranges.minimumCanopy, canopy);
            ranges.maximumCanopy = std::max(ranges.maximumCanopy, canopy);
            ranges.minimumGrass = std::min(ranges.minimumGrass, grass);
            ranges.maximumGrass = std::max(ranges.maximumGrass, grass);
            ranges.minimumShoreDistance = std::min(
                ranges.minimumShoreDistance, shoreDistance);
            ranges.maximumShoreDistance = std::max(
                ranges.maximumShoreDistance, shoreDistance);
        }
        ranges.totalTrackedDraws += static_cast<std::uint64_t>(trackedDraws);
        ranges.totalTerrainCells += static_cast<std::uint64_t>(terrainCells);
        ranges.totalContactShadows += static_cast<std::uint64_t>(contactShadows);
        ranges.totalPalms += static_cast<std::uint64_t>(palms);
        ranges.totalCanopy += static_cast<std::uint64_t>(canopy);
        ranges.totalGrass += static_cast<std::uint64_t>(grass);
        ranges.lastCameraPosition = cameraPosition_;
        ++ranges.sampleCount;
    }

    void IslandGame::RecordBenchmarkFrame()
    {
        const auto now = std::chrono::steady_clock::now();
        if (!benchmarkHasPreviousFrame_) {
            benchmarkPreviousFrameTime_ = now;
            benchmarkHasPreviousFrame_ = true;
            return;
        }

        const double frameMilliseconds =
            std::chrono::duration<double, std::milli>(now - benchmarkPreviousFrameTime_).count();
        benchmarkPreviousFrameTime_ = now;
        if (benchmarkWarmupIntervals_ < BenchmarkWarmupIntervals) {
            ++benchmarkWarmupIntervals_;
            return;
        }
        benchmarkFrameMilliseconds_.push_back(frameMilliseconds);
        benchmarkSceneSamplePending_ = true;
        if (benchmarkFrameMilliseconds_.size() >= BenchmarkSampleIntervals) {
            benchmarkReportPending_ = true;
        }
    }

    void IslandGame::WriteBenchmarkReport()
    {
        const FrameTimeSummary summary =
            CalculateFrameTimeSummary(benchmarkFrameMilliseconds_);
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const int trackedSceneDraws = TrackedSceneDrawCount();
        const BenchmarkSceneRanges& ranges = benchmarkSceneRanges_;
        const auto meanCount = [&ranges](const std::uint64_t total) {
            return ranges.sampleCount == 0U ? 0.0
                : static_cast<double>(total) / static_cast<double>(ranges.sampleCount);
        };

        std::ofstream output(benchmarkPath_);
        if (!output) {
            throw std::runtime_error(
                "Cannot create benchmark report at '" + benchmarkPath_ + "'");
        }
        output.imbue(std::locale::classic());
        output << std::fixed << std::setprecision(4)
               << "{\n"
               << "  \"schema_version\": 2,\n"
               << "  \"renderer\": \""
               << getGraphicsDeviceProperty().GetGraphicsRendererName() << "\",\n"
               << "  \"viewpoint\": \"" << ViewpointName(viewpoint_) << "\",\n"
               << "  \"resolution\": {\"width\": " << viewport.getWidthProperty()
               << ", \"height\": " << viewport.getHeightProperty() << "},\n"
               << "  \"settings\": {\"vsync\": false, \"fixed_timestep\": false, "
               << "\"warmup_intervals\": " << BenchmarkWarmupIntervals
               << ", \"sample_intervals\": " << BenchmarkSampleIntervals << "},\n"
               << "  \"timing\": {\n"
               << "    \"sample_count\": " << summary.sampleCount << ",\n"
               << "    \"mean_ms\": " << summary.meanMilliseconds << ",\n"
               << "    \"median_ms\": " << summary.medianMilliseconds << ",\n"
               << "    \"p95_ms\": " << summary.percentile95Milliseconds << ",\n"
               << "    \"p99_ms\": " << summary.percentile99Milliseconds << ",\n"
               << "    \"min_ms\": " << summary.minimumMilliseconds << ",\n"
               << "    \"max_ms\": " << summary.maximumMilliseconds << ",\n"
               << "    \"mean_fps\": " << summary.meanFramesPerSecond << "\n"
               << "  },\n"
               << "  \"route\": ";
        if (viewpoint_ == FixedViewpoint::BenchmarkTraversal) {
            output << "{\n"
                   << "    \"name\": \"island_traversal\",\n"
                   << "    \"waypoint_count\": "
                   << benchmarkTraversalRoute_.Waypoints().size() << ",\n"
                   << "    \"approximate_horizontal_distance_m\": "
                   << benchmarkTraversalRoute_.ApproximateLength() << ",\n"
                   << "    \"sampled_shore_distance_m\": {\"min\": "
                   << ranges.minimumShoreDistance << ", \"max\": "
                   << ranges.maximumShoreDistance << "},\n"
                   << "    \"first_camera_m\": {\"x\": "
                   << ranges.firstCameraPosition.X << ", \"y\": "
                   << ranges.firstCameraPosition.Y << ", \"z\": "
                   << ranges.firstCameraPosition.Z << "},\n"
                   << "    \"last_camera_m\": {\"x\": "
                   << ranges.lastCameraPosition.X << ", \"y\": "
                   << ranges.lastCameraPosition.Y << ", \"z\": "
                   << ranges.lastCameraPosition.Z << "}\n"
                   << "  },\n";
        } else {
            output << "null,\n";
        }
        output << "  \"scene_range\": {\n"
               << "    \"sample_count\": " << ranges.sampleCount << ",\n"
               << "    \"tracked_draw_submissions\": {\"min\": "
               << ranges.minimumTrackedDraws << ", \"max\": "
               << ranges.maximumTrackedDraws << ", \"mean\": "
               << meanCount(ranges.totalTrackedDraws) << "},\n"
               << "    \"terrain_cells\": {\"min\": "
               << ranges.minimumTerrainCells << ", \"max\": "
               << ranges.maximumTerrainCells << ", \"mean\": "
               << meanCount(ranges.totalTerrainCells) << "},\n"
               << "    \"contact_shadows\": {\"min\": "
               << ranges.minimumContactShadows << ", \"max\": "
               << ranges.maximumContactShadows << ", \"mean\": "
               << meanCount(ranges.totalContactShadows) << "},\n"
               << "    \"palms\": {\"min\": " << ranges.minimumPalms
               << ", \"max\": " << ranges.maximumPalms << ", \"mean\": "
               << meanCount(ranges.totalPalms) << "},\n"
               << "    \"canopy\": {\"min\": " << ranges.minimumCanopy
               << ", \"max\": " << ranges.maximumCanopy << ", \"mean\": "
               << meanCount(ranges.totalCanopy) << "},\n"
               << "    \"grass_clusters\": {\"min\": "
               << ranges.minimumGrass << ", \"max\": "
               << ranges.maximumGrass << ", \"mean\": "
               << meanCount(ranges.totalGrass) << "}\n"
               << "  },\n"
               << "  \"scene\": {\n"
               << "    \"tracked_draw_submissions\": " << trackedSceneDraws << ",\n"
               << "    \"terrain_cells\": " << terrainRenderer_->VisibleCellCount() << ",\n"
               << "    \"distant_islets\": "
               << distantSceneryRenderer_->IsletCount() << ",\n"
               << "    \"contact_shadows\": "
               << contactShadowRenderer_->VisibleShadowCount() << ",\n"
               << "    \"contact_shadow_triangles\": "
               << contactShadowRenderer_->PrimitiveCount() << ",\n"
               << "    \"palms\": " << palmRenderer_->VisiblePalmCount() << ",\n"
               << "    \"palms_near\": " << palmRenderer_->VisibleNearPalmCount() << ",\n"
               << "    \"palms_far\": " << palmRenderer_->VisibleFarPalmCount() << ",\n"
               << "    \"canopy\": " << jungleRenderer_->VisibleCanopyCount() << ",\n"
               << "    \"canopy_near\": "
               << jungleRenderer_->VisibleCanopyNearCount() << ",\n"
               << "    \"canopy_far\": "
               << jungleRenderer_->VisibleCanopyFarCount() << ",\n"
               << "    \"understory\": " << jungleRenderer_->VisibleUnderstoryCount() << ",\n"
               << "    \"bushes\": " << jungleRenderer_->VisibleBushCount() << ",\n"
               << "    \"ground_plants\": " << jungleRenderer_->VisibleGroundPlantCount()
               << ",\n"
               << "    \"grass_clusters\": " << grassRenderer_->VisiblePatchCount() << ",\n"
               << "    \"grass_interaction\": "
               << grassInteractionModel_.Strength() << ",\n"
               << "    \"rocks\": " << naturalPropRenderer_->VisibleRockCount() << ",\n"
               << "    \"reef_outcrops\": "
               << naturalPropRenderer_->VisibleReefRockCount() << ",\n"
               << "    \"fallen_logs\": " << naturalPropRenderer_->VisibleLogCount() << ",\n"
               << "    \"driftwood\": " << naturalPropRenderer_->VisibleDriftwoodCount()
               << ",\n"
               << "    \"fallen_fronds\": "
               << naturalPropRenderer_->VisibleFallenFrondCount() << ",\n"
               << "    \"beach_details\": "
               << naturalPropRenderer_->VisibleBeachDetailCount() << ",\n"
               << "    \"birds\": " << birdRenderer_->VisibleBirdCount() << ",\n"
               << "    \"fish\": " << fishRenderer_->VisibleFishCount() << ",\n"
               << "    \"shark\": " << (sharkRenderer_->IsVisible() ? 1 : 0) << ",\n"
               << "    \"player_water_ripple\": "
               << waterRippleModel_.Intensity() << ",\n"
               << "    \"player_water_ripple_phase\": "
               << waterRippleModel_.Phase() << ",\n"
               << "    \"monkeys\": " << monkeyRenderer_->VisibleMonkeyCount() << "\n"
               << "  }\n"
               << "}\n";
        if (!output) {
            throw std::runtime_error(
                "Failed while writing benchmark report at '" + benchmarkPath_ + "'");
        }
        std::cout << "island-simulator: benchmark completed " << summary.sampleCount
                  << " samples, mean " << summary.meanMilliseconds << " ms, p95 "
                  << summary.percentile95Milliseconds << " ms, report " << benchmarkPath_
                  << '\n';
    }

    void IslandGame::Draw(const GameTime& gameTime)
    {
        auto& device = getGraphicsDeviceProperty();
        if (!benchmarkPath_.empty() && !benchmarkReportPending_) {
            RecordBenchmarkFrame();
        }
        debugOverlay_->UpdateFrameTiming();
        device.Clear(Color(136, 202, 224, 255), 1.0f);
        UpdateCameraMatrices();
        skyRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        terrainRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);
        distantSceneryRenderer_->Draw(cameraView_, cameraProjection_);
        contactShadowRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                     monkeyTroop_.Monkeys());

        const float timeSeconds = static_cast<float>(gameTime.getTotalGameTimeProperty()
                                                         .getTotalSecondsProperty());
        naturalPropRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);

        jungleRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                              timeSeconds);

        const PlayerPosition& grassPlayer = playerMotion_.Position();
        grassRenderer_->Draw(
            cameraView_, cameraProjection_, renderCameraPosition_,
            Vector3(grassPlayer.x, grassPlayer.y, grassPlayer.z),
            grassInteractionModel_.Strength(), timeSeconds);

        palmRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                            timeSeconds);

        birdRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                            birdFlock_.Birds());

        fishRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                            fishSchools_.Fish());

        sharkRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                             sharkPatrol_.State());

        monkeyRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                              monkeyTroop_.Monkeys());

        const PlayerPosition& oceanPlayer = playerMotion_.Position();
        oceanRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                             timeSeconds, oceanPlayer.x, oceanPlayer.z,
                             waterRippleModel_.Intensity(), waterRippleModel_.Phase());
        shorelineRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                 timeSeconds);

        const auto& viewport = device.getViewportProperty();
        waterViewOverlay_->Draw(waterViewModel_.Intensity(),
                                playerMotion_.OceanCurrentStrength(terrain_),
                                viewport.getWidthProperty(), viewport.getHeightProperty());

        if (debugOverlayVisible_ && worldDebugVisible_) {
            const SoundscapeDebugState soundscapeDebug = soundscape_
                ? soundscape_->DebugState() : SoundscapeDebugState{};
            worldDebugRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                      playerMotion_.Position(), playerMotion_.GroundHeight(),
                                      playerMotion_.Mode() == LocomotionMode::Swimming,
                                      playerMotion_.MovementTrace(),
                                      worldCollisionGrid_.Obstacles(), birdFlock_.Birds(),
                                      fishSchools_.Fish(), fishSchools_.Schools(),
                                      sharkPatrol_.State(), monkeyTroop_.Monkeys(),
                                      soundscapeDebug);
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
            overlayState.cameraPosition = renderCameraPosition_;
            overlayState.freeCameraActive = IsDebugFreeCameraActive();
            overlayState.renderer = device.GetGraphicsRendererName();
            overlayState.swimming = playerMotion_.Mode() == LocomotionMode::Swimming;
            overlayState.groundHeight = playerMotion_.GroundHeight();
            overlayState.groundSlopeDegrees = playerMotion_.GroundSlopeRadians()
                * 180.0f / std::numbers::pi_v<float>;
            overlayState.offshoreDistance = playerMotion_.OffshoreDistance(terrain_);
            overlayState.oceanCurrentStrength = playerMotion_.OceanCurrentStrength(terrain_);
            overlayState.waterRippleIntensity = waterRippleModel_.Intensity();
            overlayState.waterRipplePhase = waterRippleModel_.Phase();
            overlayState.waterViewIntensity = waterViewModel_.Intensity();
            overlayState.waterViewDraws = waterViewOverlay_->DrawCallCount();
            overlayState.visibleTerrainCells = terrainRenderer_->VisibleCellCount();
            overlayState.totalTerrainCells = terrainRenderer_->TotalCellCount();
            overlayState.distantIslets = distantSceneryRenderer_->IsletCount();
            overlayState.distantSceneryDraws = distantSceneryRenderer_->DrawCallCount();
            overlayState.visibleContactShadows =
                contactShadowRenderer_->VisibleShadowCount();
            overlayState.visiblePalmShadows =
                contactShadowRenderer_->VisiblePalmShadowCount();
            overlayState.visibleCanopyShadows =
                contactShadowRenderer_->VisibleCanopyShadowCount();
            overlayState.visibleRockShadows =
                contactShadowRenderer_->VisibleRockShadowCount();
            overlayState.visibleWoodShadows =
                contactShadowRenderer_->VisibleWoodShadowCount();
            overlayState.visibleMonkeyShadows =
                contactShadowRenderer_->VisibleMonkeyShadowCount();
            overlayState.contactShadowDraws = contactShadowRenderer_->DrawCallCount();
            overlayState.visiblePalms = palmRenderer_->VisiblePalmCount();
            overlayState.totalPalms = palmRenderer_->TotalPalmCount();
            overlayState.nearPalms = palmRenderer_->VisibleNearPalmCount();
            overlayState.farPalms = palmRenderer_->VisibleFarPalmCount();
            overlayState.palmCells = palmRenderer_->VisibleCellCount();
            overlayState.palmDraws = palmRenderer_->DrawCallCount();
            overlayState.visibleCanopy = jungleRenderer_->VisibleCanopyCount();
            overlayState.totalCanopy = jungleRenderer_->TotalCanopyCount();
            overlayState.nearCanopy = jungleRenderer_->VisibleCanopyNearCount();
            overlayState.farCanopy = jungleRenderer_->VisibleCanopyFarCount();
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
            overlayState.visibleReefRocks = naturalPropRenderer_->VisibleReefRockCount();
            overlayState.totalReefRocks = naturalPropRenderer_->TotalReefRockCount();
            overlayState.visibleLogs = naturalPropRenderer_->VisibleLogCount();
            overlayState.totalLogs = naturalPropRenderer_->TotalLogCount();
            overlayState.visibleDriftwood = naturalPropRenderer_->VisibleDriftwoodCount();
            overlayState.totalDriftwood = naturalPropRenderer_->TotalDriftwoodCount();
            overlayState.visibleBeachDetails =
                naturalPropRenderer_->VisibleBeachDetailCount();
            overlayState.totalBeachDetails = naturalPropRenderer_->TotalBeachDetailCount();
            overlayState.visibleFallenFronds =
                naturalPropRenderer_->VisibleFallenFrondCount();
            overlayState.totalFallenFronds =
                naturalPropRenderer_->TotalFallenFrondCount();
            overlayState.propCells = naturalPropRenderer_->VisibleCellCount();
            overlayState.propDraws = naturalPropRenderer_->DrawCallCount();
            overlayState.visibleGrassPatches = grassRenderer_->VisiblePatchCount();
            overlayState.totalGrassPatches = grassRenderer_->TotalPatchCount();
            overlayState.grassCells = grassRenderer_->VisibleCellCount();
            overlayState.grassDraws = grassRenderer_->DrawCallCount();
            overlayState.grassInteractionStrength = grassInteractionModel_.Strength();
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
                overlayState.oceanCurrentAudioCue = soundscape_->WaterMix().currentCue;
                overlayState.playedWaveAccents = soundscape_->PlayedWaveAccentCount();
                overlayState.playedWaterTransitions =
                    soundscape_->PlayedWaterTransitionCount();
                overlayState.playedBirdCalls = soundscape_->PlayedBirdCallCount();
                overlayState.playedMonkeyCalls = soundscape_->PlayedMonkeyCallCount();
                overlayState.wildlifeCallActive = soundscape_->IsWildlifeCallActive();
                const SoundscapeDebugState soundscapeDebug = soundscape_->DebugState();
                overlayState.playingAudioSources = soundscapeDebug.playingSourceCount;
                overlayState.audioSourceBudget = soundscapeDebug.sourceBudget;
            }
            overlayState.worldDebugVisible = worldDebugVisible_;
            overlayState.collisionCandidates = static_cast<int>(
                worldCollisionGrid_.CandidateCount(player.x, player.z,
                                                   PlayerMotion::CapsuleRadius));
            overlayState.collisionObstacles = static_cast<int>(
                worldCollisionGrid_.Obstacles().size());
            if (worldDebugVisible_) {
                const PlayerMovementTrace& movementTrace = playerMotion_.MovementTrace();
                overlayState.worldDebugObstacles = worldDebugRenderer_->VisibleObstacleCount();
                overlayState.worldDebugCells = worldDebugRenderer_->VisibleCellCount();
                overlayState.worldDebugWildlife = worldDebugRenderer_->WildlifeMarkerCount();
                overlayState.worldDebugWildlifeTargets =
                    worldDebugRenderer_->WildlifeTargetCount();
                overlayState.worldDebugMonkeyHomes =
                    worldDebugRenderer_->MonkeyHomeRangeCount();
                overlayState.worldDebugAudioZones = worldDebugRenderer_->AudioZoneCount();
                overlayState.worldDebugAudioCandidates =
                    worldDebugRenderer_->AudioCandidateCount();
                overlayState.worldDebugActiveAudioEmitters =
                    worldDebugRenderer_->ActiveAudioEmitterCount();
                overlayState.worldDebugGroundSamples =
                    worldDebugRenderer_->GroundContactSampleCount();
                overlayState.worldDebugGroundSupports =
                    worldDebugRenderer_->GroundSupportSampleCount();
                overlayState.worldDebugSteepSamples =
                    worldDebugRenderer_->SteepGroundSampleCount();
                overlayState.movementObstacleRejections = movementTrace.obstacleRejections;
                overlayState.movementTerrainRejections = movementTrace.terrainRejections;
                overlayState.movementSlideSubsteps = movementTrace.slideSubsteps;
                overlayState.movementSafetyClamped = movementTrace.safetyClamped;
                overlayState.worldDebugLines = worldDebugRenderer_->LineCount();
                overlayState.worldDebugDraws = worldDebugRenderer_->DrawCallCount();
            }
            debugOverlay_->Draw(overlayState);
        }

        if (drawnFrames_ == 0U) {
            const PlayerPosition& player = playerMotion_.Position();
            const float currentStrength = playerMotion_.OceanCurrentStrength(terrain_);
            const float undertowTarget = WaterMovementModel::TargetMix(
                std::max(0.0f, -playerMotion_.GroundHeight()),
                playerMotion_.Mode() == LocomotionMode::Swimming,
                std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()),
                currentStrength).currentCue;
            std::cout << "island-simulator: visible terrain cells "
                      << terrainRenderer_->VisibleCellCount() << '/'
                      << terrainRenderer_->TotalCellCount() << ", collision candidates "
                      << worldCollisionGrid_.CandidateCount(
                             player.x, player.z, PlayerMotion::CapsuleRadius)
                      << '/' << worldCollisionGrid_.Obstacles().size() << ", palms "
                      << palmRenderer_->VisiblePalmCount() << '/'
                      << palmRenderer_->TotalPalmCount() << " (near "
                      << palmRenderer_->VisibleNearPalmCount() << ", far "
                      << palmRenderer_->VisibleFarPalmCount() << ") in "
                      << palmRenderer_->VisibleCellCount() << " cells, "
                      << palmRenderer_->DrawCallCount() << " draws; jungle "
                      << jungleRenderer_->VisibleCanopyCount() << '/'
                      << jungleRenderer_->TotalCanopyCount() << " canopy (near "
                      << jungleRenderer_->VisibleCanopyNearCount() << ", far "
                      << jungleRenderer_->VisibleCanopyFarCount() << "), "
                      << jungleRenderer_->VisibleUnderstoryCount() << '/'
                      << jungleRenderer_->TotalUnderstoryCount() << " understory, "
                      << jungleRenderer_->VisibleBushCount() << '/'
                      << jungleRenderer_->TotalBushCount() << " bushes, "
                      << jungleRenderer_->VisibleGroundPlantCount() << '/'
                      << jungleRenderer_->TotalGroundPlantCount() << " ground plants, "
                      << jungleRenderer_->DrawCallCount() << " draws; props "
                      << naturalPropRenderer_->VisibleRockCount() << '/'
                      << naturalPropRenderer_->TotalRockCount() << " rocks, "
                      << naturalPropRenderer_->VisibleReefRockCount() << '/'
                      << naturalPropRenderer_->TotalReefRockCount() << " reef outcrops, "
                      << naturalPropRenderer_->VisibleLogCount() << '/'
                      << naturalPropRenderer_->TotalLogCount() << " logs, "
                      << naturalPropRenderer_->VisibleDriftwoodCount() << '/'
                      << naturalPropRenderer_->TotalDriftwoodCount() << " driftwood, "
                      << naturalPropRenderer_->VisibleBeachDetailCount() << '/'
                      << naturalPropRenderer_->TotalBeachDetailCount()
                      << " shell/pebble clusters, "
                      << naturalPropRenderer_->VisibleFallenFrondCount() << '/'
                      << naturalPropRenderer_->TotalFallenFrondCount()
                      << " fallen fronds, "
                      << naturalPropRenderer_->DrawCallCount() << " draws; grass "
                      << grassRenderer_->VisiblePatchCount() << '/'
                      << grassRenderer_->TotalPatchCount() << " patches in "
                      << grassRenderer_->VisibleCellCount() << " cells, "
                      << grassRenderer_->DrawCallCount() << " draws, interaction "
                      << grassInteractionModel_.Strength() << ", custom shader "
                      << (grassRenderer_->UsesCustomShader() ? 1 : 0) << "; birds "
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
                      << monkeyRenderer_->CompletedCrossfadeCount()
                      << " active/completed; distant "
                      << distantSceneryRenderer_->IsletCount() << " islets, "
                      << distantSceneryRenderer_->DrawCallCount()
                      << " draw; contact shadows "
                      << contactShadowRenderer_->VisibleShadowCount() << ", "
                      << contactShadowRenderer_->PrimitiveCount() << " triangles, "
                      << contactShadowRenderer_->DrawCallCount()
                      << " draw; water view " << waterViewModel_.Intensity()
                      << ", " << waterViewOverlay_->DrawCallCount()
                      << " draw, player ripple " << waterRippleModel_.Intensity()
                      << ", phase " << waterRippleModel_.Phase() << ", current "
                      << currentStrength << ", undertow target " << undertowTarget << '\n';
            if (worldDebugVisible_) {
                const PlayerMovementTrace& movementTrace = playerMotion_.MovementTrace();
                std::cout << "island-simulator: world debug frame ground samples "
                          << worldDebugRenderer_->GroundContactSampleCount() << ", supports "
                          << worldDebugRenderer_->GroundSupportSampleCount()
                          << ", steep " << worldDebugRenderer_->SteepGroundSampleCount()
                          << "; wildlife markers "
                          << worldDebugRenderer_->WildlifeMarkerCount()
                          << ", targets " << worldDebugRenderer_->WildlifeTargetCount()
                          << ", homes " << worldDebugRenderer_->MonkeyHomeRangeCount()
                          << "; movement obstacle/terrain/slide/clamp "
                          << movementTrace.obstacleRejections << '/'
                          << movementTrace.terrainRejections << '/'
                          << movementTrace.slideSubsteps << '/'
                          << (movementTrace.safetyClamped ? 1 : 0)
                          << "; audio zones " << worldDebugRenderer_->AudioZoneCount()
                          << ", candidates "
                          << worldDebugRenderer_->AudioCandidateCount()
                          << ", active emitters "
                          << worldDebugRenderer_->ActiveAudioEmitterCount() << '\n';
            }
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
        if (benchmarkSceneSamplePending_) {
            RecordBenchmarkSceneSample();
            benchmarkSceneSamplePending_ = false;
        }
        if (benchmarkReportPending_) {
            WriteBenchmarkReport();
            benchmarkReportPending_ = false;
            Exit();
        }
    }

    GetTypeNameCPP(IslandGame, "IslandSimulator.IslandGame")
}
