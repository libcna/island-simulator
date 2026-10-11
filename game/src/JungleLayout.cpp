#include "IslandSimulator/JungleLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        // Dense stands need crowns that can touch: candidates every 10.5 m over the same area.
        constexpr float CanopySpacing = 10.5f;
        constexpr int CanopyRadius = 28;
        constexpr float UnderstorySpacing = 5.8f;
        constexpr int UnderstoryRadius = 50;
        constexpr float BushSpacing = 8.4f;
        constexpr int BushRadius = 35;
        constexpr float GroundPlantSpacing = 4.7f;
        constexpr int GroundPlantRadius = 58;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }
    }

    std::uint32_t JungleLayout::Hash(const std::int32_t x, const std::int32_t z,
                                     const std::uint32_t seed) noexcept
    {
        std::uint32_t value = std::bit_cast<std::uint32_t>(x) * 0x8da6b343U;
        value ^= std::bit_cast<std::uint32_t>(z) * 0xd8163841U;
        value ^= seed * 0xcb1ab31fU;
        value ^= value >> 16U;
        value *= 0x7feb352dU;
        value ^= value >> 15U;
        value *= 0x846ca68bU;
        return value ^ (value >> 16U);
    }

    float JungleLayout::UnitRandom(const std::uint32_t value) noexcept
    {
        return static_cast<float>(value & 0x00ffffffU) / static_cast<float>(0x01000000U);
    }

    float JungleLayout::ClusterNoise(const float x, const float z,
                                     const std::uint32_t seed,
                                     const float cellSize) noexcept
    {
        const float gridX = x / cellSize;
        const float gridZ = z / cellSize;
        const auto x0 = static_cast<std::int32_t>(std::floor(gridX));
        const auto z0 = static_cast<std::int32_t>(std::floor(gridZ));
        const float tx = SmoothStep(gridX - static_cast<float>(x0));
        const float tz = SmoothStep(gridZ - static_cast<float>(z0));
        const auto value = [seed](const std::int32_t cellX, const std::int32_t cellZ) {
            return UnitRandom(Hash(cellX, cellZ, seed));
        };
        const float back = value(x0, z0)
            + (value(x0 + 1, z0) - value(x0, z0)) * tx;
        const float front = value(x0, z0 + 1)
            + (value(x0 + 1, z0 + 1) - value(x0, z0 + 1)) * tx;
        return back + (front - back) * tz;
    }

    JungleLayout::JungleLayout(const IslandTerrain& terrain, const std::uint32_t seed)
    {
        canopy_.reserve(420U);
        obstacles_.reserve(420U);
        struct EmergentCandidate { std::size_t index; bool eligible; float scaleRandom; };
        std::vector<EmergentCandidate> emergentCandidates;
        for (int gridZ = -CanopyRadius; gridZ <= CanopyRadius; ++gridZ) {
            for (int gridX = -CanopyRadius; gridX <= CanopyRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * CanopySpacing * 0.66f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0xc2b2ae35U)) - 0.5f)
                    * CanopySpacing * 0.66f;
                const float x = static_cast<float>(gridX) * CanopySpacing + jitterX;
                const float z = static_cast<float>(gridZ) * CanopySpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 43.0f
                    || sample.signedShoreDistance > 238.0f
                    || sample.height < 3.8f || sample.slopeRadians > 0.4712389f
                    || sample.coastalRockiness > 0.45f
                    || VegetationLayout::IsReservedRoute(x, z)) {
                    continue;
                }

                const float interior = SmoothStep((sample.signedShoreDistance - 43.0f) / 78.0f);
                // Forest masses, not a park: a contrast-shaped stand field makes near-closed
                // stands with open ground between them, plus a thin scatter of solitary trees.
                const float broadCluster = ClusterNoise(x, z, seed ^ 0x9e3779b9U, 61.0f);
                const float fineCluster = ClusterNoise(x, z, seed ^ 0x2545f491U, 23.0f);
                const float stand = SmoothStep(
                    (broadCluster * 0.72f + fineCluster * 0.28f - 0.40f) / 0.24f);
                const float density = std::clamp(
                    (0.52f + 0.48f * interior) * (0.045f + 0.60f * stand), 0.0f, 0.95f);
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0x85ebca6bU)) >= density) {
                    continue;
                }

                CanopyPlacement tree;
                tree.x = x;
                tree.y = sample.height;
                tree.z = z;
                tree.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x27d4eb2fU))
                    * 2.0f * std::numbers::pi_v<float>;
                tree.scale = CanopyMinimumScale
                    + (CanopyMaximumScale - CanopyMinimumScale)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0x165667b1U));
                // Reserve the largest possible trunk before selecting emergents below.
                // Crowns may overhang the trail; their collision trunks may not.
                if (VegetationLayout::OverlapsReservedRoute(x, z,
                        0.43f * CanopyEmergentScale)) continue;
                emergentCandidates.push_back({canopy_.size(),
                    stand > CanopyStandThreshold
                        && UnitRandom(Hash(gridX, gridZ, seed ^ 0x3c6ef372U)) < CanopyEmergentChance,
                    UnitRandom(Hash(gridX, gridZ, seed ^ 0x7f4a7c15U))});
                tree.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xd3a2646cU) % CanopyVariantCount);
                canopy_.push_back(tree);
                obstacles_.push_back(CircularObstacle{tree.x, tree.z, 0.43f * tree.scale});
            }
        }

        // Emergents rise only above a closed stand: promote eligible trees that actually have
        // accepted neighbours, so the stand field alone cannot create a lone giant.
        for (const auto& candidate : emergentCandidates) {
            if (!candidate.eligible) continue;
            CanopyPlacement& tree = canopy_[candidate.index];
            int neighbours = 0;
            for (std::size_t other = 0; other < canopy_.size(); ++other) {
                if (other == candidate.index) continue;
                const float dx = canopy_[other].x - tree.x, dz = canopy_[other].z - tree.z;
                if (dx * dx + dz * dz < 20.0f * 20.0f) ++neighbours;
            }
            if (neighbours < 2) continue;
            tree.scale = CanopyEmergentMinimumScale
                + (CanopyEmergentScale - CanopyEmergentMinimumScale) * candidate.scaleRandom;
            obstacles_[candidate.index].radius = 0.43f * tree.scale;
        }

        understory_.reserve(2600U);
        for (int gridZ = -UnderstoryRadius; gridZ <= UnderstoryRadius; ++gridZ) {
            for (int gridX = -UnderstoryRadius; gridX <= UnderstoryRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed ^ 0x7f4a7c15U);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * UnderstorySpacing * 0.78f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0x94d049bbU)) - 0.5f)
                    * UnderstorySpacing * 0.78f;
                const float x = static_cast<float>(gridX) * UnderstorySpacing + jitterX;
                const float z = static_cast<float>(gridZ) * UnderstorySpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 31.0f
                    || sample.signedShoreDistance > 242.0f
                    || sample.height < 2.8f || sample.slopeRadians > 0.5410521f
                    || sample.coastalRockiness > 0.60f
                    || VegetationLayout::IsReservedRoute(x, z)) {
                    continue;
                }

                const float interior = SmoothStep((sample.signedShoreDistance - 31.0f) / 62.0f);
                const float broadCluster = ClusterNoise(x, z, seed ^ 0x51ed270bU, 39.0f);
                const float fineCluster = ClusterNoise(x, z, seed ^ 0x632be59bU, 17.0f);
                const float density = std::clamp(
                    (0.32f + 0.46f * interior)
                        * (0.48f + 0.54f * broadCluster + 0.20f * fineCluster),
                    0.0f, 0.92f);
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0x4cf5ad43U)) >= density) {
                    continue;
                }

                UnderstoryPlacement plant;
                plant.x = x;
                plant.y = sample.height + 0.018f;
                plant.z = z;
                plant.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0xa24baed5U))
                    * 2.0f * std::numbers::pi_v<float>;
                plant.scale = UnderstoryMinimumScale
                    + (UnderstoryMaximumScale - UnderstoryMinimumScale)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0x9fb21c65U));
                plant.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xb7e15162U) % UnderstoryVariantCount);
                understory_.push_back(plant);
            }
        }

        bushes_.reserve(900U);
        for (int gridZ = -BushRadius; gridZ <= BushRadius; ++gridZ) {
            for (int gridX = -BushRadius; gridX <= BushRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed ^ 0x6a09e667U);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * BushSpacing * 0.74f;
                const float jitterZ = (UnitRandom(Hash(
                    gridX, gridZ, seed ^ 0xbb67ae85U)) - 0.5f) * BushSpacing * 0.74f;
                const float x = static_cast<float>(gridX) * BushSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * BushSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 38.0f
                    || sample.signedShoreDistance > 232.0f
                    || sample.height < 3.0f || sample.slopeRadians > 0.5235988f
                    || sample.coastalRockiness > 0.60f
                    || VegetationLayout::IsReservedRoute(x, z)) {
                    continue;
                }

                const float interior = SmoothStep(
                    (sample.signedShoreDistance - 38.0f) / 70.0f);
                const float broadCluster = ClusterNoise(
                    x, z, seed ^ 0x3c6ef372U, 47.0f);
                const float fineCluster = ClusterNoise(
                    x, z, seed ^ 0xa54ff53aU, 21.0f);
                const float density = std::clamp(
                    (0.20f + 0.31f * interior)
                        * (0.52f + 0.62f * broadCluster + 0.18f * fineCluster),
                    0.0f, 0.68f);
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0x510e527fU)) >= density) {
                    continue;
                }

                BushPlacement bush;
                bush.x = x;
                bush.y = sample.height + 0.012f;
                bush.z = z;
                bush.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x9b05688cU))
                    * 2.0f * std::numbers::pi_v<float>;
                bush.scale = BushMinimumScale + (BushMaximumScale - BushMinimumScale)
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0x1f83d9abU));
                bush.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0x5be0cd19U) % BushVariantCount);
                bushes_.push_back(bush);
            }
        }

        groundPlants_.reserve(4500U);
        for (int gridZ = -GroundPlantRadius; gridZ <= GroundPlantRadius; ++gridZ) {
            for (int gridX = -GroundPlantRadius; gridX <= GroundPlantRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed ^ 0x243f6a88U);
                const float jitterX = (UnitRandom(siteHash) - 0.5f)
                    * GroundPlantSpacing * 0.82f;
                const float jitterZ = (UnitRandom(Hash(
                    gridX, gridZ, seed ^ 0x85a308d3U)) - 0.5f)
                    * GroundPlantSpacing * 0.82f;
                const float x = static_cast<float>(gridX) * GroundPlantSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * GroundPlantSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 27.0f
                    || sample.signedShoreDistance > 238.0f
                    || sample.height < 2.5f || sample.slopeRadians > 0.5585054f
                    || sample.coastalRockiness > 0.60f
                    || VegetationLayout::IsReservedRoute(x, z)) {
                    continue;
                }

                const float interior = SmoothStep(
                    (sample.signedShoreDistance - 27.0f) / 74.0f);
                const float broadCluster = ClusterNoise(
                    x, z, seed ^ 0x13198a2eU, 35.0f);
                const float fineCluster = ClusterNoise(
                    x, z, seed ^ 0x03707344U, 13.0f);
                const float density = std::clamp(
                    (0.20f + 0.34f * interior)
                        * (0.50f + 0.58f * broadCluster + 0.23f * fineCluster),
                    0.0f, 0.71f);
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0xa4093822U)) >= density) {
                    continue;
                }

                GroundPlantPlacement plant;
                plant.x = x;
                plant.y = sample.height + 0.009f;
                plant.z = z;
                plant.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x299f31d0U))
                    * 2.0f * std::numbers::pi_v<float>;
                plant.scale = GroundPlantMinimumScale
                    + (GroundPlantMaximumScale - GroundPlantMinimumScale)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0x082efa98U));
                plant.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xec4e6c89U) % GroundPlantVariantCount);
                groundPlants_.push_back(plant);
            }
        }

        // Independent hash channels preserve the existing island-wide placements.
        const auto clearOfRoots = [](float x, float z, const auto& plants, float separation) {
            return std::none_of(plants.begin(), plants.end(), [=](const auto& plant) {
                return std::hypot(x - plant.x, z - plant.z) < separation;
            });
        };
        const auto clearOfCanopy = [this](float x, float z, float radius) {
            return std::none_of(obstacles_.begin(), obstacles_.end(), [=](const auto& trunk) {
                return std::hypot(x - trunk.x, z - trunk.z) < radius + trunk.radius;
            });
        };

        // Seed-sized companions form irregular ellipses near some canopy trees. Do not fill
        // every tree base or overwrite the separately composed clearing-edge accents.
        forestColonies_.reserve(canopy_.size());
        for (std::size_t treeIndex=0;treeIndex<canopy_.size();++treeIndex) {
            const auto& tree=canopy_[treeIndex];
            const auto keyX=static_cast<std::int32_t>(std::lround(tree.x*1000));
            const auto keyZ=static_cast<std::int32_t>(std::lround(tree.z*1000));
            const auto random=[&](int channel) {
                return UnitRandom(Hash(keyX,keyZ,seed ^ (0x37a4c195U
                    +static_cast<std::uint32_t>(channel)*0x9e3779b9U)));
            };
            if (random(0)<0.22f) continue;
            const float direction=random(1)*2*std::numbers::pi_v<float>;
            const float reach=(3.2f+4*random(2))*tree.scale;
            ForestFloorColony colony{treeIndex,groundPlants_.size(),0,
                tree.x+std::cos(direction)*reach,tree.z+std::sin(direction)*reach,
                random(3)*2*std::numbers::pi_v<float>,2.5f+1.3f*random(4),1.1f+1.3f*random(5)};
            const float familyChoice=random(6);
            const int family=familyChoice<0.55f ? 0 : (familyChoice<0.85f ? 1 : 2);
            const int count=4+static_cast<int>(random(7)*(ForestPlantsPerColony-3));
            for (int child=0;child<count;++child) {
                const int channel=10+child*6;
                const float radial=std::sqrt(random(channel));
                const float angle=random(channel+1)*2*std::numbers::pi_v<float>;
                const float localX=std::cos(angle)*radial*colony.majorRadius;
                const float localZ=std::sin(angle)*radial*colony.minorRadius;
                const float x=colony.x+std::cos(colony.angle)*localX-std::sin(colony.angle)*localZ;
                const float z=colony.z+std::sin(colony.angle)*localX+std::cos(colony.angle)*localZ;
                const float scale=GroundPlantMinimumScale
                    +0.36f*(1-0.65f*radial)*(0.75f+0.25f*random(channel+2));
                const float footprint=GroundPlantFootprintRadius*scale;
                const auto sample=terrain.Sample(x,z);
                if (sample.signedShoreDistance<43 || sample.signedShoreDistance>238
                    || sample.height<2.5f || sample.slopeRadians>0.41887902f
                    || VegetationLayout::OverlapsReservedRoute(x,z,footprint)
                    || !clearOfCanopy(x,z,footprint)
                    || !clearOfRoots(x,z,groundPlants_,ForestRootSeparation)
                    || !clearOfRoots(x,z,bushes_,0.9f)
                    || !clearOfRoots(x,z,understory_,0.85f)) continue;
                bool clearingAccent=false;
                for (const auto clearing:{VegetationLayout::EastClearing,VegetationLayout::WestClearing})
                    if (std::hypot(x-clearing.x,z-clearing.z)<clearing.radius+16+footprint)
                        clearingAccent=true;
                if (clearingAccent) continue;
                const int variant=child==3 ? (family+1)%GroundPlantVariantCount : family;
                groundPlants_.push_back({x,sample.height+0.009f,z,
                    random(channel+3)*2*std::numbers::pi_v<float>,scale,static_cast<std::uint8_t>(variant)});
                ++colony.plantCount;
            }
            if (colony.plantCount) {
                forestGroundPlantCount_+=colony.plantCount;
                forestColonies_.push_back(colony);
            }
        }

        // Small colonies connect the shrub and floor layers at clearing edges.
        int clearingIndex = 0;
        for (const auto clearing : {VegetationLayout::EastClearing, VegetationLayout::WestClearing}) {
            for (int colony = 0; colony < ClearingColonyCandidates; ++colony) {
                const auto random = [&](int channel) {
                    return UnitRandom(Hash(colony, clearingIndex, seed ^ (0x718ab239U
                        + static_cast<std::uint32_t>(channel) * 0x9e3779b9U)));
                };
                // Leave deliberate breaks; jitter sectors/radius so colonies do not form a hedge.
                if (random(0) < 0.18f) continue;
                const float angle = (colony + 0.18f + random(1) * 0.64f)
                    * (2 * std::numbers::pi_v<float> / ClearingColonyCandidates);
                const float radial = clearing.radius + 4 + 8 * random(2);
                const float x = clearing.x + std::cos(angle) * radial;
                const float z = clearing.z + std::sin(angle) * radial;
                const auto sample = terrain.Sample(x, z);
                const float scale = BushMinimumScale + 0.38f * random(3);
                const float footprint = BushFootprintRadius * scale;
                if (sample.signedShoreDistance < 38 || sample.signedShoreDistance > 232
                    || sample.height < 3 || sample.slopeRadians > 0.5235988f
                    || VegetationLayout::OverlapsReservedRoute(x, z, footprint)
                    || !clearOfCanopy(x, z, footprint) || !clearOfRoots(x, z, bushes_, 2.8f)) continue;
                bushes_.push_back({x, sample.height + 0.012f, z,
                    random(4) * 2 * std::numbers::pi_v<float>, scale,
                    static_cast<std::uint8_t>(random(5) * BushVariantCount)});
                ++clearingBushCount_;
                const int family = static_cast<int>(random(6) * GroundPlantVariantCount);
                static_assert(ClearingPlantsPerColony >= 4);
                const int count = 4 + static_cast<int>(random(7) * (ClearingPlantsPerColony - 3));
                for (int leaf = 0; leaf < count; ++leaf) {
                    const int channel = 10 + leaf * 5;
                    const float direction = angle + leaf * 2.39996323f + random(channel) * 0.8f;
                    const float reach = 1.3f + random(channel + 1) * 2.0f;
                    const float px = x + std::cos(direction) * reach;
                    const float pz = z + std::sin(direction) * reach;
                    const auto ground = terrain.Sample(px, pz);
                    const float plantScale = GroundPlantMinimumScale + 0.48f * random(channel + 2);
                    const float plantFootprint = GroundPlantFootprintRadius * plantScale;
                    if (ground.signedShoreDistance < 27 || ground.signedShoreDistance > 238
                        || ground.height < 2.5f || ground.slopeRadians > 0.5585054f
                        || VegetationLayout::OverlapsReservedRoute(px, pz, plantFootprint)
                        || !clearOfCanopy(px, pz, plantFootprint)
                        || !clearOfRoots(px, pz, groundPlants_, 0.72f)) continue;
                    // Most companions share a species; an occasional contrasting leaf breaks cloning.
                    const int variant = leaf == 3 ? (family + 1) % GroundPlantVariantCount : family;
                    groundPlants_.push_back({px, ground.height + 0.009f, pz,
                        random(channel + 3) * 2 * std::numbers::pi_v<float>, plantScale,
                        static_cast<std::uint8_t>(variant)});
                    ++clearingGroundPlantCount_;
                }
            }
            ++clearingIndex;
        }
    }
}
