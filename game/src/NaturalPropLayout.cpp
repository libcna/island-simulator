#include "IslandSimulator/NaturalPropLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
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
        constexpr float DriftwoodSpacing = 17.5f;
        constexpr int DriftwoodRadius = 18;
        constexpr int DriftwoodSupportSamples = 5;
        constexpr float BeachDetailSpacing = 6.0f;
        constexpr int BeachDetailRadius = 47;
        constexpr int FallenFrondSupportSamples = 5;

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
                if (VegetationLayout::OverlapsReservedRoute(x, z,
                        0.63f * std::max(rock.scaleX, rock.scaleZ))) continue;
                rocks_.push_back(rock);
                if (rock.solid) {
                    obstacles_.push_back(CircularObstacle{
                        rock.x, rock.z, 0.63f * std::max(rock.scaleX, rock.scaleZ)});
                }
            }
        }

        for (int site = 0; site < ReefRockSiteCount; ++site) {
            const std::uint32_t siteHash = Hash(site, -site - 17, seed ^ 0x243f6a91U);
            if (UnitRandom(siteHash) >= 0.43f) continue;

            const float sitePhase = static_cast<float>(site)
                + (UnitRandom(Hash(site, 11, seed ^ 0xb7e15169U)) - 0.5f) * 0.72f;
            const float angle = sitePhase * 2.0f * std::numbers::pi_v<float>
                / static_cast<float>(ReefRockSiteCount);
            const float offshoreDistance = 2.2f + 6.8f
                * UnitRandom(Hash(site, 29, seed ^ 0x9e3779b0U));
            const float radius = terrain.ShoreRadius(angle) + offshoreDistance;
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            const TerrainSample sample = terrain.Sample(x, z);
            if (sample.signedShoreDistance > -1.8f
                || sample.signedShoreDistance < -9.4f || sample.height > -0.04f) {
                continue;
            }

            const float baseScale = 1.15f + 1.65f * std::pow(
                UnitRandom(Hash(site, 47, seed ^ 0xc2b2ae3cU)), 1.2f);
            RockPlacement rock;
            rock.x = x;
            rock.z = z;
            rock.yawRadians = UnitRandom(Hash(site, 71, seed ^ 0x27d4eb2eU))
                * 2.0f * std::numbers::pi_v<float>;
            rock.scaleX = baseScale
                * (0.82f + 0.44f * UnitRandom(Hash(site, 83, seed ^ 0x165667b8U)));
            rock.scaleY = baseScale
                * (0.68f + 0.52f * UnitRandom(Hash(site, 97, seed ^ 0xd3a26465U)));
            rock.scaleZ = baseScale
                * (0.80f + 0.48f * UnitRandom(Hash(site, 109, seed ^ 0xfd7046c4U)));
            rock.scaleY = std::max(rock.scaleY, (0.42f - sample.height) / 0.95f);
            rock.y = sample.height - rock.scaleY * 0.08f;
            rock.variant = static_cast<std::uint8_t>(
                2U + Hash(site, 131, seed ^ 0xb55a4f08U) % 2U);
            rock.solid = true;
            rock.offshore = true;
            rocks_.push_back(rock);
            obstacles_.push_back(CircularObstacle{
                rock.x, rock.z, 0.63f * std::max(rock.scaleX, rock.scaleZ)});
        }

        struct AuthoredClearingRock
        {
            const ClearingDefinition* clearing;
            float offsetX;
            float offsetZ;
            float yawRadians;
            float scaleX;
            float scaleY;
            float scaleZ;
            std::uint8_t variant;
        };
        constexpr std::array clearingRocks{
            AuthoredClearingRock{&VegetationLayout::EastClearing,
                                 -22.0f, 7.0f, 0.20f, 1.70f, 0.95f, 1.45f, 1U},
            AuthoredClearingRock{&VegetationLayout::EastClearing,
                                 -24.0f, 10.0f, 1.10f, 1.15f, 0.70f, 0.95f, 3U},
            AuthoredClearingRock{&VegetationLayout::EastClearing,
                                 -20.0f, 12.0f, 2.00f, 0.85f, 0.55f, 1.05f, 1U},
            AuthoredClearingRock{&VegetationLayout::WestClearing,
                                 16.0f, 12.0f, 0.60f, 1.05f, 0.70f, 0.95f, 2U},
            AuthoredClearingRock{&VegetationLayout::WestClearing,
                                 7.0f, 20.0f, 1.80f, 0.78f, 0.50f, 0.92f, 0U},
        };
        static_assert(clearingRocks.size() == AuthoredClearingRockCount);
        for (const AuthoredClearingRock& authored : clearingRocks) {
            RockPlacement rock;
            rock.x = authored.clearing->x + authored.offsetX;
            rock.z = authored.clearing->z + authored.offsetZ;
            const TerrainSample support = terrain.Sample(rock.x, rock.z);
            rock.y = support.height - 0.06f * authored.scaleY;
            rock.yawRadians = authored.yawRadians;
            rock.scaleX = authored.scaleX;
            rock.scaleY = authored.scaleY;
            rock.scaleZ = authored.scaleZ;
            rock.variant = authored.variant;
            rock.solid = true;
            rock.clearingLandmark = true;
            rocks_.push_back(rock);
            obstacles_.push_back(CircularObstacle{
                rock.x, rock.z, 0.63f * std::max(rock.scaleX, rock.scaleZ)});
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
                const auto environment=IslandEnvironmentField::SampleInland(
                    x,z,sample.signedShoreDistance);
                if (sample.signedShoreDistance < 34.0f
                    || sample.signedShoreDistance > 205.0f
                    || sample.height < 3.1f || sample.slopeRadians > 0.296706f
                    || VegetationLayout::IsReservedRoute(x, z)
                    || NearTree(x, z, 3.8f, palms, jungle) || environment.drainage > 0.72f) {
                    continue;
                }
                const float cluster = ClusterNoise(x, z, seed ^ 0x51ed270bU);
                const float density = (0.22f + 0.28f * cluster)
                    * (0.88f + 0.38f * environment.moisture);
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

        struct AuthoredClearingLog
        {
            const ClearingDefinition* clearing;
            float offsetX;
            float offsetZ;
            float yawRadians;
            float length;
            float radius;
            std::uint8_t variant;
        };
        constexpr std::array clearingLogs{
            AuthoredClearingLog{&VegetationLayout::WestClearing,
                                12.0f, 13.0f, 2.38f, 6.6f, 0.38f, 2U},
            AuthoredClearingLog{&VegetationLayout::EastClearing,
                                -22.0f, -7.0f, 2.55f, 7.1f, 0.42f, 1U},
        };
        static_assert(clearingLogs.size() == AuthoredClearingLogCount);
        for (const AuthoredClearingLog& authored : clearingLogs) {
            FallenLogPlacement log;
            log.x = authored.clearing->x + authored.offsetX;
            log.z = authored.clearing->z + authored.offsetZ;
            log.yawRadians = authored.yawRadians;
            log.length = authored.length;
            log.radius = authored.radius;
            log.variant = authored.variant;
            log.clearingLandmark = true;
            const float directionX = std::cos(log.yawRadians);
            const float directionZ = std::sin(log.yawRadians);
            float maximumSupportHeight = terrain.Height(log.x, log.z);
            for (int sampleIndex = 0; sampleIndex < LogCollisionSamples; ++sampleIndex) {
                const float t = static_cast<float>(sampleIndex)
                    / static_cast<float>(LogCollisionSamples - 1) - 0.5f;
                const float sampleX = log.x + directionX * log.length * t;
                const float sampleZ = log.z + directionZ * log.length * t;
                maximumSupportHeight = std::max(
                    maximumSupportHeight, terrain.Height(sampleX, sampleZ));
                obstacles_.push_back(CircularObstacle{
                    sampleX, sampleZ, log.radius * 0.92f});
            }
            log.y = maximumSupportHeight + log.radius * 0.68f;
            fallenLogs_.push_back(log);
        }

        driftwood_.reserve(72U);
        for (int gridZ = -DriftwoodRadius; gridZ <= DriftwoodRadius; ++gridZ) {
            for (int gridX = -DriftwoodRadius; gridX <= DriftwoodRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed ^ 0x1b873593U);
                const float jitterX = (UnitRandom(siteHash) - 0.5f)
                    * DriftwoodSpacing * 0.74f;
                const float jitterZ = (UnitRandom(
                    Hash(gridX, gridZ, seed ^ 0x85ebca77U)) - 0.5f)
                    * DriftwoodSpacing * 0.74f;
                const float x = static_cast<float>(gridX) * DriftwoodSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * DriftwoodSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 2.0f
                    || sample.signedShoreDistance > 25.0f
                    || sample.height < 0.12f || sample.slopeRadians > 0.34906585f
                    || VegetationLayout::IsReservedRoute(x, z)
                    || NearTree(x, z, 1.8f, palms, jungle)) {
                    continue;
                }

                const float cluster = ClusterNoise(x, z, seed ^ 0xc2b2ae3dU);
                const float density = 0.20f + 0.22f * cluster;
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0x27d4eb29U)) >= density) {
                    continue;
                }

                DriftwoodPlacement wood;
                wood.x = x;
                wood.z = z;
                wood.length = DriftwoodMinimumLength
                    + (DriftwoodMaximumLength - DriftwoodMinimumLength)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0x165667b9U));
                wood.radius = 0.09f + 0.09f
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0xd3a26464U));
                wood.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xfd7046c5U) % DriftwoodVariantCount);

                const float gradientX = terrain.Sample(x + 1.5f, z).signedShoreDistance
                    - terrain.Sample(x - 1.5f, z).signedShoreDistance;
                const float gradientZ = terrain.Sample(x, z + 1.5f).signedShoreDistance
                    - terrain.Sample(x, z - 1.5f).signedShoreDistance;
                const float tangentX = -gradientZ;
                const float tangentZ = gradientX;
                const float tangentYaw = std::atan2(tangentZ, tangentX);
                wood.yawRadians = tangentYaw
                    + (UnitRandom(Hash(gridX, gridZ, seed ^ 0xb55a4f09U)) - 0.5f) * 0.92f;

                const float directionX = std::cos(wood.yawRadians);
                const float directionZ = std::sin(wood.yawRadians);
                float maximumSupportHeight = sample.height;
                float minimumSupportHeight = sample.height;
                bool validFootprint = true;
                for (int sampleIndex = 0; sampleIndex < DriftwoodSupportSamples; ++sampleIndex) {
                    const float t = static_cast<float>(sampleIndex)
                        / static_cast<float>(DriftwoodSupportSamples - 1) - 0.5f;
                    const float sampleX = wood.x + directionX * wood.length * t;
                    const float sampleZ = wood.z + directionZ * wood.length * t;
                    const TerrainSample support = terrain.Sample(sampleX, sampleZ);
                    maximumSupportHeight = std::max(maximumSupportHeight, support.height);
                    minimumSupportHeight = std::min(minimumSupportHeight, support.height);
                    if (support.signedShoreDistance < 1.5f
                        || support.signedShoreDistance > 27.0f
                        || support.slopeRadians > 0.40142573f
                        || VegetationLayout::IsReservedRoute(sampleX, sampleZ)
                        || NearTree(sampleX, sampleZ, 1.05f, palms, jungle)) {
                        validFootprint = false;
                        break;
                    }
                }
                if (!validFootprint || maximumSupportHeight - minimumSupportHeight > 0.22f) {
                    continue;
                }
                wood.y = maximumSupportHeight + wood.radius * 0.58f;
                driftwood_.push_back(wood);
            }
        }

        struct AuthoredCoveDriftwood
        {
            float angleOffset;
            float inlandDistance;
            float yawOffset;
            float length;
            float radius;
            std::uint8_t variant;
        };
        constexpr std::array coveDriftwood{
            AuthoredCoveDriftwood{0.000f, 7.0f, -0.12f, 4.4f, 0.17f, 2U},
            AuthoredCoveDriftwood{0.028f, 9.5f, 0.33f, 3.3f, 0.13f, 0U},
            AuthoredCoveDriftwood{-0.026f, 10.5f, -0.40f, 2.4f, 0.10f, 1U},
        };
        static_assert(coveDriftwood.size() == AuthoredCoveDriftwoodCount);
        for (const AuthoredCoveDriftwood& authored : coveDriftwood) {
            const float angle = AuthoredCoveAngle + authored.angleOffset;
            const float radius = terrain.ShoreRadius(angle) - authored.inlandDistance;
            DriftwoodPlacement wood;
            wood.x = std::cos(angle) * radius;
            wood.z = std::sin(angle) * radius;
            wood.length = authored.length;
            wood.radius = authored.radius;
            wood.variant = authored.variant;
            wood.coveLandmark = true;

            const float gradientX = terrain.Sample(wood.x + 1.5f, wood.z).signedShoreDistance
                - terrain.Sample(wood.x - 1.5f, wood.z).signedShoreDistance;
            const float gradientZ = terrain.Sample(wood.x, wood.z + 1.5f).signedShoreDistance
                - terrain.Sample(wood.x, wood.z - 1.5f).signedShoreDistance;
            wood.yawRadians = std::atan2(gradientX, -gradientZ) + authored.yawOffset;

            const float directionX = std::cos(wood.yawRadians);
            const float directionZ = std::sin(wood.yawRadians);
            float maximumSupportHeight = terrain.Height(wood.x, wood.z);
            for (int sampleIndex = 0; sampleIndex < DriftwoodSupportSamples; ++sampleIndex) {
                const float t = static_cast<float>(sampleIndex)
                    / static_cast<float>(DriftwoodSupportSamples - 1) - 0.5f;
                maximumSupportHeight = std::max(
                    maximumSupportHeight,
                    terrain.Height(wood.x + directionX * wood.length * t,
                                   wood.z + directionZ * wood.length * t));
            }
            wood.y = maximumSupportHeight + wood.radius * 0.58f;
            driftwood_.push_back(wood);
        }

        beachDetails_.reserve(420U);
        for (int gridZ = -BeachDetailRadius; gridZ <= BeachDetailRadius; ++gridZ) {
            for (int gridX = -BeachDetailRadius; gridX <= BeachDetailRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed ^ 0x6a09e667U);
                const float jitterX = (UnitRandom(siteHash) - 0.5f)
                    * BeachDetailSpacing * 0.82f;
                const float jitterZ = (UnitRandom(
                    Hash(gridX, gridZ, seed ^ 0xbb67ae85U)) - 0.5f)
                    * BeachDetailSpacing * 0.82f;
                const float x = static_cast<float>(gridX) * BeachDetailSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * BeachDetailSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 3.0f
                    || sample.signedShoreDistance > 18.0f
                    || sample.height < 0.22f || sample.slopeRadians > 0.2617994f
                    || VegetationLayout::IsReservedRoute(x, z)) {
                    continue;
                }

                const float cluster = ClusterNoise(x, z, seed ^ 0x3c6ef372U);
                const float density = 0.28f + cluster * 0.34f;
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0xa54ff53aU)) >= density) {
                    continue;
                }

                BeachDetailPlacement detail;
                detail.x = x;
                detail.y = sample.height + 0.012f;
                detail.z = z;
                detail.yawRadians = UnitRandom(
                    Hash(gridX, gridZ, seed ^ 0x510e527fU))
                    * 2.0f * std::numbers::pi_v<float>;
                detail.scale = BeachDetailMinimumScale
                    + (BeachDetailMaximumScale - BeachDetailMinimumScale)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0x9b05688cU));
                detail.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0x1f83d9abU) % BeachDetailVariantCount);
                beachDetails_.push_back(detail);
            }
        }

        fallenFronds_.reserve(150U);
        for (const PalmPlacement& palm : palms.Palms()) {
            const auto palmX = static_cast<std::int32_t>(std::lround(palm.x * 8.0f));
            const auto palmZ = static_cast<std::int32_t>(std::lround(palm.z * 8.0f));
            const std::uint32_t siteHash = Hash(palmX, palmZ, seed ^ 0x5be0cd19U);
            if (UnitRandom(siteHash) >= 0.46f) continue;

            const float radialAngle = UnitRandom(Hash(palmX, palmZ, seed ^ 0xcbbb9d5dU))
                * 2.0f * std::numbers::pi_v<float>;
            const float offset = 2.4f + 2.8f
                * UnitRandom(Hash(palmX, palmZ, seed ^ 0x629a292aU));
            const float x = palm.x + std::cos(radialAngle) * offset;
            const float z = palm.z + std::sin(radialAngle) * offset;
            const TerrainSample centre = terrain.Sample(x, z);
            if (centre.signedShoreDistance < 7.0f
                || centre.signedShoreDistance > 188.0f
                || centre.height < 0.45f || centre.slopeRadians > 0.31415927f
                || VegetationLayout::IsReservedRoute(x, z)
                || NearTree(x, z, 1.25f, palms, jungle)) {
                continue;
            }
            bool separated = true;
            for (const FallenFrondPlacement& existing : fallenFronds_) {
                if (DistanceSquared(x, z, existing.x, existing.z) < 8.41f) {
                    separated = false;
                    break;
                }
            }
            if (!separated) continue;

            FallenFrondPlacement frond;
            frond.x = x;
            frond.z = z;
            frond.yawRadians = radialAngle
                + (UnitRandom(Hash(palmX, palmZ, seed ^ 0x9159015aU)) - 0.5f) * 0.72f;
            frond.scale = FallenFrondMinimumScale
                + (FallenFrondMaximumScale - FallenFrondMinimumScale)
                    * UnitRandom(Hash(palmX, palmZ, seed ^ 0x152fecd8U));
            frond.variant = static_cast<std::uint8_t>(
                Hash(palmX, palmZ, seed ^ 0x67332667U) % FallenFrondVariantCount);

            const float directionX = std::cos(frond.yawRadians);
            const float directionZ = std::sin(frond.yawRadians);
            float maximumSupportHeight = centre.height;
            float minimumSupportHeight = centre.height;
            bool validFootprint = true;
            for (int sampleIndex = 0; sampleIndex < FallenFrondSupportSamples; ++sampleIndex) {
                const float t = static_cast<float>(sampleIndex)
                    / static_cast<float>(FallenFrondSupportSamples - 1) - 0.5f;
                const float sampleX = frond.x + directionX * frond.scale * t;
                const float sampleZ = frond.z + directionZ * frond.scale * t;
                const TerrainSample support = terrain.Sample(sampleX, sampleZ);
                maximumSupportHeight = std::max(maximumSupportHeight, support.height);
                minimumSupportHeight = std::min(minimumSupportHeight, support.height);
                if (support.signedShoreDistance < 6.0f
                    || support.signedShoreDistance > 190.0f
                    || support.slopeRadians > 0.34906585f
                    || VegetationLayout::IsReservedRoute(sampleX, sampleZ)) {
                    validFootprint = false;
                    break;
                }
            }
            if (!validFootprint || maximumSupportHeight - minimumSupportHeight > 0.22f) {
                continue;
            }
            frond.y = maximumSupportHeight + 0.022f;
            fallenFronds_.push_back(frond);
        }
    }
}
