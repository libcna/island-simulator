#include "IslandSimulator/NaturalPropLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/StreamBedPatch.hpp"
#include "IslandSimulator/StreamRoute.hpp"
#include "IslandSimulator/TidePoolLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <numbers>
#include <stdexcept>

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
        reefHabitat_.reserve(320U);
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
                const auto experience=IslandEnvironmentField::SampleCoastalExperience(
                    x,z,sample.signedShoreDistance);
                const float density = std::clamp(
                    0.035f + coast * 0.21f + cluster * 0.095f
                        + experience.rockyShore*.14f,
                    0.0f, 0.44f);
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

        // Two low non-solid stones frame each pool without adding collision or a draw path.
        for (std::size_t poolIndex=0;poolIndex<TidePoolLayout::Definitions.size();++poolIndex) {
            const auto [poolX,poolZ]=TidePoolLayout::Centre(terrain,poolIndex);
            const auto& pool=TidePoolLayout::Definitions[poolIndex];
            for (int accent=0;accent<2;++accent) {
                const float angle=.62f+static_cast<float>(poolIndex)*.91f+accent*2.47f;
                const float radius=pool.radius*(.86f+.08f*accent);
                RockPlacement rock;
                rock.x=poolX+std::cos(angle)*radius;
                rock.z=poolZ+std::sin(angle)*radius;
                const auto support=terrain.Sample(rock.x,rock.z);
                const float scale=.48f+.10f*static_cast<float>((poolIndex+accent)%3U);
                rock.y=support.height-.055f*scale;
                rock.yawRadians=angle+.4f;
                rock.scaleX=scale*(1.15f+.12f*accent);
                rock.scaleY=scale*(.62f+.08f*accent);
                rock.scaleZ=scale*(.88f+.14f*((poolIndex+1U)%2U));
                rock.variant=static_cast<std::uint8_t>((poolIndex+accent)%RockVariantCount);
                rock.solid=false;
                rock.tidePoolLandmark=true;
                rocks_.push_back(rock);
            }
        }

        // Small non-solid stones sit just beyond the collision-authoritative bank blend.
        // Reusing the existing rock instances adds erosion cues without blocking a crossing,
        // adding a texture, or creating a stream-only draw path.
        for (int candidate = 0; candidate < StreamBankRockCandidateCount; ++candidate) {
            const auto random = [&](const std::uint32_t channel) {
                return UnitRandom(Hash(candidate, static_cast<std::int32_t>(channel),
                    seed ^ (0x51f15e2dU + channel * 0x9e3779b9U)));
            };
            const float progress = (static_cast<float>(candidate) + .25f + .5f * random(0))
                / static_cast<float>(StreamBankRockCandidateCount);
            const float baseX = -76.0f - 74.0f * progress;
            const float centreZ = StreamRoute::CentreZ(baseX);
            const float tangentZ = StreamRoute::CentreZ(baseX - .5f)
                - StreamRoute::CentreZ(baseX + .5f);
            const float tangentLength = std::hypot(1.0f, tangentZ);
            const float tangentX = -1.0f / tangentLength;
            const float tangentZUnit = tangentZ / tangentLength;
            const float side = candidate % 2 == 0 ? -1.0f : 1.0f;
            const float bankDistance = StreamRoute::HalfWidth(baseX)
                + StreamBedPatch::BankBlendWidth + .16f + .72f * random(1);
            RockPlacement rock;
            rock.x = baseX + side * bankDistance * tangentZUnit;
            rock.z = centreZ - side * bankDistance * tangentX;
            const auto support = terrain.Sample(rock.x, rock.z);
            if (support.signedShoreDistance < 8.0f || support.height < .15f
                || support.slopeRadians > .5759587f
                || VegetationLayout::IsReservedRoute(rock.x, rock.z)
                || NearTree(rock.x, rock.z, .78f, palms, jungle)) continue;
            const float scale = .46f + .24f * random(2);
            rock.y = support.height - .08f * scale;
            rock.yawRadians = random(3) * 2.0f * std::numbers::pi_v<float>;
            rock.scaleX = scale * (.90f + .27f * random(4));
            rock.scaleY = scale * (.58f + .18f * random(5));
            rock.scaleZ = scale * (.88f + .30f * random(6));
            rock.variant = static_cast<std::uint8_t>(
                Hash(candidate, 89, seed ^ 0x6d2b79f5U) % RockVariantCount);
            rock.solid = false;
            rock.streamBankLandmark = true;
            rocks_.push_back(rock);
            ++streamBankRockCount_;
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

        // Cheap non-solid coral/rock habitat follows the existing shallow reef material belt.
        // A shared analytic suitability field keeps later fish/turtle routing tied to exactly
        // the same patches instead of adding another unrelated radial band.
        for (int site = 0; site < ReefHabitatSiteCount; ++site) {
            const auto random = [&](const std::uint32_t channel) {
                return UnitRandom(Hash(site, static_cast<std::int32_t>(channel),
                    seed ^ (0x6a09e667U + channel * 0x9e3779b9U)));
            };
            // Random rather than evenly stratified angles allow real patch/gap structure; the
            // suitability field still bounds the accepted coastal sectors and depths.
            const float angle = random(0) * 2.0f * std::numbers::pi_v<float>;
            const float offshoreDistance = 18.0f + 70.0f * std::pow(random(1), .84f);
            const float radius = terrain.ShoreRadius(angle) + offshoreDistance;
            ReefHabitatPlacement habitat;
            habitat.x = std::cos(angle) * radius;
            habitat.z = std::sin(angle) * radius;
            const auto support = terrain.Sample(habitat.x, habitat.z);
            const float suitability = IslandEnvironmentField::ReefSuitability(
                habitat.x, habitat.z, support.signedShoreDistance, support.height);
            if (suitability < .12f || random(2) > .18f + .68f * suitability) continue;
            habitat.y = support.height + .025f;
            habitat.yawRadians = random(3) * 2.0f * std::numbers::pi_v<float>;
            habitat.variant = static_cast<std::uint8_t>(
                Hash(site, 97, seed ^ 0xbb67ae85U) % ReefHabitatVariantCount);
            habitat.scale = ReefHabitatMinimumScale
                + (ReefHabitatMaximumScale - ReefHabitatMinimumScale)
                    * (.20f + .80f * std::pow(random(4), 1.35f));
            habitat.scale = std::clamp(habitat.scale * (habitat.variant == 3U ? .78f : 1.14f),
                ReefHabitatMinimumScale, ReefHabitatMaximumScale);
            reefHabitat_.push_back(habitat);
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

        // A few damp-bank trunks make the lower stream read as a lived-in forest drainage.
        // Their long axes mostly follow the channel, so the complete collision footprint stays
        // on one bank instead of bridging the walkable water.
        for (int candidate = 0; candidate < StreamBankLogCandidateCount; ++candidate) {
            const auto random = [&](std::uint32_t channel) {
                return UnitRandom(Hash(candidate, static_cast<std::int32_t>(channel),
                    seed ^ (0xa6c7395dU + channel * 0x9e3779b9U)));
            };
            const float progress = (static_cast<float>(candidate) + .15f + .7f * random(0))
                / static_cast<float>(StreamBankLogCandidateCount);
            const float baseX = -82.0f - 68.0f * progress;
            const float centreZ = StreamRoute::CentreZ(baseX);
            const float side = random(1) < .48f ? -1.0f : 1.0f;
            const float bankDistance = 5.7f + 2.4f * random(2);
            const float tangentZ = StreamRoute::CentreZ(baseX - .5f)
                - StreamRoute::CentreZ(baseX + .5f);
            const float tangentLength = std::hypot(1.0f, tangentZ);
            const float tangentX = -1.0f / tangentLength;
            const float tangentZUnit = tangentZ / tangentLength;
            const float normalX = tangentZUnit;
            const float normalZ = -tangentX;
            FallenLogPlacement log;
            log.x = baseX + side * bankDistance * normalX;
            log.z = centreZ + side * bankDistance * normalZ;
            log.yawRadians = std::atan2(tangentZUnit, tangentX)
                + (random(3) - .5f) * .42f;
            log.length = 4.0f + 1.8f * random(4);
            log.radius = .24f + .07f * random(5);
            log.variant = 1U; // The darker bark reads as damp without another material/draw.
            log.wetBank = true;
            const auto centre = terrain.Sample(log.x, log.z);
            const auto environment = IslandEnvironmentField::SampleInland(
                log.x, log.z, centre.signedShoreDistance);
            if (centre.signedShoreDistance < 8.0f || environment.moisture < .38f
                || environment.drainage > .72f
                || NearTree(log.x, log.z, 2.2f, palms, jungle)) continue;
            const float directionX = std::cos(log.yawRadians);
            const float directionZ = std::sin(log.yawRadians);
            float maximumSupport = centre.height;
            float minimumSupport = centre.height;
            bool valid = true;
            for (int sampleIndex = 0; sampleIndex < LogCollisionSamples; ++sampleIndex) {
                const float t = static_cast<float>(sampleIndex)
                    / (LogCollisionSamples - 1) - .5f;
                const float x = log.x + directionX * log.length * t;
                const float z = log.z + directionZ * log.length * t;
                const auto support = terrain.Sample(x, z);
                const auto field = IslandEnvironmentField::SampleInland(
                    x, z, support.signedShoreDistance);
                maximumSupport = std::max(maximumSupport, support.height);
                minimumSupport = std::min(minimumSupport, support.height);
                if (support.signedShoreDistance < 8.0f || support.height < .5f
                    || support.slopeRadians > .34906585f || field.drainage > .76f
                    || VegetationLayout::IsReservedRoute(x, z)
                    || NearTree(x, z, 1.4f, palms, jungle)) {
                    valid = false;
                    break;
                }
            }
            if (!valid || maximumSupport - minimumSupport > .34f) continue;
            log.y = maximumSupport + log.radius * .68f;
            fallenLogs_.push_back(log);
            ++streamBankLogCount_;
            for (int sampleIndex = 0; sampleIndex < LogCollisionSamples; ++sampleIndex) {
                const float t = static_cast<float>(sampleIndex)
                    / (LogCollisionSamples - 1) - .5f;
                obstacles_.push_back({log.x + directionX * log.length * t,
                    log.z + directionZ * log.length * t, log.radius * .92f});
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
                const auto debris = IslandEnvironmentField::SampleCoastalDebris(
                    x, z, sample.signedShoreDistance);
                const float density = std::clamp(0.20f + 0.22f * cluster
                    + 0.30f * debris.accumulation, 0.0f, 0.72f);
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
            float minimumSupportHeight = maximumSupportHeight;
            float maximumSupportSlope = terrain.Sample(wood.x, wood.z).slopeRadians;
            for (int sampleIndex = 0; sampleIndex < DriftwoodSupportSamples; ++sampleIndex) {
                const float t = static_cast<float>(sampleIndex)
                    / static_cast<float>(DriftwoodSupportSamples - 1) - 0.5f;
                const auto support = terrain.Sample(
                    wood.x + directionX * wood.length * t,
                    wood.z + directionZ * wood.length * t);
                maximumSupportHeight = std::max(maximumSupportHeight, support.height);
                minimumSupportHeight = std::min(minimumSupportHeight, support.height);
                maximumSupportSlope = std::max(maximumSupportSlope, support.slopeRadians);
            }
            const auto centre = terrain.Sample(wood.x, wood.z);
            const auto coast = IslandEnvironmentField::SampleCoast(
                wood.x, wood.z, centre.signedShoreDistance);
            if (coast.beachSuitability < 0.72f || maximumSupportSlope > 0.17f
                    || maximumSupportHeight - minimumSupportHeight > 0.22f) {
                throw std::runtime_error(
                    "Authored cove driftwood must remain on broad level dry sand; slope="
                    + std::to_string(maximumSupportSlope) + ", relief="
                    + std::to_string(maximumSupportHeight - minimumSupportHeight));
            }
            wood.y = maximumSupportHeight + wood.radius * 0.58f;
            driftwood_.push_back(wood);
        }

        // A short, irregular accumulation line makes the prevailing east-to-west wind
        // legible in the protected western bay. These are ordinary existing silhouettes,
        // selected from many deterministic candidates rather than authored coordinates.
        int windSortedBayWoodCount=0;
        for (int candidate=0;candidate<160
                && windSortedBayWoodCount<WindSortedBayDriftwoodCount;++candidate) {
            const std::uint32_t candidateHash=Hash(candidate,-candidate,seed^0x8f1bbcdcU);
            const float angle=std::numbers::pi_v<float>
                +(UnitRandom(candidateHash)-.5f)*.86f;
            const float inland=5.8f+11.5f*UnitRandom(
                Hash(candidate,candidate,seed^0x4cf5ad43U));
            const float radius=terrain.ShoreRadius(angle)-inland;
            DriftwoodPlacement wood;
            wood.x=std::cos(angle)*radius;
            wood.z=std::sin(angle)*radius;
            const auto centre=terrain.Sample(wood.x,wood.z);
            const auto debris=IslandEnvironmentField::SampleCoastalDebris(
                wood.x,wood.z,centre.signedShoreDistance);
            if (debris.accumulation<.58f || centre.signedShoreDistance<2.0f
                    || centre.signedShoreDistance>25.0f || centre.height<.12f
                    || centre.slopeRadians>.34906585f
                    || VegetationLayout::IsReservedRoute(wood.x,wood.z)
                    || NearTree(wood.x,wood.z,1.8f,palms,jungle)) continue;
            wood.length=DriftwoodMinimumLength+(DriftwoodMaximumLength-DriftwoodMinimumLength)
                *UnitRandom(Hash(candidate,1,seed^0x6c8e9cf5U));
            wood.radius=.09f+.09f*UnitRandom(Hash(candidate,2,seed^0x2d98f3a9U));
            wood.variant=static_cast<std::uint8_t>(Hash(candidate,3,seed^0x71b34d65U)
                %DriftwoodVariantCount);
            wood.yawRadians=angle+std::numbers::pi_v<float>*.5f
                +(UnitRandom(Hash(candidate,4,seed^0x9e3779b9U))-.5f)*.74f;
            const float directionX=std::cos(wood.yawRadians);
            const float directionZ=std::sin(wood.yawRadians);
            float maximumSupportHeight=centre.height,minimumSupportHeight=centre.height;
            bool validFootprint=true;
            for (int sampleIndex=0;sampleIndex<DriftwoodSupportSamples;++sampleIndex) {
                const float t=static_cast<float>(sampleIndex)
                    /static_cast<float>(DriftwoodSupportSamples-1)-.5f;
                const float sampleX=wood.x+directionX*wood.length*t;
                const float sampleZ=wood.z+directionZ*wood.length*t;
                const auto support=terrain.Sample(sampleX,sampleZ);
                maximumSupportHeight=std::max(maximumSupportHeight,support.height);
                minimumSupportHeight=std::min(minimumSupportHeight,support.height);
                if (support.signedShoreDistance<1.5f || support.signedShoreDistance>27.0f
                        || support.slopeRadians>.40142573f
                        || VegetationLayout::IsReservedRoute(sampleX,sampleZ)
                        || NearTree(sampleX,sampleZ,1.05f,palms,jungle)) {
                    validFootprint=false;
                    break;
                }
            }
            if (!validFootprint || maximumSupportHeight-minimumSupportHeight>.22f) continue;
            if (std::ranges::any_of(driftwood_,[&wood](const DriftwoodPlacement& existing) {
                    return DistanceSquared(wood.x,wood.z,existing.x,existing.z)<10.24f;
                })) continue;
            wood.y=maximumSupportHeight+wood.radius*.58f;
            driftwood_.push_back(wood);
            ++windSortedBayWoodCount;
        }

        beachDetails_.reserve(440U);
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
                const auto experience=IslandEnvironmentField::SampleCoastalExperience(
                    x,z,sample.signedShoreDistance);
                const auto debris=IslandEnvironmentField::SampleCoastalDebris(
                    x,z,sample.signedShoreDistance);
                const float unsortedDensity = std::clamp(.22f + cluster*.26f
                    + experience.broadBeach*.24f + experience.shelteredBay*.18f
                    + experience.mangroveMouth*.26f,0.0f,.90f);
                // Preserve every established crab-habitat cue around the walkable coast;
                // sorting adds shell/pebble deposition in shelter instead of hollowing out
                // previously inhabited shore sectors.
                const float density=std::clamp(unsortedDensity
                    *(1.0f+.18f*debris.accumulation),0.0f,.92f);
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
                const float wrackAffinity=experience.broadBeach>.35f ? 0.0f
                    : SmoothStep((debris.accumulation-.34f)/.48f);
                const float variantRoll=UnitRandom(Hash(gridX,gridZ,seed^0x1f83d9abU));
                if (variantRoll < wrackAffinity) detail.variant=3U;
                else detail.variant=static_cast<std::uint8_t>(
                    Hash(gridX,gridZ,seed^0x5be0cd19U)%3U);
                beachDetails_.push_back(detail);
            }
        }

        int windSortedBayDetailCount=0;
        for (int candidate=0;candidate<240
                && windSortedBayDetailCount<WindSortedBayDetailCount;++candidate) {
            const float angle=std::numbers::pi_v<float>
                +(UnitRandom(Hash(candidate,5,seed^0xa4093822U))-.5f)*.90f;
            const float inland=4.0f+13.0f*UnitRandom(Hash(candidate,6,seed^0x299f31d0U));
            const float radius=terrain.ShoreRadius(angle)-inland;
            const float x=std::cos(angle)*radius,z=std::sin(angle)*radius;
            const auto sample=terrain.Sample(x,z);
            const auto debris=IslandEnvironmentField::SampleCoastalDebris(
                x,z,sample.signedShoreDistance);
            if (debris.accumulation<.58f || sample.signedShoreDistance<3.0f
                    || sample.signedShoreDistance>18.0f || sample.height<.22f
                    || sample.slopeRadians>.2617994f
                    || VegetationLayout::IsReservedRoute(x,z)) continue;
            if (std::ranges::any_of(beachDetails_,[x,z](const BeachDetailPlacement& existing) {
                    return DistanceSquared(x,z,existing.x,existing.z)<3.24f;
                })) continue;
            BeachDetailPlacement detail;
            detail.x=x;
            detail.y=sample.height+.012f;
            detail.z=z;
            detail.yawRadians=UnitRandom(Hash(candidate,7,seed^0x082efa98U))
                *2.0f*std::numbers::pi_v<float>;
            detail.scale=BeachDetailMinimumScale
                +(BeachDetailMaximumScale-BeachDetailMinimumScale)
                    *UnitRandom(Hash(candidate,8,seed^0xec4e6c89U));
            detail.variant=windSortedBayDetailCount%2==0 ? 3U
                : static_cast<std::uint8_t>(Hash(candidate,9,seed^0x452821e6U)%3U);
            beachDetails_.push_back(detail);
            ++windSortedBayDetailCount;
        }

        for (std::size_t poolIndex=0;poolIndex<TidePoolLayout::Definitions.size();++poolIndex) {
            const auto [poolX,poolZ]=TidePoolLayout::Centre(terrain,poolIndex);
            const auto& pool=TidePoolLayout::Definitions[poolIndex];
            const float angle=2.15f+static_cast<float>(poolIndex)*1.07f;
            BeachDetailPlacement detail;
            detail.x=poolX+std::cos(angle)*pool.radius*.82f;
            detail.z=poolZ+std::sin(angle)*pool.radius*.82f;
            detail.y=terrain.Height(detail.x,detail.z)+.012f;
            detail.yawRadians=angle-.7f;
            detail.scale=.82f+.07f*static_cast<float>(poolIndex%3U);
            detail.variant=static_cast<std::uint8_t>(poolIndex%3U);
            detail.tidePoolLandmark=true;
            beachDetails_.push_back(detail);
        }

        std::vector<std::array<float, 2>> turtleNestCentres;
        for (int candidate = 0; candidate < 36
             && turtleNestCentres.size() < static_cast<std::size_t>(TurtleNestingTrackCount);
             ++candidate) {
            const float angle = 2.78f + static_cast<float>(candidate) * 0.026f;
            std::vector<BeachDetailPlacement> pending;
            pending.reserve(TurtleTrackSegmentsPerNest + 1U);
            bool valid = true;
            for (int segment = 0; segment < TurtleTrackSegmentsPerNest; ++segment) {
                const float inland = 4.2f + static_cast<float>(segment) * 1.34f;
                const float radius = terrain.ShoreRadius(angle) - inland;
                const float x = std::cos(angle) * radius;
                const float z = std::sin(angle) * radius;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 3.0f
                    || sample.signedShoreDistance > 18.0f
                    || sample.height < 0.22f || sample.slopeRadians > 0.20943951f
                    || VegetationLayout::IsReservedRoute(x, z)
                    || NearTree(x, z, 1.4f, palms, jungle)
                    || std::ranges::any_of(beachDetails_, [x, z](const auto& detail) {
                           return DistanceSquared(x, z, detail.x, detail.z) < 0.64f;
                       })) {
                    valid = false;
                    break;
                }
                BeachDetailPlacement mark;
                mark.x = x;
                mark.y = sample.height + 0.008f;
                mark.z = z;
                mark.yawRadians = -angle - std::numbers::pi_v<float> * 0.5f;
                mark.scale = 0.88f + 0.025f * static_cast<float>(segment % 3);
                mark.variant = 4U;
                mark.turtleTrace = true;
                pending.push_back(mark);
            }
            const float nestInland = 14.1f;
            const float nestRadius = terrain.ShoreRadius(angle) - nestInland;
            const float nestX = std::cos(angle) * nestRadius;
            const float nestZ = std::sin(angle) * nestRadius;
            const TerrainSample nestSample = terrain.Sample(nestX, nestZ);
            if (!valid || nestSample.signedShoreDistance < 3.0f
                || nestSample.signedShoreDistance > 18.0f
                || nestSample.height < 0.22f || nestSample.slopeRadians > 0.20943951f
                || VegetationLayout::IsReservedRoute(nestX, nestZ)
                || NearTree(nestX, nestZ, 1.8f, palms, jungle)
                || std::ranges::any_of(turtleNestCentres, [nestX, nestZ](const auto& centre) {
                       return DistanceSquared(nestX, nestZ, centre[0], centre[1]) < 324.0f;
                   })
                || std::ranges::any_of(beachDetails_, [nestX, nestZ](const auto& detail) {
                       return DistanceSquared(nestX, nestZ, detail.x, detail.z) < 2.25f;
                   })) {
                continue;
            }
            BeachDetailPlacement nest;
            nest.x = nestX;
            nest.y = nestSample.height + 0.008f;
            nest.z = nestZ;
            nest.yawRadians = -angle - std::numbers::pi_v<float> * 0.5f;
            nest.scale = 1.18f;
            nest.variant = 5U;
            nest.turtleTrace = true;
            nest.turtleNest = true;
            pending.push_back(nest);
            beachDetails_.insert(beachDetails_.end(), pending.begin(), pending.end());
            turtleNestCentres.push_back({nestX, nestZ});
        }
        if (turtleNestCentres.size() != static_cast<std::size_t>(TurtleNestingTrackCount)) {
            throw std::runtime_error("Unable to place both turtle nesting traces on dry sand");
        }

        fallenFronds_.reserve(150U);
        for (const PalmPlacement& palm : palms.Palms()) {
            const auto palmX = static_cast<std::int32_t>(std::lround(palm.x * 8.0f));
            const auto palmZ = static_cast<std::int32_t>(std::lround(palm.z * 8.0f));
            const std::uint32_t siteHash = Hash(palmX, palmZ, seed ^ 0x5be0cd19U);

            const float radialAngle = UnitRandom(Hash(palmX, palmZ, seed ^ 0xcbbb9d5dU))
                * 2.0f * std::numbers::pi_v<float>;
            const float offset = 2.4f + 2.8f
                * UnitRandom(Hash(palmX, palmZ, seed ^ 0x629a292aU));
            const float x = palm.x + std::cos(radialAngle) * offset;
            const float z = palm.z + std::sin(radialAngle) * offset;
            const TerrainSample centre = terrain.Sample(x, z);
            const auto debris=IslandEnvironmentField::SampleCoastalDebris(
                x,z,centre.signedShoreDistance);
            const float coastalInfluence=1.0f-SmoothStep(
                (centre.signedShoreDistance-35.0f)/55.0f);
            const float frondDensity=std::lerp(.46f,
                .08f+.68f*debris.accumulation,coastalInfluence);
            if (UnitRandom(siteHash) >= frondDensity) continue;
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
