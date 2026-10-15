#include "IslandSimulator/VegetationLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/CaveSite.hpp"
#include "IslandSimulator/CampfireSite.hpp"
#include "IslandSimulator/HelicopterSite.hpp"
#include "IslandSimulator/BunkerSite.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        // Dense enough for a grove of touching crowns; groves, not the grid, set the spacing.
        constexpr float CandidateSpacing = 8.0f;
        constexpr int CandidateRadius = 33;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] float DistanceSquared(const float x0, const float z0,
                                            const float x1, const float z1) noexcept
        {
            const float x = x0 - x1;
            const float z = z0 - z1;
            return x * x + z * z;
        }
    }

    std::uint32_t VegetationLayout::Hash(const std::int32_t x, const std::int32_t z,
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

    float VegetationLayout::UnitRandom(const std::uint32_t value) noexcept
    {
        return static_cast<float>(value & 0x00ffffffU) / static_cast<float>(0x01000000U);
    }

    float VegetationLayout::GroveNoise(const float x, const float z, const std::uint32_t seed,
                                       const float cellSize) noexcept
    {
        const float gx = x / cellSize, gz = z / cellSize;
        const auto x0 = static_cast<std::int32_t>(std::floor(gx));
        const auto z0 = static_cast<std::int32_t>(std::floor(gz));
        const float tx = SmoothStep(gx - static_cast<float>(x0));
        const float tz = SmoothStep(gz - static_cast<float>(z0));
        const auto value = [seed](const std::int32_t cx, const std::int32_t cz) {
            return UnitRandom(Hash(cx, cz, seed));
        };
        const float back = value(x0, z0) + (value(x0 + 1, z0) - value(x0, z0)) * tx;
        const float front = value(x0, z0 + 1) + (value(x0 + 1, z0 + 1) - value(x0, z0 + 1)) * tx;
        return back + (front - back) * tz;
    }

    float VegetationLayout::ClusterNoise(const float x, const float z,
                                         const std::uint32_t seed) noexcept
    {
        constexpr float cellSize = 48.0f;
        const float gx = x / cellSize;
        const float gz = z / cellSize;
        const auto x0 = static_cast<std::int32_t>(std::floor(gx));
        const auto z0 = static_cast<std::int32_t>(std::floor(gz));
        const float tx = SmoothStep(gx - static_cast<float>(x0));
        const float tz = SmoothStep(gz - static_cast<float>(z0));
        const auto value = [seed](const std::int32_t cellX, const std::int32_t cellZ) {
            return UnitRandom(Hash(cellX, cellZ, seed));
        };
        const float back = value(x0, z0)
            + (value(x0 + 1, z0) - value(x0, z0)) * tx;
        const float front = value(x0, z0 + 1)
            + (value(x0 + 1, z0 + 1) - value(x0, z0 + 1)) * tx;
        return back + (front - back) * tz;
    }

    float VegetationLayout::PrimaryPathCenterX(const float z) noexcept
    {
        return PrimaryPathBaseX + PrimaryPathAmplitude
            * std::sin((z + PrimaryPathZOffset) * PrimaryPathFrequency);
    }

    bool VegetationLayout::IsReservedRoute(const float x, const float z) noexcept
    {
        if(CaveSite::Reserves(x,z,5)||BunkerSite::Reserves(x,z,2)||CampfireSite::Reserves(x,z)
            ||HelicopterSite::Reserves(x,z))return true;
        // A footpath-sized reserve lets the forest approach the walking line.
        // Solid trunks additionally exclude their footprint so the narrow path stays usable.
        const float pathX = PrimaryPathCenterX(z);
        if (z > PrimaryPathMinimumZ && z < PrimaryPathMaximumZ
            && std::abs(x - pathX) < PrimaryPathReservedHalfWidth) {
            return true;
        }

        // Two intentional rooms in the vegetation and a safe uncluttered player spawn.
        return IsInsideAuthoredClearing(x, z)
            || DistanceSquared(x, z, PlayerSpawnReserveX, PlayerSpawnReserveZ)
                < PlayerSpawnReserveRadius * PlayerSpawnReserveRadius;
    }

    bool VegetationLayout::IsInsideAuthoredClearing(const float x, const float z) noexcept
    {
        return DistanceSquared(x, z, EastClearing.x, EastClearing.z)
                   < EastClearing.radius * EastClearing.radius
            || DistanceSquared(x, z, WestClearing.x, WestClearing.z)
                   < WestClearing.radius * WestClearing.radius;
    }

    bool VegetationLayout::OverlapsReservedRoute(float x, float z, float radius) noexcept
    {
        if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(radius) || radius < 0)
            return true;
        if(CaveSite::Reserves(x,z,5+radius)||BunkerSite::Reserves(x,z,2+radius)||CampfireSite::Reserves(x,z,radius)
            ||HelicopterSite::Reserves(x,z,radius))return true;
        for (const auto clearing : {EastClearing, WestClearing,
                                    ClearingDefinition{PlayerSpawnReserveX, PlayerSpawnReserveZ,
                                                       PlayerSpawnReserveRadius}}) {
            if (std::hypot(x - clearing.x, z - clearing.z) <= clearing.radius + radius) return true;
        }
        // |dx - path'(z)*dz| <= radius*sqrt(1 + maximumDerivative^2). Expanding both
        // path ends is conservative too, without an iterative closest-point approximation.
        constexpr float derivative = PrimaryPathAmplitude * PrimaryPathFrequency;
        const float horizontalMargin = radius * std::sqrt(1 + derivative * derivative);
        return z >= PrimaryPathMinimumZ - radius && z <= PrimaryPathMaximumZ + radius
            && std::abs(x - PrimaryPathCenterX(z)) <= PrimaryPathReservedHalfWidth + horizontalMargin;
    }

    VegetationLayout::VegetationLayout(const IslandTerrain& terrain, const std::uint32_t seed)
    {
        palms_.reserve(384U);
        obstacles_.reserve(384U);

        for (int gridZ = -CandidateRadius; gridZ <= CandidateRadius; ++gridZ) {
            for (int gridX = -CandidateRadius; gridX <= CandidateRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * CandidateSpacing * 0.64f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0xa52c3d19U)) - 0.5f)
                    * CandidateSpacing * 0.64f;
                const float x = static_cast<float>(gridX) * CandidateSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * CandidateSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                const auto environment=IslandEnvironmentField::SampleInland(
                    x,z,sample.signedShoreDistance);

                // Palms form a broad coastal belt, with sparser groups toward the interior. The
                // eventual broadleaf canopy will own the deepest central jungle.
                if (sample.signedShoreDistance < 7.0f || sample.signedShoreDistance > 158.0f
                    || sample.height < 0.55f || sample.slopeRadians > 0.4712389f
                    || sample.coastalRockiness > 0.45f
                    || IsReservedRoute(x, z) || environment.drainage > 0.68f) {
                    continue;
                }

                // Coconut groves on the beach-backed coastal belt, thinning inland to a few old
                // solitary palms. A contrast-shaped two-octave grove field makes the groves.
                const float coastal = 1.0f - SmoothStep((sample.signedShoreDistance - 32.0f) / 70.0f);
                const float grove = SmoothStep((0.68f * GroveNoise(x, z, seed ^ 0x31415926U, 62.0f)
                    + 0.32f * GroveNoise(x, z, seed ^ 0x27182818U, 21.0f) - 0.42f) / 0.22f);
                const float density = std::clamp(0.006f + coastal * (0.012f + 0.36f * grove), 0.0f, 0.90f);
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0x6c8e9cf5U)) >= density) {
                    continue;
                }

                PalmPlacement palm;
                palm.x = x;
                palm.y = sample.height;
                palm.z = z;
                palm.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x92e5a271U))
                    * 2.0f * std::numbers::pi_v<float>;
                palm.scale = PalmMinimumScale
                    + (PalmMaximumScale - PalmMinimumScale)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0x4a39b70dU));
                palm.shoreDistance = sample.signedShoreDistance;
                palm.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xf17a245bU) % PalmVariantCount);
                if (OverlapsReservedRoute(x, z, 0.31f * palm.scale)) continue;
                palms_.push_back(palm);
                obstacles_.push_back(CircularObstacle{palm.x, palm.z, 0.31f * palm.scale});

                // A young palm beside an adult inside a grove, offset 2.6-4.4 m.
                if (grove < PalmGroveThreshold
                    || UnitRandom(Hash(gridX, gridZ, seed ^ 0x5851f42dU)) >= PalmJuvenileChance) {
                    continue;
                }
                const float angle = UnitRandom(Hash(gridX, gridZ, seed ^ 0x4c957f2dU))
                    * 2.0f * std::numbers::pi_v<float>;
                const float offset = 2.6f + 1.8f * UnitRandom(Hash(gridX, gridZ, seed ^ 0x6a5d39e9U));
                const float youngX = x + std::cos(angle) * offset;
                const float youngZ = z + std::sin(angle) * offset;
                const TerrainSample youngSample = terrain.Sample(youngX, youngZ);
                const auto youngEnvironment=IslandEnvironmentField::SampleInland(
                    youngX,youngZ,youngSample.signedShoreDistance);
                if (youngSample.signedShoreDistance < 7.0f || youngSample.height < 0.55f
                    || youngSample.coastalRockiness > 0.45f
                    || youngSample.slopeRadians > 0.4712389f || IsReservedRoute(youngX, youngZ)
                    || youngEnvironment.drainage > 0.68f) {
                    continue;
                }
                bool crowded = false;
                for (const PalmPlacement& other : palms_) {
                    if (DistanceSquared(other.x, other.z, youngX, youngZ) < 2.2f * 2.2f) { crowded = true; break; }
                }
                if (crowded) continue;
                PalmPlacement young;
                young.x = youngX;
                young.y = youngSample.height;
                young.z = youngZ;
                young.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x2545f491U))
                    * 2.0f * std::numbers::pi_v<float>;
                young.scale = PalmJuvenileMinimumScale + (PalmJuvenileMaximumScale - PalmJuvenileMinimumScale)
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0x9e3779b1U));
                young.shoreDistance = youngSample.signedShoreDistance;
                young.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0x7f4a7c13U) % PalmVariantCount);
                if (OverlapsReservedRoute(youngX, youngZ, 0.31f * young.scale)) continue;
                palms_.push_back(young);
                obstacles_.push_back(CircularObstacle{young.x, young.z, 0.31f * young.scale});
            }
        }

        // A small storm history interrupts otherwise pristine groves without increasing the
        // living-palm population. Fallen stems must remain entirely on stable dry ground and
        // clear of the authored route; their visible length receives inexpensive round collision
        // proxies so the player cannot walk through the log.
        std::size_t fallenCount = 0U;
        for (std::size_t index = 0; index < palms_.size()
                && fallenCount < FallenPalmTargetCount; ++index) {
            PalmPlacement& palm = palms_[index];
            if (palm.scale < PalmMinimumScale || palm.shoreDistance < 13.0f
                || palm.shoreDistance > 72.0f) {
                continue;
            }
            const auto indexHash = Hash(static_cast<std::int32_t>(index), 17,
                                        seed ^ 0xdab4c291U);
            if (indexHash % 7U != 0U) continue;
            const float direction = UnitRandom(indexHash ^ 0x517cc1b7U)
                * 2.0f * std::numbers::pi_v<float>;
            const float length = (8.6f + 0.45f * static_cast<float>(palm.variant)) * palm.scale;
            bool suitable = true;
            for (int step = 1; step <= 6; ++step) {
                const float distance = length * static_cast<float>(step) / 6.0f;
                const float x = palm.x + std::sin(direction) * distance;
                const float z = palm.z + std::cos(direction) * distance;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 5.0f || sample.slopeRadians > 0.31f
                    || std::abs(sample.height - palm.y) > 0.85f
                    || OverlapsReservedRoute(x, z, 0.65f)) {
                    suitable = false;
                    break;
                }
                for (const PalmPlacement& other : palms_) {
                    if (&other == &palm) continue;
                    const float clearance = step == 6 ? 2.2f : 0.85f;
                    if (DistanceSquared(other.x, other.z, x, z) < clearance * clearance) {
                        suitable = false;
                        break;
                    }
                }
                if (!suitable) break;
            }
            if (!suitable) continue;
            palm.condition = PalmCondition::Fallen;
            palm.damageYawRadians = direction;
            for (int step = 1; step <= 5; ++step) {
                const float distance = length * static_cast<float>(step) / 6.0f;
                obstacles_.push_back(CircularObstacle{
                    palm.x + std::sin(direction) * distance,
                    palm.z + std::cos(direction) * distance,
                    0.24f + 0.12f * palm.scale});
            }
            ++fallenCount;
        }

        std::size_t brokenCount = 0U;
        for (std::size_t index = 0; index < palms_.size()
                && brokenCount < BrokenPalmTargetCount; ++index) {
            PalmPlacement& palm = palms_[index];
            if (palm.condition != PalmCondition::Living || palm.scale < PalmMinimumScale
                || palm.shoreDistance < 10.0f || palm.shoreDistance > 105.0f) {
                continue;
            }
            const auto indexHash = Hash(static_cast<std::int32_t>(index), 29,
                                        seed ^ 0x6bd9a5e3U);
            if (indexHash % 5U != 0U) continue;
            palm.condition = PalmCondition::Broken;
            palm.damageYawRadians = UnitRandom(indexHash ^ 0x92d68ca3U)
                * 2.0f * std::numbers::pi_v<float>;
            ++brokenCount;
        }
    }
}
