#include "IslandSimulator/NaturalPropLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float RockSpacing = 11.5f;
        constexpr int RockRadius = 24;
        constexpr float LogSpacing = 30.0f;
        constexpr int LogRadius = 10;
        constexpr int LogCollisionSamples = 7;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] float DistanceSquared(const float x0, const float z0,
                                            const float x1, const float z1) noexcept
        {
            const float deltaX = x0 - x1;
            const float deltaZ = z0 - z1;
            return deltaX * deltaX + deltaZ * deltaZ;
        }

        [[nodiscard]] bool NearTree(const float x, const float z, const float clearance,
                                    const VegetationLayout& palms,
                                    const JungleLayout& jungle) noexcept
        {
            const float clearanceSquared = clearance * clearance;
            for (const PalmPlacement& palm : palms.Palms()) {
                if (DistanceSquared(x, z, palm.x, palm.z) < clearanceSquared) return true;
            }
            for (const CanopyPlacement& tree : jungle.Canopy()) {
                if (DistanceSquared(x, z, tree.x, tree.z) < clearanceSquared) return true;
            }
            return false;
        }
    }

    std::uint32_t NaturalPropLayout::Hash(const std::int32_t x, const std::int32_t z,
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

    float NaturalPropLayout::UnitRandom(const std::uint32_t value) noexcept
    {
        return static_cast<float>(value & 0x00ffffffU) / static_cast<float>(0x01000000U);
    }

    float NaturalPropLayout::ClusterNoise(const float x, const float z,
                                          const std::uint32_t seed) noexcept
    {
        constexpr float cellSize = 52.0f;
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

    NaturalPropLayout::NaturalPropLayout(const IslandTerrain& terrain,
                                         const VegetationLayout& palms,
                                         const JungleLayout& jungle,
                                         const std::uint32_t seed)
    {
        rocks_.reserve(280U);
        obstacles_.reserve(460U);
        for (int gridZ = -RockRadius; gridZ <= RockRadius; ++gridZ) {
            for (int gridX = -RockRadius; gridX <= RockRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * RockSpacing * 0.74f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0x68e31da4U)) - 0.5f)
                    * RockSpacing * 0.74f;
                const float x = static_cast<float>(gridX) * RockSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * RockSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 1.5f
                    || sample.signedShoreDistance > 202.0f
                    || sample.height < 0.15f || sample.slopeRadians > 0.5759587f
                    || VegetationLayout::IsReservedRoute(x, z)
                    || NearTree(x, z, 2.1f, palms, jungle)) {
                    continue;
                }

                const float coast = 1.0f - SmoothStep(
                    (sample.signedShoreDistance - 7.0f) / 58.0f);
                const float cluster = ClusterNoise(x, z, seed ^ 0x243f6a88U);
                const float density = std::clamp(
                    0.035f + coast * 0.21f + cluster * 0.095f, 0.0f, 0.30f);
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0xb7e15162U)) >= density) {
                    continue;
                }

                const float baseScale = RockMinimumScale
                    + (RockMaximumScale - RockMinimumScale)
                        * std::pow(UnitRandom(Hash(gridX, gridZ, seed ^ 0x9e3779b9U)), 1.55f);
                RockPlacement rock;
                rock.x = x;
                rock.y = sample.height - 0.055f * baseScale;
                rock.z = z;
                rock.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x85ebca6bU))
                    * 2.0f * std::numbers::pi_v<float>;
                rock.scaleX = baseScale
                    * (0.80f + 0.42f * UnitRandom(Hash(gridX, gridZ, seed ^ 0xc2b2ae35U)));
                rock.scaleY = baseScale
                    * (0.52f + 0.39f * UnitRandom(Hash(gridX, gridZ, seed ^ 0x27d4eb2fU)));
                rock.scaleZ = baseScale
                    * (0.78f + 0.47f * UnitRandom(Hash(gridX, gridZ, seed ^ 0x165667b1U)));
                rock.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xd3a2646cU) % RockVariantCount);
                rock.solid = rock.scaleY >= 0.48f && std::max(rock.scaleX, rock.scaleZ) >= 0.70f;
                rocks_.push_back(rock);
                if (rock.solid) {
                    obstacles_.push_back(CircularObstacle{
                        rock.x, rock.z, 0.63f * std::max(rock.scaleX, rock.scaleZ)});
                }
            }
        }

        fallenLogs_.reserve(36U);
        for (int gridZ = -LogRadius; gridZ <= LogRadius; ++gridZ) {
            for (int gridX = -LogRadius; gridX <= LogRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed ^ 0x94d049bbU);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * LogSpacing * 0.72f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0x7f4a7c15U)) - 0.5f)
                    * LogSpacing * 0.72f;
                const float x = static_cast<float>(gridX) * LogSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * LogSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 34.0f
                    || sample.signedShoreDistance > 205.0f
                    || sample.height < 3.1f || sample.slopeRadians > 0.296706f
                    || VegetationLayout::IsReservedRoute(x, z)
                    || NearTree(x, z, 3.8f, palms, jungle)) {
                    continue;
                }
                const float cluster = ClusterNoise(x, z, seed ^ 0x51ed270bU);
                const float density = 0.22f + 0.28f * cluster;
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0x632be59bU)) >= density) continue;

                FallenLogPlacement log;
                log.x = x;
                log.z = z;
                log.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x4cf5ad43U))
                    * 2.0f * std::numbers::pi_v<float>;
                log.length = LogMinimumLength + (LogMaximumLength - LogMinimumLength)
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0xa24baed5U));
                log.radius = 0.24f + 0.19f
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0x9fb21c65U));
                log.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0x3c6ef372U) % LogVariantCount);

                const float directionX = std::cos(log.yawRadians);
                const float directionZ = std::sin(log.yawRadians);
                float maximumSupportHeight = sample.height;
                float minimumSupportHeight = sample.height;
                bool validFootprint = true;
                for (int sampleIndex = 0; sampleIndex < LogCollisionSamples; ++sampleIndex) {
                    const float t = static_cast<float>(sampleIndex)
                        / static_cast<float>(LogCollisionSamples - 1) - 0.5f;
                    const float sampleX = log.x + directionX * log.length * t;
                    const float sampleZ = log.z + directionZ * log.length * t;
                    const TerrainSample support = terrain.Sample(sampleX, sampleZ);
                    maximumSupportHeight = std::max(maximumSupportHeight, support.height);
                    minimumSupportHeight = std::min(minimumSupportHeight, support.height);
                    if (support.signedShoreDistance < 30.0f
                        || support.slopeRadians > 0.34906585f
                        || VegetationLayout::IsReservedRoute(sampleX, sampleZ)
                        || NearTree(sampleX, sampleZ, 1.55f, palms, jungle)) {
                        validFootprint = false;
                        break;
                    }
                }
                if (!validFootprint || maximumSupportHeight - minimumSupportHeight > 0.34f) {
                    continue;
                }
                log.y = maximumSupportHeight + log.radius * 0.68f;
                fallenLogs_.push_back(log);

                for (int sampleIndex = 0; sampleIndex < LogCollisionSamples; ++sampleIndex) {
                    const float t = static_cast<float>(sampleIndex)
                        / static_cast<float>(LogCollisionSamples - 1) - 0.5f;
                    obstacles_.push_back(CircularObstacle{
                        log.x + directionX * log.length * t,
                        log.z + directionZ * log.length * t,
                        log.radius * 0.92f});
                }
            }
        }
    }
}
