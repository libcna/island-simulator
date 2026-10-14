#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/StreamRoute.hpp"
#include "IslandSimulator/HutLayout.hpp"

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
        struct CanopyScaleCandidate {
            std::size_t index;
            bool emergentEligible;
            bool juvenileEligible;
            float emergentScaleRandom;
            float juvenileScaleRandom;
        };
        std::vector<CanopyScaleCandidate> scaleCandidates;
        for (int gridZ = -CanopyRadius; gridZ <= CanopyRadius; ++gridZ) {
            for (int gridX = -CanopyRadius; gridX <= CanopyRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * CanopySpacing * 0.66f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0xc2b2ae35U)) - 0.5f)
                    * CanopySpacing * 0.66f;
                const float x = static_cast<float>(gridX) * CanopySpacing + jitterX;
                const float z = static_cast<float>(gridZ) * CanopySpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                const auto environment=IslandEnvironmentField::SampleInland(x,z,sample.signedShoreDistance);
                if (sample.signedShoreDistance < 43.0f
                    || sample.signedShoreDistance > 238.0f
                    || sample.height < 3.8f || sample.slopeRadians > 0.4712389f
                    || sample.coastalRockiness > 0.45f
                    || VegetationLayout::IsReservedRoute(x, z) || environment.drainage > 0.68f) {
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
                scaleCandidates.push_back({canopy_.size(),
                    stand > CanopyStandThreshold
                        && UnitRandom(Hash(gridX, gridZ, seed ^ 0x3c6ef372U)) < CanopyEmergentChance,
                    canopy_.size() >= 64U && stand > 0.28f
                        && UnitRandom(Hash(gridX, gridZ, seed ^ 0x6a09e667U)) < CanopyJuvenileChance,
                    UnitRandom(Hash(gridX, gridZ, seed ^ 0x7f4a7c15U)),
                    UnitRandom(Hash(gridX, gridZ, seed ^ 0xbb67ae85U))});
                tree.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xd3a2646cU) % NaturalCanopyFormCount);
                canopy_.push_back(tree);
                obstacles_.push_back(CircularObstacle{tree.x, tree.z, 0.43f * tree.scale});
            }
        }

        // Emergents rise only above a closed stand: promote eligible trees that actually have
        // accepted neighbours, so the stand field alone cannot create a lone giant.
        for (const auto& candidate : scaleCandidates) {
            if (!candidate.emergentEligible) continue;
            CanopyPlacement& tree = canopy_[candidate.index];
            int neighbours = 0;
            for (std::size_t other = 0; other < canopy_.size(); ++other) {
                if (other == candidate.index) continue;
                const float dx = canopy_[other].x - tree.x, dz = canopy_[other].z - tree.z;
                if (dx * dx + dz * dz < 20.0f * 20.0f) ++neighbours;
            }
            if (neighbours < 2) continue;
            tree.scale = CanopyEmergentMinimumScale
                + (CanopyEmergentScale - CanopyEmergentMinimumScale)
                    * candidate.emergentScaleRandom;
            obstacles_[candidate.index].radius = 0.43f * tree.scale;
        }

        // Saplings reuse the full tree mesh at a genuinely young 3--6 m stature. Keep them
        // beside established forest rather than sprinkling miniature solitary trees into open
        // ground, and never replace a promoted emergent.
        for (const auto& candidate : scaleCandidates) {
            if (!candidate.juvenileEligible) continue;
            CanopyPlacement& tree = canopy_[candidate.index];
            if (tree.scale > CanopyMaximumScale) continue;
            int neighbours = 0;
            for (std::size_t other = 0; other < canopy_.size(); ++other) {
                if (other == candidate.index) continue;
                const float dx = canopy_[other].x - tree.x, dz = canopy_[other].z - tree.z;
                if (dx * dx + dz * dz < 18.0f * 18.0f) ++neighbours;
            }
            if (neighbours < 1) continue;
            tree.scale = CanopyJuvenileMinimumScale
                + (CanopyJuvenileMaximumScale - CanopyJuvenileMinimumScale)
                    * candidate.juvenileScaleRandom;
            // Retain a conservative root footprint without making a thin young trunk feel like
            // an adult collision cylinder.
            obstacles_[candidate.index].radius = std::max(0.22f, 0.43f * tree.scale);
            ++juvenileCanopyCount_;
        }

        // One compact brackish colony follows the final stream reach. Low crowns and wide
        // root collision proxies distinguish these trees from the interior canopy without
        // spreading mangroves around unrelated coasts.
        for (int candidate = 0; candidate < MangroveCandidateCount; ++candidate) {
            const auto random = [&](std::uint32_t channel) {
                return UnitRandom(Hash(candidate, static_cast<std::int32_t>(channel),
                    seed ^ (0xd7b71e49U + channel * 0x9e3779b9U)));
            };
            // Retain the original paired sweep, then spend the four reserve probes
            // on distinct upstream gaps rather than continuing beyond the sea mouth.
            const int station = candidate < 44 ? candidate / 2 : candidate - 44;
            const float baseX = -141.0f - static_cast<float>(station) * .98f
                - random(0) * .55f;
            const float centreZ = StreamRoute::CentreZ(baseX);
            const float tangentZ = StreamRoute::CentreZ(baseX - .5f)
                - StreamRoute::CentreZ(baseX + .5f);
            const float tangentLength = std::hypot(1.0f, tangentZ);
            const float tangentX = -1.0f / tangentLength;
            const float tangentZUnit = tangentZ / tangentLength;
            const float normalX = tangentZUnit;
            const float normalZ = -tangentX;
            const float side = candidate % 2 == 0 ? -1.0f : 1.0f;
            // Follow the real local channel profile: widened pools push stilt roots
            // outward instead of silently losing trees to the drainage exclusion.
            const float bankDistance = StreamRoute::HalfWidth(baseX)
                + 2.6f + 3.8f * random(1);
            const float alongJitter = (random(2) - .5f) * 1.8f;
            CanopyPlacement tree;
            tree.x = baseX + alongJitter * tangentX + side * bankDistance * normalX;
            tree.z = centreZ + alongJitter * tangentZUnit + side * bankDistance * normalZ;
            const auto sample = terrain.Sample(tree.x, tree.z);
            const auto environment = IslandEnvironmentField::SampleInland(
                tree.x, tree.z, sample.signedShoreDistance);
            tree.scale = MangroveMinimumScale
                + (MangroveMaximumScale - MangroveMinimumScale) * random(3);
            const float rootRadius = 1.08f * tree.scale;
            if (sample.signedShoreDistance < .4f || sample.signedShoreDistance > 24.0f
                || sample.height < -.05f || sample.height > 3.4f
                || sample.slopeRadians > .38f || sample.coastalRockiness > .38f
                || environment.moisture < .30f || environment.saltExposure < .06f
                || environment.drainage > .76f
                || VegetationLayout::OverlapsReservedRoute(tree.x, tree.z, rootRadius)) continue;
            const bool clear = std::none_of(obstacles_.begin(), obstacles_.end(),
                [&](const CircularObstacle& obstacle) {
                    return std::hypot(tree.x - obstacle.x, tree.z - obstacle.z)
                        < rootRadius + obstacle.radius + .95f;
                });
            if (!clear) continue;
            tree.y = sample.height;
            tree.yawRadians = random(4) * 2.0f * std::numbers::pi_v<float>;
            tree.variant = static_cast<std::uint8_t>(MangroveCanopyVariant);
            canopy_.push_back(tree);
            obstacles_.push_back({tree.x, tree.z, rootRadius});
            ++mangroveCount_;
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
                const auto environment=IslandEnvironmentField::SampleInland(x,z,sample.signedShoreDistance);
                if (sample.signedShoreDistance < 31.0f
                    || sample.signedShoreDistance > 242.0f
                    || sample.height < 2.8f || sample.slopeRadians > 0.5410521f
                    || sample.coastalRockiness > 0.60f
                    || VegetationLayout::IsReservedRoute(x, z) || environment.drainage > 0.82f) {
                    continue;
                }

                const float interior = SmoothStep((sample.signedShoreDistance - 31.0f) / 62.0f);
                const float broadCluster = ClusterNoise(x, z, seed ^ 0x51ed270bU, 39.0f);
                const float fineCluster = ClusterNoise(x, z, seed ^ 0x632be59bU, 17.0f);
                const float density = std::clamp(
                    (0.32f + 0.46f * interior)
                        * (0.48f + 0.54f * broadCluster + 0.20f * fineCluster)
                        * (1.0f + 0.34f * environment.moisture),
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
                if (HutLayout::ReservesDecorativeClearing(
                        plant.x, plant.z, plant.scale * 1.8f)) continue;
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
                const auto environment=IslandEnvironmentField::SampleInland(x,z,sample.signedShoreDistance);
                if (sample.signedShoreDistance < 38.0f
                    || sample.signedShoreDistance > 232.0f
                    || sample.height < 3.0f || sample.slopeRadians > 0.5235988f
                    || sample.coastalRockiness > 0.60f
                    || VegetationLayout::IsReservedRoute(x, z) || environment.drainage > 0.78f) {
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
                        * (0.52f + 0.62f * broadCluster + 0.18f * fineCluster)
                        * (1.0f + 0.22f * environment.moisture),
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
                if (HutLayout::ReservesDecorativeClearing(
                        bush.x, bush.z, bush.scale * 1.2f)) continue;
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
                const auto environment=IslandEnvironmentField::SampleInland(x,z,sample.signedShoreDistance);
                if (sample.signedShoreDistance < 27.0f
                    || sample.signedShoreDistance > 238.0f
                    || sample.height < 2.5f || sample.slopeRadians > 0.5585054f
                    || sample.coastalRockiness > 0.60f
                    || VegetationLayout::IsReservedRoute(x, z) || environment.drainage > 0.84f) {
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
                        * (0.50f + 0.58f * broadCluster + 0.23f * fineCluster)
                        * (1.0f + 0.48f * environment.moisture),
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
                if (HutLayout::ReservesDecorativeClearing(
                        plant.x, plant.z, plant.scale * .5f)) continue;
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

        // Small mixed colonies make the shared moisture field visible at eye level. Each
        // colony chooses one bank, a different setback, and an irregular local footprint;
        // skipped candidates leave readable gaps instead of producing paired planted rows.
        // They reuse the existing fern/broadleaf meshes and renderer batches.
        for (int colony = 0; colony < StreamBankColonyCount; ++colony) {
            const auto colonyRandom = [&](std::uint32_t channel) {
                return UnitRandom(Hash(colony, static_cast<std::int32_t>(channel),
                    seed ^ (0x53a93cf1U + channel * 0x9e3779b9U)));
            };
            if (colonyRandom(0) < .17f) continue;
            const float progress = (static_cast<float>(colony) + .18f + .64f * colonyRandom(1))
                / static_cast<float>(StreamBankColonyCount);
            const float baseX = -79.0f - 72.0f * progress;
            const float baseZ = StreamRoute::CentreZ(baseX);
            const float side = colonyRandom(2) < .47f ? -1.0f : 1.0f;
            const float bankDistance = 4.3f + 4.1f * colonyRandom(3);
            const float nextZ = StreamRoute::CentreZ(baseX - .5f);
            const float previousZ = StreamRoute::CentreZ(baseX + .5f);
            const float tangentZ = nextZ - previousZ;
            const float tangentLength = std::hypot(1.0f, tangentZ);
            const float tangentX = -1.0f / tangentLength;
            const float tangentZUnit = tangentZ / tangentLength;
            const float normalX = tangentZUnit;
            const float normalZ = -tangentX;
            const float colonyX = baseX + side * bankDistance * normalX;
            const float colonyZ = baseZ + side * bankDistance * normalZ;
            for (int child = 0; child < StreamBankPlantsPerColony; ++child) {
                const auto random = [&](std::uint32_t channel) {
                    return UnitRandom(Hash(colony, child,
                        seed ^ (0x6c8e9cf5U + channel * 0x9e3779b9U)));
                };
                const float alongJitter = (random(1) - .5f) * 4.8f;
                const float acrossJitter = (random(2) - .5f) * 2.5f;
                const float x = colonyX + alongJitter * tangentX
                    + acrossJitter * normalX;
                const float z = colonyZ + alongJitter * tangentZUnit
                    + acrossJitter * normalZ;
                const auto sample = terrain.Sample(x, z);
                const auto environment = IslandEnvironmentField::SampleInland(
                    x, z, sample.signedShoreDistance);
                const float scale = .82f + .34f * random(3);
                const float footprint = GroundPlantFootprintRadius * scale;
                if (sample.signedShoreDistance < 8 || sample.height < .7f
                    || sample.slopeRadians > .5585054f || environment.moisture < .38f
                    || environment.drainage > .72f
                    || VegetationLayout::OverlapsReservedRoute(x, z, footprint)
                    || !clearOfCanopy(x, z, footprint)
                    || !clearOfRoots(x, z, groundPlants_, .68f)
                    || HutLayout::ReservesDecorativeClearing(x,z,.5f*scale)) continue;
                groundPlants_.push_back({x, sample.height + .009f, z,
                    random(4) * 2 * std::numbers::pi_v<float>, scale,
                    static_cast<std::uint8_t>((child
                        + static_cast<int>(colonyRandom(4) * 3.0f))
                        % GroundPlantVariantCount), true});
                ++streamBankGroundPlantCount_;
            }
        }

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
                const auto environment=IslandEnvironmentField::SampleInland(x,z,sample.signedShoreDistance);
                if (sample.signedShoreDistance<43 || sample.signedShoreDistance>238
                    || sample.height<2.5f || sample.slopeRadians>0.41887902f
                    || environment.drainage>0.84f
                    || VegetationLayout::OverlapsReservedRoute(x,z,footprint)
                    || !clearOfCanopy(x,z,footprint)
                    || !clearOfRoots(x,z,groundPlants_,ForestRootSeparation)
                    || !clearOfRoots(x,z,bushes_,0.9f)
                    || !clearOfRoots(x,z,understory_,0.85f)
                    || HutLayout::ReservesDecorativeClearing(x,z,.5f*scale)) continue;
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
                    || !clearOfCanopy(x, z, footprint) || !clearOfRoots(x, z, bushes_, 2.8f)
                    || HutLayout::ReservesDecorativeClearing(x,z,1.2f*scale)) continue;
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
                        || !clearOfRoots(px, pz, groundPlants_, 0.72f)
                        || HutLayout::ReservesDecorativeClearing(px,pz,.5f*plantScale)) continue;
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

        // A few upright remnants complete the forest age structure without replacing healthy
        // canopy trees. Choose them only after every living layer has been composed, so their
        // solid bases cannot silently intersect an existing plant root or change the established
        // deterministic vegetation populations. Collision proxies are appended after the canopy
        // prefix: callers that associate obstacle i with canopy i retain that stable mapping.
        standingSnags_.reserve(StandingSnagTargetCount);
        for (int candidate = 0;
             candidate < StandingSnagCandidateCount
                 && standingSnags_.size() < StandingSnagTargetCount;
             ++candidate) {
            const auto random = [&](const std::uint32_t channel) {
                return UnitRandom(Hash(candidate, static_cast<std::int32_t>(channel),
                    seed ^ (0x4f1bbcdcU + channel * 0x9e3779b9U)));
            };
            StandingSnagPlacement snag;
            snag.x = (random(0) * 2.0f - 1.0f) * 198.0f;
            snag.z = (random(1) * 2.0f - 1.0f) * 198.0f;
            const auto sample = terrain.Sample(snag.x, snag.z);
            const auto environment = IslandEnvironmentField::SampleInland(
                snag.x, snag.z, sample.signedShoreDistance);
            snag.scale = StandingSnagMinimumScale
                + (StandingSnagMaximumScale - StandingSnagMinimumScale) * random(2);
            const float rootRadius = .76f * snag.scale;
            if (sample.signedShoreDistance < 58.0f || sample.signedShoreDistance > 232.0f
                || sample.height < 4.0f || sample.slopeRadians > .42f
                || sample.coastalRockiness > .46f || environment.drainage > .67f
                || VegetationLayout::OverlapsReservedRoute(snag.x, snag.z, rootRadius)
                || HutLayout::ReservesDecorativeClearing(snag.x, snag.z, rootRadius)) continue;

            int matureNeighbours = 0;
            for (const auto& tree : canopy_) {
                if (tree.variant == MangroveCanopyVariant
                    || tree.scale < CanopyMinimumScale) continue;
                const float distance = std::hypot(snag.x - tree.x, snag.z - tree.z);
                matureNeighbours += distance >= 3.0f && distance <= 25.0f ? 1 : 0;
            }
            if (matureNeighbours < 2) continue;
            const bool clearOfSolidTrees = std::none_of(
                obstacles_.begin(), obstacles_.end(), [&](const CircularObstacle& tree) {
                    return std::hypot(snag.x - tree.x, snag.z - tree.z)
                        < rootRadius + tree.radius + .85f;
                });
            const bool clearOfSnags = std::none_of(
                standingSnags_.begin(), standingSnags_.end(), [&](const auto& other) {
                    return std::hypot(snag.x - other.x, snag.z - other.z)
                        < 7.0f + rootRadius + .76f * other.scale;
                });
            const bool clearOfUnderstory = std::none_of(
                understory_.begin(), understory_.end(), [&](const auto& plant) {
                    return std::hypot(snag.x - plant.x, snag.z - plant.z)
                        < rootRadius + .30f * plant.scale;
                });
            const bool clearOfBushes = std::none_of(
                bushes_.begin(), bushes_.end(), [&](const auto& bush) {
                    return std::hypot(snag.x - bush.x, snag.z - bush.z)
                        < rootRadius + BushFootprintRadius * bush.scale;
                });
            const bool clearOfFloor = std::none_of(
                groundPlants_.begin(), groundPlants_.end(), [&](const auto& plant) {
                    return std::hypot(snag.x - plant.x, snag.z - plant.z)
                        < rootRadius + GroundPlantFootprintRadius * plant.scale;
                });
            if (!clearOfSolidTrees || !clearOfSnags || !clearOfUnderstory
                || !clearOfBushes || !clearOfFloor) continue;
            snag.y = sample.height;
            snag.yawRadians = random(3) * 2.0f * std::numbers::pi_v<float>;
            snag.variant = static_cast<std::uint8_t>(
                candidate % StandingSnagVariantCount);
            standingSnags_.push_back(snag);
        }
        for (const auto& snag : standingSnags_)
            obstacles_.push_back({snag.x, snag.z, .76f * snag.scale});
    }
}
