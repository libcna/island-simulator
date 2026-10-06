#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cassert>
#include <bit>
#include <cmath>
#include <limits>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float DeepSeabedHeight = -22.0f;

        [[nodiscard]] bool IsSupportedCoordinate(const float x, const float z) noexcept
        {
            return std::isfinite(x) && std::isfinite(z)
                && std::fabs(x) <= IslandTerrain::MaximumQueryCoordinate
                && std::fabs(z) <= IslandTerrain::MaximumQueryCoordinate;
        }

        [[nodiscard]] TerrainSample OutOfDomainSample() noexcept
        {
            return TerrainSample{
                .height = DeepSeabedHeight,
                .signedShoreDistance = -IslandTerrain::MaximumQueryCoordinate,
                .normal = TerrainNormal{0.0f, 1.0f, 0.0f},
                .slopeRadians = 0.0f,
                .surface = TerrainSurface::DeepSeabed};
        }

        [[nodiscard]] constexpr float SmoothStep(const float value) noexcept
        {
            const float clamped = std::clamp(value, 0.0f, 1.0f);
            return clamped * clamped * (3.0f - 2.0f * clamped);
        }

        [[nodiscard]] constexpr float Lerp(const float a, const float b, const float amount) noexcept
        {
            return a + (b - a) * amount;
        }
    }

    std::uint32_t IslandTerrain::Hash(const std::int32_t x, const std::int32_t z,
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

    float IslandTerrain::ValueNoise(const float x, const float z, const std::uint32_t seed) noexcept
    {
        assert(std::isfinite(x) && std::isfinite(z));
#ifndef NDEBUG
        constexpr float MaximumNoiseCoordinate =
            static_cast<float>(std::numeric_limits<std::int32_t>::max() - 2);
        assert(std::fabs(x) <= MaximumNoiseCoordinate
               && std::fabs(z) <= MaximumNoiseCoordinate);
#endif
        const auto x0 = static_cast<std::int32_t>(std::floor(x));
        const auto z0 = static_cast<std::int32_t>(std::floor(z));
        const auto unit = [seed](const std::int32_t cellX, const std::int32_t cellZ) {
            constexpr float inverse = 1.0f / static_cast<float>(0x00ffffffU);
            return static_cast<float>(Hash(cellX, cellZ, seed) & 0x00ffffffU) * inverse * 2.0f
                - 1.0f;
        };

        const float tx = SmoothStep(x - static_cast<float>(x0));
        const float tz = SmoothStep(z - static_cast<float>(z0));
        const float top = Lerp(unit(x0, z0), unit(x0 + 1, z0), tx);
        const float bottom = Lerp(unit(x0, z0 + 1), unit(x0 + 1, z0 + 1), tx);
        return Lerp(top, bottom, tz);
    }

    float IslandTerrain::FractalNoise(float x, float z, const std::uint32_t seed) noexcept
    {
        float value = 0.0f;
        float amplitude = 0.58f;
        float amplitudeSum = 0.0f;
        for (std::uint32_t octave = 0; octave < 4U; ++octave) {
            value += ValueNoise(x, z, seed + octave * 101U) * amplitude;
            amplitudeSum += amplitude;
            x *= 2.03f;
            z *= 2.03f;
            amplitude *= 0.48f;
        }
        return value / amplitudeSum;
    }

    float IslandTerrain::ShoreRadius(const float angleRadians) const noexcept
    {
        const float finiteAngle = std::isfinite(angleRadians) ? angleRadians : 0.0f;
        const float safeAngle = std::fabs(finiteAngle) <= std::numbers::pi_v<float>
            ? finiteAngle
            : std::remainder(finiteAngle, 2.0f * std::numbers::pi_v<float>);
        const float broad = 12.0f * std::sin(safeAngle * 3.0f + 0.55f)
            + 8.0f * std::sin(safeAngle * 5.0f - 1.3f)
            + 4.5f * std::sin(safeAngle * 9.0f + 2.1f);
        const float directionX = std::cos(safeAngle);
        const float directionZ = std::sin(safeAngle);
        const float irregular = 9.0f * FractalNoise(directionX * 2.4f + 3.7f,
                                                    directionZ * 2.4f - 1.9f, 0x51a7U);
        return NominalDiameter * 0.46f + broad + irregular;
    }

    float IslandTerrain::SignedShoreDistance(const float x, const float z) const noexcept
    {
        if (!IsSupportedCoordinate(x, z)) {
            return -MaximumQueryCoordinate;
        }
        const float radius = std::hypot(x, z);
        const float angle = radius > 0.0001f ? std::atan2(z, x) : 0.0f;
        return ShoreRadius(angle) - radius;
    }

    float IslandTerrain::Height(const float x, const float z) const noexcept
    {
        if (!IsSupportedCoordinate(x, z)) {
            return DeepSeabedHeight;
        }
        const float inland = SignedShoreDistance(x, z);
        if (inland <= 0.0f) {
            const float offshore = -inland;
            const float shelf = -0.035f * offshore - 0.00012f * offshore * offshore;
            const float seabedDetail = 0.35f * FractalNoise(x * 0.025f, z * 0.025f, 0xb34cU);
            return std::max(DeepSeabedHeight,
                            shelf + seabedDetail * SmoothStep(offshore / 14.0f));
        }

        const float beach = std::min(inland, 18.0f) * 0.095f;
        const float interiorBlend = SmoothStep((inland - 11.0f) / 75.0f);
        const float broadHills = 8.5f + 7.0f * FractalNoise(x * 0.009f, z * 0.009f, 0xc0a5U);
        const float localRelief = 2.2f * FractalNoise(x * 0.027f, z * 0.027f, 0x711dU);
        const float offsetX = x - 38.0f;
        const float offsetZ = z + 22.0f;
        const float viewpointHill = 8.0f * std::exp(-(offsetX * offsetX + offsetZ * offsetZ)
                                                    / (2.0f * 78.0f * 78.0f));
        return beach + interiorBlend * (broadHills + localRelief + viewpointHill);
    }

    TerrainSample IslandTerrain::Sample(const float x, const float z) const noexcept
    {
        if (!IsSupportedCoordinate(x, z)) {
            return OutOfDomainSample();
        }
        TerrainSample sample;
        sample.height = Height(x, z);
        sample.signedShoreDistance = SignedShoreDistance(x, z);

        const float left = Height(x - SurfaceSampleDistance, z);
        const float right = Height(x + SurfaceSampleDistance, z);
        const float back = Height(x, z - SurfaceSampleDistance);
        const float front = Height(x, z + SurfaceSampleDistance);
        const float normalX = left - right;
        const float normalY = 2.0f * SurfaceSampleDistance;
        const float normalZ = back - front;
        const float inverseLength = 1.0f / std::hypot(normalY, std::hypot(normalX, normalZ));
        sample.normal = TerrainNormal{normalX * inverseLength, normalY * inverseLength,
                                     normalZ * inverseLength};
        sample.slopeRadians = std::acos(std::clamp(sample.normal.y, 0.0f, 1.0f));

        constexpr float steepSlope = 0.55850536f; // 32 degrees
        if (sample.height < -4.0f) {
            sample.surface = TerrainSurface::DeepSeabed;
        } else if (sample.height < -0.12f) {
            sample.surface = TerrainSurface::ShallowSeabed;
        } else if (sample.height <= 0.22f || sample.signedShoreDistance < 2.5f) {
            sample.surface = TerrainSurface::WetSand;
        } else if (sample.signedShoreDistance < 18.0f || sample.height < 1.8f) {
            sample.surface = TerrainSurface::DrySand;
        } else if (sample.slopeRadians > steepSlope) {
            sample.surface = TerrainSurface::SteepGround;
        } else if (sample.signedShoreDistance < 48.0f) {
            sample.surface = TerrainSurface::CoastalGround;
        } else {
            sample.surface = TerrainSurface::InteriorGround;
        }
        return sample;
    }

    bool IslandTerrain::IsLand(const float x, const float z) const noexcept
    {
        return Height(x, z) > 0.0f;
    }
}
