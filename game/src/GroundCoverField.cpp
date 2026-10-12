#include "IslandSimulator/GroundCoverField.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace island
{
    namespace
    {
        [[nodiscard]] float Smooth(const float a, const float b, const float x) noexcept
        {
            const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] std::uint32_t Hash(const std::int32_t x, const std::int32_t z,
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

        [[nodiscard]] float LatticeValue(const std::int32_t x, const std::int32_t z,
                                         const std::uint32_t seed) noexcept
        {
            return static_cast<float>(Hash(x, z, seed) & 0x00ffffffU)
                / static_cast<float>(0x01000000U);
        }

        // Value lookup is injected so the field bake can reuse cached corners; arithmetic is
        // identical to the direct reference, so cached and direct results agree bit for bit.
        template <typename Value>
        [[nodiscard]] float Lattice(const float x, const float z, const float cell,
                                    const Value& value) noexcept
        {
            const float gridX = x / cell, gridZ = z / cell;
            const auto x0 = static_cast<std::int32_t>(std::floor(gridX));
            const auto z0 = static_cast<std::int32_t>(std::floor(gridZ));
            const float tx = Smooth(0.0f, 1.0f, gridX - static_cast<float>(x0));
            const float tz = Smooth(0.0f, 1.0f, gridZ - static_cast<float>(z0));
            const float back = std::lerp(value(x0, z0), value(x0 + 1, z0), tx);
            const float front = std::lerp(value(x0, z0 + 1), value(x0 + 1, z0 + 1), tx);
            return std::lerp(back, front, tz);
        }

        struct OctaveCache
        {
            float cell;
            std::uint32_t seed;
            std::int32_t first, count;
            std::vector<float> values;

            OctaveCache(const float cellSize, const std::uint32_t octaveSeed)
                : cell(cellSize), seed(octaveSeed)
            {
                constexpr float H = CanopyLightField::HalfExtent;
                first = static_cast<std::int32_t>(std::floor(-H / cell)) - 1;
                count = static_cast<std::int32_t>(std::floor(H / cell)) + 3 - first;
                values.resize(static_cast<std::size_t>(count) * static_cast<std::size_t>(count));
                for (std::int32_t z = 0; z < count; ++z)
                    for (std::int32_t x = 0; x < count; ++x)
                        values[static_cast<std::size_t>(z * count + x)] = LatticeValue(first + x, first + z, seed);
            }

            float operator()(const std::int32_t x, const std::int32_t z) const noexcept
            {
                const std::int32_t ix = x - first, iz = z - first;
                if (ix < 0 || iz < 0 || ix >= count || iz >= count) return LatticeValue(x, z, seed);
                return values[static_cast<std::size_t>(iz * count + ix)];
            }
        };

        constexpr std::uint32_t MediumSeed = 0x1b873593U, FineSeed = 0x85ebca6bU;
    }

    float GroundCover::PatchFromNoise(const float noise)
    {
        if (!std::isfinite(noise) || noise < 0 || noise > 1)
            throw std::invalid_argument("Ground-cover noise must be finite and normalized");
        return Smooth(PatchLow, PatchHigh, noise);
    }

    float GroundCover::Density(const float transmission, const float patch,
                               const float shoreDistance, const float slopeRadians,
                               const float rockiness, const float beachSuitability,
                               const float moisture, const float drainage)
    {
        if (!std::isfinite(transmission) || !std::isfinite(patch) || !std::isfinite(shoreDistance)
            || !std::isfinite(slopeRadians) || transmission < 0 || transmission > 1
            || patch < 0 || patch > 1 || !std::isfinite(rockiness)
            || !std::isfinite(beachSuitability) || rockiness < 0 || rockiness > 1
            || beachSuitability < 0 || beachSuitability > 1 || !std::isfinite(moisture)
            || !std::isfinite(drainage) || moisture < 0 || moisture > 1
            || drainage < 0 || drainage > 1)
            throw std::invalid_argument("Invalid ground-cover input");
        const float inland = Smooth(ShoreStart, ShoreEnd, shoreDistance);
        const float slope = 1.0f - Smooth(SlopeStart, SlopeEnd, slopeRadians);
        const float light = LightFloor + (1.0f - LightFloor) * Smooth(LightLow, LightHigh, transmission);
        const float soil = (1.0f - rockiness)
            * (1.0f - Smooth(BeachSuppressionStart, BeachSuppressionEnd, beachSuitability));
        const float wetBiome = (1.0f + 0.10f * moisture) * (1.0f - 0.82f * drainage);
        return std::clamp(inland * slope * light * (PatchFloor + (1.0f - PatchFloor) * patch)
            * soil * wetBiome, 0.0f, 1.0f);
    }

    std::string GroundCover::Shader()
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10)
            << "const vec4 groundCoverLight=vec4(" << LightFloor << ',' << LightLow << ','
            << LightHigh << ',' << PatchFloor << ");\n"
            << "const vec4 groundCoverTerrain=vec4(" << ShoreStart << ',' << ShoreEnd << ','
            << SlopeStart << ',' << SlopeEnd << ");\n"
            << "const vec2 groundCoverBeach=vec2(" << BeachSuppressionStart << ','
            << BeachSuppressionEnd << ");\n";
        // "patch" is reserved in GLSL ES 3.00: ANGLE rejects it even though Mesa's GLSL 330 does not.
        out << R"(
float groundCoverDensity(float transmission,float patchValue,float shoreDistance,float slopeRadians,
                         float rockiness,float beachSuitability,float moisture,float drainage) {
    float inland=smoothstep(groundCoverTerrain.x,groundCoverTerrain.y,shoreDistance);
    float slope=1.0-smoothstep(groundCoverTerrain.z,groundCoverTerrain.w,slopeRadians);
    float light=groundCoverLight.x+(1.0-groundCoverLight.x)
        *smoothstep(groundCoverLight.y,groundCoverLight.z,transmission);
    float soil=(1.0-rockiness)
        *(1.0-smoothstep(groundCoverBeach.x,groundCoverBeach.y,beachSuitability));
    float wetBiome=(1.0+0.10*moisture)*(1.0-0.82*drainage);
    return clamp(inland*slope*light*(groundCoverLight.w+(1.0-groundCoverLight.w)*patchValue)
        *soil*wetBiome,0.0,1.0);
}
)";
        return out.str();
    }

    float GroundCoverField::Noise(const float x, const float z, const std::uint32_t seed) noexcept
    {
        const auto direct = [](const std::uint32_t octaveSeed) {
            return [octaveSeed](const std::int32_t cx, const std::int32_t cz) {
                return LatticeValue(cx, cz, octaveSeed);
            };
        };
        return 0.52f * Lattice(x, z, BroadCell, direct(seed))
            + 0.31f * Lattice(x, z, MediumCell, direct(seed ^ MediumSeed))
            + 0.17f * Lattice(x, z, FineCell, direct(seed ^ FineSeed));
    }

    GroundCoverField::GroundCoverField(const int resolution, const std::uint32_t seed)
        : resolution_(resolution)
    {
        if (resolution < 16 || resolution > CanopyLightField::DefaultResolution
            || (resolution & (resolution - 1))) {
            throw std::invalid_argument("Ground-cover resolution must be a power of two from 16 to 1024");
        }
        texels_.resize(static_cast<std::size_t>(resolution) * resolution);
        const OctaveCache broad(BroadCell, seed), medium(MediumCell, seed ^ MediumSeed),
            fine(FineCell, seed ^ FineSeed);
        for (int row = 0; row < resolution; ++row) {
            const float z = CanopyLightField::TexelCoordinate(row, resolution);
            for (int column = 0; column < resolution; ++column) {
                const float x = CanopyLightField::TexelCoordinate(column, resolution);
                const float noise = 0.52f * Lattice(x, z, BroadCell, broad)
                    + 0.31f * Lattice(x, z, MediumCell, medium)
                    + 0.17f * Lattice(x, z, FineCell, fine);
                const float patch = Smooth(GroundCover::PatchLow, GroundCover::PatchHigh, noise);
                texels_[static_cast<std::size_t>(row) * resolution + column]
                    = static_cast<std::uint8_t>(std::lround(255.0f * patch));
            }
        }
    }

    float GroundCoverField::SamplePatch(const float x, const float z) const noexcept
    {
        constexpr float H = CanopyLightField::HalfExtent;
        if (!std::isfinite(x) || !std::isfinite(z) || std::abs(x) > H || std::abs(z) > H) return 0.0f;
        const float column = std::clamp((x + H) * resolution_ / (2 * H) - 0.5f,
                                        0.0f, static_cast<float>(resolution_ - 1));
        const float row = std::clamp((z + H) * resolution_ / (2 * H) - 0.5f,
                                     0.0f, static_cast<float>(resolution_ - 1));
        const int ix = static_cast<int>(column), iz = static_cast<int>(row);
        const int nx = std::min(ix + 1, resolution_ - 1), nz = std::min(iz + 1, resolution_ - 1);
        const auto value = [&](const int cx, const int cz) {
            return texels_[static_cast<std::size_t>(cz) * resolution_ + cx] / 255.0f;
        };
        return std::lerp(std::lerp(value(ix, iz), value(nx, iz), column - ix),
                         std::lerp(value(ix, nz), value(nx, nz), column - ix), row - iz);
    }
}
