#pragma once

#include "IslandSimulator/StreamRoute.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    struct CoastalEnvironmentSample
    {
        float rockiness = 0.0f;
        float beachSuitability = 0.0f;
    };

    struct InlandEnvironmentSample
    {
        float moisture = 0.0f;
        float drainage = 0.0f;
        float saltExposure = 0.0f;
    };

    struct CoastalExperienceSample
    {
        float broadBeach = 0.0f;
        float shelteredBay = 0.0f;
        float rockyShore = 0.0f;
        float mangroveMouth = 0.0f;
    };

    /** @brief Shared coastal and first inland abiotic controls; canopy light remains separate. */
    class IslandEnvironmentField final
    {
    public:
        [[nodiscard]] static float Smooth(float low, float high, float value) noexcept
        {
            const float t = std::clamp((value - low) / (high - low), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        /** @brief Height-independent geology: safe to use while constructing terrain. */
        [[nodiscard]] static float CoastalRockPotential(float angle) noexcept
        {
            if (!std::isfinite(angle)) return 0.0f;
            const auto lobe = [angle](float centre, float width) {
                return std::exp((std::cos(angle - centre) - 1.0f) / (width * width));
            };
            return std::clamp(0.98f * lobe(0.35f, 0.23f)
                + 0.90f * lobe(2.48f, 0.20f) + 0.82f * lobe(-2.48f, 0.20f), 0.0f, 1.0f);
        }

        /** @brief Broad sheltered bay apron, ordinary beach elsewhere, narrow rocky margin. */
        [[nodiscard]] static float BeachWidth(float angle) noexcept
        {
            if (!std::isfinite(angle)) return 18.0f;
            const float shelteredBay = std::exp(-(1.0f + std::cos(angle)) / (0.45f * 0.45f));
            return (18.0f + 18.0f * shelteredBay)
                * (1.0f - 0.65f * CoastalRockPotential(angle));
        }

        /** @brief Finite normalized coastal material/placement controls; no terrain recursion. */
        [[nodiscard]] static CoastalEnvironmentSample SampleCoast(
            float x, float z, float signedShoreDistance) noexcept
        {
            if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(signedShoreDistance))
                return {};
            const float angle = std::atan2(z, x);
            const float regional = CoastalRockPotential(angle);
            const float beachWidth = BeachWidth(angle);
            const float band = Smooth(-8.0f, -1.0f, signedShoreDistance)
                * (1.0f - Smooth(25.0f, 65.0f, signedShoreDistance));
            return {regional * band, (1.0f - regional)
                * (1.0f - Smooth(beachWidth * 0.40f, beachWidth * 1.05f, signedShoreDistance))};
        }

        /** @brief Four overlapping but locally dominant identities for coastal composition. */
        [[nodiscard]] static CoastalExperienceSample SampleCoastalExperience(
            float x, float z, float signedShoreDistance) noexcept
        {
            if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(signedShoreDistance))
                return {};
            const float angle = std::atan2(z, x);
            const auto direction = [angle](float centre, float width) {
                return std::exp((std::cos(angle-centre)-1.0f)/(width*width));
            };
            const float coastalBand = Smooth(-6.0f, 1.0f, signedShoreDistance)
                * (1.0f-Smooth(42.0f, 68.0f, signedShoreDistance));
            const auto coast = SampleCoast(x,z,signedShoreDistance);
            const float mouthX = StreamRoute::SourceX-StreamRoute::NominalLength;
            const float mouthZ = StreamRoute::CentreZ(mouthX);
            const float mouth = coastalBand
                * (1.0f-Smooth(8.0f, 28.0f,std::hypot(x-mouthX,z-mouthZ)));
            const float sand = 1.0f-coast.rockiness;
            return {
                coastalBand*sand*direction(-1.57079632679f,.48f),
                coastalBand*sand*direction(3.14159265359f,.45f)*(1.0f-.72f*mouth),
                coast.rockiness,
                mouth,
            };
        }

        /** @brief Shared valley/stream moisture, drainage concentration and coastal salt field. */
        [[nodiscard]] static InlandEnvironmentSample SampleInland(
            float x, float z, float signedShoreDistance) noexcept
        {
            if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(signedShoreDistance))
                return {};
            const float along = StreamRoute::Along(x);
            const float streamRange = Smooth(0.0f, 5.0f, along)
                * (1.0f - Smooth(98.0f, 104.0f, along));
            const float streamAcross = std::abs(z - StreamRoute::CentreZ(x));
            const float t = std::clamp(along / 98.0f, 0.0f, 1.0f);
            const float shaped = t * t * (3.0f - 2.0f * t);
            const float halfWidth = std::lerp(0.45f, 1.5f, shaped);
            const float streamDrainage = streamRange
                * (1.0f - Smooth(halfWidth + 0.35f, halfWidth + 8.0f, streamAcross));
            const float streamMoisture = streamRange
                * (1.0f - Smooth(2.0f, 25.0f, streamAcross));

            const float southCentre = 5.0f + 12.0f * std::sin(z * 0.018f);
            const float southWidth = 12.0f + 12.0f * Smooth(0.0f, 180.0f, -z);
            const float southValley = (1.0f - Smooth(southWidth * 0.35f, southWidth,
                std::abs(x - southCentre))) * (1.0f - Smooth(5.0f, 70.0f, z));
            const auto coast = SampleCoast(x, z, signedShoreDistance);
            const float salt = (coast.rockiness * 0.78f + coast.beachSuitability * 0.42f)
                * (1.0f - Smooth(18.0f, 58.0f, signedShoreDistance));
            return {std::clamp(std::max(streamMoisture, southValley * 0.42f), 0.0f, 1.0f),
                std::clamp(std::max(streamDrainage, southValley * 0.28f), 0.0f, 1.0f),
                std::clamp(salt, 0.0f, 1.0f)};
        }

        /** @brief Bounded shallow-water habitat potential shared by reef props and fauna. */
        [[nodiscard]] static float ReefSuitability(float x, float z,
                                                   float signedShoreDistance,
                                                   float terrainHeight) noexcept
        {
            if (!std::isfinite(x) || !std::isfinite(z)
                || !std::isfinite(signedShoreDistance) || !std::isfinite(terrainHeight)) {
                return 0.0f;
            }
            const float offshore = -signedShoreDistance;
            const float depth = -terrainHeight;
            const float band = Smooth(13.0f, 25.0f, offshore)
                * (1.0f - Smooth(82.0f, 96.0f, offshore))
                * Smooth(0.35f, 0.90f, depth)
                * (1.0f - Smooth(5.2f, 7.5f, depth));
            const float angle = std::atan2(z, x);
            const float patch = 0.50f + 0.28f * std::sin(angle * 5.0f + 0.7f)
                + 0.22f * std::sin(angle * 11.0f - 1.1f);
            return std::clamp(band * Smooth(0.20f, 0.74f, patch), 0.0f, 1.0f);
        }

        /** @brief Shallow sand/grass meadow potential beside and inside reef patches. */
        [[nodiscard]] static float SeagrassSuitability(float x, float z,
                                                       float signedShoreDistance,
                                                       float terrainHeight) noexcept
        {
            if (!std::isfinite(x) || !std::isfinite(z)
                || !std::isfinite(signedShoreDistance) || !std::isfinite(terrainHeight)) {
                return 0.0f;
            }
            const float offshore = -signedShoreDistance;
            const float depth = -terrainHeight;
            const float band = Smooth(5.0f, 11.0f, offshore)
                * (1.0f - Smooth(39.0f, 54.0f, offshore))
                * Smooth(0.42f, 0.82f, depth)
                * (1.0f - Smooth(2.6f, 3.5f, depth));
            const float patch = 0.62f + 0.22f * std::sin(x * .041f + z * .027f)
                + 0.16f * std::sin(x * .083f - z * .061f + .8f);
            return std::clamp(band * Smooth(0.30f, 0.76f, patch), 0.0f, 1.0f);
        }

        [[nodiscard]] static float AquaticHabitatSuitability(float x, float z,
                                                             float signedShoreDistance,
                                                             float terrainHeight) noexcept
        {
            return std::max(ReefSuitability(x, z, signedShoreDistance, terrainHeight),
                            SeagrassSuitability(x, z, signedShoreDistance, terrainHeight));
        }

        /** @brief GLSL counterpart used by terrain material consumers. */
        inline static constexpr const char* InlandShader = R"(
vec3 inlandEnvironment(vec2 worldPosition,float shoreDistance,vec2 coastalField) {
    float along=-worldPosition.x-65.0;
    float streamRange=smoothstep(0.0,5.0,along)*(1.0-smoothstep(98.0,104.0,along));
    float meanderEnvelope=smoothstep(0.0,9.0,along)*(1.0-smoothstep(86.0,98.0,along));
    float streamCentre=-55.0-worldPosition.x*0.23+meanderEnvelope
        *(1.55*sin(along*0.117)+0.62*sin(along*0.253+0.7));
    float streamAcross=abs(worldPosition.y-streamCentre);
    float t=clamp(along/98.0,0.0,1.0);float shaped=t*t*(3.0-2.0*t);
    float halfWidth=mix(0.45,1.5,shaped);
    float streamDrainage=streamRange*(1.0-smoothstep(halfWidth+0.35,halfWidth+8.0,streamAcross));
    float streamMoisture=streamRange*(1.0-smoothstep(2.0,25.0,streamAcross));
    float southCentre=5.0+12.0*sin(worldPosition.y*0.018);
    float southWidth=12.0+12.0*smoothstep(0.0,180.0,-worldPosition.y);
    float southValley=(1.0-smoothstep(southWidth*0.35,southWidth,abs(worldPosition.x-southCentre)))
        *(1.0-smoothstep(5.0,70.0,worldPosition.y));
    float coastBand=(coastalField.x*0.78+coastalField.y*0.42)
        *(1.0-smoothstep(18.0,58.0,shoreDistance));
    return vec3(clamp(max(streamMoisture,southValley*0.42),0.0,1.0),
        clamp(max(streamDrainage,southValley*0.28),0.0,1.0),coastBand);
}
)";
    };
}
