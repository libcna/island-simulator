#include "IslandSimulator/GrassLayout.hpp"

#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
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
        constexpr int CandidateRadius = 132;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] float ClearingInfluence(const float x, const float z,
                                              const ClearingDefinition& clearing) noexcept
        {
            const float offsetX = x - clearing.x;
            const float offsetZ = z - clearing.z;
            const float normalizedDistance = std::sqrt(offsetX * offsetX + offsetZ * offsetZ)
                / clearing.radius;
            return 1.0f - SmoothStep((normalizedDistance - 0.68f) / 0.32f);
        }
    }

    std::uint32_t GrassLayout::Hash(const std::int32_t x, const std::int32_t z,
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

    float GrassLayout::UnitRandom(const std::uint32_t value) noexcept
    {
        return static_cast<float>(value & 0x00ffffffU) / static_cast<float>(0x01000000U);
    }

    GrassLayout::GrassLayout(const IslandTerrain& terrain, const CanopyLightField& light,
                             const GroundCoverField& cover, const std::uint32_t seed)
    {
        patches_.reserve(28000U);
        const auto accepted = [&terrain](const float x, const float z, TerrainSample& sample) {
            sample = terrain.Sample(x, z);
            return sample.signedShoreDistance >= 20.0f && sample.height >= 2.1f
                && sample.slopeRadians <= 0.4886922f && sample.coastalRockiness < 0.60f
                && sample.beachSuitability < GroundCover::BeachSuppressionEnd;
        };
        const auto coverAt = [&light, &cover](const float x, const float z,
                                              const TerrainSample& sample) {
            return GroundCover::Density(std::clamp(light.Sample(x, z), 0.0f, 1.0f),
                std::clamp(cover.SamplePatch(x, z), 0.0f, 1.0f),
                sample.signedShoreDistance, sample.slopeRadians,
                sample.coastalRockiness, sample.beachSuitability);
        };
        for (int gridZ = -CandidateRadius; gridZ <= CandidateRadius; ++gridZ) {
            for (int gridX = -CandidateRadius; gridX <= CandidateRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * CandidateSpacing * 0.78f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0x91e10da5U)) - 0.5f)
                    * CandidateSpacing * 0.78f;
                const float x = static_cast<float>(gridX) * CandidateSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * CandidateSpacing + jitterZ;
                TerrainSample sample;
                if (!accepted(x, z, sample)) continue;

                const bool pathScale = VegetationLayout::IsReservedRoute(x, z);
                const float coastalBlend = SmoothStep((sample.signedShoreDistance - 20.0f) / 42.0f);
                // Sunlit, patch-rich ground forms continuous swards; deep canopy shade and
                // patch-poor ground keep isolated tufts over exposed humus and litter.
                const float groundCover = coverAt(x, z, sample);
                float density = 0.05f + 0.95f * groundCover;
                if (pathScale) {
                    // Keep the route legible through lower blades rather than a conspicuous bare
                    // stripe. Its denser short turf also softens the still-procedural ground.
                    density = std::max(0.62f, density * 0.88f);
                }
                if (UnitRandom(Hash(gridX, gridZ, seed ^ 0xa54ff53aU)) >= density) {
                    continue;
                }

                GrassPlacement patch;
                patch.x = x;
                patch.y = sample.height;
                patch.z = z;
                patch.yawRadians = UnitRandom(Hash(gridX, gridZ, seed ^ 0x510e527fU))
                    * 2.0f * std::numbers::pi_v<float>;
                patch.scale = MinimumScale + (MaximumScale - MinimumScale)
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0x9b05688cU));
                patch.spreadScale = MinimumSpreadScale
                    + (MaximumSpreadScale - MinimumSpreadScale)
                        * UnitRandom(Hash(gridX, gridZ, seed ^ 0xbb67ae85U));
                if (pathScale) {
                    patch.scale *= 0.58f;
                }
                patch.windPhase = UnitRandom(Hash(gridX, gridZ, seed ^ 0x1f83d9abU));
                patch.colorVariation = UnitRandom(Hash(gridX, gridZ, seed ^ 0x5be0cd19U));
                // A smooth material field gives each island region an identity without changing
                // patch density, geometry, or draw count. Warm coastal turf transitions to cooler
                // jungle grass; the path and east clearing are sun-dried while the shaded west
                // clearing stays leaf-rich and cool.
                patch.biomeTint = 0.62f + (0.34f - 0.62f) * coastalBlend;
                if (pathScale && !VegetationLayout::IsInsideAuthoredClearing(x, z)) {
                    patch.biomeTint = std::max(patch.biomeTint, 0.70f);
                }
                const float eastInfluence = ClearingInfluence(
                    x, z, VegetationLayout::EastClearing);
                const float westInfluence = ClearingInfluence(
                    x, z, VegetationLayout::WestClearing);
                patch.biomeTint += (0.88f - patch.biomeTint) * eastInfluence;
                patch.biomeTint += (0.13f - patch.biomeTint) * westInfluence;
                patch.biomeTint = std::clamp(patch.biomeTint, 0.0f, 1.0f);
                patch.pathScale = pathScale;
                patches_.push_back(patch);

                // One companion fills the gap to the next site where cover is rich. It inherits
                // the parent's biome identity but has its own yaw, stature and wind phase.
                const float satelliteChance = SmoothStep(
                    (groundCover - SatelliteStart) / (1.0f - SatelliteStart)) * 0.92f;
                if (pathScale || UnitRandom(Hash(gridX, gridZ, seed ^ 0x6a09e667U)) >= satelliteChance) {
                    continue;
                }
                const float angle = UnitRandom(Hash(gridX, gridZ, seed ^ 0x3c6ef372U))
                    * 2.0f * std::numbers::pi_v<float>;
                const float offset = SatelliteMinimumOffset + (SatelliteMaximumOffset
                    - SatelliteMinimumOffset) * UnitRandom(Hash(gridX, gridZ, seed ^ 0xa54ff53bU));
                const float satelliteX = x + std::cos(angle) * offset;
                const float satelliteZ = z + std::sin(angle) * offset;
                TerrainSample satelliteSample;
                if (!accepted(satelliteX, satelliteZ, satelliteSample)
                    || VegetationLayout::IsReservedRoute(satelliteX, satelliteZ)) {
                    continue;
                }
                GrassPlacement satellite = patch;
                satellite.x = satelliteX;
                satellite.y = satelliteSample.height;
                satellite.z = satelliteZ;
                satellite.yawRadians = std::fmod(patch.yawRadians + 2.39996323f,
                                                 2.0f * std::numbers::pi_v<float>);
                satellite.scale = MinimumScale + (MaximumScale - MinimumScale)
                    * UnitRandom(Hash(gridX, gridZ, seed ^ 0x9b05688dU));
                satellite.windPhase = UnitRandom(Hash(gridX, gridZ, seed ^ 0x1f83d9acU));
                satellite.colorVariation = UnitRandom(Hash(gridX, gridZ, seed ^ 0x5be0cd1aU));
                patches_.push_back(satellite);
            }
        }
    }
}
