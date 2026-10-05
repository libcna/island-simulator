#include "IslandSimulator/VegetationLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float CandidateSpacing = 14.0f;
        constexpr int CandidateRadius = 19;

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
        return -5.0f + 21.0f * std::sin((z + 34.0f) * 0.018f);
    }

    bool VegetationLayout::IsReservedRoute(const float x, const float z) noexcept
    {
        // A broad south-to-north walking line makes dense vegetation readable and keeps the
        // initial beach approach open. Its gentle bend prevents it reading as a road.
        const float pathX = PrimaryPathCenterX(z);
        if (z > -225.0f && z < 190.0f && std::abs(x - pathX) < 6.2f) {
            return true;
        }

        // Two intentional rooms in the vegetation and a safe uncluttered player spawn.
        return DistanceSquared(x, z, 43.0f, -18.0f) < 32.0f * 32.0f
            || DistanceSquared(x, z, -82.0f, 62.0f) < 24.0f * 24.0f
            || DistanceSquared(x, z, 0.0f, -225.0f) < 17.0f * 17.0f;
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

                // Palms form a broad coastal belt, with sparser groups toward the interior. The
                // eventual broadleaf canopy will own the deepest central jungle.
                if (sample.signedShoreDistance < 7.0f || sample.signedShoreDistance > 158.0f
                    || sample.height < 0.55f || sample.slopeRadians > 0.4712389f
                    || IsReservedRoute(x, z)) {
                    continue;
                }

                const float coastal = 1.0f - SmoothStep(
                    (sample.signedShoreDistance - 38.0f) / 112.0f);
                const float cluster = ClusterNoise(x, z, seed ^ 0x31415926U);
                const float density = std::clamp((0.24f + 0.43f * coastal)
                                                 * (0.42f + 0.88f * cluster),
                                                 0.0f, 0.90f);
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
                palm.variant = static_cast<std::uint8_t>(
                    Hash(gridX, gridZ, seed ^ 0xf17a245bU) % PalmVariantCount);
                palms_.push_back(palm);
                obstacles_.push_back(CircularObstacle{palm.x, palm.z, 0.31f * palm.scale});
            }
        }
    }
}
