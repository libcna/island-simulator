#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/OceanMeshLayout.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
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
