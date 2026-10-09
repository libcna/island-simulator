#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>

namespace island
{
    inline constexpr float GrassBasalEnd = 0.24f;
    inline constexpr std::array<float, 3> GrassBasalShift{0.22f, -0.13f, -0.21f};
    inline constexpr float GrassRootOcclusion = 0.62f;
    inline constexpr float GrassGrainCycles = 4.5f; // Nine veins across a complete blade.
    inline constexpr float GrassGrainStrength = 0.045f;
    inline constexpr float GrassMidribStrength = 0.06f;
    inline constexpr float GrassEdgeStrength = 0.025f;

    [[nodiscard]] inline float GrassSmoothStep(float start, float end, float value)
    {
        const float t = std::clamp((value - start) / (end - start), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Occlusion is separate from pigment and directional light. Short turf uses its own
    // normalized height, not the smaller wind weight of a short blade.
    [[nodiscard]] inline float GrassHeightOcclusion(float height)
    {
        if (!std::isfinite(height) || height < 0 || height > 1) {
            throw std::invalid_argument("Grass occlusion requires a normalized blade height");
        }
        return std::lerp(GrassRootOcclusion, 1.0f, GrassSmoothStep(0, 1, height));
    }

    // CPU reference for the fragment shader. Footprint is fwidth of the interpolated
    // signed across-blade coordinate; callers exclude seed-head geometry.
    [[nodiscard]] inline float GrassSurfaceResponse(float across, float phase, float footprint)
    {
        if (!std::isfinite(across) || std::abs(across) > 1
            || !std::isfinite(phase) || phase < 0 || phase > 1
            || !std::isfinite(footprint) || footprint < 0) {
            throw std::invalid_argument("Invalid grass surface coordinate or footprint");
        }
        const float resolved = 1 - GrassSmoothStep(0.12f, 0.5f, footprint);
        const float veinResolved = 1 - GrassSmoothStep(0.2f, 0.5f,
                                                       footprint * GrassGrainCycles);
        const float tau = 2 * std::numbers::pi_v<float>;
        const float veins = std::cos((across * GrassGrainCycles + phase) * tau);
        const float midrib = 1 - GrassSmoothStep(0.015f, 0.13f, std::abs(across));
        const float edge = GrassSmoothStep(0.7f, 1, std::abs(across));
        return 1 + veins * GrassGrainStrength * veinResolved
            + (midrib * GrassMidribStrength - edge * GrassEdgeStrength) * resolved;
    }
    /** @brief Restrained basal pigment; independent of distance, LOD and wind amplitude.
     * Pass each blade's normalized geometric height, including short turf. Upper leaf and seed
     * colours remain unchanged. This is material colour, not a replacement for soil or shadows.
     */
    [[nodiscard]] inline std::array<float, 3> GrassBasalTint(float bladeHeight)
    {
        if (!std::isfinite(bladeHeight) || bladeHeight < 0 || bladeHeight > 1) {
            throw std::invalid_argument("Grass basal tint requires a normalized blade height");
        }
        const float t = std::clamp(bladeHeight / GrassBasalEnd, 0.0f, 1.0f);
        const float weight = 1.0f - t * t * (3.0f - 2.0f * t);
        return {1.0f + GrassBasalShift[0] * weight, 1.0f + GrassBasalShift[1] * weight,
                1.0f + GrassBasalShift[2] * weight};
    }

    // Custom grass explicitly outputs opaque alpha; its otherwise unused vertex-alpha byte
    // carries geometric height. Stock-effect geometry retains its original vertex colours.
    [[nodiscard]] inline std::uint8_t EncodeGrassMaterialHeight(float bladeHeight)
    {
        (void) GrassBasalTint(bladeHeight); // Enforce the same finite normalized domain.
        return static_cast<std::uint8_t>(std::lround(bladeHeight * 255.0f));
    }

    [[nodiscard]] inline std::string GrassMaterialShaderConstants()
    {
        std::ostringstream source;
        source.imbue(std::locale::classic());
        source << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10)
               << "const float grassBasalEnd = " << GrassBasalEnd << ";\n"
               << "const vec3 grassBasalShift = vec3(" << GrassBasalShift[0] << ", "
               << GrassBasalShift[1] << ", " << GrassBasalShift[2] << ");\n"
               << "const float grassRootOcclusion = " << GrassRootOcclusion << ";\n"
               << "const float grassGrainCycles = " << GrassGrainCycles << ";\n"
               << "const float grassGrainStrength = " << GrassGrainStrength << ";\n"
               << "const float grassMidribStrength = " << GrassMidribStrength << ";\n"
               << "const float grassEdgeStrength = " << GrassEdgeStrength << ";\n";
        return source.str();
    }
}
