#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/OceanMeshLayout.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/SoundscapeModel.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <numbers>
#include <string_view>

namespace
{
    int failures = 0;

    void Check(const bool condition, const std::string_view description)
    {
        if (!condition) {
            std::cerr << "FAIL: " << description << '\n';
            ++failures;
        }
    }

    void CheckNear(const float actual, const float expected, const float tolerance,
                   const std::string_view description)
    {
        Check(std::abs(actual - expected) <= tolerance, description);
    }
}

int main()
{
    const island::AmbientLoop surfLoop = island::GenerateSurfLoop();
    const island::AmbientLoop repeatedSurfLoop = island::GenerateSurfLoop();
    const island::AmbientLoop breezeLoop = island::GenerateBreezeLoop();
    Check(surfLoop.sampleRate == 22050 && breezeLoop.sampleRate == 22050,
          "procedural ambience uses the intended portable sample rate");
    Check(surfLoop.samples.size() == 220500U && breezeLoop.samples.size() == 220500U,
          "procedural ambient loops are ten seconds long");
    Check(surfLoop.samples == repeatedSurfLoop.samples,
          "procedural surf generation is deterministic");
    const auto peakMagnitude = [](const island::AmbientLoop& loop) {
        int peak = 0;
        for (const std::int16_t sample : loop.samples) {
            peak = std::max(peak, std::abs(static_cast<int>(sample)));
        }
        return peak;
    };
    const auto rootMeanSquare = [](const island::AmbientLoop& loop) {
        double squaredSum = 0.0;
        for (const std::int16_t sample : loop.samples) {
            const double value = static_cast<double>(sample);
            squaredSum += value * value;
        }
        return std::sqrt(squaredSum / static_cast<double>(loop.samples.size()));
    };
    Check(peakMagnitude(surfLoop) < 32767 && peakMagnitude(breezeLoop) < 32767,
          "procedural ambience retains headroom before runtime mixing");
    Check(rootMeanSquare(surfLoop) > 1200.0 && rootMeanSquare(breezeLoop) > 900.0,
          "procedural ambience contains meaningful audible energy");
    Check(std::abs(static_cast<int>(surfLoop.samples.front())
                   - static_cast<int>(surfLoop.samples.back())) < 8000,
          "surf loop boundary does not contain a large discontinuity");
    Check(std::abs(static_cast<int>(breezeLoop.samples.front())
                   - static_cast<int>(breezeLoop.samples.back())) < 8000,
          "breeze loop boundary does not contain a large discontinuity");

    const island::SoundscapeMix beachMix = island::SoundscapeModel::TargetMix(2.0f, false);
    const island::SoundscapeMix jungleMix = island::SoundscapeModel::TargetMix(140.0f, false);
    const island::SoundscapeMix swimmingMix = island::SoundscapeModel::TargetMix(-8.0f, true);
    Check(beachMix.surfVolume > jungleMix.surfVolume,
          "surf is louder at the beach than deep inland");
    Check(jungleMix.breezeVolume > beachMix.breezeVolume,
          "canopy breeze grows toward the island interior");
    Check(swimmingMix.surfVolume >= 0.35f && swimmingMix.breezeVolume < beachMix.breezeVolume,
          "swimming emphasizes nearby water while softening canopy breeze");
    island::SoundscapeModel soundscapeModel;
    soundscapeModel.Reset(2.0f, false);
    const float initialSurfVolume = soundscapeModel.Mix().surfVolume;
    for (int step = 0; step < 300; ++step) {
        (void)soundscapeModel.Update(140.0f, false, 1.0f / 60.0f);
    }
    Check(soundscapeModel.Mix().surfVolume < initialSurfVolume
              && soundscapeModel.Mix().surfVolume > jungleMix.surfVolume,
          "soundscape transitions smoothly toward the new zone instead of snapping");
    CheckNear(soundscapeModel.Mix().surfVolume, jungleMix.surfVolume, 0.025f,
              "soundscape approaches the inland surf target over several seconds");

    island::TouchControls touchControls;
    std::array<island::TouchContact, 2> touchContacts = {{
        {11, 180.0f, 520.0f, island::TouchPhase::Pressed},
        {23, 1000.0f, 360.0f, island::TouchPhase::Pressed},
    }};
    island::TouchControlState touchState = touchControls.Update(touchContacts, 1280.0f, 720.0f);
    Check(touchState.movementActive && touchState.lookActive,
          "touch controls assign simultaneous left movement and right look contacts");
    CheckNear(touchState.moveForward, 0.0f, 0.0001f,
              "movement touch begins at a neutral floating-stick anchor");
    CheckNear(touchState.lookDeltaX, 0.0f, 0.0001f,
              "look touch begins without a camera jump");
    CheckNear(touchState.lookTouchX, 1000.0f, 0.0001f,
              "look presentation tracks its initial contact position");

    touchContacts[0] = {11, 230.0f, 440.0f, island::TouchPhase::Moved};
    touchContacts[1] = {23, 1032.0f, 344.0f, island::TouchPhase::Moved};
    touchState = touchControls.Update(touchContacts, 1280.0f, 720.0f);
    Check(touchState.moveForward > 0.4f && touchState.moveRight > 0.2f,
          "left touch produces proportional forward and strafe movement");
    CheckNear(touchState.lookDeltaX, 32.0f, 0.0001f,
              "right touch produces relative horizontal look delta");
    CheckNear(touchState.lookDeltaY, -16.0f, 0.0001f,
              "right touch produces relative vertical look delta");
    CheckNear(touchState.lookTouchX, 1032.0f, 0.0001f,
              "look presentation follows the owned contact");
    CheckNear(touchState.lookTouchY, 344.0f, 0.0001f,
              "look presentation preserves vertical contact position");

    touchContacts[0] = {11, 900.0f, -200.0f, island::TouchPhase::Moved};
    touchState = touchControls.Update(touchContacts, 1280.0f, 720.0f);
    Check(std::hypot(touchState.moveRight, touchState.moveForward) <= 1.0001f,
          "movement stick clamps extreme drags to unit magnitude");
    Check(touchState.movementActive && touchState.lookActive,
          "touch roles remain owned after contacts cross the screen centre");

    const std::array<island::TouchContact, 1> releasedMovement = {{
        {11, 900.0f, -200.0f, island::TouchPhase::Released},
    }};
    touchState = touchControls.Update(releasedMovement, 1280.0f, 720.0f);
    Check(!touchState.movementActive && !touchState.lookActive,
          "released and missing contacts cancel both touch roles");
    CheckNear(touchState.moveForward, 0.0f, 0.0001f,
              "cancelled movement returns neutral output");

    const std::array<island::TouchContact, 1> tinyMovement = {{
        {31, 160.0f, 500.0f, island::TouchPhase::Pressed},
    }};
    (void)touchControls.Update(tinyMovement, 1280.0f, 720.0f);
    const std::array<island::TouchContact, 1> tinyMovementMoved = {{
        {31, 164.0f, 496.0f, island::TouchPhase::Moved},
    }};
    touchState = touchControls.Update(tinyMovementMoved, 1280.0f, 720.0f);
    CheckNear(touchState.moveForward, 0.0f, 0.0001f,
              "movement dead zone rejects small finger jitter");
    touchState = touchControls.Update(tinyMovementMoved, 1000.0f, 600.0f);
    Check(touchState.movementActive && touchState.moveForward == 0.0f,
          "display resize safely re-anchors an active movement contact");

    const island::IslandTerrain terrain;

    Check(island::OceanMeshLayout::VertexCount == 25921U,
          "ocean grid remains within the portable 16-bit vertex budget");
    Check(island::OceanMeshLayout::TriangleCount == 51200U,
          "ocean grid triangle count is deterministic");
    CheckNear(island::OceanMeshLayout::Coordinate(0), -island::OceanMeshLayout::Extent,
              0.0001f, "ocean grid begins at its documented extent");
    CheckNear(island::OceanMeshLayout::Coordinate(island::OceanMeshLayout::QuadsPerAxis),
              island::OceanMeshLayout::Extent, 0.0001f,
              "ocean grid ends at its documented extent");

    Check(island::TerrainMeshLayout::CellCount == 64U,
          "terrain layout has the expected bounded cell count");
    std::size_t partitionTriangleCount = 0;
    for (std::size_t index = 0; index < island::TerrainMeshLayout::CellCount; ++index) {
        const island::TerrainCellLayout cell = island::TerrainMeshLayout::Cell(index);
        Check(cell.quadColumns == island::TerrainMeshLayout::QuadsPerCell
                  && cell.quadRows == island::TerrainMeshLayout::QuadsPerCell,
              "terrain cells use a uniform portable grid");
        Check(cell.minimumX < cell.maximumX && cell.minimumZ < cell.maximumZ,
              "terrain cell bounds have positive area");
        partitionTriangleCount += static_cast<std::size_t>(cell.quadColumns * cell.quadRows * 2);
        if (cell.column + 1 < island::TerrainMeshLayout::CellsPerAxis) {
            const island::TerrainCellLayout right = island::TerrainMeshLayout::Cell(index + 1U);
            CheckNear(cell.maximumX, right.minimumX, 0.0001f,
                      "horizontal terrain cell boundaries share an exact seam");
        }
        if (cell.row + 1 < island::TerrainMeshLayout::CellsPerAxis) {
            const island::TerrainCellLayout front = island::TerrainMeshLayout::Cell(
                index + island::TerrainMeshLayout::CellsPerAxis);
            CheckNear(cell.maximumZ, front.minimumZ, 0.0001f,
                      "vertical terrain cell boundaries share an exact seam");
        }
    }
    Check(partitionTriangleCount
              == static_cast<std::size_t>(island::TerrainMeshLayout::QuadsPerAxis
                                           * island::TerrainMeshLayout::QuadsPerAxis * 2),
          "terrain cells cover every source-grid triangle exactly once");

    Check(terrain.IsLand(0.0f, 0.0f), "island centre is land");
    Check(!terrain.IsLand(420.0f, 0.0f), "far offshore point is water");
    Check(terrain.Height(420.0f, 0.0f) < -3.0f, "offshore seabed descends");
    Check(terrain.Height(0.0f, 0.0f) > 8.0f, "interior has useful elevation");

    const island::TerrainSample centreSample = terrain.Sample(0.0f, 0.0f);
    const float centreNormalLength = std::hypot(
        centreSample.normal.y, std::hypot(centreSample.normal.x, centreSample.normal.z));
    CheckNear(centreNormalLength, 1.0f, 0.0001f, "terrain sample normal is normalized");
    Check(centreSample.normal.y > 0.0f, "terrain sample normal points upward");
    Check(centreSample.surface == island::TerrainSurface::InteriorGround
              || centreSample.surface == island::TerrainSurface::SteepGround,
          "island centre has an interior ground classification");

    const island::TerrainSample deepWaterSample = terrain.Sample(420.0f, 0.0f);
    Check(deepWaterSample.IsSubmerged(), "deep-water sample reports submerged terrain");
    Check(deepWaterSample.surface == island::TerrainSurface::DeepSeabed,
          "far offshore terrain is classified as deep seabed");

    float minimumRadius = 1000.0f;
    float maximumRadius = 0.0f;
    for (int step = 0; step < 720; ++step) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(step) / 720.0f;
        const float radius = terrain.ShoreRadius(angle);
        minimumRadius = std::min(minimumRadius, radius);
        maximumRadius = std::max(maximumRadius, radius);
        const float x = std::cos(angle) * radius;
        const float z = std::sin(angle) * radius;
        CheckNear(terrain.SignedShoreDistance(x, z), 0.0f, 0.001f,
                  "shore parameterization agrees with shore distance");
        Check(std::isfinite(terrain.Height(x, z)), "shore height is finite");
    }
    Check(minimumRadius > 185.0f, "shoreline never collapses inward");
    Check(maximumRadius < 275.0f, "shoreline remains near the 500 metre target");
    Check(maximumRadius - minimumRadius > 20.0f, "shoreline silhouette has variation");

    const island::VegetationLayout vegetation(terrain);
    const island::VegetationLayout repeatedVegetation(terrain);
    Check(vegetation.Palms().size() >= 180U && vegetation.Palms().size() <= 520U,
          "palm population stays within the intended first performance budget");
    Check(vegetation.Palms().size() == repeatedVegetation.Palms().size(),
          "palm population is deterministic");
    Check(vegetation.Obstacles().size() == vegetation.Palms().size(),
          "every palm has exactly one trunk collision proxy");
    for (int z = -216; z <= 180; z += 18) {
        const float pathX = island::VegetationLayout::PrimaryPathCenterX(static_cast<float>(z));
        Check(island::VegetationLayout::IsReservedRoute(pathX, static_cast<float>(z)),
              "the authored primary path centre remains reserved from vegetation");
    }
    for (std::size_t index = 0; index < vegetation.Palms().size(); ++index) {
        const island::PalmPlacement& palm = vegetation.Palms()[index];
        const island::PalmPlacement& repeated = repeatedVegetation.Palms()[index];
        const island::TerrainSample sample = terrain.Sample(palm.x, palm.z);
        Check(palm.x == repeated.x && palm.y == repeated.y && palm.z == repeated.z
                  && palm.yawRadians == repeated.yawRadians && palm.scale == repeated.scale
                  && palm.variant == repeated.variant,
              "palm placement data is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 7.0f && sample.signedShoreDistance <= 158.0f,
              "palms remain inside the authored coastal/interior belt");
        Check(sample.slopeRadians <= 0.4712389f,
              "palms reject terrain steeper than the placement limit");
        Check(!island::VegetationLayout::IsReservedRoute(palm.x, palm.z),
              "palms keep paths, clearings, and player spawn open");
        Check(palm.variant < island::VegetationLayout::PalmVariantCount,
              "palm variant index remains in range");
        Check(palm.scale >= island::VegetationLayout::PalmMinimumScale
                  && palm.scale <= island::VegetationLayout::PalmMaximumScale,
              "palm scale remains in the authored range");
        const island::CircularObstacle& obstacle = vegetation.Obstacles()[index];
        CheckNear(obstacle.x, palm.x, 0.0001f, "palm obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, palm.z, 0.0001f, "palm obstacle shares its visual z coordinate");
        Check(obstacle.radius > 0.20f && obstacle.radius < 0.45f,
              "palm collision radius stays close to the visible trunk");
    }

    const island::GrassLayout grass(terrain);
    const island::GrassLayout repeatedGrass(terrain);
    Check(grass.Patches().size() >= 10000U && grass.Patches().size() <= 40000U,
          "grass patch population stays inside the first geometry budget");
    Check(grass.Patches().size() == repeatedGrass.Patches().size(),
          "grass population is deterministic");
    for (std::size_t index = 0; index < grass.Patches().size(); ++index) {
        const island::GrassPlacement& patch = grass.Patches()[index];
        const island::GrassPlacement& repeated = repeatedGrass.Patches()[index];
        const island::TerrainSample sample = terrain.Sample(patch.x, patch.z);
        Check(patch.x == repeated.x && patch.y == repeated.y && patch.z == repeated.z
                  && patch.yawRadians == repeated.yawRadians && patch.scale == repeated.scale
                  && patch.spreadScale == repeated.spreadScale
                  && patch.windPhase == repeated.windPhase
                  && patch.colorVariation == repeated.colorVariation
                  && patch.pathScale == repeated.pathScale,
              "grass placement data is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 20.0f && sample.height >= 2.1f,
              "grass remains above the sand transition");
        Check(sample.slopeRadians <= 0.4886922f,
              "grass rejects terrain steeper than its placement limit");
        Check(patch.windPhase >= 0.0f && patch.windPhase < 1.0f
                  && patch.colorVariation >= 0.0f && patch.colorVariation < 1.0f,
              "grass shader variation values stay normalized");
        Check(patch.spreadScale >= island::GrassLayout::MinimumSpreadScale
                  && patch.spreadScale <= island::GrassLayout::MaximumSpreadScale,
              "grass horizontal spread remains in the authored range");
        if (patch.pathScale) {
            Check(island::VegetationLayout::IsReservedRoute(patch.x, patch.z),
                  "short grass is confined to authored route regions");
            Check(patch.scale < island::GrassLayout::MinimumScale,
                  "authored route grass remains shorter than ordinary grass");
        } else {
            Check(patch.scale >= island::GrassLayout::MinimumScale
                      && patch.scale <= island::GrassLayout::MaximumScale,
                  "ordinary grass scale remains in the authored range");
        }
    }

    const island::JungleLayout jungle(terrain);
    const island::JungleLayout repeatedJungle(terrain);
    Check(jungle.Canopy().size() >= 100U && jungle.Canopy().size() <= 600U,
          "broadleaf canopy population stays inside its first performance budget");
    Check(jungle.Understory().size() >= 800U && jungle.Understory().size() <= 4000U,
          "understory population stays inside its first performance budget");
    Check(jungle.Bushes().size() >= 150U && jungle.Bushes().size() <= 1400U,
          "low-bush population stays inside its first performance budget");
    Check(jungle.Canopy().size() == repeatedJungle.Canopy().size()
              && jungle.Understory().size() == repeatedJungle.Understory().size()
              && jungle.Bushes().size() == repeatedJungle.Bushes().size(),
          "jungle populations are deterministic");
    Check(jungle.Obstacles().size() == jungle.Canopy().size(),
          "every broadleaf canopy trunk has one collision proxy");
    for (std::size_t index = 0; index < jungle.Canopy().size(); ++index) {
        const island::CanopyPlacement& tree = jungle.Canopy()[index];
        const island::CanopyPlacement& repeated = repeatedJungle.Canopy()[index];
        const island::TerrainSample sample = terrain.Sample(tree.x, tree.z);
        Check(tree.x == repeated.x && tree.y == repeated.y && tree.z == repeated.z
                  && tree.yawRadians == repeated.yawRadians && tree.scale == repeated.scale
                  && tree.variant == repeated.variant,
              "broadleaf canopy placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 43.0f
                  && sample.signedShoreDistance <= 238.0f,
              "broadleaf trees remain in the interior jungle belt");
        Check(sample.height >= 3.8f && sample.slopeRadians <= 0.4712389f,
              "broadleaf trees remain on suitable interior ground");
        Check(!island::VegetationLayout::IsReservedRoute(tree.x, tree.z),
              "broadleaf trees keep paths, clearings, and player spawn open");
        Check(tree.variant < island::JungleLayout::CanopyVariantCount,
              "broadleaf canopy variant index remains in range");
        Check(tree.scale >= island::JungleLayout::CanopyMinimumScale
                  && tree.scale <= island::JungleLayout::CanopyMaximumScale,
              "broadleaf canopy scale remains in the authored range");
        const island::CircularObstacle& obstacle = jungle.Obstacles()[index];
        CheckNear(obstacle.x, tree.x, 0.0001f,
                  "broadleaf obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, tree.z, 0.0001f,
                  "broadleaf obstacle shares its visual z coordinate");
        Check(obstacle.radius > 0.30f && obstacle.radius < 0.56f,
              "broadleaf collision radius stays close to the visible trunk");
    }
    for (std::size_t index = 0; index < jungle.Understory().size(); ++index) {
        const island::UnderstoryPlacement& plant = jungle.Understory()[index];
        const island::UnderstoryPlacement& repeated = repeatedJungle.Understory()[index];
        const island::TerrainSample sample = terrain.Sample(plant.x, plant.z);
        Check(plant.x == repeated.x && plant.y == repeated.y && plant.z == repeated.z
                  && plant.yawRadians == repeated.yawRadians && plant.scale == repeated.scale
                  && plant.variant == repeated.variant,
              "understory placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 31.0f
                  && sample.signedShoreDistance <= 242.0f,
              "understory remains inside its interior placement belt");
        Check(sample.height >= 2.8f && sample.slopeRadians <= 0.5410521f,
              "understory remains on suitable interior ground");
        Check(!island::VegetationLayout::IsReservedRoute(plant.x, plant.z),
              "understory keeps paths, clearings, and player spawn open");
        Check(plant.variant < island::JungleLayout::UnderstoryVariantCount,
              "understory variant index remains in range");
        Check(plant.scale >= island::JungleLayout::UnderstoryMinimumScale
                  && plant.scale <= island::JungleLayout::UnderstoryMaximumScale,
              "understory scale remains in the authored range");
    }
    for (std::size_t index = 0; index < jungle.Bushes().size(); ++index) {
        const island::BushPlacement& bush = jungle.Bushes()[index];
        const island::BushPlacement& repeated = repeatedJungle.Bushes()[index];
        const island::TerrainSample sample = terrain.Sample(bush.x, bush.z);
        Check(bush.x == repeated.x && bush.y == repeated.y && bush.z == repeated.z
                  && bush.yawRadians == repeated.yawRadians && bush.scale == repeated.scale
                  && bush.variant == repeated.variant,
              "low-bush placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 38.0f
                  && sample.signedShoreDistance <= 232.0f,
              "low bushes remain inside their interior placement belt");
        Check(sample.height >= 3.0f && sample.slopeRadians <= 0.5235988f,
              "low bushes remain on suitable interior ground");
        Check(!island::VegetationLayout::IsReservedRoute(bush.x, bush.z),
              "low bushes keep paths, clearings, and player spawn open");
        Check(bush.variant < island::JungleLayout::BushVariantCount,
              "low-bush variant index remains in range");
        Check(bush.scale >= island::JungleLayout::BushMinimumScale
                  && bush.scale <= island::JungleLayout::BushMaximumScale,
              "low-bush scale remains in the authored range");
    }

    const island::NaturalPropLayout props(terrain, vegetation, jungle);
    const island::NaturalPropLayout repeatedProps(terrain, vegetation, jungle);
    Check(props.Rocks().size() >= 30U && props.Rocks().size() <= 500U,
          "rock population stays inside its first performance budget");
    Check(props.FallenLogs().size() >= 6U && props.FallenLogs().size() <= 80U,
          "fallen-log population stays inside its first composition budget");
    Check(props.Driftwood().size() >= 12U && props.Driftwood().size() <= 100U,
          "driftwood population stays inside its sparse beach-composition budget");
    Check(props.Rocks().size() == repeatedProps.Rocks().size()
              && props.FallenLogs().size() == repeatedProps.FallenLogs().size()
              && props.Driftwood().size() == repeatedProps.Driftwood().size(),
          "natural-prop populations are deterministic");
    std::size_t solidRockCount = 0U;
    for (std::size_t index = 0; index < props.Rocks().size(); ++index) {
        const island::RockPlacement& rock = props.Rocks()[index];
        const island::RockPlacement& repeated = repeatedProps.Rocks()[index];
        const island::TerrainSample sample = terrain.Sample(rock.x, rock.z);
        Check(rock.x == repeated.x && rock.y == repeated.y && rock.z == repeated.z
                  && rock.yawRadians == repeated.yawRadians
                  && rock.scaleX == repeated.scaleX && rock.scaleY == repeated.scaleY
                  && rock.scaleZ == repeated.scaleZ && rock.variant == repeated.variant
                  && rock.solid == repeated.solid,
              "rock placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 1.5f
                  && sample.signedShoreDistance <= 202.0f,
              "rocks stay inside their coastal/interior placement belt");
        Check(sample.height >= 0.15f && sample.slopeRadians <= 0.5759587f,
              "rocks remain on suitable unsubmerged ground");
        Check(!island::VegetationLayout::IsReservedRoute(rock.x, rock.z),
              "rocks keep authored routes and clearings open");
        Check(rock.variant < island::NaturalPropLayout::RockVariantCount,
              "rock variant index remains in range");
        Check(rock.scaleX > 0.35f && rock.scaleX < 2.30f
                  && rock.scaleY > 0.24f && rock.scaleY < 1.70f
                  && rock.scaleZ > 0.35f && rock.scaleZ < 2.35f,
              "rock axis scales remain inside conservative visual bounds");
        if (rock.solid) ++solidRockCount;
    }
    for (std::size_t index = 0; index < props.FallenLogs().size(); ++index) {
        const island::FallenLogPlacement& log = props.FallenLogs()[index];
        const island::FallenLogPlacement& repeated = repeatedProps.FallenLogs()[index];
        Check(log.x == repeated.x && log.y == repeated.y && log.z == repeated.z
                  && log.yawRadians == repeated.yawRadians && log.length == repeated.length
                  && log.radius == repeated.radius && log.variant == repeated.variant,
              "fallen-log placement is bitwise deterministic for one seed");
        Check(log.length >= island::NaturalPropLayout::LogMinimumLength
                  && log.length <= island::NaturalPropLayout::LogMaximumLength,
              "fallen-log length remains in the authored range");
        Check(log.radius >= 0.24f && log.radius <= 0.43f,
              "fallen-log radius remains in the authored range");
        Check(log.variant < island::NaturalPropLayout::LogVariantCount,
              "fallen-log variant index remains in range");
        const float directionX = std::cos(log.yawRadians);
        const float directionZ = std::sin(log.yawRadians);
        for (int sampleIndex = 0; sampleIndex < 7; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 6.0f - 0.5f;
            const float sampleX = log.x + directionX * log.length * t;
            const float sampleZ = log.z + directionZ * log.length * t;
            Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                  "the complete fallen-log footprint stays outside authored routes");
            Check(terrain.Sample(sampleX, sampleZ).signedShoreDistance >= 30.0f,
                  "the complete fallen-log footprint stays in the island interior");
        }
    }
    for (std::size_t index = 0; index < props.Driftwood().size(); ++index) {
        const island::DriftwoodPlacement& wood = props.Driftwood()[index];
        const island::DriftwoodPlacement& repeated = repeatedProps.Driftwood()[index];
        Check(wood.x == repeated.x && wood.y == repeated.y && wood.z == repeated.z
                  && wood.yawRadians == repeated.yawRadians && wood.length == repeated.length
                  && wood.radius == repeated.radius && wood.variant == repeated.variant,
              "driftwood placement is bitwise deterministic for one seed");
        Check(wood.length >= island::NaturalPropLayout::DriftwoodMinimumLength
                  && wood.length <= island::NaturalPropLayout::DriftwoodMaximumLength,
              "driftwood length remains in the authored range");
        Check(wood.radius >= 0.09f && wood.radius <= 0.18f,
              "driftwood radius remains in the authored range");
        Check(wood.variant < island::NaturalPropLayout::DriftwoodVariantCount,
              "driftwood variant index remains in range");
        const float directionX = std::cos(wood.yawRadians);
        const float directionZ = std::sin(wood.yawRadians);
        for (int sampleIndex = 0; sampleIndex < 5; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 4.0f - 0.5f;
            const float sampleX = wood.x + directionX * wood.length * t;
            const float sampleZ = wood.z + directionZ * wood.length * t;
            const island::TerrainSample support = terrain.Sample(sampleX, sampleZ);
            Check(support.signedShoreDistance >= 1.5f
                      && support.signedShoreDistance <= 27.0f,
                  "the complete driftwood footprint stays in the dry beach belt");
            Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                  "the complete driftwood footprint stays outside authored routes");
        }
    }
    Check(props.Obstacles().size() == solidRockCount + props.FallenLogs().size() * 7U,
          "natural-prop collision excludes small driftwood and matches solid prop spans");

    for (int z = -360; z <= 360; z += 12) {
        for (int x = -360; x <= 360; x += 12) {
            const float first = terrain.Height(static_cast<float>(x), static_cast<float>(z));
            const float second = terrain.Height(static_cast<float>(x), static_cast<float>(z));
            Check(std::isfinite(first), "terrain height is finite across the world grid");
            Check(first == second, "terrain generation is deterministic");
            Check(first >= -22.01f && first < 40.0f, "terrain height stays in the authored scale");
            const island::TerrainSample sample = terrain.Sample(static_cast<float>(x),
                                                                 static_cast<float>(z));
            Check(std::isfinite(sample.slopeRadians), "terrain slope is finite across the world grid");
            Check(sample.slopeRadians >= 0.0f
                      && sample.slopeRadians <= std::numbers::pi_v<float> * 0.5f,
                  "terrain slope remains in the upward-normal range");
        }
    }

    island::PlayerMotion player;
    player.SetPosition(terrain, 0.0f, 0.0f);
    const island::PlayerPosition start = player.Position();
    Check(player.GroundHeight() >= terrain.Height(0.0f, 0.0f),
          "capsule footprint resolves at or above its centre terrain height");
    Check(player.Position().y >= player.GroundHeight() + island::PlayerMotion::EyeHeight - 0.0001f,
          "walking eye never starts below its resolved ground support");
    Check(player.GroundSlopeRadians() >= terrain.Sample(0.0f, 0.0f).slopeRadians,
          "capsule ground slope includes the centre sample");
    player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f);
    Check(player.Position().x > start.x, "walking input accelerates the player");
    Check(player.VelocityX() > 0.0f && player.VelocityX() < island::PlayerMotion::WalkSpeed,
          "walking acceleration does not jump to full speed");
    Check(player.Mode() == island::LocomotionMode::Walking,
          "interior movement stays in walking mode");
    CheckNear(player.OceanCurrentStrength(terrain), 0.0f, 0.0001f,
              "ocean current remains inactive while walking");

    const float eastShore = terrain.ShoreRadius(0.0f);
    player.SetPosition(terrain, eastShore + 24.0f, 0.0f);
    Check(player.Mode() == island::LocomotionMode::Swimming,
          "sufficiently deep near-shore water enters swimming mode");
    CheckNear(player.Position().y, 0.58f, 0.001f, "swimming eye rests near the water surface");
    CheckNear(player.OceanCurrentStrength(terrain), 0.0f, 0.0001f,
              "ocean current remains inactive inside the near-shore swimming area");

    player.SetPosition(terrain, eastShore + island::PlayerMotion::FullCurrentDistance, 0.0f);
    CheckNear(player.OceanCurrentStrength(terrain), 1.0f, 0.0001f,
              "ocean current reaches full normalized strength at its documented distance");

    player.SetPosition(terrain, eastShore + 80.0f, 0.0f);
    const float offshoreBeforeCurrent = player.OffshoreDistance(terrain);
    for (int step = 0; step < 120; ++step) {
        player.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f);
    }
    Check(player.OffshoreDistance(terrain) < offshoreBeforeCurrent,
          "outer-ocean current gently carries an idle swimmer shoreward");

    player.SetPosition(terrain, eastShore + 88.0f, 0.0f);
    for (int step = 0; step < 900; ++step) {
        player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f);
        Check(player.OffshoreDistance(terrain) <= island::PlayerMotion::SafetyDistance + 0.01f,
              "swimmer cannot escape the ocean safety boundary");
    }
    Check(player.VelocityX() <= 0.01f,
          "safety boundary removes residual outward velocity");

    player.SetPosition(terrain, 0.0f, 0.0f);
    player.Update(terrain, 1.0f, 0.0f, 5.0f);
    Check(player.Position().x - 0.0f < 0.2f,
          "long frame is clamped and cannot teleport the player");

    player.SetPosition(terrain, 35.0f, -20.0f);
    for (int step = 0; step < 600; ++step) {
        const float inputX = std::sin(static_cast<float>(step) * 0.071f);
        const float inputZ = std::cos(static_cast<float>(step) * 0.053f);
        player.Update(terrain, inputX, inputZ, step % 37 == 0 ? 0.1f : 1.0f / 60.0f);
        if (player.Mode() == island::LocomotionMode::Walking) {
            Check(player.Position().y
                      >= player.GroundHeight() + island::PlayerMotion::EyeHeight - 0.0001f,
                  "walking camera remains above the capsule terrain support");
        }
    }

    player.SetPosition(terrain, 0.0f, 0.0f);
    const island::CircularObstacle testTrunk{1.2f, 0.0f, 0.42f};
    for (int step = 0; step < 180; ++step) {
        player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f,
                      std::span<const island::CircularObstacle>(&testTrunk, 1U));
    }
    const float trunkDistance = std::hypot(player.Position().x - testTrunk.x,
                                            player.Position().z - testTrunk.z);
    Check(trunkDistance >= island::PlayerMotion::CapsuleRadius + testTrunk.radius - 0.001f,
          "player capsule cannot enter a palm trunk proxy");
    Check(player.Position().x < testTrunk.x,
          "blocked movement does not tunnel through a trunk");

    if (failures != 0) {
        std::cerr << failures << " core test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "IslandCoreTests: all checks passed\n";
    return EXIT_SUCCESS;
}
