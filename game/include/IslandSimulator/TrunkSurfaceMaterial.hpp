#pragma once

#include "IslandSimulator/BarkMaterial.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace island::TrunkSurface
{
    // Soil, litter and low plants hide part of the sky from the lowest bark.
    inline constexpr float ContactFloor = 0.64f, ContactHeight = 0.90f;
    // Damp lower bark: soil splash plus algae/moss, with a noise-broken upper edge.
    inline constexpr float StainBase = 0.70f, StainRange = 1.10f, StainSoftness = 0.30f;
    inline constexpr float MossStrength = 0.62f;
    // Pale crustose lichen patches on the mid trunk, never at the soil-stained base.
    inline constexpr float LichenStart = 0.66f, LichenEnd = 0.78f, LichenStrength = 0.46f;
    inline constexpr float LichenLow = 1.40f, LichenHigh = 7.50f;
    // Numerical mean of smoothstep(LichenStart, LichenEnd, barkNoise) over its lattice.
    inline constexpr float LichenMean = 0.1797f;
    inline constexpr std::array<float, 3> MossAlbedo{0.250f, 0.296f, 0.148f};
    inline constexpr std::array<float, 3> LichenAlbedo{0.600f, 0.612f, 0.566f};

    inline float ContactOcclusion(float aboveGround)
    {
        if (!std::isfinite(aboveGround)) throw std::invalid_argument("Trunk height must be finite");
        return std::lerp(ContactFloor, 1.0f, JungleMaterial::Smooth(0.0f, ContactHeight, aboveGround));
    }

    // edgeNoise is the [0,1] lattice value; unresolved callers pass its 0.5 mean.
    inline float StainWeight(float aboveGround, float edgeNoise)
    {
        if (!std::isfinite(aboveGround) || !std::isfinite(edgeNoise) || edgeNoise < 0 || edgeNoise > 1)
            throw std::invalid_argument("Invalid trunk stain input");
        const float edge = StainBase + StainRange * edgeNoise;
        return 1.0f - JungleMaterial::Smooth(edge - StainSoftness, edge + StainSoftness, aboveGround);
    }

    inline float LichenWindow(float aboveGround)
    {
        if (!std::isfinite(aboveGround)) throw std::invalid_argument("Trunk height must be finite");
        return JungleMaterial::Smooth(LichenLow - 0.5f, LichenLow + 0.5f, aboveGround)
            * (1.0f - JungleMaterial::Smooth(LichenHigh - 1.5f, LichenHigh, aboveGround));
    }

    /** @brief CPU reference for the shader's albedo; all noise inputs are already filtered. */
    inline std::array<float, 3> Albedo(std::array<float, 3> bark, float aboveGround,
                                       float edgeNoise, float mossNoise, float lichenCover)
    {
        for (float value : bark)
            if (!std::isfinite(value) || value < 0 || value > 1) throw std::invalid_argument("Invalid bark albedo");
        if (!std::isfinite(mossNoise) || mossNoise < 0 || mossNoise > 1
            || !std::isfinite(lichenCover) || lichenCover < 0 || lichenCover > 1)
            throw std::invalid_argument("Invalid trunk surface noise");
        const float moss = StainWeight(aboveGround, edgeNoise) * MossStrength * (0.55f + 0.45f * mossNoise);
        const float lichen = lichenCover * LichenWindow(aboveGround) * LichenStrength;
        for (std::size_t i = 0; i < 3; ++i) {
            bark[i] = std::lerp(bark[i], MossAlbedo[i], moss);
            bark[i] = std::lerp(bark[i], LichenAlbedo[i], lichen);
        }
        return bark;
    }

    // GLSL 330 / ES 300. Requires JungleMaterial and BarkMaterial shader text first.
    inline std::string Shader()
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10)
            << "const vec2 trunkContactTerms=vec2(" << ContactFloor << ',' << ContactHeight << ");\n"
            << "const vec4 trunkStainTerms=vec4(" << StainBase << ',' << StainRange << ','
            << StainSoftness << ',' << MossStrength << ");\n"
            << "const vec4 trunkLichenTerms=vec4(" << LichenStart << ',' << LichenEnd << ','
            << LichenStrength << ',' << LichenMean << ");\n"
            << "const vec2 trunkLichenHeights=vec2(" << LichenLow << ',' << LichenHigh << ");\n"
            << "const vec3 trunkMossAlbedo=vec3(" << MossAlbedo[0] << ',' << MossAlbedo[1] << ','
            << MossAlbedo[2] << ");\n"
            << "const vec3 trunkLichenAlbedo=vec3(" << LichenAlbedo[0] << ',' << LichenAlbedo[1] << ','
            << LichenAlbedo[2] << ");\n";
        out << R"(
float trunkContactOcclusion(float aboveGround) {
    return mix(trunkContactTerms.x,1.0,smoothstep(0.0,trunkContactTerms.y,aboveGround));
}
// woodCoordinate: angle, local axial metres, radians/pixel, axial metres/pixel.
vec3 trunkSurfaceAlbedo(vec3 bark,float aboveGround,vec4 woodCoordinate,uint seed) {
    float angle=woodCoordinate.x, along=woodCoordinate.y;
    float height=max(aboveGround,0.0);
    float edgeWeight=materialDetailWeight(woodCoordinate.z*(6.0/6.28318530718)+woodCoordinate.w*0.8);
    float edgeNoise=mix(0.5,barkNoise(angle,height*0.8,6,seed^517u),edgeWeight);
    float edge=trunkStainTerms.x+trunkStainTerms.y*edgeNoise;
    float stain=1.0-smoothstep(edge-trunkStainTerms.z,edge+trunkStainTerms.z,height);
    float mossWeight=materialDetailWeight(woodCoordinate.z*(11.0/6.28318530718)+woodCoordinate.w*1.6);
    float mossNoise=mix(0.5,barkNoise(angle,height*1.6,11,seed^753u),mossWeight);
    float moss=stain*trunkStainTerms.w*(0.55+0.45*mossNoise);
    float lichenWeight=materialDetailWeight(woodCoordinate.z*(9.0/6.28318530718)+woodCoordinate.w*0.7);
    float lichenCover=mix(trunkLichenTerms.w,
        smoothstep(trunkLichenTerms.x,trunkLichenTerms.y,barkNoise(angle,along*0.7,9,seed^1201u)),
        lichenWeight);
    float lichenWindow=smoothstep(trunkLichenHeights.x-0.5,trunkLichenHeights.x+0.5,height)
        *(1.0-smoothstep(trunkLichenHeights.y-1.5,trunkLichenHeights.y,height));
    vec3 albedo=mix(bark,trunkMossAlbedo,moss);
    return mix(albedo,trunkLichenAlbedo,lichenCover*lichenWindow*trunkLichenTerms.z);
}
)";
        return out.str();
    }
}
