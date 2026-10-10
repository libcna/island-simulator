#pragma once

#include "IslandSimulator/CanopyLightField.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace island
{
    /**
     * @brief Shared ground-cover response for grass placement and the terrain turf undercoat.
     *
     * Cover combines direct canopy transmission, a deterministic patch field, shore distance
     * and slope. Grass density and the terrain's green undercoat both use exactly this formula,
     * so dense swards sit on turf and shaded or patch-poor ground keeps exposed humus and litter.
     */
    namespace GroundCover
    {
        inline constexpr float PatchLow = 0.34f, PatchHigh = 0.66f, PatchFloor = 0.10f;
        inline constexpr float LightFloor = 0.18f, LightLow = 0.12f, LightHigh = 0.80f;
        inline constexpr float ShoreStart = 18.0f, ShoreEnd = 34.0f;
        inline constexpr float SlopeStart = 0.30f, SlopeEnd = 0.49f;

        /** @brief Contrast-shaped [0,1] patch value from the raw multi-octave noise. */
        [[nodiscard]] float PatchFromNoise(float noise);
        /** @brief Cover in [0,1]; throws on non-finite input or out-of-range light/patch. */
        [[nodiscard]] float Density(float transmission, float patch,
                                    float shoreDistance, float slopeRadians);
        /** @brief GLSL 330 / ES 300 groundCoverDensity() with the same constants. */
        [[nodiscard]] std::string Shader();
    }

    /**
     * @brief CPU-only deterministic patch field on the canopy-light texel grid.
     *
     * Texels hold the contrast-shaped patch value (255 = richest cover potential). The field is
     * independent of terrain and light; consumers combine it through GroundCover::Density. It is
     * uploaded in the otherwise unused green channel of the canopy-light texture.
     */
    class GroundCoverField final
    {
    public:
        static constexpr std::uint32_t DefaultSeed = 0x2b7e1516U;
        static constexpr float BroadCell = 38.0f, MediumCell = 13.0f, FineCell = 5.5f;

        explicit GroundCoverField(int resolution = CanopyLightField::DefaultResolution,
                                  std::uint32_t seed = DefaultSeed);
        [[nodiscard]] int Resolution() const noexcept { return resolution_; }
        [[nodiscard]] std::span<const std::uint8_t> PatchTexels() const noexcept { return texels_; }
        /** @brief Bilinear sample with CanopyLightField's texel-centre convention; 0 outside. */
        [[nodiscard]] float SamplePatch(float x, float z) const noexcept;
        /** @brief Unquantized [0,1] multi-octave value noise before contrast shaping. */
        [[nodiscard]] static float Noise(float x, float z, std::uint32_t seed) noexcept;

    private:
        int resolution_;
        std::vector<std::uint8_t> texels_;
    };
}
