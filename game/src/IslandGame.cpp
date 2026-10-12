#include "IslandSimulator/IslandGame.hpp"
#include "IslandSimulator/CanopyLighting.hpp"

#include "IslandSimulator/FirstPersonInput.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/GpuDiagnostics.hpp"
#include "IslandSimulator/PerformanceStatistics.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WorldIdentity.hpp"

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
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <emscripten/html5.h>
#endif

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsProfile.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "CNA/Input/Clipboard.hpp"
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
        constexpr float LocomotionModeNoticeDuration = 1.4f;

        [[nodiscard]] const char* ViewpointName(const FixedViewpoint viewpoint) noexcept
        {
            switch (viewpoint) {
            case FixedViewpoint::Overview: return "overview";
            case FixedViewpoint::BayOverview: return "bay";
            case FixedViewpoint::BayBeachDetail: return "bay-ground";
            case FixedViewpoint::HeadlandDetail: return "headland";
            case FixedViewpoint::BeachPath: return "beach";
            case FixedViewpoint::JungleTrail: return "jungle";
            case FixedViewpoint::EastClearingDetail: return "east";
            case FixedViewpoint::ClearingEdgeDetail: return "clearing-edge";
            case FixedViewpoint::CanopyInspection: return "canopy";
            case FixedViewpoint::WestClearingDetail: return "west";
            case FixedViewpoint::StreamDetail: return "stream";
            case FixedViewpoint::MangroveDetail: return "mangroves";
            case FixedViewpoint::GrassDetail: return "grass";
            case FixedViewpoint::GroundPlantDetail: return "ground";
            case FixedViewpoint::GroundBroadleafDetail: return "ground-broadleaf";
            case FixedViewpoint::BushDetail: return "bush";
            case FixedViewpoint::FallenLeafDetail: return "litter";
            case FixedViewpoint::GroundContactDetail: return "contact";
            case FixedViewpoint::MovementTraceDetail: return "movement";
            case FixedViewpoint::PalmDetail: return "palms";
            case FixedViewpoint::WaterlineDetail: return "water";
            case FixedViewpoint::WetSandDetail: return "wet-sand";
            case FixedViewpoint::CurrentBoundaryDetail: return "current";
            case FixedViewpoint::DistantSceneryDetail: return "distant";
            case FixedViewpoint::PropDetail: return "props";
            case FixedViewpoint::DriftwoodDetail: return "driftwood";
            case FixedViewpoint::ReefDetail: return "reef";
            case FixedViewpoint::ReefHabitatDetail: return "reef-habitat";
            case FixedViewpoint::FallenFrondDetail: return "fronds";
            case FixedViewpoint::BirdDetail: return "birds";
            case FixedViewpoint::FishDetail: return "fish";
            case FixedViewpoint::SharkDetail: return "shark";
            case FixedViewpoint::MonkeyDetail: return "monkeys";
            case FixedViewpoint::MonkeyTreeDetail: return "monkey-tree";
            case FixedViewpoint::MonkeyClimbDetail: return "monkey-climb";
            case FixedViewpoint::ShipwreckDetail: return "shipwreck";
            case FixedViewpoint::HutDetail: return "hut";
            case FixedViewpoint::HutMaterialDetail: return "hut-material";
            case FixedViewpoint::CrabDetail: return "crabs";
            case FixedViewpoint::BoarDetail: return "boars";
            case FixedViewpoint::GoatDetail: return "goats";
            case FixedViewpoint::SpiderDetail: return "spiders";
            case FixedViewpoint::SpiderBrightDetail: return "spiders-bright";
            case FixedViewpoint::SnakeDetail: return "snakes";
            case FixedViewpoint::SnakeHeadDetail: return "snake-head";
            case FixedViewpoint::ButterflyDetail: return "butterflies";
            case FixedViewpoint::SeaTurtleDetail: return "turtles";
            case FixedViewpoint::BenchmarkTraversal: return "traversal";
            case FixedViewpoint::None: return "interactive";
            }
            return "unknown";
        }

        [[nodiscard]] const char* TouchSensitivityName(
            const TouchLookSensitivityLevel sensitivity) noexcept
        {
            switch (sensitivity) {
            case TouchLookSensitivityLevel::Low: return "low";
            case TouchLookSensitivityLevel::Standard: return "standard";
            case TouchLookSensitivityLevel::High: return "high";
            }
            return "standard";
        }
    }

    IslandGame::IslandGame(const bool smokeTest, std::string capturePath,
                           FixedViewpoint viewpoint, const bool showDebugOverlay,
                           const bool inputDiagnostics, const bool showWorldDebug,
                           std::string benchmarkPath, std::string startupReportPath,
                           const bool userSettingsEnabled,
                           std::optional<CanopyLodInspection> canopyLodInspection)
        : graphics_(this),
          birdFlock_(terrain_),
          butterflyMeadow_(terrain_),
          crabColony_(terrain_),
          boarHerd_(terrain_),
          fishSchools_(terrain_),
          seaTurtleGroup_(terrain_),
          sharkPatrol_(terrain_),
          monkeyTroop_(terrain_),
          vegetationLayout_(terrain_),
          jungleLayout_(terrain_),
          canopyLightField_(terrain_, MakeCanopyOccluders(jungleLayout_.Canopy())),
          grassLayout_(terrain_, canopyLightField_, groundCoverField_),
          naturalPropLayout_(terrain_, vegetationLayout_, jungleLayout_),
          shipwreckLayout_(terrain_),
          cameraPosition_(0.0f, 3.0f, -225.0f),
          renderCameraPosition_(cameraPosition_),
          smokeTest_(smokeTest),
          viewpoint_(viewpoint),
          capturePath_(std::move(capturePath)),
          canopyLodInspection_(canopyLodInspection),
          debugOverlayVisible_(showDebugOverlay),
          worldDebugVisible_(showWorldDebug),
          userSettingsEnabled_(userSettingsEnabled),
          inputDiagnostics_(inputDiagnostics),
          benchmarkPath_(std::move(benchmarkPath)),
          startupReportPath_(std::move(startupReportPath))
    {
        std::vector<CircularObstacle> worldObstacles;
        worldObstacles.reserve(vegetationLayout_.Obstacles().size()
                               + jungleLayout_.Obstacles().size()
                               + naturalPropLayout_.Obstacles().size()
                               + shipwreckLayout_.Obstacles().size());
        worldObstacles.insert(worldObstacles.end(), vegetationLayout_.Obstacles().begin(),
                              vegetationLayout_.Obstacles().end());
        worldObstacles.insert(worldObstacles.end(), jungleLayout_.Obstacles().begin(),
                              jungleLayout_.Obstacles().end());
        worldObstacles.insert(worldObstacles.end(), naturalPropLayout_.Obstacles().begin(),
                              naturalPropLayout_.Obstacles().end());
        worldObstacles.insert(worldObstacles.end(), shipwreckLayout_.Obstacles().begin(),
                              shipwreckLayout_.Obstacles().end());
        hutPlacement_=HutLayout::FindInWorld(terrain_,jungleLayout_,worldObstacles);
        if(hutPlacement_) worldObstacles.push_back({hutPlacement_->x,hutPlacement_->z,HutLayout::CollisionRadius});
        worldCollisionGrid_.Reset(worldObstacles);
        monkeyTroop_.SetObstacles(worldCollisionGrid_.Obstacles());
        monkeyTroop_.SetCanopy(jungleLayout_.Canopy());
        boarHerd_.SetObstacles(worldCollisionGrid_.Obstacles());
        snakes_ = std::make_unique<SnakePopulation>(
            terrain_,jungleLayout_,worldCollisionGrid_.Obstacles());
        goats_ = std::make_unique<GoatHerd>(terrain_,jungleLayout_,worldCollisionGrid_.Obstacles());
        spiders_ = std::make_unique<SpiderColony>(terrain_,jungleLayout_,worldCollisionGrid_.Obstacles());
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
        } else if (viewpoint_ == FixedViewpoint::BayOverview) {
            cameraPosition_ = Vector3(-365.0f, 115.0f, 0.0f);
            yaw_ = std::numbers::pi_v<float> * 0.5f;
            pitch_ = -0.38f;
        } else if (viewpoint_ == FixedViewpoint::BayBeachDetail) {
            constexpr float x = -147.0f, z = -8.0f;
            cameraPosition_ = Vector3(x, terrain_.Height(x, z) + PlayerMotion::EyeHeight, z);
            yaw_ = 0.12f;
            pitch_ = -0.07f;
        } else if (viewpoint_ == FixedViewpoint::HeadlandDetail) {
            cameraPosition_ = Vector3(300.0f, 28.0f, 110.0f);
            yaw_ = std::atan2(-63.0f, -23.0f);
            pitch_ = -0.33f;
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
        } else if (viewpoint_ == FixedViewpoint::EastClearingDetail) {
            const ClearingDefinition& clearing = VegetationLayout::EastClearing;
            cameraPosition_ = Vector3(clearing.x, terrain_.Height(clearing.x, clearing.z)
                                                   + PlayerMotion::EyeHeight,
                                      clearing.z);
            yaw_ = -std::numbers::pi_v<float> * 0.5f;
            pitch_ = -0.085f;
            std::cout << "island-simulator: east clearing QA camera at authored centre\n";
        } else if (viewpoint_ == FixedViewpoint::ClearingEdgeDetail) {
            // Fixed world coordinates keep comparisons stable if colony placement changes.
            const auto& clearing = VegetationLayout::EastClearing;
            cameraPosition_ = Vector3(clearing.x + 26, terrain_.Height(clearing.x + 26, clearing.z + 6)
                + PlayerMotion::EyeHeight, clearing.z + 6);
            const Vector3 target(clearing.x + 40, terrain_.Height(clearing.x + 40, clearing.z + 10) + 0.4f,
                clearing.z + 10);
            const auto direction = target - cameraPosition_;
            yaw_ = std::atan2(direction.X, direction.Z);
            pitch_ = std::atan2(direction.Y, std::hypot(direction.X, direction.Z));
        } else if (viewpoint_ == FixedViewpoint::StreamDetail) {
            cameraPosition_=Vector3(-110,terrain_.Height(-110,-37)+3.5f,-37);
            const Vector3 target(-122,terrain_.Height(-122,-26.94f)-.18f,-26.94f);
            const auto direction=target-cameraPosition_;
            yaw_=std::atan2(direction.X,direction.Z);
            pitch_=std::atan2(direction.Y,std::hypot(direction.X,direction.Z));
        } else if (viewpoint_ == FixedViewpoint::MangroveDetail) {
            const auto selected = std::max_element(jungleLayout_.Canopy().begin(),
                jungleLayout_.Canopy().end(), [](const CanopyPlacement& left,
                                                  const CanopyPlacement& right) {
                    const float leftRadius = left.variant == MangroveCanopyVariant
                        ? std::hypot(left.x, left.z) : -1.0f;
                    const float rightRadius = right.variant == MangroveCanopyVariant
                        ? std::hypot(right.x, right.z) : -1.0f;
                    return leftRadius < rightRadius;
                });
            if (selected == jungleLayout_.Canopy().end()
                || selected->variant != MangroveCanopyVariant)
                throw std::runtime_error("Mangrove QA camera found no target");
            const float radius = std::hypot(selected->x, selected->z);
            const float outwardX = selected->x / radius;
            const float outwardZ = selected->z / radius;
            const float cameraX = selected->x + outwardX * 8.5f - outwardZ * 2.2f;
            const float cameraZ = selected->z + outwardZ * 8.5f + outwardX * 2.2f;
            cameraPosition_ = Vector3(cameraX, selected->y + PlayerMotion::EyeHeight, cameraZ);
            const Vector3 target(selected->x, selected->y + 3.4f * selected->scale, selected->z);
            const auto direction = target - cameraPosition_;
            yaw_ = std::atan2(direction.X, direction.Z);
            pitch_ = std::atan2(direction.Y, std::hypot(direction.X, direction.Z));
            std::cout << "island-simulator: mangrove QA target exposes stilt roots and low crown\n";
        } else if (viewpoint_ == FixedViewpoint::WestClearingDetail) {
            const ClearingDefinition& clearing = VegetationLayout::WestClearing;
            cameraPosition_ = Vector3(clearing.x, terrain_.Height(clearing.x, clearing.z)
                                                   + PlayerMotion::EyeHeight,
                                      clearing.z);
            yaw_ = std::numbers::pi_v<float> * 0.25f;
            pitch_ = -0.085f;
            std::cout << "island-simulator: west clearing QA camera at authored centre\n";
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
        } else if (viewpoint_ == FixedViewpoint::BushDetail) {
            const BushPlacement* selected = nullptr;
            float bestScore = std::numeric_limits<float>::max();
            float selectedCameraX=0,selectedCameraZ=0;
            for (const auto& bush : jungleLayout_.Bushes()) {
                if (bush.variant != 0) continue;
                const float pathDistance = std::abs(bush.x - VegetationLayout::PrimaryPathCenterX(bush.z));
                for (int heading=0;heading<8;++heading) {
                    const float angle=heading*std::numbers::pi_v<float>/4;
                    const float offsetX=std::cos(angle)*3.8f,offsetZ=std::sin(angle)*3.8f;
                    const auto distanceToSightline=[&](float x,float z) {
                        const float t=std::clamp(((x-bush.x)*offsetX+(z-bush.z)*offsetZ)/(3.8f*3.8f),0.0f,1.0f);
                        return std::hypot(x-bush.x-offsetX*t,z-bush.z-offsetZ*t);
                    };
                    bool obstructed=false;
                    for (const auto& plant:jungleLayout_.Understory()) {
                        if (distanceToSightline(plant.x,plant.z)<1.8f) { obstructed=true; break; }
                    }
                    if (obstructed) continue;
                    for (const auto& obstacle:worldCollisionGrid_.Obstacles()) {
                        if (distanceToSightline(obstacle.x,obstacle.z)<obstacle.radius+0.65f) {
                            obstructed=true; break;
                        }
                    }
                    if (obstructed) continue;
                    const float score=std::abs(terrain_.SignedShoreDistance(bush.x,bush.z)-70.0f)
                        + std::abs(pathDistance-5.0f)*0.4f - bush.scale*2.0f;
                    if (score<bestScore) {
                        bestScore=score; selected=&bush;
                        selectedCameraX=bush.x+offsetX; selectedCameraZ=bush.z+offsetZ;
                    }
                }
            }
            if (!selected) throw std::runtime_error("Bush QA camera found no unobstructed variant 0 target");
            cameraPosition_=Vector3(selectedCameraX,terrain_.Height(selectedCameraX,selectedCameraZ)
                +PlayerMotion::EyeHeight,selectedCameraZ);
            const Vector3 target(selected->x,selected->y+0.6f*selected->scale,selected->z);
            const Vector3 delta=target-cameraPosition_;
            yaw_=std::atan2(delta.X,delta.Z);
            pitch_=std::atan2(delta.Y,std::hypot(delta.X,delta.Z));
            std::cout << "island-simulator: bush QA target variant 0 at player-eye detail distance\n";
        } else if ((viewpoint_ == FixedViewpoint::GroundPlantDetail
                    || viewpoint_ == FixedViewpoint::GroundBroadleafDetail
                    || viewpoint_ == FixedViewpoint::FallenLeafDetail)
                   && !jungleLayout_.GroundPlants().empty()) {
            const std::uint8_t targetVariant = viewpoint_ == FixedViewpoint::FallenLeafDetail
                ? 0U : (viewpoint_ == FixedViewpoint::GroundBroadleafDetail ? 1U : 2U);
            const GroundPlantPlacement* selected = nullptr;
            float bestScore = std::numeric_limits<float>::max();
            for (const GroundPlantPlacement& plant : jungleLayout_.GroundPlants()) {
                if (plant.variant != targetVariant) continue;
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
                    if (plant.variant == targetVariant) {
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
            std::cout << "island-simulator: "
                      << (viewpoint_ == FixedViewpoint::FallenLeafDetail
                              ? "ground-litter" : (viewpoint_ == FixedViewpoint::GroundBroadleafDetail
                                  ? "ground-broadleaf" : "ground"))
                      << " QA target variant "
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
                // Inspect a representative sward, not an isolated tuft on exposed humus.
                const TerrainSample coverSample = terrain_.Sample(patch.x, patch.z);
                if (GroundCover::Density(std::clamp(canopyLightField_.Sample(patch.x, patch.z), 0.0f, 1.0f),
                        std::clamp(groundCoverField_.SamplePatch(patch.x, patch.z), 0.0f, 1.0f),
                        coverSample.signedShoreDistance, coverSample.slopeRadians) < 0.6f) continue;
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
        } else if (viewpoint_ == FixedViewpoint::WetSandDetail) {
            const float x = -(terrain_.ShoreRadius(std::numbers::pi_v<float>) - 1.0f);
            cameraPosition_ = Vector3(x, terrain_.Height(x, 0.0f) + PlayerMotion::EyeHeight, 0.0f);
            yaw_ = 0.0f;
            pitch_ = -0.40f;
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
            const auto selected = std::find_if(
                naturalPropLayout_.Driftwood().begin(), naturalPropLayout_.Driftwood().end(),
                [](const DriftwoodPlacement& wood) {
                    return wood.coveLandmark && wood.variant == 2U;
                });
            const DriftwoodPlacement& wood = selected != naturalPropLayout_.Driftwood().end()
                ? *selected : naturalPropLayout_.Driftwood().front();
            float targetX = 0.0f;
            float targetY = 0.0f;
            float targetZ = 0.0f;
            int landmarkCount = 0;
            for (const DriftwoodPlacement& candidate : naturalPropLayout_.Driftwood()) {
                if (!candidate.coveLandmark) continue;
                targetX += candidate.x;
                targetY += candidate.y + candidate.radius;
                targetZ += candidate.z;
                ++landmarkCount;
            }
            if (landmarkCount > 0) {
                targetX /= static_cast<float>(landmarkCount);
                targetY /= static_cast<float>(landmarkCount);
                targetZ /= static_cast<float>(landmarkCount);
            } else {
                targetX = wood.x;
                targetY = wood.y + wood.radius;
                targetZ = wood.z;
            }
            const float targetRadius = std::hypot(targetX, targetZ);
            const float outwardX = targetRadius > 0.001f ? targetX / targetRadius : 0.0f;
            const float outwardZ = targetRadius > 0.001f ? targetZ / targetRadius : -1.0f;
            const float cameraX = targetX - outwardX * 12.0f - outwardZ * 3.5f;
            const float cameraZ = targetZ - outwardZ * 12.0f + outwardX * 3.5f;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            yaw_ = std::atan2(targetX - cameraX, targetZ - cameraZ);
            pitch_ = std::atan2(targetY - cameraPosition_.Y,
                                std::hypot(targetX - cameraX, targetZ - cameraZ));
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
        } else if (viewpoint_ == FixedViewpoint::ReefHabitatDetail) {
            const ReefHabitatPlacement* selected = nullptr;
            int selectedNeighbours = -1;
            for (const auto& candidate : naturalPropLayout_.ReefHabitat()) {
                if (candidate.variant >= 3U) continue;
                int neighbours = 0;
                for (const auto& other : naturalPropLayout_.ReefHabitat()) {
                    if (&candidate != &other && other.variant < 3U
                        && std::hypot(candidate.x - other.x, candidate.z - other.z) < 16.0f) {
                        ++neighbours;
                    }
                }
                if (neighbours > selectedNeighbours
                    || (neighbours == selectedNeighbours
                        && (!selected || candidate.scale > selected->scale))) {
                    selected = &candidate;
                    selectedNeighbours = neighbours;
                }
            }
            if (!selected) throw std::runtime_error("Reef-habitat QA camera found no target");
            const Vector3 radial = Vector3::Normalize(Vector3(selected->x, 0, selected->z));
            const Vector3 tangent(-radial.Z, 0, radial.X);
            cameraPosition_ = Vector3(selected->x - radial.X * 1.4f + tangent.X * 4.3f,
                                      std::min(-.35f, selected->y + 1.05f),
                                      selected->z - radial.Z * 1.4f + tangent.Z * 4.3f);
            const Vector3 target(selected->x, selected->y + .42f * selected->scale,
                                 selected->z);
            const auto direction = target - cameraPosition_;
            yaw_ = std::atan2(direction.X, direction.Z);
            pitch_ = std::atan2(direction.Y, std::hypot(direction.X, direction.Z));
            std::cout << "island-simulator: reef habitat QA camera underwater with "
                      << selectedNeighbours << " neighbours\n";
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
                - flightForward * 12.0f + flightRight * 3.2f - Vector3::Up * 2.5f;
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
        } else if (viewpoint_ == FixedViewpoint::SeaTurtleDetail
                   && !seaTurtleGroup_.Turtles().empty()) {
            const SeaTurtleState& turtle = seaTurtleGroup_.Turtles()[2U];
            const Vector3 radial = Vector3::Normalize(Vector3(turtle.x, 0.0f, turtle.z));
            const Vector3 tangent(-radial.Z, 0.0f, radial.X);
            cameraPosition_ = Vector3(turtle.x, turtle.y, turtle.z)
                + radial * 4.1f + tangent * 1.1f + Vector3::Up * 1.75f;
            const Vector3 toTurtle = Vector3(turtle.x, turtle.y, turtle.z) - cameraPosition_;
            yaw_ = std::atan2(toTurtle.X, toTurtle.Z);
            pitch_ = std::atan2(toTurtle.Y, std::hypot(toTurtle.X, toTurtle.Z));
            std::cout << "island-simulator: sea-turtle QA camera views a surfacing route\n";
        } else if (viewpoint_ == FixedViewpoint::SharkDetail) {
            const SharkState& shark = sharkPatrol_.State();
            const Vector3 radial = Vector3::Normalize(Vector3(shark.x, 0.0f, shark.z));
            const Vector3 tangent(-radial.Z, 0.0f, radial.X);
            cameraPosition_ = Vector3(shark.x, shark.y, shark.z)
                + radial * 14.0f + tangent * 3.5f + Vector3::Up * 5.8f;
            const Vector3 toShark = Vector3(shark.x, shark.y, shark.z) - cameraPosition_;
            yaw_ = std::atan2(toShark.X, toShark.Z);
            pitch_ = std::atan2(toShark.Y, std::hypot(toShark.X, toShark.Z));
        } else if (viewpoint_ == FixedViewpoint::MonkeyDetail || viewpoint_ == FixedViewpoint::MonkeyTreeDetail
            || viewpoint_ == FixedViewpoint::MonkeyClimbDetail) {
            if (viewpoint_ == FixedViewpoint::MonkeyTreeDetail) {
                monkeyTroop_.SetClimbingEnabled(false); // Preserve the separate inspection-only regression.
                const MonkeyTroop initialTroop(monkeyTroop_);
                bool ready = false;
                for (int step = 0; step < 12000 && !ready; ++step) {
                    const auto subject = monkeyTroop_.Monkeys().front();
                    monkeyTroop_.Update(1.0f/30.0f,subject.x+12,subject.y,subject.z);
                    for (std::size_t index = 0; index < monkeyTroop_.Monkeys().size() && !ready; ++index) {
                        const auto& current = monkeyTroop_.Monkeys()[index];
                        ready = current.behavior == MonkeyBehavior::InspectTree
                            && current.treeInspectionSeconds >= 2.7f;
                        if (ready) monkeyQaSubjectIndex_ = index;
                    }
                }
                if (!ready) throw std::runtime_error("monkey-tree inspection could not reach a tree in bounded simulation");
                std::cout << "island-simulator: monkey-tree QA holds naturally reached inspection; wind remains live\n";
                if (!capturePath_.empty()) {
                    const auto selected = monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
                    const float startClock = selected.animationSeconds-selected.treeInspectionSeconds;
                    MonkeyTroop replay(initialTroop);
                    constexpr std::array<float,7> phaseTimes{.05f,1.35f,2.1f,3.3f,4.8f,5.4f,5.8f};
                    bool entered = false;
                    for (int step = 0; step < 12240 && monkeyTreeQaPoses_.size() < 8; ++step) {
                        const auto observer = replay.Monkeys().front();
                        replay.Update(1.0f/30.0f,observer.x+12,observer.y,observer.z);
                        const auto& current = replay.Monkeys()[monkeyQaSubjectIndex_];
                        const bool sameInspection = current.behavior == MonkeyBehavior::InspectTree
                            && current.treeIndex == selected.treeIndex
                            && std::abs(current.animationSeconds-current.treeInspectionSeconds-startClock) < .01f;
                        if (sameInspection) {
                            entered = true;
                            if (monkeyTreeQaPoses_.size() < phaseTimes.size()
                                && current.treeInspectionSeconds >= phaseTimes[monkeyTreeQaPoses_.size()])
                                monkeyTreeQaPoses_.emplace_back(replay.Monkeys().begin(),replay.Monkeys().end());
                        } else if (entered) {
                            if (monkeyTreeQaPoses_.size() != phaseTimes.size())
                                throw std::runtime_error("monkey-tree transfer interrupted before all phases");
                            monkeyTreeQaPoses_.emplace_back(replay.Monkeys().begin(),replay.Monkeys().end());
                        }
                    }
                    if (monkeyTreeQaPoses_.size() != 8)
                        throw std::runtime_error("monkey-tree replay did not retain all eight transfer stages");
                }
            }
            if (viewpoint_ == FixedViewpoint::MonkeyClimbDetail) {
                const MonkeyTroop initial(monkeyTroop_);
                bool ready=false;
                for (int step=0;step<18000 && !ready;++step) {
                    const auto observer=monkeyTroop_.Monkeys().front();
                    monkeyTroop_.Update(1.0f/30,observer.x+12,observer.y,observer.z);
                    for (std::size_t i=0;i<monkeyTroop_.Monkeys().size();++i) {
                        const auto& current=monkeyTroop_.Monkeys()[i];
                        if (current.treeExcursion && !current.treeClock.returning && current.treeClock.seconds>=18.5f) {
                            monkeyQaSubjectIndex_=i; ready=true; break;
                        }
                    }
                }
                if (!ready) throw std::runtime_error("No natural monkey climb reached summit in bounded QA warmup");
                const auto selected=monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
                const float start=selected.animationSeconds-selected.treeClock.seconds;
                MonkeyTroop replay(initial);
                constexpr std::array<float,7> times{0,5,10.1f,18.5f,24,29.1f,38.9f};
                bool entered=false;
                for (int step=0;step<19500 && monkeyTreeQaPoses_.size()<8;++step) {
                    const auto observer=replay.Monkeys().front();
                    replay.Update(1.0f/30,observer.x+12,observer.y,observer.z);
                    const auto& current=replay.Monkeys()[monkeyQaSubjectIndex_];
                    const bool same=current.treeExcursion && current.treeIndex==selected.treeIndex
                        && std::abs(current.animationSeconds-current.treeClock.seconds-start)<.02f;
                    if (same) {
                        entered=true;
                        if (monkeyTreeQaPoses_.size()<times.size()
                            && current.treeClock.seconds>=times[monkeyTreeQaPoses_.size()])
                            monkeyTreeQaPoses_.emplace_back(replay.Monkeys().begin(),replay.Monkeys().end());
                    } else if (entered && !current.treeReleasing) {
                        if (monkeyTreeQaPoses_.size()!=7) throw std::runtime_error("Natural monkey climb interrupted during QA replay");
                        monkeyTreeQaPoses_.emplace_back(replay.Monkeys().begin(),replay.Monkeys().end());
                    }
                }
                if (monkeyTreeQaPoses_.size()!=8) throw std::runtime_error("Monkey climb QA missing cycle stages");
            }
            const MonkeyState& monkey = monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
            const Vector3 forward(std::cos(monkey.headingRadians), 0.0f,
                                  std::sin(monkey.headingRadians));
            const Vector3 side(-forward.Z, 0.0f, forward.X);
            cameraPosition_ = Vector3(monkey.x, monkey.y, monkey.z)
                + forward * 0.6f - side * 2.1f + Vector3::Up * 1.10f;
            if (viewpoint_==FixedViewpoint::MonkeyClimbDetail)
                cameraPosition_=Vector3(monkey.x,monkey.y,monkey.z)+forward*.65f-side*5.0f+Vector3::Up*2.2f;
            std::ostringstream monkeyCameraReport;
            monkeyCameraReport << "island-simulator: monkey QA subject " << monkey.x << ','
                << monkey.y << ',' << monkey.z << "; camera " << cameraPosition_.X << ','
                << cameraPosition_.Y << ',' << cameraPosition_.Z << "; camera terrain "
                << terrain_.Height(cameraPosition_.X, cameraPosition_.Z);
            std::cout << monkeyCameraReport.str() << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(monkeyCameraReport.str());
            const Vector3 toMonkey = Vector3(monkey.x, monkey.y + (viewpoint_==FixedViewpoint::MonkeyClimbDetail ? 1.8f : .52f), monkey.z)
                - cameraPosition_;
            yaw_ = std::atan2(toMonkey.X, toMonkey.Z);
            pitch_ = std::atan2(toMonkey.Y, std::hypot(toMonkey.X, toMonkey.Z));
        } else if (viewpoint_ == FixedViewpoint::ShipwreckDetail) {
            const ShipwreckPlacement& ship = shipwreckLayout_.Ship();
            const float radius = std::hypot(ship.x, ship.z);
            const float outwardX = ship.x / radius;
            const float outwardZ = ship.z / radius;
            const float tangentX = -outwardZ;
            const float tangentZ = outwardX;
            const float cameraX = ship.x - outwardX * 17.5f + tangentX * 4.8f;
            const float cameraZ = ship.z - outwardZ * 17.5f + tangentZ * 4.8f;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            const Vector3 toShip = Vector3(ship.x, 1.2f, ship.z) - cameraPosition_;
            yaw_ = std::atan2(toShip.X, toShip.Z);
            pitch_ = std::atan2(toShip.Y, std::hypot(toShip.X, toShip.Z));
            std::cout << "island-simulator: shipwreck QA camera views the 20m wreck "
                         "from the beach\n";
        } else if (viewpoint_ == FixedViewpoint::HutDetail || viewpoint_ == FixedViewpoint::HutMaterialDetail) {
            if(!hutPlacement_) throw std::runtime_error("Hut QA requires a valid site");
            const auto& h=*hutPlacement_;
            const float c=std::cos(h.yawRadians),s=std::sin(h.yawRadians);
            const bool close=viewpoint_==FixedViewpoint::HutMaterialDetail;
            const float x=close?3.8f:6.0f,z=close?4.8f:9.0f;
            cameraPosition_=Vector3(h.x+c*x+s*z,h.floorY+3.2f,h.z-s*x+c*z);
            const Vector3 direction=Vector3(h.x,h.floorY+(close?2.0f:1.5f),h.z)-cameraPosition_;
            yaw_=std::atan2(direction.X,direction.Z);
            pitch_=std::atan2(direction.Y,std::hypot(direction.X,direction.Z));
        } else if (viewpoint_ == FixedViewpoint::SnakeDetail || viewpoint_ == FixedViewpoint::SnakeHeadDetail) {
            const auto& snake = snakes_->Snakes().front();
            const auto head = snake.body.front();
            Vector3 centre = Vector3::Zero;
            for (const auto& p : snake.body) centre += Vector3(p.x,p.y,p.z);
            centre /= static_cast<float>(SnakeState::StationCount);
            // Frame the entire body from above; the old oblique head-relative
            // camera crossed nearby broad leaves and lost the retreating head.
            cameraPosition_ = centre + Vector3(0.0f,2.0f,0.35f);
            if(viewpoint_ == FixedViewpoint::SnakeHeadDetail)
                cameraPosition_ = Vector3(head.x+0.35f,head.y+0.25f,head.z+0.30f);
            const Vector3 target = viewpoint_ == FixedViewpoint::SnakeHeadDetail
                ? Vector3(head.x+0.01f,head.y,head.z) : centre;
            const Vector3 direction = target-cameraPosition_;
            yaw_ = std::atan2(direction.X,direction.Z);
            pitch_ = std::atan2(direction.Y,std::hypot(direction.X,direction.Z));
        } else if (UsesSpiderViewpoint()) {
            if(viewpoint_==FixedViewpoint::SpiderBrightDetail) {
                float brightest=-1;
                for(std::size_t i=0;i<spiders_->Spiders().size();++i) {
                    const auto& candidate=spiders_->Spiders()[i];
                    const float transmission=canopyLightField_.SampleReceiver(
                        candidate.x,candidate.y+.04f,candidate.z,candidate.y);
                    std::ostringstream row;row.precision(9);
                    row << "island-simulator: spider habitat candidate=" << i
                        << " transmission=" << transmission;
                    std::cout << row.str() << std::endl;MirrorRuntimeDiagnosticToPlatformLog(row.str());
                    if(transmission>brightest) {brightest=transmission;spiderViewSubject_=i;}
                }
                const auto selected="island-simulator: spider habitat selected="+std::to_string(spiderViewSubject_);
                std::cout << selected << std::endl;MirrorRuntimeDiagnosticToPlatformLog(selected);
            }
            const auto& spider=spiders_->Spiders()[spiderViewSubject_];
            cameraPosition_=Vector3(spider.x+.36f,spider.y+.28f,spider.z+.45f);
            const auto direction=Vector3(spider.x,spider.y+.035f,spider.z)-cameraPosition_;
            yaw_=std::atan2(direction.X,direction.Z);
            pitch_=std::atan2(direction.Y,std::hypot(direction.X,direction.Z));
        } else if (viewpoint_ == FixedViewpoint::GoatDetail) {
            const auto& goat=goats_->Goats().front();
            const float x=goat.x+7.5f,z=goat.z+4.0f;
            cameraPosition_=Vector3(x,std::max(terrain_.Height(x,z)+.4f,goat.y+2.1f),z);
            const auto direction=Vector3(goat.x,goat.y+.75f,goat.z)-cameraPosition_;
            yaw_=std::atan2(direction.X,direction.Z);
            pitch_=std::atan2(direction.Y,std::hypot(direction.X,direction.Z));
        } else if (viewpoint_ == FixedViewpoint::BoarDetail) {
            const auto& boar = boarHerd_.Boars().front();
            // Telephoto inspection outside the 6m retreat trigger; habitat stays unchanged.
            const float c=std::cos(boar.headingRadians),s=std::sin(boar.headingRadians);
            const float x = boar.x + c*2.8f + s*6.6f;
            const float z = boar.z + s*2.8f - c*6.6f;
            cameraPosition_ = Vector3(x, std::max(terrain_.Height(x,z)+0.4f,boar.y+2.4f), z);
            const Vector3 direction = Vector3(boar.x, boar.y + 0.5f, boar.z) - cameraPosition_;
            yaw_ = std::atan2(direction.X, direction.Z);
            pitch_ = std::atan2(direction.Y, std::hypot(direction.X, direction.Z));
            std::cout << "island-simulator: boar QA camera views non-hostile inland wildlife\n";
        } else if (viewpoint_ == FixedViewpoint::CrabDetail
                   && crabColony_.Crabs().size() > 2U) {
            const CrabState& crab = crabColony_.Crabs()[2U];
            const float radius = std::hypot(crab.x, crab.z);
            const float outwardX = crab.x / radius;
            const float outwardZ = crab.z / radius;
            const float tangentX = -outwardZ;
            const float tangentZ = outwardX;
            const float cameraX = crab.x - outwardX * 6.0f + tangentX * 1.5f;
            const float cameraZ = crab.z - outwardZ * 6.0f + tangentZ * 1.5f;
            cameraPosition_ = Vector3(cameraX, terrain_.Height(cameraX, cameraZ)
                                                   + PlayerMotion::EyeHeight,
                                      cameraZ);
            const Vector3 toCrab = Vector3(crab.x, crab.y + 0.16f, crab.z)
                - cameraPosition_;
            yaw_ = std::atan2(toCrab.X, toCrab.Z);
            pitch_ = std::atan2(toCrab.Y, std::hypot(toCrab.X, toCrab.Z));
            std::cout << "island-simulator: crab QA camera views an active beach colony\n";
        } else if (viewpoint_ == FixedViewpoint::ButterflyDetail
                   && butterflyMeadow_.Butterflies().size() > 2U) {
            const ButterflyHomeRange& home = butterflyMeadow_.Homes()[2U];
            const float cameraX = home.x - 3.0f;
            const float cameraZ = home.z - 2.2f;
            cameraPosition_ = Vector3(
                cameraX, terrain_.Height(cameraX, cameraZ) + 1.18f, cameraZ);
            const Vector3 toGroup = Vector3(
                home.x, terrain_.Height(home.x, home.z) + 0.72f, home.z) - cameraPosition_;
            yaw_ = std::atan2(toGroup.X, toGroup.Z);
            pitch_ = std::atan2(
                toGroup.Y, std::hypot(toGroup.X, toGroup.Z));
            std::cout << "island-simulator: butterfly QA camera views a sunny clearing group\n";
        }
        graphics_.setGraphicsProfileProperty(GraphicsProfile::HiDef);
        graphics_.setPreferMultiSamplingProperty(qualityProfile_.preferredBackBufferSamples > 0);
        // PreferMultiSampling alone requests up to eight samples in current CNA. Bound the
        // request through its supported preparation event, including future device resets.
        // Capture only the immutable policy value; this callback does not depend on game lifetime.
        graphics_.PreparingDeviceSettings.Add(
            [samples = qualityProfile_.preferredBackBufferSamples](
                System::Object*, const PreparingDeviceSettingsEventArgs& args) {
                args.getGraphicsDeviceInformationEXT().getPresentationParametersProperty()
                    .setMultiSampleCountProperty(samples);
            });
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
        // Capture-only clock: no wall-time catch-up updates between recorded frames.
        setIsFixedTimeStepProperty(benchmarkPath_.empty() && !UsesSnakeCaptureClock() && !UsesGoatCaptureClock() && !UsesSpiderCaptureClock() && !UsesBoarCaptureClock());
        InitializeUserSettings();
#if defined(__EMSCRIPTEN__)
        soundscapeNeedsGesture_ = true;
#endif
        constructionFinished_ = std::chrono::steady_clock::now();
        worldGenerationMilliseconds_ = std::chrono::duration<double, std::milli>(
            constructionFinished_ - constructionStarted_).count();
    }

    void IslandGame::InitializeUserSettings()
    {
        if (!userSettingsEnabled_) {
            std::cout << "island-simulator: user settings disabled for isolated QA run\n";
            return;
        }

        std::string error;
        userSettingsStore_ = UserSettingsStore::Open(error);
        if (!userSettingsStore_) {
            const std::string diagnostic =
                "island-simulator: user settings unavailable; using defaults: " + error;
            std::cerr << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
            return;
        }

        const UserSettingsLoadResult loaded = userSettingsStore_->Load();
        userSettings_ = loaded.settings;
        locomotionPaceToggle_.SetRunningSelected(userSettings_.runningSelected);
        cameraMotion_.SetEnabled(userSettings_.cameraMotionEnabled);
        touchControls_.SetLookSensitivity(userSettings_.touchLookSensitivity);
        if (!loaded.warning.empty()) {
            const std::string warning = "island-simulator: " + loaded.warning
                + "; using " + (loaded.usedBackup ? "backup" : "defaults");
            std::cerr << warning << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(warning);
        }
        std::ostringstream diagnostic;
        diagnostic << "island-simulator: user settings "
                   << (loaded.loaded ? (loaded.usedBackup ? "recovered" : "loaded")
                                     : "defaults")
                   << " run " << (userSettings_.runningSelected ? "enabled" : "disabled")
                   << " camera motion "
                   << (userSettings_.cameraMotionEnabled ? "enabled" : "disabled")
                   << " audio " << (userSettings_.audioMuted ? "muted" : "enabled")
                   << " touch " << TouchSensitivityName(userSettings_.touchLookSensitivity)
                   << " at " << userSettingsStore_->Location();
        const std::string diagnosticText = diagnostic.str();
        std::cout << diagnosticText << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(diagnosticText);
    }

    void IslandGame::PersistUserSettings()
    {
        if (!userSettingsStore_) return;
        std::string error;
        if (!userSettingsStore_->Save(userSettings_, error)) {
            const std::string diagnostic =
                "island-simulator: user settings could not be saved: " + error;
            std::cerr << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        } else {
            std::cout << "island-simulator: user settings saved\n";
            MirrorRuntimeDiagnosticToPlatformLog("island-simulator: user settings saved");
        }
    }

    void IslandGame::LoadContent()
    {
        const auto contentStarted = std::chrono::steady_clock::now();
        frameworkInitializationMilliseconds_ = std::chrono::duration<double, std::milli>(
            contentStarted - constructionFinished_).count();
        if (!qualityProfile_.IsValid()) {
            throw std::logic_error("The selected runtime quality profile is invalid");
        }
        if (GeneratedTextureBudget::TotalRuntimeRgba8Bytes
                > qualityProfile_.textureByteBudget
            || Soundscape::MaximumPlayingSourceCount
                > qualityProfile_.maximumPlayingAudioSources) {
            throw std::logic_error(
                "Runtime texture or audio inventory exceeds the selected quality profile");
        }
        std::ostringstream qualityDiagnostic;
        qualityDiagnostic << "island-simulator: quality profile " << qualityProfile_.name
                          << " grass "
                          << static_cast<int>(qualityProfile_.grassMaximumDistance)
                          << "m shadows "
                          << static_cast<int>(qualityProfile_.contactShadowDistanceScale * 100.0f)
                          << "% wildlife "
                          << static_cast<int>(qualityProfile_.wildlifeMaximumDistanceScale * 100.0f)
                          << "% ocean " << qualityProfile_.oceanQuadsPerAxis << 'x'
                          << qualityProfile_.oceanQuadsPerAxis << " textures "
                          << GeneratedTextureBudget::TotalRuntimeRgba8Bytes << '/'
                          << qualityProfile_.textureByteBudget << " bytes audio "
                          << Soundscape::MaximumPlayingSourceCount << '/'
                          << qualityProfile_.maximumPlayingAudioSources << " sources";
        std::cout << qualityDiagnostic.str() << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(qualityDiagnostic.str());
        auto& device = getGraphicsDeviceProperty();
        const int appliedSamples = device.getPresentationParametersProperty().getMultiSampleCountProperty();
        std::ostringstream antialiasingDiagnostic;
        antialiasingDiagnostic << "island-simulator: edge antialiasing requested "
            << qualityProfile_.preferredBackBufferSamples << "x renderer-applied " << appliedSamples << 'x';
        if (appliedSamples < qualityProfile_.preferredBackBufferSamples) {
            antialiasingDiagnostic << " (device-limited; geometric edge quality reduced)";
        }
        std::cout << antialiasingDiagnostic.str() << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(antialiasingDiagnostic.str());
        RecordGraphicsRuntimeDiagnostics(device);
        InitializeGpuDiagnostics();
        // Build consumers against a temporary owner. Commit them before replacing the old
        // owner so a repeated LoadContent never leaves old renderers referencing freed data.
        auto canopyLighting = std::make_unique<CanopyLighting>(device, canopyLightField_, groundCoverField_, terrain_);
        auto terrainHeightMap = std::make_unique<RenderedTerrainHeightMap>(device, terrain_);
        auto terrainRenderer = std::make_unique<TerrainRenderer>(device, terrain_, *canopyLighting);
        distantSceneryRenderer_ = std::make_unique<DistantSceneryRenderer>(
            device, distantSceneryLayout_);
        contactShadowRenderer_ = std::make_unique<ContactShadowRenderer>(
            device, terrain_, vegetationLayout_, jungleLayout_, naturalPropLayout_,
            shipwreckLayout_, qualityProfile_.contactShadowDistanceScale, hutPlacement_?&*hutPlacement_:nullptr);
        constexpr auto coastalShadowDiagnostic =
            "island-simulator: coastal contact patches follow rendered seabed triangles in the existing batch";
        std::cout << coastalShadowDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(coastalShadowDiagnostic);
        constexpr auto landShadowDiagnostic =
            "island-simulator: land prop shadows match model yaw and rendered terrain triangles";
        std::cout << landShadowDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(landShadowDiagnostic);
        birdRenderer_ = std::make_unique<BirdRenderer>(
            device, 690.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        butterflyRenderer_ = std::make_unique<ButterflyRenderer>(
            device, 72.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        crabRenderer_ = std::make_unique<CrabRenderer>(
            device, 92.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        auto boarRenderer = std::make_unique<BoarRenderer>(
            device, terrain_, canopyLighting->Field(), 135.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        auto snakeRenderer = std::make_unique<SnakeRenderer>(device,canopyLighting->Field());
        auto goatRenderer = std::make_unique<GoatRenderer>(device,canopyLighting->Field(),
            120.0f*qualityProfile_.wildlifeMaximumDistanceScale);
        auto spiderRenderer = std::make_unique<SpiderRenderer>(device,canopyLighting->Field());
        fishRenderer_ = std::make_unique<FishRenderer>(
            device, 215.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        seaTurtleRenderer_ = std::make_unique<SeaTurtleRenderer>(
            device, 190.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        sharkRenderer_ = std::make_unique<SharkRenderer>(
            device, 710.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        auto monkeyRenderer = std::make_unique<MonkeyRenderer>(
            device, terrain_, canopyLighting->Field(), 235.0f * qualityProfile_.wildlifeMaximumDistanceScale);
        auto grassRenderer = std::make_unique<GrassRenderer>(
            device, terrain_, grassLayout_, *canopyLighting, *terrainHeightMap, qualityProfile_.grassMaximumDistance);
        auto jungleRenderer = std::make_unique<JungleRenderer>(device, terrain_, jungleLayout_,
                                                              *canopyLighting, *terrainHeightMap);
        if (canopyLodInspection_) {
            const auto camera = jungleRenderer->SetLodInspection(*canopyLodInspection_);
            if (viewpoint_ == FixedViewpoint::CanopyInspection) {
                cameraPosition_ = Vector3(camera.position[0], camera.position[1], camera.position[2]);
                const Vector3 direction = Vector3(camera.target[0], camera.target[1], camera.target[2]) - cameraPosition_;
                yaw_ = std::atan2(direction.X, direction.Z);
                pitch_ = std::atan2(direction.Y, std::hypot(direction.X, direction.Z));
                std::cout << "island-simulator: elevated canopy inspection camera position="
                          << cameraPosition_.X << ',' << cameraPosition_.Y << ',' << cameraPosition_.Z
                          << " target=" << camera.target[0] << ',' << camera.target[1] << ',' << camera.target[2]
                          << " full-scene=1\n";
            }
        }
        terrainRenderer_ = std::move(terrainRenderer);
        playerMotion_.SetGroundPatch(&terrainRenderer_->StreamBed());
        streamWaterRenderer_=std::make_unique<StreamWaterRenderer>(device,terrain_,terrainRenderer_->StreamBed());
        grassRenderer_ = std::move(grassRenderer);
        jungleRenderer_ = std::move(jungleRenderer);
        boarRenderer_ = std::move(boarRenderer);
        snakeRenderer_ = std::move(snakeRenderer);
        goatRenderer_ = std::move(goatRenderer);
        spiderRenderer_ = std::move(spiderRenderer);
        std::cout << "island-simulator: ground spiders " << spiders_->Spiders().size()
                  << " agents with retained stance anchors; webs not yet installed\n";
        std::cout << "island-simulator: goats " << goats_->Goats().size()
                  << " agents on shared-field dry open slopes, terrain-fitted limbs, directional fur material\n";
        std::cout << "island-simulator: boars " << boarHerd_.Boars().size()
                  << " agents in moist non-channel interior habitat\n";
        std::cout << "island-simulator: snakes " << snakes_->Snakes().size()
                  << " agents with separated dense-understory-margin homes\n";
        monkeyRenderer_ = std::move(monkeyRenderer);
        canopyLighting_ = std::move(canopyLighting);
        terrainHeightMap_ = std::move(terrainHeightMap);
        naturalPropRenderer_ = std::make_unique<NaturalPropRenderer>(device, naturalPropLayout_);
        shipwreckRenderer_ = std::make_unique<ShipwreckRenderer>(device, shipwreckLayout_);
        if(hutPlacement_) hutRenderer_=std::make_unique<HutRenderer>(device,*hutPlacement_);
        palmRenderer_ = std::make_unique<PalmRenderer>(device, vegetationLayout_);
        oceanRenderer_ = std::make_unique<OceanRenderer>(
            device, terrain_, qualityProfile_.oceanQuadsPerAxis);
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
            || viewpoint_ == FixedViewpoint::ReefHabitatDetail
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
        if (viewpoint_ == FixedViewpoint::WaterlineDetail) {
            waterRippleModel_.TriggerSplash(true, initialRippleSpeed);
        }
        const float initialGrassSpeed = viewpoint_ == FixedViewpoint::GrassDetail
            ? 3.2f
            : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ());
        grassInteractionModel_.Reset(initialGrassSpeed);
        if (!smokeTest_ && viewpoint_ == FixedViewpoint::None) {
            const PlayerPosition& position = playerMotion_.Position();
            cameraPosition_ = Vector3(position.x, position.y, position.z);
        }
        soundscape_ = std::make_unique<Soundscape>();
        if (userSettings_.audioMuted) soundscape_->SetMuted(true);
        if (!smokeTest_ && !soundscapeNeedsGesture_) {
            TryStartSoundscape();
        }
        contentFinished_ = std::chrono::steady_clock::now();
        contentCreationMilliseconds_ = std::chrono::duration<double, std::milli>(
            contentFinished_ - contentStarted).count();

        std::string rendererDiagnostic = "island-simulator: renderer ";
        rendererDiagnostic.append(device.GetGraphicsRendererName());
        std::cout << rendererDiagnostic << '\n'
                  << "island-simulator: terrain " << terrainRenderer_->VertexCount()
                  << " vertices, " << terrainRenderer_->PrimitiveCount() << " triangles in "
                  << terrainRenderer_->TotalCellCount()
                  << " cull cells with shared buffers\n"
                  << "island-simulator: collision grid "
                  << worldCollisionGrid_.Obstacles().size() << " static obstacles in "
                  << worldCollisionGrid_.CellCount() << " occupied cells\n"
                  << "island-simulator: birds " << birdFlock_.Birds().size()
                  << " agents in a distance-throttled one-batch simulation\n"
                  << "island-simulator: bird flight uses slow radial drift, banking, and "
                     "intermittent glide\n"
                  << "island-simulator: clearing butterflies "
                  << butterflyMeadow_.Butterflies().size()
                  << " agents in deterministic low-altitude habitats\n"
                  << "island-simulator: beach crabs " << crabColony_.Crabs().size()
                  << " agents in deterministic sand-constrained colonies\n"
                  << "island-simulator: fish " << fishSchools_.Fish().size()
                  << " agents in " << FishSchools::SchoolCount
                  << " distance-throttled shallow-water schools\n"
                  << "island-simulator: sea turtles " << seaTurtleGroup_.Turtles().size()
                  << " agents in bounded habitat routes, three based in the sheltered western bay\n"
                  << "island-simulator: one occasional non-hostile offshore shark patrol\n"
                  << "island-simulator: aquatic wildlife uses depth pitch, banked turns, "
                     "forked tails, and near head detail\n"
                  << "island-simulator: aquatic wildlife uses depth tint and restrained moving "
                     "sunlight bands in CPU vertex colours\n"
                  << "island-simulator: fish near/far geometry "
                  << FishRenderer::NearVertexCount << '/' << FishRenderer::FarVertexCount
                  << " vertices, single-surface fins and CPU faceted daylight\n"
                  << "island-simulator: shark near/far geometry "
                  << SharkRenderer::NearVertexCount << '/' << SharkRenderer::FarVertexCount
                  << " vertices, single-surface fins and CPU faceted daylight\n"
                  << "island-simulator: monkeys " << monkeyTroop_.Monkeys().size()
                  << " agents with portable GPU skeletal animation, "
                  << MonkeyRenderer::SilhouetteCount << " silhouettes, 4 palettes\n"
                  << "island-simulator: monkeys use a horizontal lofted trunk and distance-driven quadrupedal gait\n"
                  << "island-simulator: near monkeys use bone-following finger and toe silhouettes\n"
                  << "island-simulator: calm monkeys use bounded skeletal head attention\n"
                  << "island-simulator: terrain-conforming contact shadows in one portable "
                     "near-field batch\n"
                  << "island-simulator: near-surface swimming view uses generated depth haze, "
                     "surface scatter, and peripheral veil in one draw\n"
                  << "island-simulator: movement ripples and shoreline-transition bursts "
                     "integrated into the portable ocean shader\n"
                  << "island-simulator: outer-ocean current uses progressive inward return "
                     "with mild coherent circulation\n"
                  << "island-simulator: keyboard run/walk toggle uses a transient mode "
                     "confirmation\n";
        if (debugOverlayVisible_) {
            std::cout << "island-simulator: debug overlay active\n";
        }
        if (worldDebugVisible_) {
            std::cout << "island-simulator: world debug visualization active with 9 ground-contact "
                         "samples, resolved movement trace, wildlife navigation guides, 3 bird "
                         "route regions, 24 bird spawns, 6 audio-zone contours, and positional-"
                         "emitter markers\n";
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
        MirrorRuntimeDiagnosticToPlatformLog(rendererDiagnostic);
        std::ostringstream butterflyPopulationDiagnostic;
        butterflyPopulationDiagnostic << "island-simulator: clearing butterflies "
                                      << butterflyMeadow_.Butterflies().size()
                                      << " agents in deterministic low-altitude habitats";
        MirrorRuntimeDiagnosticToPlatformLog(butterflyPopulationDiagnostic.str());
        std::ostringstream crabPopulationDiagnostic;
        crabPopulationDiagnostic << "island-simulator: beach crabs "
                                 << crabColony_.Crabs().size()
                                 << " agents in deterministic sand-constrained colonies";
        MirrorRuntimeDiagnosticToPlatformLog(crabPopulationDiagnostic.str());
        std::ostringstream seaTurtlePopulationDiagnostic;
        seaTurtlePopulationDiagnostic << "island-simulator: sea turtles "
                                      << seaTurtleGroup_.Turtles().size()
                                      << " agents in bounded near-shore cruise, forage, and surface routes";
        MirrorRuntimeDiagnosticToPlatformLog(seaTurtlePopulationDiagnostic.str());
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

    void IslandGame::ReportControlTransition(const ControlTransitionEvent event) noexcept
    {
        try {
            const auto message=ControlTransitionMessage(event);
            std::cout << message << '\n' << std::flush;
            MirrorRuntimeDiagnosticToPlatformLog(message);
            const auto p=flightMotion_.Position();
            const ControlTransitionSnapshot snapshot{drawnFrames_,
                {cameraPosition_.X,cameraPosition_.Y,cameraPosition_.Z}, {p.x,p.y,p.z},
                yaw_,pitch_,flightMotion_.IsActive(),graphics_.getIsFullScreenProperty()};
            MirrorRuntimeDiagnosticToPlatformLog(FormatControlTransition(event,snapshot));
        } catch (...) {
            // Logging must not turn an otherwise successful input transition into a failure.
        }
    }

    void IslandGame::Update(GameTime& gameTime)
    {
        const bool snakeCaptureClock=UsesSnakeCaptureClock();
        const bool goatCaptureClock=UsesGoatCaptureClock();
        const bool boarCaptureClock=UsesBoarCaptureClock();
        if(boarCaptureClock)++boarCaptureUpdates_;
        const bool spiderCaptureClock=UsesSpiderCaptureClock();
        if(spiderCaptureClock)++spiderCaptureUpdates_;
        if(goatCaptureClock)++goatCaptureUpdates_;
        if (!snakeFirstDrawReturnReported_ && drawnFrames_ > 0U
            && (viewpoint_ == FixedViewpoint::SnakeDetail || viewpoint_ == FixedViewpoint::SnakeHeadDetail)) {
            snakeFirstDrawReturnReported_ = true;
            MirrorRuntimeDiagnosticToPlatformLog("island-simulator: snake QA update resumed after first draw");
        }
        if(snakeCaptureClock) ++snakeCaptureUpdates_;
        const float deltaSeconds = canopyLodInspection_ ? 0.0f : (snakeCaptureClock||goatCaptureClock||spiderCaptureClock||boarCaptureClock) ? 0.1f : std::min(
            0.1f, static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty())
                / 1000.0f);
        worldSeedCopyFeedbackSeconds_ = std::max(
            0.0f, worldSeedCopyFeedbackSeconds_ - deltaSeconds);
        locomotionModeNoticeSeconds_ = std::max(
            0.0f, locomotionModeNoticeSeconds_ - deltaSeconds);
        // Fixed QA cameras deliberately reject player movement, but their diagnostic overlay
        // remains interactive so automated and manual reviewers can inspect an unobstructed view.
        const KeyboardState frameKeyboard = Keyboard::GetState();
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
        // Web fullscreen is requested synchronously by browser_fullscreen.js in the
        // trusted key event. Android keeps its existing immersive presentation.
        const bool fullscreenDown = frameKeyboard.IsKeyDown(Keys::F11);
        if (fullscreenDown && !fullscreenToggleWasDown_ && getIsActiveProperty()) {
            graphics_.ToggleFullScreen();
            ReportControlTransition(graphics_.getIsFullScreenProperty()
                ? ControlTransitionEvent::FullscreenOn : ControlTransitionEvent::FullscreenOff);
        }
        fullscreenToggleWasDown_ = fullscreenDown;
#endif
        const bool debugOverlayToggleDown = frameKeyboard.IsKeyDown(Keys::F1);
        if (debugOverlayToggleDown && !debugOverlayToggleWasDown_) {
            debugOverlayVisible_ = !debugOverlayVisible_;
            std::cout << "island-simulator: debug overlay "
                      << (debugOverlayVisible_ ? "enabled" : "disabled") << '\n';
        }
        debugOverlayToggleWasDown_ = debugOverlayToggleDown;
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
            || viewpoint_ == FixedViewpoint::ReefHabitatDetail
            || viewpoint_ == FixedViewpoint::BenchmarkTraversal;
        (void)waterViewModel_.Update(
            waterViewAllowed && !flightMotion_.IsActive() && playerMotion_.Mode() == LocomotionMode::Swimming,
            playerMotion_.GroundHeight(), playerMotion_.OceanCurrentStrength(terrain_),
            deltaSeconds);
        const float rippleSpeed = viewpoint_ == FixedViewpoint::WaterlineDetail
            ? 1.35f
            : (viewpoint_ == FixedViewpoint::BenchmarkTraversal
                   ? benchmarkTraversalPresentationSpeed_
                   : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()));
        (void)waterRippleModel_.Update(
            !flightMotion_.IsActive() && playerMotion_.Mode() == LocomotionMode::Swimming,
            playerMotion_.GroundHeight(), rippleSpeed, deltaSeconds);
        const float grassInteractionSpeed = viewpoint_ == FixedViewpoint::GrassDetail
            ? 3.2f
            : (viewpoint_ == FixedViewpoint::BenchmarkTraversal
                   ? benchmarkTraversalPresentationSpeed_
                   : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()));
        (void)grassInteractionModel_.Update(grassInteractionSpeed, deltaSeconds);
        birdFlock_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        butterflyMeadow_.Update(
            deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        crabColony_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        const auto previousBoar=boarHerd_.Boars().front();
        boarHerd_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        if(boarCaptureClock) {
            const auto& current=boarHerd_.Boars().front();
            cameraPosition_+=Vector3(current.x-previousBoar.x,current.y-previousBoar.y,current.z-previousBoar.z);
        }
        const auto previousGoat=goats_->Goats().front();
        const auto previousSpider=spiders_->Spiders()[spiderViewSubject_];
        spiders_->Update(deltaSeconds,{cameraPosition_.X,cameraPosition_.Y,cameraPosition_.Z});
        if(UsesSpiderViewpoint()) {
            const auto& current=spiders_->Spiders()[spiderViewSubject_];
            cameraPosition_+=Vector3(current.x-previousSpider.x,current.y-previousSpider.y,current.z-previousSpider.z);
        }
        goats_->Update(deltaSeconds,{cameraPosition_.X,cameraPosition_.Y,cameraPosition_.Z});
        if(viewpoint_==FixedViewpoint::GoatDetail) {
            const auto& current=goats_->Goats().front();
            cameraPosition_+=Vector3(current.x-previousGoat.x,current.y-previousGoat.y,current.z-previousGoat.z);
        }
        const auto snakeCentre = [this]() {
            Vector3 centre = Vector3::Zero;
            const auto population = snakes_->Snakes();
            if(viewpoint_==FixedViewpoint::SnakeHeadDetail) {
                const auto& head=population.front().body.front();
                return Vector3(head.x,head.y,head.z);
            }
            for (const auto& p : population.front().body) centre += Vector3(p.x,p.y,p.z);
            return centre/static_cast<float>(SnakeState::StationCount);
        };
        const auto previousSnakeCentre = snakeCentre();
        snakes_->Update(deltaSeconds,cameraPosition_.X,cameraPosition_.Y,cameraPosition_.Z);
        if (viewpoint_ == FixedViewpoint::SnakeDetail || viewpoint_ == FixedViewpoint::SnakeHeadDetail) {
            // Follow translation only: preserve the viewing direction and live retreat behavior.
            // Software frames can span many simulation updates; otherwise the close subject exits.
            cameraPosition_ += snakeCentre()-previousSnakeCentre;
        }
        fishSchools_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        seaTurtleGroup_.Update(
            deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        sharkPatrol_.Update(deltaSeconds, cameraPosition_.X, cameraPosition_.Y, cameraPosition_.Z);
        Vector3 monkeyObserver = cameraPosition_;
        if (viewpoint_ == FixedViewpoint::MonkeyDetail) {
            const MonkeyState& subject = monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
            Vector3 direction(cameraPosition_.X - subject.x, 0.0f,
                              cameraPosition_.Z - subject.z);
            if (direction.LengthSquared() > 0.0001f) {
                direction.Normalize();
                monkeyObserver = Vector3(subject.x, cameraPosition_.Y, subject.z)
                    + direction * 12.0f;
            }
        }
        const MonkeyState previousMonkeySubject = monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
        if (viewpoint_ != FixedViewpoint::MonkeyTreeDetail && viewpoint_ != FixedViewpoint::MonkeyClimbDetail)
            monkeyTroop_.Update(deltaSeconds, monkeyObserver.X, monkeyObserver.Y, monkeyObserver.Z,
                snakeCaptureClock ? static_cast<float>(snakeCaptureUpdates_)*0.1f
                    : static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty()));
        if (viewpoint_ == FixedViewpoint::MonkeyDetail) {
            // Keep the animated subject in the close QA view even when software rendering
            // requires many simulation updates between captured frames. Preserve the original
            // viewing direction rather than orbiting whenever the animal changes heading.
            const MonkeyState& subject = monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
            cameraPosition_ += Vector3(subject.x - previousMonkeySubject.x,
                                       subject.y - previousMonkeySubject.y,
                                       subject.z - previousMonkeySubject.z);
        }
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
                          << viewport.getHeightProperty()
                          << " run " << (playerMotion_.IsRunning() ? 1 : 0)
                          << " velocity " << playerMotion_.VelocityX() << ' '
                          << playerMotion_.VelocityZ()
                          << " grass " << (grassRenderer_ ? grassRenderer_->VisibleNearPatchCount() : 0)
                          << ' ' << (grassRenderer_ ? grassRenderer_->VisibleMiddlePatchCount() : 0)
                          << ' ' << (grassRenderer_ ? grassRenderer_->VisibleFarPatchCount() : 0)
                          << " cells " << (grassRenderer_ ? grassRenderer_->VisibleCellCount() : 0)
                          << " draws " << (grassRenderer_ ? grassRenderer_->DrawCallCount() : 0);
                if (jungleRenderer_) jungleRenderer_->WriteVegetationLodDiagnostics(std::cout);
                else std::cout << " canopy 0 0 trees none shrubs 0 0 bushes none";
                std::cout << '\n';
                if (soundscape_) {
                    const auto audio = soundscape_->DebugState();
                    std::cout << "island-simulator: audio sample available " << soundscape_->IsAvailable()
                        << " muted " << soundscape_->IsMuted()
                        << " boars " << soundscape_->PlayedBoarCallCount()
                        << " sources " << audio.playingSourceCount
                        << " wildlife " << static_cast<int>(audio.activeWildlifeCall.type) << '\n';
                }
                if (flightMotion_.IsActive())
                    std::cout << "island-simulator: flight position " << cameraPosition_.X << ' '
                              << cameraPosition_.Y << ' ' << cameraPosition_.Z
                              << " above-ground " << flightMotion_.HeightAboveGround() << '\n';
                if(playerMotion_.Mode()==LocomotionMode::Swimming && !flightMotion_.IsActive()) {
                    const auto p=playerMotion_.Position();
                    std::cout << "island-simulator: swim position " << p.x << ' ' << p.y << ' ' << p.z
                        << " depth " << playerMotion_.SubmergedDepth() << '\n';
                }
                std::cout << std::flush;
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
        const bool flightDown = keyboard.IsKeyDown(Keys::Tab);
        if (flightDown && !flightToggleWasDown_ && getIsActiveProperty()) {
            if (flightMotion_.IsActive()) {
                const auto landing=flightMotion_.FindLanding(terrain_,worldCollisionGrid_);
                if (landing) {
                    playerMotion_.SetPosition(terrain_,landing->x,landing->z);
                    playerMotion_.Update(terrain_,0,0,0,worldCollisionGrid_);
                    flightMotion_.Deactivate();
                    cameraPosition_=Vector3(landing->x,landing->y,landing->z);
                    ReportControlTransition(ControlTransitionEvent::FlightOff);
                } else {
                    ReportControlTransition(ControlTransitionEvent::LandingRefused);
                }
            } else {
#ifndef NDEBUG
                debugFreeCamera_.Deactivate();
#endif
                flightMotion_.Activate(terrain_,{cameraPosition_.X,cameraPosition_.Y,cameraPosition_.Z});
                const auto p=playerMotion_.Position();
                playerMotion_.SetPosition(terrain_,p.x,p.z); // Stop stale walking velocity/footsteps.
                ReportControlTransition(ControlTransitionEvent::FlightOn);
            }
            cameraMotion_.Reset();
            locomotionModeNoticeSeconds_=LocomotionModeNoticeDuration;
        }
        flightToggleWasDown_=flightDown;
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
        if (touchControlState_.lookSensitivityChanged) {
            userSettings_.touchLookSensitivity = touchControlState_.lookSensitivity;
            PersistUserSettings();
            const std::string_view diagnostic = [this]() -> std::string_view {
                switch (touchControlState_.lookSensitivity) {
                case TouchLookSensitivityLevel::Low:
                    return "island-simulator: touch look sensitivity low (0.72x)";
                case TouchLookSensitivityLevel::Standard:
                    return "island-simulator: touch look sensitivity standard (1.00x)";
                case TouchLookSensitivityLevel::High:
                    return "island-simulator: touch look sensitivity high (1.35x)";
                }
                return "island-simulator: touch look sensitivity standard (1.00x)";
            }();
            std::cout << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        }
        const bool touchInteracting = touchControlState_.movementActive
            || touchControlState_.lookActive
            || touchControlState_.sensitivitySelectorActive;
#if defined(__EMSCRIPTEN__)
        // The page acquires pointer lock synchronously in the trusted click handler. A short
        // click can finish between game updates, so polled button state is not its receipt.
        // Observe the browser's granted lock instead of relying on the transient button state.
        EmscriptenPointerlockChangeEvent pointerLock{};
        if (emscripten_get_pointerlock_status(&pointerLock) != EMSCRIPTEN_RESULT_SUCCESS) {
            throw std::runtime_error("Unable to query browser pointer-lock state");
        }
        const bool browserPointerLocked = pointerLock.isActive
            && std::string_view(pointerLock.id) == "canvas";
        if (!browserPointerLocked) {
            browserPointerReleasePending_ = false;
            if (Mouse::getIsRelativeMouseModeEXTProperty()) {
                Mouse::setIsRelativeMouseModeEXTProperty(false);
                setIsMouseVisibleProperty(true);
            }
            mouseCaptured_ = false;
        }
#endif

        const bool runToggleDown = keyboard.IsKeyDown(Keys::LeftShift)
            || keyboard.IsKeyDown(Keys::RightShift);
        if (locomotionPaceToggle_.Update(runToggleDown, !IsDebugFreeCameraActive() && !flightMotion_.IsActive()
            && playerMotion_.Mode()!=LocomotionMode::Swimming)) {
            userSettings_.runningSelected = locomotionPaceToggle_.IsRunningSelected();
            PersistUserSettings();
            locomotionModeNoticeSeconds_ = LocomotionModeNoticeDuration;
            const std::string_view diagnostic = locomotionPaceToggle_.IsRunningSelected()
                ? "island-simulator: run mode enabled (8.0 m/s)"
                : "island-simulator: run mode disabled (4.3 m/s)";
            std::cout << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        }

        const FirstPersonDigitalInput digitalInput{
            keyboard.IsKeyDown(Keys::W) || keyboard.IsKeyDown(Keys::Up),
            keyboard.IsKeyDown(Keys::S) || keyboard.IsKeyDown(Keys::Down),
            keyboard.IsKeyDown(Keys::A) || keyboard.IsKeyDown(Keys::Left),
            keyboard.IsKeyDown(Keys::D) || keyboard.IsKeyDown(Keys::Right),
            locomotionPaceToggle_.IsRunningSelected() || touchControlState_.runRequested,
        };
        const bool movementKeyDown = digitalInput.forward || digitalInput.backward
            || digitalInput.left || digitalInput.right;
        const bool movementKeyPressed = movementKeyDown && !movementKeyWasDown_;
        movementKeyWasDown_ = movementKeyDown;
        if (soundscapeNeedsGesture_
            && (mouse.getLeftButtonProperty() == ButtonState::Pressed
                || !touchContacts.empty() || movementKeyDown)) {
            TryStartSoundscape();
        }

        const bool muteToggleDown = keyboard.IsKeyDown(Keys::M);
        if (muteToggleDown && !muteToggleWasDown_ && soundscape_) {
            soundscape_->ToggleMuted();
            userSettings_.audioMuted = soundscape_->IsMuted();
            PersistUserSettings();
        }
        muteToggleWasDown_ = muteToggleDown;

        const bool cameraMotionToggleDown = keyboard.IsKeyDown(Keys::B);
        if (cameraMotionToggleDown && !cameraMotionToggleWasDown_) {
            cameraMotion_.SetEnabled(!cameraMotion_.IsEnabled());
            userSettings_.cameraMotionEnabled = cameraMotion_.IsEnabled();
            PersistUserSettings();
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

        const bool worldDebugToggleDown = keyboard.IsKeyDown(Keys::F2);
        if (worldDebugToggleDown && !worldDebugToggleWasDown_) {
            worldDebugVisible_ = !worldDebugVisible_;
            std::cout << "island-simulator: world debug visualization "
                      << (worldDebugVisible_
                              ? "enabled with 9 ground-contact samples, resolved movement trace, "
                                "wildlife navigation guides, 3 bird route regions, 24 bird "
                                "spawns, 6 audio-zone contours, and positional-emitter markers"
                              : "disabled")
                      << '\n';
        }
        worldDebugToggleWasDown_ = worldDebugToggleDown;

        const bool worldSeedCopyDown = keyboard.IsKeyDown(Keys::F4);
        if (debugOverlayVisible_ && worldSeedCopyDown && !worldSeedCopyWasDown_) {
            const std::string seedText = WorldSeedClipboardText(AuthoritativeWorldSeeds());
            CNA::Input::Clipboard::SetTextEXT(seedText);
            worldSeedCopySucceeded_ = CNA::Input::Clipboard::GetTextEXT() == seedText;
            worldSeedCopyFeedbackSeconds_ = 3.0f;
            std::cout << "island-simulator: world seed clipboard copy "
                      << (worldSeedCopySucceeded_ ? "confirmed" : "unavailable")
                      << '\n' << std::flush;
        }
        worldSeedCopyWasDown_ = worldSeedCopyDown;

#ifndef NDEBUG
        const bool debugFreeCameraToggleDown = keyboard.IsKeyDown(Keys::F3);
        if (debugOverlayVisible_ && !flightMotion_.IsActive() && debugFreeCameraToggleDown
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

        constexpr std::array<Keys, 9> teleportKeys = {
            Keys::F5, Keys::F6, Keys::F7, Keys::F8, Keys::F9, Keys::F10, Keys::J,
            Keys::F12, Keys::K};
        constexpr std::array<const char*, 9> teleportNames = {
            "south beach", "jungle trail", "east clearing", "west clearing",
            "fish shallows", "offshore shark", "monkey troop", "offshore current", "boar herd"};
        for (std::size_t index = 0; index < teleportKeys.size(); ++index) {
            const bool down = keyboard.IsKeyDown(teleportKeys[index]);
            if (debugOverlayVisible_ && down && !debugTeleportWasDown_[index]) {
                flightMotion_.Deactivate();
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
                    x = VegetationLayout::EastClearing.x;
                    z = VegetationLayout::EastClearing.z;
                    yaw_ = -std::numbers::pi_v<float> * 0.5f;
                } else if (index == 3U) {
                    x = VegetationLayout::WestClearing.x;
                    z = VegetationLayout::WestClearing.z;
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
                } else if (index == 8U) {
                    const auto boars = boarHerd_.Boars();
                    const auto& boar = boars[boarTeleportIndex_++ % boars.size()];
                    x = boar.x + std::cos(boar.headingRadians) * 7.0f;
                    z = boar.z + std::sin(boar.headingRadians) * 7.0f;
                    yaw_ = std::atan2(boar.x - x, boar.z - z);
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

        const bool escapeDown = keyboard.IsKeyDown(Keys::Escape);
        const bool escapePressed = escapeDown && !escapeWasDown_;
        escapeWasDown_ = escapeDown;
        if (escapePressed) {
#if defined(__EMSCRIPTEN__)
            // Browser Escape can release pointer lock before the game observes this key edge.
            // Never turn that ordering difference into a request to terminate the Web session.
            std::cout << "island-simulator: browser Escape state before release captured "
                      << mouseCaptured_ << " relative "
                      << Mouse::getIsRelativeMouseModeEXTProperty() << '\n';
            // The HTML page can own a lock before CNA relative mode is enabled. Release that
            // lock explicitly too; SDL's already-disabled relative mode is otherwise a no-op.
            browserPointerReleasePending_ = browserPointerLocked;
            if (browserPointerLocked && emscripten_exit_pointerlock() != EMSCRIPTEN_RESULT_SUCCESS) {
                throw std::runtime_error("Unable to release browser pointer lock");
            }
            mouseCaptured_ = false;
            Mouse::setIsRelativeMouseModeEXTProperty(false);
            setIsMouseVisibleProperty(true);
            std::cout << "island-simulator: Escape released browser pointer lock without exiting\n"
                      << std::flush;
#else
            if (mouseCaptured_) {
                mouseCaptured_ = false;
                Mouse::setIsRelativeMouseModeEXTProperty(false);
                setIsMouseVisibleProperty(true);
            } else {
                Exit();
            }
#endif
            return;
        }

        bool acquiredMouseThisFrame = false;
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
        (void)movementKeyPressed;
        constexpr bool keyboardCanCaptureMouse = false;
#else
        const bool keyboardCanCaptureMouse = movementKeyPressed;
#endif
#if defined(__EMSCRIPTEN__)
        (void)keyboardCanCaptureMouse;
        const bool pointerCanCapture = browserPointerLocked && !browserPointerReleasePending_;
#else
        const bool pointerCanCapture = mouse.getLeftButtonProperty() == ButtonState::Pressed
            || keyboardCanCaptureMouse;
#endif
        if (!touchInteracting && !smokeTest_ && !mouseCaptured_
            && pointerCanCapture) {
            mouseCaptured_ = true;
            Mouse::setIsRelativeMouseModeEXTProperty(true);
            setIsMouseVisibleProperty(false);
            acquiredMouseThisFrame = true;
        }
        if (touchControlState_.lookSensitivityChanged && !mouseCaptured_) {
            constexpr std::string_view diagnostic =
                "island-simulator: touch sensitivity selector retained touch ownership "
                "without mouse capture";
            std::cout << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        }
        if (mouseCaptured_ && !acquiredMouseThisFrame) {
            yaw_ -= static_cast<float>(mouse.getXProperty()) * MouseSensitivity;
            pitch_ = std::clamp(pitch_ - static_cast<float>(mouse.getYProperty()) * MouseSensitivity,
                                -1.48f, 1.48f);
        }
        if (touchControlState_.lookActive) {
            const float resolutionScale = 720.0f / std::max(1.0f, touchHeight);
            const float sensitivityScale = TouchControls::LookSensitivityScale(
                touchControlState_.lookSensitivity);
            yaw_ -= touchControlState_.lookDeltaX * TouchLookSensitivity
                * sensitivityScale * resolutionScale;
            pitch_ = std::clamp(
                pitch_ - touchControlState_.lookDeltaY * TouchLookSensitivity
                    * sensitivityScale * resolutionScale,
                -1.48f, 1.48f);
        }

        const Vector3 forward(std::sin(yaw_), 0.0f, std::cos(yaw_));
        const Vector3 right(-forward.Z, 0.0f, forward.X);
        const FirstPersonMovementIntent movementIntent = FirstPersonInput::Resolve(
            digitalInput, touchControlState_.moveForward,
            touchControlState_.moveRight, yaw_);
        const Vector3 wish(movementIntent.worldX, 0.0f, movementIntent.worldZ);
        playerMotion_.SetRunning(movementIntent.run);

        if (flightMotion_.IsActive()) {
            const float vertical=(keyboard.IsKeyDown(Keys::Space) ? 1.0f : 0.0f)
                -(runToggleDown ? 1.0f : 0.0f);
            flightMotion_.Update(wish.X,vertical,wish.Z,false,deltaSeconds);
            const auto p=flightMotion_.Position();
            cameraPosition_=Vector3(p.x,p.y,p.z);
            cameraMotion_.Reset();
        } else {
#ifndef NDEBUG
        if (debugFreeCamera_.IsActive()) {
            const float vertical = (keyboard.IsKeyDown(Keys::E) ? 1.0f : 0.0f)
                - (keyboard.IsKeyDown(Keys::Q) ? 1.0f : 0.0f);
            const bool fast = runToggleDown;
            debugFreeCamera_.Update(
                yaw_, DebugFreeCameraInput{Vector3::Dot(wish, forward),
                                            Vector3::Dot(wish, right),
                                            vertical, fast},
                deltaSeconds);
            const DebugFreeCameraPosition& freePosition = debugFreeCamera_.Position();
            cameraPosition_ = Vector3(freePosition.x, freePosition.y, freePosition.z);
            cameraMotion_.Reset();
        } else {
#endif
            const PlayerPosition positionBeforeMovement = playerMotion_.Position();
            const LocomotionMode modeBeforeMovement = playerMotion_.Mode();
            const bool ascend=keyboard.IsKeyDown(Keys::Space);
            if(modeBeforeMovement==LocomotionMode::Swimming && (ascend||runToggleDown)
                && !swimTerrainSurface_)
                swimTerrainSurface_=std::make_unique<RenderedTerrainSurface>(terrain_);
            playerMotion_.SetSwimInput(swimTerrainSurface_.get(),ascend,runToggleDown);
            playerMotion_.Update(terrain_, wish.X, wish.Z, deltaSeconds, worldCollisionGrid_);
            const PlayerPosition& position = playerMotion_.Position();
            cameraPosition_ = Vector3(position.x, position.y, position.z);
            if (!swimmingEntryDiagnosticReported_
                && modeBeforeMovement == LocomotionMode::Walking
                && playerMotion_.Mode() == LocomotionMode::Swimming) {
                constexpr std::string_view diagnostic =
                    "island-simulator: first traversal entered swimming state";
                swimmingEntryDiagnosticReported_ = true;
                std::cout << diagnostic << '\n' << std::flush;
                MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
            }
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
        }
        if (soundscape_) {
            const PlayerPosition& position = playerMotion_.Position();
            const SoundscapeListenerState listener{
                cameraPosition_.X,
                cameraPosition_.Y,
                cameraPosition_.Z,
                forward.X * std::cos(pitch_),
                std::sin(pitch_),
                forward.Z * std::cos(pitch_),
                IsDebugFreeCameraActive() || flightMotion_.IsActive() ? 0.0f : playerMotion_.VelocityX(),
                IsDebugFreeCameraActive() || flightMotion_.IsActive() ? 0.0f : playerMotion_.VelocityZ(),
            };
            soundscape_->Update(terrain_.SignedShoreDistance(flightMotion_.IsActive() ? cameraPosition_.X : position.x,
                                                            flightMotion_.IsActive() ? cameraPosition_.Z : position.z),
                                !flightMotion_.IsActive() && playerMotion_.Mode() == LocomotionMode::Swimming,
                                flightMotion_.IsActive() ? 0.0f : std::max(0.0f, -playerMotion_.GroundHeight()),
                                flightMotion_.IsActive() ? 0.0f : std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()),
                                flightMotion_.IsActive() ? 0.0f : playerMotion_.OceanCurrentStrength(terrain_),
                                deltaSeconds, listener,
                                birdFlock_.Birds(), monkeyTroop_.Monkeys(), boarHerd_.Boars());
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
            && !IsDebugFreeCameraActive() && !flightMotion_.IsActive()
            && playerMotion_.SubmergedDepth()==0.0f) {
            const CameraMotionOffset& motion = cameraMotion_.Offset();
            const Vector3 right(std::cos(yaw_), 0.0f, -std::sin(yaw_));
            renderCameraPosition_ = renderCameraPosition_
                + right * motion.lateralMeters + Vector3::Up * motion.verticalMeters;
        }
        cameraView_ = Matrix::CreateLookAt(renderCameraPosition_,
                                           renderCameraPosition_ + forward,
                                           Vector3::Up);
        cameraProjection_ = Matrix::CreatePerspectiveFieldOfView(
            (viewpoint_==FixedViewpoint::BoarDetail||viewpoint_==FixedViewpoint::GoatDetail||UsesSpiderViewpoint())
                ?0.50f:std::numbers::pi_v<float> / 3.0f,
            aspect, 0.08f, 1200.0f);
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
        return 2 + terrainRenderer_->DrawCallCount()+streamWaterRenderer_->DrawCallCount()
            + palmRenderer_->DrawCallCount() + jungleRenderer_->DrawCallCount()
            + naturalPropRenderer_->DrawCallCount() + grassRenderer_->DrawCallCount()
            + shipwreckRenderer_->DrawCallCount()
            + (hutRenderer_?hutRenderer_->DrawCallCount():0)
            + distantSceneryRenderer_->DrawCallCount()
            + contactShadowRenderer_->DrawCallCount()
            + birdRenderer_->DrawCallCount() + butterflyRenderer_->DrawCallCount()
            + crabRenderer_->DrawCallCount()
            + boarRenderer_->DrawCallCount()
            + goatRenderer_->DrawCallCount()
            + spiderRenderer_->DrawCallCount()
            + snakeRenderer_->DrawCallCount()
            + fishRenderer_->DrawCallCount()
            + seaTurtleRenderer_->DrawCallCount()
            + sharkRenderer_->DrawCallCount() + monkeyRenderer_->DrawCallCount()
            + shorelineRenderer_->DrawCallCount() + waterViewOverlay_->DrawCallCount();
    }

    void IslandGame::RecordBenchmarkSceneSample()
    {
        const int trackedDraws = TrackedSceneDrawCount();
        const int terrainCells = terrainRenderer_->VisibleCellCount();
        const int terrainDraws = terrainRenderer_->DrawCallCount();
        const int contactShadows = contactShadowRenderer_->VisibleShadowCount();
        const int palms = palmRenderer_->VisiblePalmCount();
        const int canopy = jungleRenderer_->VisibleCanopyCount();
        const int grass = grassRenderer_->VisiblePatchCount();
        const int crabs = crabRenderer_->VisibleCrabCount();
        const int boars = boarRenderer_->VisibleCount();
        const int butterflies = butterflyRenderer_->VisibleButterflyCount();
        const int seaTurtles = seaTurtleRenderer_->VisibleTurtleCount();
        const float shoreDistance = terrain_.SignedShoreDistance(
            cameraPosition_.X, cameraPosition_.Z);
        BenchmarkSceneRanges& ranges = benchmarkSceneRanges_;
        if (ranges.sampleCount == 0U) {
            ranges.minimumTrackedDraws = ranges.maximumTrackedDraws = trackedDraws;
            ranges.minimumTerrainCells = ranges.maximumTerrainCells = terrainCells;
            ranges.minimumTerrainDraws = ranges.maximumTerrainDraws = terrainDraws;
            ranges.minimumContactShadows = ranges.maximumContactShadows = contactShadows;
            ranges.minimumPalms = ranges.maximumPalms = palms;
            ranges.minimumCanopy = ranges.maximumCanopy = canopy;
            ranges.minimumGrass = ranges.maximumGrass = grass;
            ranges.minimumCrabs = ranges.maximumCrabs = crabs;
            ranges.minimumBoars = ranges.maximumBoars = boars;
            ranges.minimumButterflies = ranges.maximumButterflies = butterflies;
            ranges.minimumSeaTurtles = ranges.maximumSeaTurtles = seaTurtles;
            ranges.minimumShoreDistance = ranges.maximumShoreDistance = shoreDistance;
            ranges.firstCameraPosition = cameraPosition_;
        } else {
            ranges.minimumTrackedDraws = std::min(ranges.minimumTrackedDraws, trackedDraws);
            ranges.maximumTrackedDraws = std::max(ranges.maximumTrackedDraws, trackedDraws);
            ranges.minimumTerrainCells = std::min(ranges.minimumTerrainCells, terrainCells);
            ranges.maximumTerrainCells = std::max(ranges.maximumTerrainCells, terrainCells);
            ranges.minimumTerrainDraws = std::min(ranges.minimumTerrainDraws, terrainDraws);
            ranges.maximumTerrainDraws = std::max(ranges.maximumTerrainDraws, terrainDraws);
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
            ranges.minimumCrabs = std::min(ranges.minimumCrabs, crabs);
            ranges.maximumCrabs = std::max(ranges.maximumCrabs, crabs);
            ranges.minimumBoars = std::min(ranges.minimumBoars, boars);
            ranges.maximumBoars = std::max(ranges.maximumBoars, boars);
            ranges.minimumButterflies = std::min(
                ranges.minimumButterflies, butterflies);
            ranges.maximumButterflies = std::max(
                ranges.maximumButterflies, butterflies);
            ranges.minimumSeaTurtles = std::min(
                ranges.minimumSeaTurtles, seaTurtles);
            ranges.maximumSeaTurtles = std::max(
                ranges.maximumSeaTurtles, seaTurtles);
            ranges.minimumShoreDistance = std::min(
                ranges.minimumShoreDistance, shoreDistance);
            ranges.maximumShoreDistance = std::max(
                ranges.maximumShoreDistance, shoreDistance);
        }
        ranges.totalTrackedDraws += static_cast<std::uint64_t>(trackedDraws);
        ranges.totalTerrainCells += static_cast<std::uint64_t>(terrainCells);
        ranges.totalTerrainDraws += static_cast<std::uint64_t>(terrainDraws);
        ranges.totalContactShadows += static_cast<std::uint64_t>(contactShadows);
        ranges.totalPalms += static_cast<std::uint64_t>(palms);
        ranges.totalCanopy += static_cast<std::uint64_t>(canopy);
        ranges.totalGrass += static_cast<std::uint64_t>(grass);
        ranges.totalCrabs += static_cast<std::uint64_t>(crabs);
        ranges.totalBoars += static_cast<std::uint64_t>(boars);
        ranges.totalButterflies += static_cast<std::uint64_t>(butterflies);
        ranges.totalSeaTurtles += static_cast<std::uint64_t>(seaTurtles);
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
               << "  \"schema_version\": 3,\n"
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
               << "    \"mean_fps\": " << summary.meanFramesPerSecond << ",\n"
               << "    \"budget_misses\": {\"frames_over_8_333_ms\": "
               << summary.framesOver120HzBudget
               << ", \"frames_over_16_667_ms\": " << summary.framesOver60HzBudget
               << ", \"frames_over_33_333_ms\": " << summary.framesOver30HzBudget
               << "}\n"
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
               << "    \"terrain_draw_submissions\": {\"min\": "
               << ranges.minimumTerrainDraws << ", \"max\": "
               << ranges.maximumTerrainDraws << ", \"mean\": "
               << meanCount(ranges.totalTerrainDraws) << "},\n"
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
               << meanCount(ranges.totalGrass) << "},\n"
               << "    \"crabs\": {\"min\": " << ranges.minimumCrabs
               << ", \"max\": " << ranges.maximumCrabs << ", \"mean\": "
               << meanCount(ranges.totalCrabs) << "},\n"
               << "    \"boars\": {\"min\": " << ranges.minimumBoars
               << ", \"max\": " << ranges.maximumBoars << ", \"mean\": "
               << meanCount(ranges.totalBoars) << "},\n"
               << "    \"butterflies\": {\"min\": "
               << ranges.minimumButterflies << ", \"max\": "
               << ranges.maximumButterflies << ", \"mean\": "
               << meanCount(ranges.totalButterflies) << "},\n"
               << "    \"sea_turtles\": {\"min\": "
               << ranges.minimumSeaTurtles << ", \"max\": "
               << ranges.maximumSeaTurtles << ", \"mean\": "
               << meanCount(ranges.totalSeaTurtles) << "}\n"
               << "  },\n"
               << "  \"scene\": {\n"
               << "    \"tracked_draw_submissions\": " << trackedSceneDraws << ",\n"
               << "    \"terrain_cells\": " << terrainRenderer_->VisibleCellCount() << ",\n"
               << "    \"terrain_draw_submissions\": "
               << terrainRenderer_->DrawCallCount() << ",\n"
               << "    \"distant_islets\": "
               << distantSceneryRenderer_->IsletCount() << ",\n"
               << "    \"contact_shadows\": "
               << contactShadowRenderer_->VisibleShadowCount() << ",\n"
               << "    \"contact_shadow_triangles\": "
               << contactShadowRenderer_->PrimitiveCount() << ",\n"
               << "    \"coastal_contact_shadows\": "
               << contactShadowRenderer_->VisibleCoastalShadowCount() << ",\n"
               << "    \"palms\": " << palmRenderer_->VisiblePalmCount() << ",\n"
               << "    \"palms_near\": " << palmRenderer_->VisibleNearPalmCount() << ",\n"
               << "    \"palms_far\": " << palmRenderer_->VisibleFarPalmCount() << ",\n"
               << "    \"canopy\": " << jungleRenderer_->VisibleCanopyCount() << ",\n"
               << "    \"canopy_near\": "
               << jungleRenderer_->VisibleCanopyNearCount() << ",\n"
               << "    \"canopy_far\": "
               << jungleRenderer_->VisibleCanopyFarCount() << ",\n"
               << "    \"canopy_frustum_rejected\": "
               << jungleRenderer_->CanopyFrustumRejectedCount() << ",\n"
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
               << "    \"reef_habitat\": "
               << naturalPropRenderer_->VisibleReefHabitatCount() << ",\n"
               << "    \"fallen_logs\": " << naturalPropRenderer_->VisibleLogCount() << ",\n"
               << "    \"driftwood\": " << naturalPropRenderer_->VisibleDriftwoodCount()
               << ",\n"
               << "    \"fallen_fronds\": "
               << naturalPropRenderer_->VisibleFallenFrondCount() << ",\n"
               << "    \"beach_details\": "
               << naturalPropRenderer_->VisibleBeachDetailCount() << ",\n"
               << "    \"birds\": " << birdRenderer_->VisibleBirdCount() << ",\n"
               << "    \"butterflies\": "
               << butterflyRenderer_->VisibleButterflyCount() << ",\n"
               << "    \"crabs\": " << crabRenderer_->VisibleCrabCount() << ",\n"
               << "    \"boars\": " << boarRenderer_->VisibleCount() << ",\n"
               << "    \"goats\": " << goatRenderer_->VisibleCount() << ",\n"
               << "    \"spiders\": " << spiderRenderer_->VisibleCount() << ",\n"
               << "    \"fish\": " << fishRenderer_->VisibleFishCount() << ",\n"
               << "    \"sea_turtles\": "
               << seaTurtleRenderer_->VisibleTurtleCount() << ",\n"
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

    void IslandGame::RecordStartupTiming(
        const std::chrono::steady_clock::time_point drawStarted)
    {
        if (startupTimingRecorded_) {
            return;
        }
        const auto drawFinished = std::chrono::steady_clock::now();
        postContentMilliseconds_ = std::chrono::duration<double, std::milli>(
            drawStarted - contentFinished_).count();
        firstDrawMilliseconds_ = std::chrono::duration<double, std::milli>(
            drawFinished - drawStarted).count();
        totalStartupMilliseconds_ = std::chrono::duration<double, std::milli>(
            drawFinished - constructionStarted_).count();
        startupTimingRecorded_ = true;

        std::ostringstream status;
        status.imbue(std::locale::classic());
        status << std::fixed << std::setprecision(3)
               << "island-simulator: startup world " << worldGenerationMilliseconds_
               << " ms, framework " << frameworkInitializationMilliseconds_
               << " ms, content " << contentCreationMilliseconds_
               << " ms, pre-draw " << postContentMilliseconds_
               << " ms, first draw " << firstDrawMilliseconds_
               << " ms, total " << totalStartupMilliseconds_ << " ms\n";
        const std::string startupDiagnostic = status.str();
        std::cout << startupDiagnostic;
        MirrorRuntimeDiagnosticToPlatformLog(startupDiagnostic);
        if (!startupReportPath_.empty()) {
            WriteStartupReport();
            std::cout << "island-simulator: startup report " << startupReportPath_
                      << " first draw recorded\n";
        }
    }

    void IslandGame::WriteStartupReport()
    {
        std::ofstream output(startupReportPath_, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error(
                "Cannot create startup report at '" + startupReportPath_ + "'");
        }
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        output.imbue(std::locale::classic());
        output << std::fixed << std::setprecision(4)
               << "{\n"
               << "  \"schema_version\": 1,\n"
               << "  \"renderer\": \""
               << getGraphicsDeviceProperty().GetGraphicsRendererName() << "\",\n"
               << "  \"viewpoint\": \"" << ViewpointName(viewpoint_) << "\",\n"
               << "  \"resolution\": {\"width\": " << viewport.getWidthProperty()
               << ", \"height\": " << viewport.getHeightProperty() << "},\n"
               << "  \"audio\": {\"attempted\": "
               << (soundscape_ && soundscape_->WasStartAttempted() ? "true" : "false")
               << ", \"available\": "
               << (soundscape_ && soundscape_->IsAvailable() ? "true" : "false")
               << ", \"deferred_for_gesture\": "
               << (soundscapeNeedsGesture_ ? "true" : "false")
               << ", \"skipped_for_smoke_test\": "
               << (smokeTest_ ? "true" : "false") << "},\n"
               << "  \"timing_ms\": {\n"
               << "    \"world_generation\": " << worldGenerationMilliseconds_ << ",\n"
               << "    \"framework_initialization\": "
               << frameworkInitializationMilliseconds_ << ",\n"
               << "    \"gpu_content_creation\": " << contentCreationMilliseconds_ << ",\n"
               << "    \"post_content_to_first_draw\": " << postContentMilliseconds_ << ",\n"
               << "    \"first_draw\": " << firstDrawMilliseconds_ << ",\n"
               << "    \"total\": " << totalStartupMilliseconds_ << "\n"
               << "  }\n"
               << "}\n";
        if (!output) {
            throw std::runtime_error(
                "Failed while writing startup report at '" + startupReportPath_ + "'");
        }
    }

    void IslandGame::Draw(const GameTime& gameTime)
    {
        const std::size_t treeQaStage = drawnFrames_/4U;
        const std::span<const MonkeyState> renderedMonkeys = treeQaStage < monkeyTreeQaPoses_.size()
            ? std::span<const MonkeyState>(monkeyTreeQaPoses_[treeQaStage]) : monkeyTroop_.Monkeys();
        const auto drawStarted = std::chrono::steady_clock::now();
        auto& device = getGraphicsDeviceProperty();
        BeginGpuDiagnosticFrame(drawnFrames_, drawnFrames_ < 2U
            && (viewpoint_ == FixedViewpoint::SnakeDetail || viewpoint_ == FixedViewpoint::SnakeHeadDetail));
        if (drawnFrames_ == 1U) RecordRuntimeThreadInventory();
        if (!benchmarkPath_.empty() && !benchmarkReportPending_) {
            RecordBenchmarkFrame();
        }
        debugOverlay_->UpdateFrameTiming();
        {
            GpuDiagnosticScope gpuPass("clear");
            device.Clear(Color(136, 202, 224, 255), 1.0f);
        }
        UpdateCameraMatrices();
        {
            GpuDiagnosticScope gpuPass("sky");
            skyRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);
        }
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        {
            GpuDiagnosticScope gpuPass("terrain");
            terrainRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);
        }
        {
            GpuDiagnosticScope gpuPass("distant-scenery");
            distantSceneryRenderer_->Draw(cameraView_, cameraProjection_);
        }
        {
            GpuDiagnosticScope gpuPass("contact-shadows");
            contactShadowRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                         renderedMonkeys, boarHerd_.Boars(),*snakes_,goats_->Goats(),spiders_->Spiders());
        }

        const float timeSeconds = canopyLodInspection_ ? 1.0f : UsesSnakeCaptureClock()
            ? static_cast<float>(snakeCaptureUpdates_)*0.1f : UsesGoatCaptureClock()
            ? static_cast<float>(goatCaptureUpdates_)*0.1f : UsesSpiderCaptureClock()
            ? static_cast<float>(spiderCaptureUpdates_)*0.1f : UsesBoarCaptureClock()
            ? static_cast<float>(boarCaptureUpdates_)*0.1f : static_cast<float>(gameTime.getTotalGameTimeProperty()
                                                         .getTotalSecondsProperty());
        {
            GpuDiagnosticScope gpuPass("natural-props");
            naturalPropRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);
        }
        {
            GpuDiagnosticScope gpuPass("shipwreck");
            shipwreckRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_);
        }
        if(hutRenderer_) {
            GpuDiagnosticScope gpuPass("hut");
            hutRenderer_->Draw(cameraView_,cameraProjection_,renderCameraPosition_);
        }

        {
            GpuDiagnosticScope gpuPass("jungle");
            jungleRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                  timeSeconds,renderedMonkeys);
        }

        const PlayerPosition& grassPlayer = playerMotion_.Position();
        {
            GpuDiagnosticScope gpuPass("grass");
            grassRenderer_->Draw(
                cameraView_, cameraProjection_, renderCameraPosition_,
                Vector3(grassPlayer.x, grassPlayer.y, grassPlayer.z),
                grassInteractionModel_.Strength(), timeSeconds, SnakeGrassContact::Build(snakes_->Snakes()));
        }

        {
            GpuDiagnosticScope gpuPass("palms");
            palmRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                timeSeconds);
        }

        {
            GpuDiagnosticScope gpuPass("butterflies");
            butterflyRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                     butterflyMeadow_.Butterflies());
        }

        {
            GpuDiagnosticScope gpuPass("crabs");
            crabRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                crabColony_.Crabs());
        }

        {
            GpuDiagnosticScope gpuPass("boars");
            boarRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_, boarHerd_.Boars());
        }
        {
            GpuDiagnosticScope gpuPass("snakes");
            snakeRenderer_->Draw(cameraView_,cameraProjection_,renderCameraPosition_,snakes_->Snakes());
        }
        {
            GpuDiagnosticScope gpuPass("goats");
            goatRenderer_->Draw(cameraView_,cameraProjection_,renderCameraPosition_,goats_->Goats());
        }
        {
            GpuDiagnosticScope gpuPass("spiders");
            spiderRenderer_->Draw(cameraView_,cameraProjection_,renderCameraPosition_,spiders_->Spiders());
        }

        {
            GpuDiagnosticScope gpuPass("birds");
            birdRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                birdFlock_.Birds());
        }

        {
            GpuDiagnosticScope gpuPass("fish");
            fishRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                fishSchools_.Fish(), timeSeconds);
        }

        {
            GpuDiagnosticScope gpuPass("sea-turtles");
            seaTurtleRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                     seaTurtleGroup_.Turtles(), timeSeconds);
        }

        {
            GpuDiagnosticScope gpuPass("shark");
            sharkRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                 sharkPatrol_.State(), timeSeconds);
        }

        {
            GpuDiagnosticScope gpuPass("monkeys");
            monkeyRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                  renderedMonkeys,timeSeconds,monkeyQaSubjectIndex_);
        }

        const PlayerPosition& oceanPlayer = playerMotion_.Position();
        {
            GpuDiagnosticScope gpuPass("ocean");
            oceanRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                 timeSeconds, oceanPlayer.x, oceanPlayer.z,
                                 waterRippleModel_.Intensity(), waterRippleModel_.Phase(),
                                 waterRippleModel_.SplashIntensity(),
                                 waterRippleModel_.SplashPhase());
        }
        {
            GpuDiagnosticScope gpuPass("shoreline");
            shorelineRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                     timeSeconds);
        }
        {
            GpuDiagnosticScope gpuPass("stream-water");
            streamWaterRenderer_->Draw(cameraView_,cameraProjection_,renderCameraPosition_,timeSeconds);
        }

        const auto& viewport = device.getViewportProperty();
        {
            GpuDiagnosticScope gpuPass("water-view-overlay");
            waterViewOverlay_->Draw(waterViewModel_.Intensity(),
                                    playerMotion_.OceanCurrentStrength(terrain_),
                                    viewport.getWidthProperty(), viewport.getHeightProperty());
        }

        if (locomotionModeNoticeSeconds_ > 0.0f) {
            const float opacity = std::clamp(
                locomotionModeNoticeSeconds_ / 0.32f, 0.0f, 1.0f);
            GpuDiagnosticScope gpuPass("locomotion-overlay");
            debugOverlay_->DrawLocomotionModeNotice(
                locomotionPaceToggle_.IsRunningSelected(), opacity,
                viewport.getWidthProperty(), viewport.getHeightProperty(),flightMotion_.IsActive());
        }

        if (debugOverlayVisible_ && worldDebugVisible_) {
            const SoundscapeDebugState soundscapeDebug = soundscape_
                ? soundscape_->DebugState() : SoundscapeDebugState{};
            const OceanCurrentDrift oceanCurrent =
                playerMotion_.OceanCurrentVelocity(terrain_);
            GpuDiagnosticScope gpuPass("world-debug");
            worldDebugRenderer_->Draw(cameraView_, cameraProjection_, renderCameraPosition_,
                                      playerMotion_.Position(), playerMotion_.GroundHeight(),
                                      playerMotion_.Mode() == LocomotionMode::Swimming,
                                      oceanCurrent,
                                      playerMotion_.MovementTrace(),
                                      worldCollisionGrid_.Obstacles(), birdFlock_.Birds(),
                                      birdFlock_.Routes(), birdFlock_.RouteRegions(),
                                      fishSchools_.Fish(), fishSchools_.Schools(),
                                      sharkPatrol_.State(), monkeyTroop_.Monkeys(),
                                      soundscapeDebug);
        }

        if (touchOverlayVisible_) {
            const auto& viewport = device.getViewportProperty();
            GpuDiagnosticScope gpuPass("touch-overlay");
            touchOverlay_->Draw(touchControlState_, touchDisplayWidth_, touchDisplayHeight_,
                                viewport.getWidthProperty(), viewport.getHeightProperty(), true);
        }

        if (debugOverlayVisible_) {
            const PlayerPosition& player = playerMotion_.Position();
            DebugOverlayState overlayState;
            overlayState.playerPosition = Vector3(player.x, player.y, player.z);
            overlayState.cameraPosition = renderCameraPosition_;
            overlayState.freeCameraActive = IsDebugFreeCameraActive();
            overlayState.flightActive = flightMotion_.IsActive();
            overlayState.renderer = device.GetGraphicsRendererName();
            overlayState.viewportWidth = viewport.getWidthProperty();
            overlayState.viewportHeight = viewport.getHeightProperty();
            overlayState.worldSeeds = AuthoritativeWorldSeeds();
            overlayState.worldSeedCopyFeedbackActive =
                worldSeedCopyFeedbackSeconds_ > 0.0f;
            overlayState.worldSeedCopySucceeded = worldSeedCopySucceeded_;
            overlayState.swimming = playerMotion_.Mode() == LocomotionMode::Swimming;
            overlayState.running = playerMotion_.IsRunning();
            overlayState.groundHeight = playerMotion_.GroundHeight();
            overlayState.groundSlopeDegrees = playerMotion_.GroundSlopeRadians()
                * 180.0f / std::numbers::pi_v<float>;
            overlayState.offshoreDistance = playerMotion_.OffshoreDistance(terrain_);
            overlayState.oceanCurrentStrength = playerMotion_.OceanCurrentStrength(terrain_);
            const OceanCurrentDrift oceanCurrent =
                playerMotion_.OceanCurrentVelocity(terrain_);
            overlayState.oceanCurrentX = oceanCurrent.x;
            overlayState.oceanCurrentZ = oceanCurrent.z;
            overlayState.waterRippleIntensity = waterRippleModel_.Intensity();
            overlayState.waterRipplePhase = waterRippleModel_.Phase();
            overlayState.waterSplashIntensity = waterRippleModel_.SplashIntensity();
            overlayState.waterSplashPhase = waterRippleModel_.SplashPhase();
            overlayState.waterViewIntensity = waterViewModel_.Intensity();
            overlayState.waterViewDraws = waterViewOverlay_->DrawCallCount();
            overlayState.visibleTerrainCells = terrainRenderer_->VisibleCellCount();
            overlayState.totalTerrainCells = terrainRenderer_->TotalCellCount();
            overlayState.terrainDraws = terrainRenderer_->DrawCallCount();
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
            overlayState.nearGrassPatches = grassRenderer_->VisibleNearPatchCount();
            overlayState.middleGrassPatches = grassRenderer_->VisibleMiddlePatchCount();
            overlayState.farGrassPatches = grassRenderer_->VisibleFarPatchCount();
            overlayState.mixedGrassLodCells = grassRenderer_->MixedLodCellCount();
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
            overlayState.visibleButterflies =
                butterflyRenderer_->VisibleButterflyCount();
            overlayState.totalButterflies = static_cast<int>(
                butterflyMeadow_.Butterflies().size());
            overlayState.nearButterflies =
                butterflyRenderer_->VisibleNearButterflyCount();
            overlayState.farButterflies =
                butterflyRenderer_->VisibleFarButterflyCount();
            overlayState.fullSimulationButterflies =
                butterflyMeadow_.FullSimulationCount();
            overlayState.reducedSimulationButterflies =
                butterflyMeadow_.ReducedSimulationCount();
            overlayState.dormantSimulationButterflies =
                butterflyMeadow_.DormantSimulationCount();
            for (const ButterflyState& butterfly : butterflyMeadow_.Butterflies()) {
                switch (butterfly.behavior) {
                case ButterflyBehavior::Rest: ++overlayState.restingButterflies; break;
                case ButterflyBehavior::NectarHover: ++overlayState.hoveringButterflies; break;
                case ButterflyBehavior::Flutter: ++overlayState.flutteringButterflies; break;
                }
            }
            overlayState.butterflyDraws = butterflyRenderer_->DrawCallCount();
            overlayState.visibleCrabs = crabRenderer_->VisibleCrabCount();
            overlayState.visibleBoars = boarRenderer_->VisibleCount();
            overlayState.visibleGoats = goatRenderer_->VisibleCount();
            overlayState.totalGoats = static_cast<int>(goats_->Goats().size());
            overlayState.visibleSpiders = spiderRenderer_->VisibleCount();
            overlayState.totalSpiders = static_cast<int>(spiders_->Spiders().size());
            overlayState.spiderDraws = spiderRenderer_->DrawCallCount();
            overlayState.spiderShadows = contactShadowRenderer_->VisibleSpiderShadowCount();
            overlayState.goatDraws = goatRenderer_->DrawCallCount();
            overlayState.goatShadows = contactShadowRenderer_->VisibleGoatShadowCount();
            overlayState.totalBoars = static_cast<int>(boarHerd_.Boars().size());
            overlayState.boarDraws = boarRenderer_->DrawCallCount();
            overlayState.boarShadows = contactShadowRenderer_->VisibleBoarShadowCount();
            overlayState.visibleSnakes = snakeRenderer_->VisibleCount();
            overlayState.totalSnakes = static_cast<int>(snakes_->Snakes().size());
            overlayState.snakeShadows = contactShadowRenderer_->VisibleSnakeShadowCount();
            overlayState.snakeDraws = snakeRenderer_->DrawCallCount();
            overlayState.totalCrabs = static_cast<int>(crabColony_.Crabs().size());
            overlayState.nearCrabs = crabRenderer_->VisibleNearCrabCount();
            overlayState.farCrabs = crabRenderer_->VisibleFarCrabCount();
            overlayState.fullSimulationCrabs = crabColony_.FullSimulationCount();
            overlayState.reducedSimulationCrabs = crabColony_.ReducedSimulationCount();
            overlayState.dormantSimulationCrabs = crabColony_.DormantSimulationCount();
            overlayState.crabDraws = crabRenderer_->DrawCallCount();
            overlayState.visibleFish = fishRenderer_->VisibleFishCount();
            overlayState.totalFish = static_cast<int>(fishSchools_.Fish().size());
            overlayState.nearFish = fishRenderer_->VisibleNearFishCount();
            overlayState.farFish = fishRenderer_->VisibleFarFishCount();
            overlayState.fullSimulationFish = fishSchools_.FullSimulationCount();
            overlayState.reducedSimulationFish = fishSchools_.ReducedSimulationCount();
            overlayState.dormantSimulationFish = fishSchools_.DormantSimulationCount();
            overlayState.fishDraws = fishRenderer_->DrawCallCount();
            overlayState.visibleSeaTurtles = seaTurtleRenderer_->VisibleTurtleCount();
            overlayState.totalSeaTurtles = static_cast<int>(seaTurtleGroup_.Turtles().size());
            overlayState.nearSeaTurtles = seaTurtleRenderer_->VisibleNearTurtleCount();
            overlayState.farSeaTurtles = seaTurtleRenderer_->VisibleFarTurtleCount();
            overlayState.fullSimulationSeaTurtles = seaTurtleGroup_.FullSimulationCount();
            overlayState.reducedSimulationSeaTurtles = seaTurtleGroup_.ReducedSimulationCount();
            overlayState.dormantSimulationSeaTurtles = seaTurtleGroup_.DormantSimulationCount();
            for (const SeaTurtleState& turtle : seaTurtleGroup_.Turtles()) {
                switch (turtle.behavior) {
                case SeaTurtleBehavior::Cruise: ++overlayState.cruisingSeaTurtles; break;
                case SeaTurtleBehavior::Forage: ++overlayState.foragingSeaTurtles; break;
                case SeaTurtleBehavior::Surface: ++overlayState.surfacingSeaTurtles; break;
                }
            }
            overlayState.seaTurtleDraws = seaTurtleRenderer_->DrawCallCount();
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
                case MonkeyBehavior::InspectTree:
                    ++overlayState.observeMonkeys;
                    break;
                case MonkeyBehavior::Wander:
                case MonkeyBehavior::Walk:
                case MonkeyBehavior::ApproachTree:
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
                overlayState.playedFootsteps = soundscape_->PlayedFootstepCount();
                overlayState.playedBirdCalls = soundscape_->PlayedBirdCallCount();
                overlayState.playedMonkeyCalls = soundscape_->PlayedMonkeyCallCount();
                overlayState.playedBoarCalls = soundscape_->PlayedBoarCallCount();
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
                overlayState.worldDebugBirdRouteRegions =
                    worldDebugRenderer_->BirdRouteRegionCount();
                overlayState.worldDebugBirdSpawns =
                    worldDebugRenderer_->BirdSpawnMarkerCount();
                overlayState.worldDebugBirdRouteTargets =
                    worldDebugRenderer_->BirdRouteTargetCount();
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
                overlayState.movementObstacleDepenetrations =
                    movementTrace.obstacleDepenetrations;
                overlayState.movementSafetyClamped = movementTrace.safetyClamped;
                overlayState.worldDebugLines = worldDebugRenderer_->LineCount();
                overlayState.worldDebugDraws = worldDebugRenderer_->DrawCallCount();
            }
            GpuDiagnosticScope gpuPass("debug-overlay");
            debugOverlay_->Draw(overlayState);
        }

        if (drawnFrames_ == 0U) {
            RecordStartupTiming(drawStarted);
            const PlayerPosition& player = playerMotion_.Position();
            const float currentStrength = playerMotion_.OceanCurrentStrength(terrain_);
            const float undertowTarget = WaterMovementModel::TargetMix(
                std::max(0.0f, -playerMotion_.GroundHeight()),
                playerMotion_.Mode() == LocomotionMode::Swimming,
                std::hypot(playerMotion_.VelocityX(), playerMotion_.VelocityZ()),
                currentStrength).currentCue;
            std::ostringstream viewportStatus;
            viewportStatus << "island-simulator: viewport "
                           << viewport.getWidthProperty() << 'x'
                           << viewport.getHeightProperty() << " logical backbuffer";
            const std::string viewportDiagnostic = viewportStatus.str();
            std::cout << viewportDiagnostic << std::endl;
            MirrorRuntimeDiagnosticToPlatformLog(viewportDiagnostic);
            std::cout << "island-simulator: visible terrain cells "
                      << terrainRenderer_->VisibleCellCount() << '/'
                      << terrainRenderer_->TotalCellCount() << " in "
                      << terrainRenderer_->DrawCallCount()
                      << " shared-buffer draws, collision candidates "
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
                      << jungleRenderer_->TotalUnderstoryCount() << " understory (near "
                      << jungleRenderer_->VisibleUnderstoryNearCount() << ", far "
                      << jungleRenderer_->VisibleUnderstoryFarCount() << "), "
                      << jungleRenderer_->VisibleBushCount() << '/'
                      << jungleRenderer_->TotalBushCount() << " bushes (near "
                      << jungleRenderer_->VisibleBushNearCount() << ", far "
                      << jungleRenderer_->VisibleBushFarCount() << "), "
                      << jungleRenderer_->VisibleGroundPlantCount() << '/'
                      << jungleRenderer_->TotalGroundPlantCount() << " ground plants (near "
                      << jungleRenderer_->VisibleGroundPlantNearCount() << ", far "
                      << jungleRenderer_->VisibleGroundPlantFarCount() << "), "
                      << jungleRenderer_->DrawCallCount() << " draws; canopy frustum rejects "
                      << jungleRenderer_->CanopyFrustumRejectedCount() << "; props "
                      << naturalPropRenderer_->VisibleRockCount() << '/'
                      << naturalPropRenderer_->TotalRockCount() << " rocks, "
                      << naturalPropRenderer_->VisibleReefRockCount() << '/'
                      << naturalPropRenderer_->TotalReefRockCount() << " reef outcrops, "
                      << naturalPropRenderer_->VisibleReefHabitatCount() << '/'
                      << naturalPropRenderer_->TotalReefHabitatCount() << " reef habitat, "
                      << naturalPropRenderer_->VisibleLogCount() << '/'
                      << naturalPropRenderer_->TotalLogCount() << " logs, "
                      << naturalPropRenderer_->VisibleDriftwoodCount() << '/'
                      << naturalPropRenderer_->TotalDriftwoodCount() << " driftwood (near "
                      << naturalPropRenderer_->VisibleDriftwoodNearCount() << ", far "
                      << naturalPropRenderer_->VisibleDriftwoodFarCount() << "), "
                      << naturalPropRenderer_->VisibleBeachDetailCount() << '/'
                      << naturalPropRenderer_->TotalBeachDetailCount()
                      << " shell/pebble/wrack clusters, "
                      << naturalPropRenderer_->VisibleFallenFrondCount() << '/'
                      << naturalPropRenderer_->TotalFallenFrondCount()
                      << " fallen fronds, "
                      << naturalPropRenderer_->DrawCallCount() << " draws; shipwreck "
                      << (shipwreckRenderer_->IsVisible() ? "visible" : "culled") << ", "
                      << shipwreckRenderer_->DebrisCount() << " debris, "
                      << shipwreckRenderer_->DrawCallCount() << " draw; hut "
                      << (hutRenderer_?hutRenderer_->DrawCallCount():0) << " draw, "
                      << contactShadowRenderer_->VisibleHutShadowCount() << " contact patches; grass "
                      << grassRenderer_->VisiblePatchCount() << '/'
                      << grassRenderer_->TotalPatchCount() << " patches in "
                      << grassRenderer_->VisibleCellCount() << " cells, "
                      << grassRenderer_->DrawCallCount() << " draws, interaction "
                      << grassInteractionModel_.Strength() << ", custom shader "
                      << (grassRenderer_->UsesCustomShader() ? 1 : 0) << ", biome tint "
                      << (grassRenderer_->UsesCustomShader() ? 1 : 0) << ", LOD near "
                      << grassRenderer_->VisibleNearPatchCount() << ", middle "
                      << grassRenderer_->VisibleMiddlePatchCount() << ", far "
                      << grassRenderer_->VisibleFarPatchCount() << ", mixed cells "
                      << grassRenderer_->MixedLodCellCount() << "; birds "
                      << birdRenderer_->VisibleBirdCount() << '/'
                      << birdFlock_.Birds().size() << " visible (near "
                      << birdRenderer_->VisibleNearBirdCount() << ", far "
                      << birdRenderer_->VisibleFarBirdCount() << "), "
                      << birdRenderer_->DrawCallCount() << " draw; butterflies "
                      << butterflyRenderer_->VisibleButterflyCount() << '/'
                      << butterflyMeadow_.Butterflies().size() << " visible (near "
                      << butterflyRenderer_->VisibleNearButterflyCount() << ", far "
                      << butterflyRenderer_->VisibleFarButterflyCount() << "), "
                      << butterflyRenderer_->DrawCallCount() << " draw; crabs "
                      << crabRenderer_->VisibleCrabCount() << '/'
                      << crabColony_.Crabs().size() << " visible (near "
                      << crabRenderer_->VisibleNearCrabCount() << ", far "
                      << crabRenderer_->VisibleFarCrabCount() << "), "
                      << crabRenderer_->DrawCallCount() << " draw; boars "
                      << boarRenderer_->VisibleCount() << '/' << boarHerd_.Boars().size()
                      << " visible, " << boarRenderer_->DrawCallCount() << " draw; goats "
                      << goatRenderer_->VisibleCount() << '/' << goats_->Goats().size()
                      << " visible, " << goatRenderer_->DrawCallCount() << " draw; snakes "
                      << snakeRenderer_->VisibleCount() << '/' << snakes_->Snakes().size()
                      << " visible, " << snakeRenderer_->DrawCallCount() << " draw, "
                      << contactShadowRenderer_->VisibleSnakeShadowCount() << " shadows; fish "
                      << fishRenderer_->VisibleFishCount() << '/'
                      << fishSchools_.Fish().size() << " visible (near "
                      << fishRenderer_->VisibleNearFishCount() << ", far "
                      << fishRenderer_->VisibleFarFishCount() << "), "
                      << fishRenderer_->DrawCallCount() << " draw; turtles "
                      << seaTurtleRenderer_->VisibleTurtleCount() << '/'
                      << seaTurtleGroup_.Turtles().size() << " visible (near "
                      << seaTurtleRenderer_->VisibleNearTurtleCount() << ", far "
                      << seaTurtleRenderer_->VisibleFarTurtleCount() << "), "
                      << seaTurtleRenderer_->DrawCallCount() << " draw; shark "
                      << (sharkRenderer_->IsVisible() ? "visible" : "culled")
                      << " activity " << sharkPatrol_.State().activity << ", "
                      << sharkRenderer_->DrawCallCount() << " draw; monkeys "
                      << monkeyRenderer_->VisibleMonkeyCount() << '/'
                      << monkeyTroop_.Monkeys().size() << " visible, "
                      << monkeyRenderer_->DrawCallCount() << " draws, blends "
                      << monkeyRenderer_->ActiveCrossfadeCount() << '/'
                      << monkeyRenderer_->CompletedCrossfadeCount()
                      << " active/completed, canopy transmission "
                      << monkeyRenderer_->MinimumCanopyTransmission() << ".."
                      << monkeyRenderer_->MaximumCanopyTransmission() << "; distant "
                      << distantSceneryRenderer_->IsletCount() << " islets, "
                      << distantSceneryRenderer_->DrawCallCount()
                      << " draw; contact shadows "
                      << contactShadowRenderer_->VisibleShadowCount() << ", "
                      << contactShadowRenderer_->PrimitiveCount() << " triangles, "
                      << contactShadowRenderer_->DrawCallCount()
                      << " draw; coastal contact shadows " << contactShadowRenderer_->VisibleCoastalShadowCount()
                      << "; water view " << waterViewModel_.Intensity()
                      << ", " << waterViewOverlay_->DrawCallCount()
                      << " draw, player ripple " << waterRippleModel_.Intensity()
                      << ", phase " << waterRippleModel_.Phase()
                      << ", splash " << waterRippleModel_.SplashIntensity()
                      << ", phase " << waterRippleModel_.SplashPhase() << ", current "
                      << currentStrength << ", undertow target " << undertowTarget << '\n';
            if (viewpoint_ == FixedViewpoint::GroundPlantDetail) {
                std::cout << "island-simulator: ground QA per-instance near LOD "
                          << jungleRenderer_->VisibleGroundPlantNearCount() << " plants\n";
            }
            if (viewpoint_ == FixedViewpoint::DriftwoodDetail) {
                std::cout << "island-simulator: driftwood QA per-instance near LOD "
                          << naturalPropRenderer_->VisibleDriftwoodNearCount() << " pieces\n";
            }
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
                          << "; bird routes " << worldDebugRenderer_->BirdRouteRegionCount()
                          << ", spawns " << worldDebugRenderer_->BirdSpawnMarkerCount()
                          << ", radial targets "
                          << worldDebugRenderer_->BirdRouteTargetCount()
                          << "; movement depenetrate/obstacle/terrain/slide/clamp "
                          << movementTrace.obstacleDepenetrations << '/'
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
        if(UsesSpiderViewpoint()&&drawnFrames_==1U) {
            std::ostringstream report;
            report << "island-simulator: spider render proof: spiders " << spiderRenderer_->VisibleCount()
                << '/' << spiders_->Spiders().size() << " visible, " << spiderRenderer_->DrawCallCount() << " draw;"
                << " contact-shadows=" << contactShadowRenderer_->VisibleSpiderShadowCount();
            std::cout << report.str() << std::endl;MirrorRuntimeDiagnosticToPlatformLog(report.str());
            const auto& subject=spiders_->Spiders()[spiderViewSubject_];
            const float groundTransmission=canopyLightField_.Sample(subject.x,subject.z);
            const float bodyTransmission=canopyLightField_.SampleReceiver(
                subject.x,subject.y+.04f,subject.z,subject.y);
            const auto tint=CanopyTransmissionTint(bodyTransmission);
            std::ostringstream lighting;lighting.precision(9);
            lighting << "island-simulator: spider lighting ground-transmission=" << groundTransmission
                << " body-transmission=" << bodyTransmission
                << " body-tint=" << tint.red << ',' << tint.green << ',' << tint.blue;
            std::cout << lighting.str() << std::endl;MirrorRuntimeDiagnosticToPlatformLog(lighting.str());
        }
        if(viewpoint_==FixedViewpoint::GoatDetail&&drawnFrames_==1U) {
            std::ostringstream report;
            report << "island-simulator: goat render proof: goats " << goatRenderer_->VisibleCount()
                << '/' << goats_->Goats().size() << " visible, " << goatRenderer_->DrawCallCount() << " draw;";
            std::cout << report.str() << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(report.str());
        }
        const bool snakeCapture = UsesSnakeCaptureClock();
        const bool goatCapture = UsesGoatCaptureClock();
        const bool boarCapture = UsesBoarCaptureClock();
        if(boarCapture&&drawnFrames_%8U==0&&drawnFrames_<=64U) {
            GpuDiagnosticScope gpuPass("capture-readback");
            CaptureFrame(capturePath_+".boar-stage-"+std::to_string(drawnFrames_/8U-1U)+".ppm");
            const auto& subject=boarHerd_.Boars().front();
            std::ostringstream report;report.precision(9);
            report << "island-simulator: boar motion stage=" << drawnFrames_/8U-1U
                << " updates=" << boarCaptureUpdates_ << " time=" << timeSeconds
                << " visible=" << boarRenderer_->VisibleCount() << " position="
                << subject.x << ',' << subject.y << ',' << subject.z
                << " speed=" << subject.speed << " phase=" << subject.gaitPhase
                << " rooting=" << subject.rootingPose;
            std::cout << report.str() << std::endl;
        }
        const bool spiderCapture = UsesSpiderCaptureClock();
        if(spiderCapture&&drawnFrames_%8U==0&&drawnFrames_<=64U) {
            GpuDiagnosticScope gpuPass("capture-readback");
            CaptureFrame(capturePath_+".spider-stage-"+std::to_string(drawnFrames_/8U-1U)+".ppm");
            const auto& subject=spiders_->Spiders()[spiderViewSubject_];
            std::ostringstream report;report.precision(9);
            report << "island-simulator: spider motion stage=" << drawnFrames_/8U-1U
                << " updates=" << spiderCaptureUpdates_ << " time=" << timeSeconds
                << " visible=" << spiderRenderer_->VisibleCount() << " position="
                << subject.x << ',' << subject.y << ',' << subject.z
                << " heading=" << subject.heading << " speed=" << subject.speed << " feet=";
            for(std::size_t leg=0;leg<8;++leg) {
                const auto p=subject.pose.feet[leg];
                report << p.x << ',' << p.y << ',' << p.z << ',' << (subject.pose.stance[leg]?1:0) << ';';
            }
            std::cout << report.str() << std::endl;
        }
        if(goatCapture&&drawnFrames_%8U==0&&drawnFrames_<=64U) {
            GpuDiagnosticScope gpuPass("capture-readback");
            CaptureFrame(capturePath_+".goat-stage-"+std::to_string(drawnFrames_/8U-1U)+".ppm");
            const auto& subject=goats_->Goats().front();
            std::cout << "island-simulator: goat motion stage=" << drawnFrames_/8U-1U
                << " updates=" << goatCaptureUpdates_ << " time=" << timeSeconds
                << " visible=" << goatRenderer_->VisibleCount() << " position="
                << subject.x << ',' << subject.y << ',' << subject.z
                << " speed=" << subject.speed << " phase=" << subject.phase
                << " grazing=" << subject.grazingPose << '\n';
        }
        if(!snakeCapture && (viewpoint_==FixedViewpoint::SnakeDetail || viewpoint_==FixedViewpoint::SnakeHeadDetail)
            && drawnFrames_%8U==0 && drawnFrames_<=32U) {
            const auto& subject=snakes_->Snakes().front();
            const auto head=subject.body.front();
            std::ostringstream report;
            report << "island-simulator: snake runtime sample frame=" << drawnFrames_
                << " head=" << head.x << ',' << head.y << ',' << head.z
                << " speed=" << subject.speed << " visible=" << snakeRenderer_->VisibleCount();
            std::cout << report.str() << '\n' << std::flush;
            MirrorRuntimeDiagnosticToPlatformLog(report.str());
        }
        if (snakeCapture && drawnFrames_%8U == 0 && drawnFrames_ <= 32U) {
            GpuDiagnosticScope gpuPass("capture-readback");
            CaptureFrame(capturePath_+".snake-stage-"+std::to_string(drawnFrames_/8U-1U)+".ppm");
            const auto& snake = snakes_->Snakes().front();
            std::cout << "island-simulator: snake motion stage=" << drawnFrames_/8U-1U
                << " updates=" << snakeCaptureUpdates_ << " time=" << timeSeconds
                << " visible=" << snakeRenderer_->VisibleCount() << " speed=" << snake.speed
                << " phase=" << snake.phase << " body=";
            for (const auto& p : snake.body) std::cout << p.x << ',' << p.y << ',' << p.z << ';';
            std::cout << '\n';
        }
        if (treeQaStage < monkeyTreeQaPoses_.size() && drawnFrames_%4U == 0) {
            GpuDiagnosticScope gpuPass("capture-readback");
            if (!capturePath_.empty()) CaptureFrame(capturePath_+".tree-stage-"+std::to_string(treeQaStage)+".ppm");
            const auto& subject = renderedMonkeys[monkeyQaSubjectIndex_];
            const auto weights = monkeyRenderer_->SubjectTreeReachWeights();
            std::cout << "island-simulator: monkey tree stage=" << treeQaStage
                << " subject=" << monkeyQaSubjectIndex_ << " tree=" << subject.treeIndex
                << " elapsed=" << subject.treeInspectionSeconds << " clock=" << subject.animationSeconds
                << " weights=" << weights[0] << ',' << weights[1] << '\n';
            if (viewpoint_==FixedViewpoint::MonkeyClimbDetail) {
                const auto centre=MonkeyVisualCentre(subject);
                std::ostringstream report;
                report << "island-simulator: monkey climb stage=" << treeQaStage << " subject=" << monkeyQaSubjectIndex_
                    << " tree=" << subject.treeIndex << " time=" << subject.treeClock.seconds
                    << " active=" << static_cast<bool>(subject.treeExcursion) << " centre-y=" << centre.y;
                std::cout << report.str() << '\n';
                if (capturePath_.empty()) MirrorRuntimeDiagnosticToPlatformLog(report.str());
            }
        }
        if (viewpoint_==FixedViewpoint::MonkeyClimbDetail && drawnFrames_==36U) {
            const char* report="island-simulator: monkey climb held summit presented after eight cycle stages";
            std::cout << report << '\n';
            if (capturePath_.empty()) MirrorRuntimeDiagnosticToPlatformLog(report);
        }
        const std::uint32_t targetFrames = goatCapture||spiderCapture||boarCapture ? 64U : snakeCapture ? 32U : !monkeyTreeQaPoses_.empty() ? 36U : !capturePath_.empty()
                && (viewpoint_ == FixedViewpoint::MonkeyDetail || viewpoint_ == FixedViewpoint::MonkeyTreeDetail || viewpoint_ == FixedViewpoint::MonkeyClimbDetail)
            ? MonkeyCaptureFrames : SmokeTestFrames;
        if (!capturePath_.empty() && drawnFrames_ == targetFrames) {
            {
                GpuDiagnosticScope gpuPass("capture-readback");
                CaptureFrame(capturePath_);
            }
            if (viewpoint_ == FixedViewpoint::MonkeyDetail || viewpoint_ == FixedViewpoint::MonkeyTreeDetail || viewpoint_ == FixedViewpoint::MonkeyClimbDetail) {
                const auto& subject = monkeyTroop_.Monkeys()[monkeyQaSubjectIndex_];
                std::cout << "island-simulator: monkey capture visible "
                          << monkeyRenderer_->VisibleMonkeyCount() << "; near "
                          << monkeyRenderer_->VisibleNearMonkeyCount() << "; draws "
                          << monkeyRenderer_->DrawCallCount() << "; subject "
                          << subject.x << ',' << subject.y << ',' << subject.z << '\n';
                std::cout << "island-simulator: monkey subject paired tree grips "
                          << monkeyRenderer_->SubjectHasTreeGrips() << '\n';
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
        EndGpuDiagnosticFrame();
    }

    GetTypeNameCPP(IslandGame, "IslandSimulator.IslandGame")
}
