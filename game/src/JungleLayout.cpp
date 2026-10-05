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
        constexpr float CanopySpacing = 14.5f;
        constexpr int CanopyRadius = 20;
        constexpr float UnderstorySpacing = 5.8f;
        constexpr int UnderstoryRadius = 50;

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
                    || VegetationLayout::IsReservedRoute(x, z)) {
                    continue;
                }

                const float interior = SmoothStep((sample.signedShoreDistance - 43.0f) / 78.0f);
                const float cluster = ClusterNoise(x, z, seed ^ 0x9e3779b9U, 61.0f);
                const float density = std::clamp(
                    (0.34f + 0.46f * interior) * (0.48f + 0.78f * cluster),
                    0.0f, 0.91f);
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
                tree.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xd3a2646cU) % CanopyVariantCount);
                canopy_.push_back(tree);
                obstacles_.push_back(CircularObstacle{tree.x, tree.z, 0.43f * tree.scale});
            }
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
    }
}
