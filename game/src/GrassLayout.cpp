#include "IslandSimulator/GrassLayout.hpp"

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

    float GrassLayout::ClusterNoise(const float x, const float z,
                                    const std::uint32_t seed) noexcept
    {
        constexpr float cellSize = 31.0f;
        const float gridX = x / cellSize;
        const float gridZ = z / cellSize;
        const auto x0 = static_cast<std::int32_t>(std::floor(gridX));
        const auto z0 = static_cast<std::int32_t>(std::floor(gridZ));
        const float tx = SmoothStep(gridX - static_cast<float>(x0));
        const float tz = SmoothStep(gridZ - static_cast<float>(z0));
        const auto value = [seed](const std::int32_t cellX, const std::int32_t cellZ) {
            return UnitRandom(Hash(cellX, cellZ, seed));
        };
        const float back = value(x0, z0) + (value(x0 + 1, z0) - value(x0, z0)) * tx;
        const float front = value(x0, z0 + 1)
            + (value(x0 + 1, z0 + 1) - value(x0, z0 + 1)) * tx;
        return back + (front - back) * tz;
    }

    GrassLayout::GrassLayout(const IslandTerrain& terrain, const std::uint32_t seed)
    {
        patches_.reserve(28000U);
        for (int gridZ = -CandidateRadius; gridZ <= CandidateRadius; ++gridZ) {
            for (int gridX = -CandidateRadius; gridX <= CandidateRadius; ++gridX) {
                const std::uint32_t siteHash = Hash(gridX, gridZ, seed);
                const float jitterX = (UnitRandom(siteHash) - 0.5f) * CandidateSpacing * 0.78f;
                const float jitterZ = (UnitRandom(Hash(gridX, gridZ, seed ^ 0x91e10da5U)) - 0.5f)
                    * CandidateSpacing * 0.78f;
                const float x = static_cast<float>(gridX) * CandidateSpacing + jitterX;
                const float z = static_cast<float>(gridZ) * CandidateSpacing + jitterZ;
                const TerrainSample sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 20.0f || sample.height < 2.1f
                    || sample.slopeRadians > 0.4886922f) {
                    continue;
                }

                const bool pathScale = VegetationLayout::IsReservedRoute(x, z);
                const float coastalBlend = SmoothStep((sample.signedShoreDistance - 20.0f) / 42.0f);
                const float interiorReduction = 1.0f
                    - 0.31f * SmoothStep((sample.signedShoreDistance - 92.0f) / 72.0f);
                const float cluster = ClusterNoise(x, z, seed ^ 0x3c6ef372U);
                float density = (0.48f + 0.34f * coastalBlend) * interiorReduction
                    * (0.67f + 0.45f * cluster);
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
            }
        }
    }
}
