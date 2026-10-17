#include "IslandSimulator/TerrainRenderer.hpp"
#include "IslandSimulator/CoastalMaterial.hpp"
#include "IslandSimulator/StreamBedPatch.hpp"
#include "IslandSimulator/CaveGroundPatch.hpp"
#include "IslandSimulator/CampfireLighting.hpp"
#include "IslandSimulator/BunkerLayout.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/CanopyLighting.hpp"
#include "IslandSimulator/ForestFloorMaterial.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/PrevailingWind.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/TerrainMaterialVertex.hpp"
#include "IslandSimulator/TerrainSurfaceFrame.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElement.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementUsage.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr std::string_view ClearingFloorDiagnostic =
            "island-simulator: clearing floors reuse existing noise fields for broad patch breakup";
        constexpr std::string_view ForestFloorDiagnostic =
            "island-simulator: terrain interior humus and moss reuse portable material fields";
        constexpr std::string_view SoilReliefDiagnostic =
            "island-simulator: terrain close soil uses footprint-filtered clods and analytic relief";
        constexpr std::string_view FallenLeafDiagnostic =
            "island-simulator: forest soil separates humus and moss with footprint-filtered fallen leaves";
        constexpr std::string_view CoastalSandDiagnostic =
            "island-simulator: coastal sand stays continuous below water with filtered seabed ripples";
        constexpr std::string_view SandMineralDiagnostic =
            "island-simulator: exposed beach sand reuses filtered broad fields for warm and cool mineral patches";
        constexpr std::string_view WetSandBandDiagnostic =
            "island-simulator: wet sand uses a broader tide-darkened band with cool grazing sky sheen";
        constexpr std::string_view WetSandDrainageDiagnostic =
            "island-simulator: wet sand adds filtered shore-normal drainage threads and a warm shallow-film response";
        constexpr std::string_view ReefMacroDiagnostic =
            "island-simulator: reef substrate separates pale lagoon, shared habitat patches, crest, sand channels, and deep drop-off";
        constexpr std::string_view SubstrateDistanceDiagnostic =
            "island-simulator: terrain substrate persists at distance with footprint-filtered noise";
        constexpr std::string_view AerialGroundScaleDiagnostic =
            "island-simulator: inland terrain retains an eight-metre footprint-filtered material scale in aerial views";
        constexpr std::string_view AerialMaterialContrastDiagnostic =
            "island-simulator: aerial humus, moss, and ridge-outcrop classifications retain substrate with a second far-silhouette contrast fade";
        constexpr std::string_view SlopeLightDiagnostic =
            "island-simulator: terrain palette overrides preserve smooth slope daylight; opaque 28-byte vertices";
        constexpr std::string_view StreamBankDiagnostic =
            "island-simulator: stream terrain uses a compact incised profile and slope-limited exposed bank strata";
        constexpr std::string_view CliffMaterialDiagnostic =
            "island-simulator: terrain steep faces use dominant-axis rock projection with broken bedding and joints";
        constexpr std::string_view CliffMacroProjectionDiagnostic =
            "island-simulator: steep terrain rock macrocolour follows the cliff face instead of top-down curtains";
        constexpr std::string_view RidgeSkyFillDiagnostic =
            "island-simulator: inland ridge rock starts on sustained slopes and retains cool sky fill in fixed-sun shade";
        constexpr std::string_view RidgeOutcropDiagnostic =
            "island-simulator: inland ridge outcrops follow measured steepness with broken medium-scale exposure";
        constexpr std::string_view CoastalCliffFillDiagnostic =
            "island-simulator: low coastal cliffs retain salt-worn mineral colour, distance-stable macro strata, drainage streaks, and cool open-sky fill";

        const VertexDeclaration& TerrainDeclaration()
        {
            static const VertexDeclaration declaration{
                VertexElement(offsetof(TerrainMaterialVertex, position), VertexElementFormat::Vector3,
                              VertexElementUsage::Position, 0),
                VertexElement(offsetof(TerrainMaterialVertex, color), VertexElementFormat::Color,
                              VertexElementUsage::Color, 0),
                VertexElement(offsetof(TerrainMaterialVertex, data), VertexElementFormat::Vector3,
                              VertexElementUsage::TextureCoordinate, 0)};
            return declaration;
        }

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec3 aTerrainData;

uniform mat4 uViewProjection;

out vec3 vWorld;
out vec3 vBaseColor;
out vec2 vTerrainData;
out float vGroundLight;

void main()
{
    vWorld = aPosition;
    vBaseColor = aColor.rgb;
    vTerrainData = aTerrainData.xy;
    vGroundLight = aTerrainData.z;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vWorld;
in vec3 vBaseColor;
in vec2 vTerrainData;
in float vGroundLight;
uniform vec3 uCaveEntrance;
uniform vec2 uCaveForward;
uniform vec3 uBunkerEntrance;
uniform vec4 uBunkerFrame;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;
uniform vec2 uSandWind;
uniform vec4 uPrimaryPathShape;
uniform vec2 uPrimaryPathZRange;
uniform vec3 uEastClearing;
uniform vec3 uWestClearing;

out vec4 fragColor;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 fraction = fract(p);
    fraction = fraction * fraction * (3.0 - 2.0 * fraction);
    float lower = mix(hash12(cell), hash12(cell + vec2(1.0, 0.0)), fraction.x);
    float upper = mix(hash12(cell + vec2(0.0, 1.0)),
                      hash12(cell + vec2(1.0, 1.0)), fraction.x);
    return mix(lower, upper, fraction.y) - 0.5;
}

// Value plus analytic local-space gradient; four hashes, not finite-difference resampling.
vec3 soilNoiseGradient(vec2 p)
{
    vec2 cell = floor(p);
    vec2 t = fract(p);
    vec2 u = t * t * (3.0 - 2.0 * t);
    vec2 du = 6.0 * t * (1.0 - t);
    float a = hash12(cell);
    float b = hash12(cell + vec2(1.0, 0.0));
    float c = hash12(cell + vec2(0.0, 1.0));
    float d = hash12(cell + vec2(1.0, 1.0));
    float lower = mix(a, b, u.x);
    float upper = mix(c, d, u.x);
    return vec3(mix(lower, upper, u.y) - 0.5,
                mix(b - a, d - c, u.y) * du.x,
                (upper - lower) * du.y);
}

// Coverage, restrained dry-leaf shading and per-leaf colour variation. The maximum rotated
// radius, bend and jitter stay within a cell, so single-cell evaluation cannot clip a leaf.
// worldPixelWidth is evaluated before material branching; do not use derivatives here.
vec3 leafLitter(vec2 worldPosition, float scale, vec2 offset, float threshold,
                float worldPixelWidth)
{
    vec2 grid = worldPosition * scale + offset;
    vec2 cell = floor(grid);
    vec2 local = fract(grid) - 0.5;
    float seed = hash12(cell + offset * 0.37);
    vec2 jitter = fract(vec2(seed * 17.17, seed * 41.73)) - 0.5;
    local -= jitter * 0.21;
    float angle = fract(seed * 7.31) * 6.28318530718;
    vec2 axis = vec2(cos(angle), sin(angle));
    vec2 rotated = vec2(dot(local, axis), dot(local, vec2(-axis.y, axis.x)));
    float sizeVariation = mix(0.75, 1.22, fract(seed * 11.91));
    rotated /= sizeVariation;
    rotated.y -= sin(rotated.x * 13.0 + seed * 5.0) * 0.012;
    float along = abs(rotated.x) / 0.25;
    float halfWidth = 0.07 * pow(max(0.0, 1.0 - along), 0.65);
    float footprint = worldPixelWidth * scale / sizeVariation;
    float edgeWidth = clamp(footprint, 0.002, 0.035);
    float pointedEnds = 1.0 - smoothstep(0.25 - edgeWidth, 0.25 + edgeWidth,
                                        abs(rotated.x));
    float curvedSides = 1.0 - smoothstep(-edgeWidth, edgeWidth,
                                        abs(rotated.y) - halfWidth);
    float visibility = 1.0 - smoothstep(0.025, 0.10, footprint);
    float coverage = pointedEnds * curvedSides * visibility
        * step(threshold, fract(seed * 23.47));
    float veinVisibility = 1.0 - smoothstep(0.007, 0.025, footprint);
    float midrib = (1.0 - smoothstep(0.002, 0.007 + edgeWidth, abs(rotated.y)))
        * veinVisibility;
    float shade = 0.84 + 0.16 * smoothstep(-0.07, 0.07, rotated.y)
        + midrib * 0.10;
    return vec3(coverage, shade, fract(seed * 37.13));
}

float groundFleck(vec2 worldPosition, float scale, vec2 offset, float threshold)
{
    vec2 grid = worldPosition * scale + offset;
    vec2 cell = floor(grid);
    vec2 local = fract(grid) - 0.5;
    float seed = hash12(cell + offset * 0.23);
    vec2 jitter = fract(vec2(seed * 31.17, seed * 53.61)) - 0.5;
    local -= jitter * 0.70;
    float angle = fract(seed * 9.73) * 6.28318530718;
    vec2 axis = vec2(cos(angle), sin(angle));
    vec2 rotated = vec2(dot(local, axis), dot(local, vec2(-axis.y, axis.x)));
    vec2 radii = vec2(mix(0.045, 0.082, fract(seed * 17.31)),
                      mix(0.025, 0.052, fract(seed * 29.17)));
    float ellipticalDistance = length(rotated / radii);
    float shape = 1.0 - smoothstep(0.82, 1.18, ellipticalDistance);
    return shape * step(threshold, fract(seed * 19.41));
}

void main()
{
    vec2 p = vWorld.xz;
    // Derivatives must be evaluated outside material-dependent, non-uniform branches.
    vec3 worldDx = dFdx(vWorld);
    vec3 worldDy = dFdy(vWorld);
    float groundPixelWidth = max(length(worldDx.xz), length(worldDy.xz));
    float distanceToCamera = length(vWorld - uCameraPosition);
    // The island remains a heightfield above the buried facility. Only fragments
    // physically below its roof are removed, so the entrance and rooms are clear
    // from inside while the natural ridge remains intact when viewed from above.
    vec2 bunkerOffset=p-uBunkerEntrance.xz;
    float bunkerAcross=dot(bunkerOffset,uBunkerFrame.xy);
    float bunkerDepth=dot(bunkerOffset,uBunkerFrame.zw);
    float bunkerFloor=bunkerDepth<=0.0?0.11*bunkerDepth
        :(bunkerDepth<10.0?-0.32*bunkerDepth:-3.2);
    bool bunkerCorridor=abs(bunkerAcross)<=1.40
        &&bunkerDepth>=-0.80&&bunkerDepth<=36.35;
    bool bunkerComputer=bunkerAcross>=1.02&&bunkerAcross<=9.30
        &&bunkerDepth>=10.95&&bunkerDepth<=23.25;
    bool bunkerReactor=bunkerAcross>=-10.15&&bunkerAcross<=-1.02
        &&bunkerDepth>=22.75&&bunkerDepth<=36.25;
    bool bunkerService=bunkerAcross>=1.02&&bunkerAcross<=7.15
        &&bunkerDepth>=25.75&&bunkerDepth<=35.55;
    float bunkerCeiling=uBunkerEntrance.y+bunkerFloor
        +(bunkerDepth<10.0?2.75:3.05);
    if((bunkerCorridor||bunkerComputer||bunkerReactor||bunkerService)
        &&vWorld.y<bunkerCeiling+0.04) discard;
    float detailFade = 1.0 - smoothstep(95.0, 285.0, distanceToCamera);

    // Substrate is a world property: never fade humus, clearings or paths back to
    // the legacy green vertex palette as the camera retreats. Only unresolved
    // frequencies fade. Footprint is in metres/pixel, frequency in cells/metre.
    float broad = valueNoise(p * 0.060)
        * (1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.060));
    float landMiddle = valueNoise(p * 0.115 + vec2(-31.7, 22.4))
        * (1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.115));
    float medium = 0.0;
    float fine = 0.0;
    float mediumVisibility = 1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.31);
    if (mediumVisibility > 0.0) {
        medium = valueNoise(p * 0.31 + vec2(17.3, -9.7)) * mediumVisibility;
    }
    // The product's largest angular frequency is below 2.67 radians/metre.
    float fineVisibility = detailFade
        * (1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.43));
    if (fineVisibility > 0.0) {
        fine = sin(p.x * 1.73 + p.y * 0.41)
            * sin(p.y * 1.37 - p.x * 0.56) * fineVisibility;
    }

    float signedShoreDistance = vTerrainData.x;
    float slopeRadians = vTerrainData.y;
    vec2 coastField = coastalGroundField(p);
    vec3 inlandField = inlandEnvironment(p, signedShoreDistance, coastField);
    float sand = coastField.y * smoothstep(-6.0, -0.2, vWorld.y);
    float interior = smoothstep(8.0, 25.0, signedShoreDistance) * (1.0 - sand);
    float steep = smoothstep(0.30, 0.70, slopeRadians) * interior;
    float eastClearingMask = 0.0;
    float westClearingMask = 0.0;
    if (interior > 0.0 && steep < 1.0) {
        vec3 clearing = p.x > -18.0 ? uEastClearing : uWestClearing;
        vec2 clearingDelta = p - clearing.xy;
        float irregularRadius = clearing.z - broad * 2.20 - medium * 0.85;
        float innerRadius = irregularRadius - 2.8;
        float outerRadius = irregularRadius + 1.2;
        float clearingMask = (1.0 - smoothstep(innerRadius * innerRadius,
                                               outerRadius * outerRadius,
                                               dot(clearingDelta, clearingDelta)))
            * interior * (1.0 - steep);
        if (p.x > -18.0) {
            eastClearingMask = clearingMask;
        } else {
            westClearingMask = clearingMask;
        }
    }

    vec3 color = vBaseColor;
    float sandVariation = (broad * 0.255 + medium * 0.165 + fine * 0.030)
        * sand * detailFade;
    color *= vec3(1.0 + sandVariation * 0.92,
                  1.0 + sandVariation * 0.72,
                  1.0 + sandVariation * 0.43);
    color += vec3(0.043, 0.026, -0.011) * medium * sand * detailFade * vGroundLight;
    float exposedBeach = sand * smoothstep(0.8, 4.5, signedShoreDistance)
        * smoothstep(0.10, 0.62, vWorld.y);
    float sandMineralField = broad * 0.74 + medium * 0.38;
    float warmMineral = smoothstep(0.055, 0.255, sandMineralField) * exposedBeach;
    float coolMineral = (1.0 - smoothstep(-0.255, -0.055, sandMineralField))
        * exposedBeach;
    color = mix(color, color * vec3(1.065, 1.025, 0.925), warmMineral * 0.34);
    color = mix(color, color * vec3(0.900, 0.955, 0.975), coolMineral * 0.24);

    float closeSand = sand * (1.0 - smoothstep(28.0, 92.0, distanceToCamera));
    if (closeSand > 0.0) {
        float sandGranulation = valueNoise(p * 1.18 + vec2(4.7, -12.9));
        float grain = (sandGranulation * 0.064 + fine * 0.012) * closeSand;
        color *= vec3(1.0 + grain * 0.90,
                      1.0 + grain * 0.72,
                      1.0 + grain * 0.46);

        // Millimetre-scale dry relief: wind-normal ripple crests, with warped phase
        // rather than ruler-straight stripes. Fade the highest harmonic before Nyquist.
        float dryRelief = closeSand * smoothstep(2.0, 6.0, signedShoreDistance)
            * smoothstep(0.18, 0.65, vWorld.y);
        float rippleVisibility = 1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 9.0);
        float grainVisibility = 1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 85.0);
        if (dryRelief * max(rippleVisibility, grainVisibility) > 0.0) {
            vec3 bend = soilNoiseGradient(p * 0.45 + vec2(18.2, -6.3));
            float phase = dot(p, uSandWind) * 28.0 + bend.x * 4.0;
            vec2 phaseGradient = uSandWind * 28.0 + bend.yz * 1.80;
            float rippleDerivative = cos(phase) + 0.36 * cos(phase * 2.0 + 0.7);
            float rippleHeight = sin(phase) + 0.18 * sin(phase * 2.0 + 0.7);
            float rippleEnvelope = 0.60 + bend.x * 0.80;
            vec3 grains = soilNoiseGradient(p * 85.0 + vec2(-14.2, 9.6));
            vec2 sandSlope = (phaseGradient * rippleDerivative * rippleEnvelope
                + rippleHeight * bend.yz * 0.36) * (0.0022 * rippleVisibility)
                + grains.yz * (85.0 * 0.00015 * grainVisibility);
            vec3 sandNormal = terrainSurfaceNormal(cross(worldDx, worldDy));
            vec3 reliefNormal = sandNormal - vec3(sandSlope.x, 0.0, sandSlope.y) * sandNormal.y;
            reliefNormal *= inversesqrt(max(dot(reliefNormal, reliefNormal), 0.00000001));
            vec3 toSun = normalize(uToSun);
            float flatLight = 0.55 + 0.45 * max(0.0, dot(sandNormal, toSun));
            float reliefLight = 0.55 + 0.45 * max(0.0, dot(reliefNormal, toSun));
            color *= mix(1.0, reliefLight / flatLight, dryRelief);
            color *= 1.0 + grains.x * 0.12 * grainVisibility * dryRelief;
        }

        float rippleMask = closeSand
            * smoothstep(0.35, 1.40, signedShoreDistance)
            * (1.0 - smoothstep(8.0, 14.0, signedShoreDistance));
        float ripplePhase = signedShoreDistance * 3.80
            + broad * 1.15 + medium * 0.55
            + dot(p, vec2(0.016, -0.011));
        float narrowCrest = 1.0 - abs(sin(ripplePhase));
        float brokenCrest = (narrowCrest - 0.36)
            * (0.68 + 0.32 * sin(signedShoreDistance * 1.37 + p.x * 0.045));
        color *= vec3(1.0 + brokenCrest * rippleMask * 0.025,
                      1.0 + brokenCrest * rippleMask * 0.021,
                      1.0 + brokenCrest * rippleMask * 0.014);

        float seabedMask = closeSand * (1.0 - smoothstep(-0.18, 0.02, vWorld.y))
            * (1.0 - smoothstep(0.045, 0.14, groundPixelWidth));
        float bedPhase = signedShoreDistance * 9.5 + broad * 2.1 + medium * 1.2;
        float bedRipple = sin(bedPhase) * (0.65 + medium * 0.4);
        color *= 1.0 + bedRipple * seabedMask * 0.055;

        float drySubstrate = sand
            * (1.0 - smoothstep(18.0, 58.0, distanceToCamera))
            * smoothstep(1.4, 4.8, signedShoreDistance);
        if (drySubstrate > 0.0) {
            float darkPebble = groundFleck(p, 0.90, vec2(8.2, -15.7), 0.60);
            float paleShell = groundFleck(p, 1.15, vec2(-11.4, 6.9), 0.79);
            color = mix(color, vec3(0.255, 0.240, 0.195) * vGroundLight,
                        darkPebble * drySubstrate * 0.36);
            color = mix(color, vec3(0.940, 0.855, 0.650) * vGroundLight,
                        paleShell * drySubstrate * 0.42);
        }
    }

    // Submerged habitat is world-anchored substrate, not a second water tint.
    // Broad fields survive distant views; only fine mottling is footprint-filtered.
    // Keep this straight-line: indexed constant loops crash the Android software backend.
    float bedDepth = max(-vWorld.y, 0.0);
    float submergedHabitat = smoothstep(0.25, 0.65, bedDepth)
        * (1.0 - smoothstep(5.0, 8.0, bedDepth));
    if (submergedHabitat > 0.0) {
        float offshore = max(-signedShoreDistance, 0.0);
        float meadowField = valueNoise(p * 0.038 + vec2(43.7, -21.6));
        float reefField = valueNoise(p * 0.085 + vec2(-13.2, 57.9));
        // Project onto a fixed-radius circle to make radial channels without an atan seam.
        vec2 coastArc = p / max(length(p), 1.0) * 250.0;
        float channelField = valueNoise(coastArc * 0.065 + vec2(5.1, 19.3)
            + vec2(broad * 0.65));
        float sandChannel = smoothstep(0.10, 0.28, channelField);
        float lagoonZone = smoothstep(6.0, 14.0, offshore)
            * (1.0 - smoothstep(34.0, 48.0, offshore))
            * (1.0 - smoothstep(3.0, 5.0, bedDepth));
        float meadow = smoothstep(-0.16, 0.17, meadowField + medium * 0.22)
            * smoothstep(0.45, 0.85, bedDepth)
            * (1.0 - smoothstep(1.8, 2.6, bedDepth));
        float reefZone = smoothstep(24.0, 36.0, offshore)
            * (1.0 - smoothstep(72.0, 88.0, offshore));
        float reefCrest = smoothstep(66.0, 73.0, offshore + meadowField * 12.0)
            * (1.0 - smoothstep(78.0, 86.0, offshore + meadowField * 12.0));
        float coastAngle = atan(p.y, p.x);
        float sharedPatch = 0.50 + 0.28 * sin(coastAngle * 5.0 + 0.7)
            + 0.22 * sin(coastAngle * 11.0 - 1.1);
        float sharedReef = smoothstep(13.0, 25.0, offshore)
            * (1.0 - smoothstep(82.0, 96.0, offshore))
            * smoothstep(0.35, 0.90, bedDepth)
            * (1.0 - smoothstep(5.2, 7.5, bedDepth))
            * smoothstep(0.20, 0.74, sharedPatch);
        float reefCover = smoothstep(-0.10, 0.19, reefField + meadowField * 0.38)
            * max(reefZone * 0.88, reefCrest)
            * mix(0.62, 1.15, sharedReef);
        float habitatCoverage = submergedHabitat * (1.0 - sandChannel);
        vec3 lagoonBed = vec3(0.70, 0.675, 0.425) * vGroundLight;
        vec3 grassBed = vec3(0.22, 0.265, 0.105)
            * (1.0 + meadowField * 0.28 + medium * 0.20) * vGroundLight;
        vec3 reefBed = mix(vec3(0.29, 0.255, 0.145), vec3(0.49, 0.405, 0.24),
                           clamp(0.5 + reefField + medium * 0.65, 0.0, 1.0))
            * vGroundLight;
        color = mix(color, lagoonBed, lagoonZone
            * (0.20 + sandChannel * 0.20) * (1.0 - meadow * 0.45));
        color = mix(color, grassBed, meadow * habitatCoverage * 0.86);
        color = mix(color, reefBed, reefCover * habitatCoverage);
        float macroCrest = smoothstep(62.0, 69.0, offshore)
            * (1.0 - smoothstep(79.0, 86.0, offshore))
            * smoothstep(0.5, 1.4, bedDepth)
            * (1.0 - smoothstep(6.0, 8.0, bedDepth));
        float crestMark = max(reefCrest * mix(0.45, 1.0, sharedReef),
                              macroCrest * mix(0.58, 1.0, sharedReef))
            * (1.0 - sandChannel * 0.68) * submergedHabitat;
        color = mix(color, vec3(0.335, 0.275, 0.145) * vGroundLight,
                    crestMark * 0.58);
        float dropoff = smoothstep(78.0, 104.0, offshore)
            * smoothstep(4.0, 8.0, bedDepth) * submergedHabitat;
        color = mix(color, vec3(0.095, 0.145, 0.145) * vGroundLight, dropoff * 0.58);

        // Sparse shell fragments remain readable on the close sandy gaps between
        // lagoon grass and reef cover. Reuse the filtered material fleck primitive
        // instead of adding geometry or another underwater draw.
        float shallowSandDetail = submergedHabitat
            * (1.0 - smoothstep(3.0, 4.6, bedDepth))
            * (1.0 - smoothstep(30.0, 62.0, distanceToCamera))
            * smoothstep(0.34, 0.72, sandChannel)
            * (1.0 - meadow * 0.78) * (1.0 - reefCover * 0.88);
        if (shallowSandDetail > 0.0) {
            float ivoryShell = groundFleck(p, 0.88, vec2(21.7, -4.3), 0.80);
            float roseShell = groundFleck(p, 1.04, vec2(-7.8, 26.1), 0.88);
            color = mix(color, vec3(0.78, 0.73, 0.57) * vGroundLight,
                        ivoryShell * shallowSandDetail * 0.34);
            color = mix(color, vec3(0.66, 0.49, 0.40) * vGroundLight,
                        roseShell * shallowSandDetail * 0.25);
        }
    }

    float groundVariation = (broad * 0.160 + landMiddle * 0.075
        + medium * 0.105 + fine * 0.024)
        * interior;
    color *= vec3(1.0 + groundVariation * 0.55,
                  1.0 + groundVariation,
                  1.0 + groundVariation * 0.48);
    float exposedSoil = smoothstep(0.16, 0.40,
        broad * 0.52 + landMiddle * 0.18 + medium * 0.30)
        * interior;
    color = mix(color, color * vec3(0.82, 0.76, 0.61), exposedSoil * 0.18);
    color = mix(color, color * vec3(0.82, 0.78, 0.68), steep * 0.24);

    // Break up the broad green inland floor with restrained, material-specific
    // humus and moss fields. These broad material fields also remain beyond 285 m;
    // close clods and individual fallen leaves retain their separate detail fades.
    float forestFloorMask = smoothstep(24.0, 54.0, signedShoreDistance)
        * (1.0 - steep) * (1.0 - sand)
        * (1.0 - max(eastClearingMask, westClearingMask) * 0.72);
    float forestMaterialField = broad * 0.70 + medium * 0.45 + fine * 0.035;
    // Shared with grass placement: sunlit, patch-rich ground carries a living turf layer,
    // shaded or patch-poor ground keeps exposed humus and fallen leaves.
    vec2 groundField = canopyGroundField(p);
    float turfAmount = groundCoverDensity(groundField.x, groundField.y,
        signedShoreDistance, slopeRadians, coastField.x, coastField.y,
        inlandField.x, inlandField.y) * 0.86 * (1.0 - steep);
    // Tropical understory grows through exposed humus; it is not a continuous green lawn.
    // Keep grass/moss islands, and leave the authored sunny clearing and beach palettes intact.
    color = mix(color, vec3(0.300, 0.223, 0.139) * vGroundLight, forestFloorMask * 0.82);
    float leafHumus = smoothstep(-0.015, 0.245, forestMaterialField)
        * forestFloorMask;
    float coolMoss = (1.0 - smoothstep(-0.245, -0.075, forestMaterialField))
        * forestFloorMask;
    // Preserve the qualified mid-scale field from aerial views, but reduce only its
    // darkest classification amplitude after individual patches become landscape
    // masses. The underlying brown forest substrate never fades back to legacy green.
    float farSilhouetteCalm=mix(1.0,0.55,smoothstep(315.0,460.0,distanceToCamera));
    float aerialPatchStrength=mix(1.0,0.30,smoothstep(165.0,315.0,distanceToCamera))
        * farSilhouetteCalm;
    color = mix(color, vec3(0.205, 0.151, 0.091) * vGroundLight,
                leafHumus * 0.54 * aerialPatchStrength);
    color = mix(color, vec3(0.185, 0.265, 0.115) * vGroundLight,
                coolMoss * 0.64 * aerialPatchStrength);

    // Preserve broad authored material/lighting cues; the litter source supplies close detail.
    // Fade before the hashed lattice itself becomes subpixel. Its mip chain filters texels,
    // while this separate footprint fade filters changing window identities.
    float imageFloor = uForestFloorEnabled * forestFloorMask
        * (1.0 - smoothstep(18.0, 48.0, distanceToCamera))
        * (1.0 - smoothstep(0.035, 0.12, groundPixelWidth))
        * (1.0 - turfAmount * 0.35);
    if (imageFloor > 0.0) {
        vec3 litter = forestFloorColor(p, worldDx.xz, worldDy.xz);
        color = mix(color, litter * vGroundLight * (0.82 + broad*0.12), imageFloor * 0.88);
    }

    vec3 sunlitClearingGround = vec3(0.475, 0.375, 0.215) * vGroundLight;
    vec3 leafRichClearingGround = vec3(0.235, 0.205, 0.122) * vGroundLight;
    color = mix(color, sunlitClearingGround,
                eastClearingMask * (0.58 + broad * 0.055 + medium * 0.035));
    color = mix(color, leafRichClearingGround,
                westClearingMask * (0.30 + broad * 0.045));

    // Reuse the material's existing noise fields to break up the two broad clearing floors.
    // This adds no texture lookup or extra procedural-noise evaluation.
    float clearingPatchField = broad * 0.72 + medium * 0.46 + fine * 0.035;
    float eastDryPatch = smoothstep(0.055, 0.29, clearingPatchField)
        * eastClearingMask;
    float eastCoolPatch = (1.0 - smoothstep(-0.27, -0.045, clearingPatchField))
        * eastClearingMask;
    color = mix(color, vec3(0.560, 0.445, 0.265) * vGroundLight, eastDryPatch * 0.40);
    color = mix(color, color * vec3(0.84, 0.89, 0.76), eastCoolPatch * 0.18);

    float westLeafPatch = smoothstep(0.035, 0.27, clearingPatchField)
        * westClearingMask;
    float westMossPatch = (1.0 - smoothstep(-0.26, -0.035, clearingPatchField))
        * westClearingMask;
    color = mix(color, vec3(0.150, 0.125, 0.075) * vGroundLight, westLeafPatch * 0.32);
    color = mix(color, vec3(0.230, 0.315, 0.090) * vGroundLight, westMossPatch * 0.22);

    // Living turf undercoat: olive inland, warmer and drier toward the coast and in the
    // sun-dried east clearing, cooler in the leaf-rich west clearing. It persists at distance.
    float turfDryness = clamp(0.30 + broad * 0.68 + landMiddle * 0.24 + medium * 0.31
        + (1.0 - smoothstep(26.0, 80.0, signedShoreDistance)) * 0.30
        + eastClearingMask * 0.40 - westClearingMask * 0.25, 0.0, 1.0);
    vec3 turf = mix(vec3(0.160, 0.235, 0.070), vec3(0.315, 0.300, 0.135), turfDryness)
        * (1.0 + medium * 0.20 + fine * 0.05);
    float routeMask = 0.0;

    if (signedShoreDistance > 20.0 && steep < 1.0) {
        float pathCentre = uPrimaryPathShape.x + uPrimaryPathShape.y
            * sin((p.y + uPrimaryPathShape.z) * uPrimaryPathShape.w);
        float pathRange = smoothstep(uPrimaryPathZRange.x,
                                     uPrimaryPathZRange.x + 16.0, p.y)
            * (1.0 - smoothstep(uPrimaryPathZRange.y - 16.0,
                                uPrimaryPathZRange.y, p.y));
        float lateralOffset = p.x - pathCentre;
        float pathWidth = 0.75 + (broad + 0.5) * 0.45;
        float brokenLateralDistance = abs(lateralOffset)
            + medium * 0.48 + fine * 0.095;
        float pathEdgeWidth = max(0.28, groundPixelWidth);
        float pathMask = (1.0 - smoothstep(pathWidth + 0.10 - pathEdgeWidth,
                                           pathWidth + 0.10 + pathEdgeWidth,
                                           brokenLateralDistance))
            * pathRange * smoothstep(20.0, 42.0, signedShoreDistance)
            * (1.0 - steep);
        routeMask = pathMask;
        if (pathMask > 0.0) {
            float compactedCentre = 1.0 - smoothstep(
                0.16, 0.94, abs(lateralOffset + medium * 0.14));
            float pathMottle = clamp(medium * 0.86 + fine * 0.08, -0.5, 0.5);
            vec3 shadedEarth = vec3(0.285, 0.192, 0.072);
            vec3 sunDryEarth = vec3(0.370, 0.275, 0.126);
            vec3 dryEarth = mix(shadedEarth, sunDryEarth, pathMottle + 0.5);
            float pathBlend = pathMask
                * (0.46 + compactedCentre * 0.16 + pathMottle * 0.055);
            color = mix(color, dryEarth * vGroundLight, pathBlend);
        }
    }

    float closeDetail = (1.0 - smoothstep(24.0, 68.0, distanceToCamera))
        * smoothstep(22.0, 38.0, signedShoreDistance)
        * (1.0 - smoothstep(0.34, 0.66, slopeRadians))
        * (1.0 - imageFloor * 0.90);
    if (closeDetail > 0.0) {
        float micro = valueNoise(p * 1.26 + vec2(-21.4, 8.3));
        color *= vec3(1.0 + micro * 0.050,
                      1.0 + micro * 0.082,
                      1.0 + micro * 0.043) * closeDetail
            + vec3(1.0 - closeDetail);
        float clodVisibility = closeDetail
            * (1.0 - smoothstep(0.025, 0.085, groundPixelWidth))
            * (1.0 - smoothstep(18.0, 40.0, distanceToCamera));
        if (clodVisibility > 0.0) {
            vec3 clod = soilNoiseGradient(p * 6.5 + vec2(37.1, -12.6));
            vec3 surfaceNormal = terrainSurfaceNormal(cross(worldDx, worldDy));
            // A 1.2 cm relief field; derivatives are converted from noise to world units.
            vec2 slope = clod.yz * (6.5 * 0.012) * clodVisibility;
            vec3 reliefVector = surfaceNormal
                - vec3(slope.x, 0.0, slope.y) * surfaceNormal.y;
            vec3 reliefNormal = reliefVector
                * inversesqrt(max(dot(reliefVector, reliefVector), 0.00000001));
            vec3 sunDirection = normalize(uToSun);
            float smoothLight = 0.55 + 0.45 * max(0.0, dot(surfaceNormal, sunDirection));
            float reliefLight = 0.55 + 0.45 * max(0.0, dot(reliefNormal, sunDirection));
            color *= reliefLight / smoothLight;
            color *= 1.0 + clod.x * 0.20 * clodVisibility;
            float poreVisibility = clodVisibility
                * (1.0 - smoothstep(0.006, 0.020, groundPixelWidth));
            float pores = valueNoise(p * 37.0 + vec2(-9.3, 16.7));
            color *= 1.0 + pores * 0.12 * poreVisibility;
        }
        // Short turf breaks into darker gaps and brighter shoot crowns at several scales.
        float turfDetail = turfAmount * closeDetail;
        if (turfDetail > 0.0) {
            float clumpVisibility = 1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 4.7);
            float tuftVisibility = 1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 9.3);
            float clump = valueNoise(p * 2.1 + vec2(5.3, -7.9))
                + valueNoise(p * 4.7 + vec2(-3.1, 2.2)) * 0.6 * clumpVisibility
                + valueNoise(p * 9.3 + vec2(8.8, 1.7)) * 0.45 * tuftVisibility;
            color *= 1.0 + clump * 0.40 * turfDetail;
        }
        float litterPatch = clamp(0.52 + broad * 0.82 + medium * 0.30, 0.18, 1.0)
            * mix(1.0, 0.58, eastClearingMask)
            * mix(1.0, 1.22, westClearingMask);
        vec3 warmLeaf = leafLitter(p, 1.65, vec2(13.7, -4.3), 0.42, groundPixelWidth);
        vec3 darkLeaf = leafLitter(p, 2.70, vec2(-7.1, 19.6), 0.65, groundPixelWidth);
        // The west-clearing density boost may exceed one; never extrapolate leaf colours.
        float litterAmount = clamp(closeDetail * litterPatch, 0.0, 1.0) * (1.0 - turfAmount * 0.45);
        vec3 dryLeafColor = mix(vec3(0.235, 0.142, 0.060), vec3(0.445, 0.315, 0.145),
                                warmLeaf.z) * warmLeaf.y * vGroundLight;
        vec3 oldLeafColor = mix(vec3(0.120, 0.091, 0.054), vec3(0.245, 0.188, 0.092),
                                darkLeaf.z) * darkLeaf.y * vGroundLight;
        color = mix(color, dryLeafColor, warmLeaf.x * litterAmount * 0.90);
        color = mix(color, oldLeafColor, darkLeaf.x * litterAmount * 0.82);
    }

    // Re-hue the detailed substrate toward living turf instead of painting over it: the
    // luminance of litter, clods and clumps survives, so close turf keeps its texture.
    float turfWeight = turfAmount * (1.0 - routeMask) * 0.82;
    if (turfWeight > 0.0) {
        const vec3 luminanceWeights = vec3(0.30, 0.55, 0.15);
        vec3 turfChroma = turf / max(dot(turf, luminanceWeights), 0.0001);
        vec3 tinted = dot(color, luminanceWeights) * turfChroma;
        color = mix(color, mix(tinted, turf * vGroundLight, 0.35), turfWeight);
    }

    // Wind-exposed upper ridges carry a sparse sun-cured turf transition between the
    // dark interior humus and bare rock. This broad, footprint-filtered field remains
    // legible in overview without adding grass instances or painting one uniform cap.
    float highlandPatch = smoothstep(-0.18, 0.27,
        broad * 0.72 + landMiddle * 0.43 + medium * 0.15);
    float highlandTurf = smoothstep(13.0, 29.0, vWorld.y)
        * (1.0 - smoothstep(0.20, 0.50, slopeRadians)) * interior
        * (0.28 + highlandPatch * 0.72);
    vec3 highlandColor = mix(vec3(0.285, 0.285, 0.105),
        vec3(0.390, 0.355, 0.145), clamp(0.5 + broad * 0.85, 0.0, 1.0))
        * vGroundLight;
    color = mix(color, highlandColor, highlandTurf * 0.34);

    // Thin soil on steep ridge faces exposes weathered rock instead of the old
    // featureless green vertex palette. Existing filtered fields break the edge.
    float ridgeRock = smoothstep(0.27, 0.54,
        slopeRadians + broad * 0.045 + landMiddle * 0.022 + medium * 0.030)
        * interior * smoothstep(4.0, 10.0, vWorld.y);
    float coastalCliff = coastField.x * smoothstep(0.26, 0.58, slopeRadians)
        * smoothstep(-0.25, 0.35, vWorld.y)
        * (1.0 - smoothstep(9.0, 17.0, vWorld.y));
    ridgeRock = max(ridgeRock, coastField.x * smoothstep(-0.25, 0.35, vWorld.y));
    float rockCoverage = ridgeRock;
    float cliffMacroModulation = 0.0;
    vec3 weatheredRock = vec3(0.36, 0.33, 0.27);
    if (ridgeRock > 0.0) {
        // Height-field x/z noise stretches into muddy vertical curtains on a steep face.
        // Project detail through the dominant horizontal face axis and use world height for
        // the other coordinate so cliffs retain compact rock blocks and readable bedding.
        // This island is approximately radial. A smooth radial face proxy avoids changing
        // projection weights at every height-field triangle while still selecting the
        // appropriate vertical plane around the shoreline and inland ridges.
        float xFacing = abs(vWorld.x)
            / max(abs(vWorld.x) + abs(vWorld.z), 0.0001);
        float faceAcross = mix(vWorld.x, vWorld.z, xFacing);
        float projectedMacro = mix(
            valueNoise(vec2(vWorld.x, vWorld.y) * vec2(0.055, 0.075) + vec2(2.4, -1.7)),
            valueNoise(vec2(vWorld.z, vWorld.y) * vec2(0.055, 0.075) + vec2(-3.8, 2.1)),
            xFacing);
        float projectedMottle = mix(
            valueNoise(vec2(vWorld.x, vWorld.y) * vec2(0.31, 0.38) + vec2(7.6, -3.1)),
            valueNoise(vec2(vWorld.z, vWorld.y) * vec2(0.31, 0.38) + vec2(-11.8, 5.7)),
            xFacing);
        float rockTone = clamp(0.50 + projectedMacro * 0.82
            + projectedMottle * 0.30, 0.0, 1.0);
        weatheredRock = mix(vec3(0.255, 0.272, 0.268),
                            vec3(0.455, 0.392, 0.300), rockTone);
        vec3 saltWornRock = mix(vec3(0.350, 0.370, 0.350),
                                vec3(0.485, 0.425, 0.330),
                                clamp(0.52 + projectedMacro * 0.76
                                    + projectedMottle * 0.34, 0.0, 1.0));
        weatheredRock = mix(weatheredRock, saltWornRock, coastalCliff * 0.62);
        // The close bedding and grain deliberately filter away with footprint size. Keep a
        // much broader, low-contrast geological rhythm for the whole-headland view so the
        // seven-metre face does not collapse into one flat grey curtain at distance.
        float macroStrata = sin(vWorld.y * 1.08 + projectedMacro * 0.68
            + sin(faceAcross * 0.085) * 0.34);
        float drainageField = valueNoise(vec2(faceAcross * 0.092, vWorld.y * 0.052)
            + vec2(-14.6, 7.3));
        float drainageStreak = smoothstep(0.18, 0.70,
            drainageField + projectedMottle * 0.20);
        float warmBed = smoothstep(0.30, 0.82,
            macroStrata + projectedMacro * 0.26);
        weatheredRock = mix(weatheredRock, vec3(0.470, 0.392, 0.292),
            coastalCliff * warmBed * 0.27);
        weatheredRock *= 1.0 + coastalCliff
            * (macroStrata * 0.135 - drainageStreak * 0.155);
        cliffMacroModulation = coastalCliff
            * (macroStrata * 0.075 - drainageStreak * 0.080);
        weatheredRock *= 1.0 + projectedMottle * 0.25;
        float beddingPhase = vWorld.y * 1.72 + projectedMacro * 0.58
            + sin(faceAcross * 0.11 + projectedMottle * 1.4) * 0.30;
        // Thin soil leaves connected but irregular outcrops, rather than tinting an entire
        // inland hillside as one dark muddy curtain. Low coastal cliffs remain more exposed.
        // Macro colour may span tens of metres, so it must only bias exposure.  Let the
        // three-metre mottle and broken bedding define the actual boundary; otherwise a
        // modest slope turns into one soft, dark oval that resembles a painted stain.
        float outcropField = projectedMacro * 0.18 + projectedMottle * 0.72
            + sin(beddingPhase * 0.47 + faceAcross * 0.035) * 0.16;
        // Coverage is read most often from above, where face-projected fields alone form
        // smooth oval islands. Two footprint-filtered ground-plane fields notch those rims
        // and leave small soil shelves across otherwise exposed faces. They affect only the
        // rock/soil boundary; bedding and joints retain the correct face projection.
        float outcropEdgeFilter = 1.0 - smoothstep(0.18, 0.50,
            groundPixelWidth * 0.085);
        float outcropEdge = valueNoise(p * 0.085 + vec2(18.7, -23.4))
            * outcropEdgeFilter;
        float outcropShelf = valueNoise(p * 0.205 + vec2(-7.2, 31.6))
            * (1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.205));
        float outcropBreakup = smoothstep(0.075, 0.39,
            outcropField + outcropEdge * 0.30 + outcropShelf * 0.12);
        float soilShelf = smoothstep(0.14, 0.42,
            -outcropShelf + outcropEdge * 0.22) * (1.0 - coastalCliff);
        outcropBreakup *= 1.0 - soilShelf * 0.34;
        // The coastal ground field keeps salt-worn headlands continuously mineral; only
        // inland ridge soil breaks strongly between exposed rock patches.
        float minimumExposure = mix(0.045, 0.88, coastField.x);
        rockCoverage *= mix(minimumExposure, 1.0,
                            max(outcropBreakup, coastalCliff * 0.72));
        float beddingWave = sin(beddingPhase) + 0.34 * sin(beddingPhase * 2.07 + 0.8);
        float beddingVisibility = 1.0 - smoothstep(0.16, 0.48, groundPixelWidth * 2.05);
        float bedding = beddingWave * beddingVisibility;
        float seamDistance = abs(fract(beddingPhase / 6.28318530718) - 0.5);
        float seamWidth = max(0.018, groundPixelWidth * 0.035);
        float beddingSeam = 1.0 - smoothstep(0.030, 0.030 + seamWidth, seamDistance);
        float seamBreak = valueNoise(vec2(faceAcross * 0.16, vWorld.y * 0.27)
            + vec2(-6.8, 12.1));
        beddingSeam *= smoothstep(-0.34, 0.16, seamBreak + projectedMottle * 0.45);
        float jointCoordinate = faceAcross * 0.18
            + sin(vWorld.y * 0.21 + projectedMottle * 1.8) * 0.14
            + valueNoise(vec2(vWorld.y * 0.18, faceAcross * 0.065) + vec2(4.9, -8.2)) * 0.70;
        float jointDistance = abs(fract(jointCoordinate) - 0.5);
        float jointWidth = max(0.012, groundPixelWidth * 0.022);
        float verticalJoint = 1.0 - smoothstep(0.020, 0.020 + jointWidth, jointDistance);
        float jointBreak = valueNoise(vec2(faceAcross * 0.07, vWorld.y * 0.36)
            + vec2(18.4, -2.7));
        verticalJoint *= smoothstep(-0.28, 0.13, jointBreak + projectedMacro * 0.35);
        float rockVisibility = 1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.90);
        float rockGrain = valueNoise(vec2(faceAcross, vWorld.y) * vec2(0.90, 1.05)
            + vec2(31.8, -8.4)) * rockVisibility;
        float fractureField = valueNoise(vec2(faceAcross * 0.19,
            vWorld.y * 0.23) + vec2(4.2, 9.3));
        float fractureWidth = max(0.014, groundPixelWidth * 0.19);
        float fracture = 1.0 - smoothstep(0.018, 0.018 + fractureWidth, abs(fractureField));
        weatheredRock *= 1.0 + rockGrain * 0.24 + projectedMottle * 0.18
            + bedding * 0.055 - beddingSeam * 0.085 - verticalJoint * 0.075
            - fracture * 0.06 * (1.0 - smoothstep(0.10, 0.35, groundPixelWidth * 0.19));
        // Aerial rock still receives open sky. This small distance-only lift prevents the
        // remaining broken patches from collapsing into near-black spots without bleaching
        // their close mineral contrast or changing coverage.
        weatheredRock += vec3(0.028, 0.030, 0.028)
            * smoothstep(150.0, 330.0, distanceToCamera) * (1.0 - coastalCliff * 0.45);
    }
    vec3 litRidgeRock = weatheredRock * vGroundLight;
    float ridgeSkyFill = 1.0 - smoothstep(0.38, 0.72, vGroundLight);
    litRidgeRock += vec3(0.090, 0.112, 0.120) * ridgeSkyFill
        * (0.72 + smoothstep(0.24, 0.62, slopeRadians) * 0.28);
    litRidgeRock += vec3(0.038, 0.054, 0.058) * coastalCliff
        * (0.62 + ridgeSkyFill * 0.38);
    float aerialOutcropStrength=mix(0.98,0.30,
        smoothstep(165.0,315.0,distanceToCamera)) * farSilhouetteCalm;
    color = mix(color, litRidgeRock, rockCoverage * aerialOutcropStrength);
    color *= 1.0 + rockCoverage * cliffMacroModulation;

    // One shared environment field darkens the carved drainage and its irregular damp banks.
    // Existing broad/medium fields break the straight surveyed valley without moving water
    // outside the collision-authoritative bed.
    float streamEdgeNoise = broad * 1.65 + medium * 0.62;
    float streamWet = smoothstep(0.20, 0.94, inlandField.y + streamEdgeNoise * 0.15)
        * (0.72 + 0.28 * inlandField.x);
    vec3 dampEarth = mix(vec3(0.205, 0.155, 0.090), vec3(0.185, 0.265, 0.125),
        clamp(inlandField.x * 0.62 + max(-medium, 0.0) * 0.30, 0.0, 1.0)) * vGroundLight;
    color = mix(color, dampEarth, streamWet * 0.55);
    float streamStone = groundFleck(p, 0.72, vec2(19.1, -3.8), 0.68) * streamWet;
    color = mix(color, vec3(0.205, 0.220, 0.180) * vGroundLight, streamStone * 0.48);

    // The seasonal basin has no permanent water surface. Dark humic soil, cool moss flecks and
    // an irregular drying rim distinguish it from both the flowing stream and ordinary jungle.
    float seasonalWetland=seasonalWetlandPotential(p,signedShoreDistance);
    float wetlandBreakup=clamp(.72+broad*.38+medium*.22,0.32,1.0);
    float wetlandSoil=seasonalWetland*wetlandBreakup;
    vec3 humicEarth=mix(vec3(.105,.083,.050),vec3(.105,.180,.082),
        clamp(inlandField.x*.58+max(-fine,0.0)*.24,0.0,1.0))*vGroundLight;
    color=mix(color,humicEarth,wetlandSoil*.72);
    float wetlandFleck=groundFleck(p,1.24,vec2(-7.2,23.4),.61)*seasonalWetland;
    color=mix(color,vec3(.165,.235,.112)*vGroundLight,wetlandFleck*.34);

    // Reveal the actual incised bank faces with restrained exposed-soil strata. The mask
    // follows the same route/width function as the CPU bed and is additionally limited by
    // measured terrain slope, so it cannot become a flat painted stripe across the valley.
    vec3 streamGeometry = streamRouteGeometry(p);
    float bankInner = smoothstep(streamGeometry.z * 0.85,
                                 streamGeometry.z + 0.02, streamGeometry.y);
    float bankOuter = 1.0 - smoothstep(streamGeometry.z + 0.54,
                                       streamGeometry.z + 1.00, streamGeometry.y);
    float bankFace = streamGeometry.x * bankInner * bankOuter
        * smoothstep(0.16, 0.38, slopeRadians);
    bankFace *= clamp(0.64 + broad * 0.56 + medium * 0.24, 0.28, 1.0);
    float bankStrata = 0.5 + 0.5 * sin(vWorld.y * 31.0 + broad * 3.1 + medium * 1.7);
    vec3 exposedBank = mix(vec3(0.145, 0.108, 0.064), vec3(0.235, 0.174, 0.094),
                           0.38 + bankStrata * 0.28) * vGroundLight;
    exposedBank *= 1.0 + fine * 0.10;
    color = mix(color, exposedBank, bankFace * 0.52);

    float wetHeight = vWorld.y + broad * 0.06 + medium * 0.025;
    float wetEdge = (1.0 - smoothstep(0.06, 0.68, wetHeight))
        * smoothstep(-0.12, 0.06, vWorld.y) * sand;
    // Dark wet sand keeps a little of the beach's warm mineral identity instead of collapsing
    // into one olive multiplier. Reused broad/medium fields vary the film without another lookup.
    vec3 wetSand = color * vec3(0.58, 0.64, 0.60);
    vec3 wetMineral = vec3(0.355, 0.300, 0.190) * vGroundLight;
    wetSand = mix(wetSand, wetMineral,
        0.12 + clamp(-medium * 0.13 + broad * 0.045, 0.0, 0.12));
    color = mix(color, wetSand, wetEdge * 0.80);
    if (wetEdge > 0.001) {
        vec3 wetNormal = terrainSurfaceNormal(cross(worldDx, worldDy));
        vec3 toView = normalize(uCameraPosition - vWorld);
        vec3 halfVector = normalize(uToSun) + toView;
        vec3 halfDirection = halfVector
            * inversesqrt(max(dot(halfVector, halfVector), 0.00000001));
        float sunSheen = pow(max(dot(wetNormal, halfDirection), 0.0), 28.0);
        float grazingSheen = pow(1.0 - max(dot(wetNormal, toView), 0.0), 2.2);

        // Project noise onto a fixed-radius copy of the coast. Its value changes across the
        // shoreline but stays coherent shore-normal, producing eroded drainage threads rather
        // than another isotropic mottled texture. Footprint fading prevents distant shimmer.
        vec2 coastArc = p / max(length(p), 1.0) * 250.0;
        float drainageNoise = valueNoise(coastArc * 0.16 + vec2(12.7, -8.1));
        float drainageVisibility = 1.0 - smoothstep(0.12, 0.42, groundPixelWidth);
        float drainageThread = (1.0 - smoothstep(0.045, 0.135, abs(drainageNoise)))
            * drainageVisibility * (0.72 + 0.28 * sin(signedShoreDistance * 0.83
                + broad * 2.1));
        color = mix(color, color * vec3(0.72, 0.77, 0.73),
                    wetEdge * drainageThread * 0.34);
        float wetSheen = wetEdge * (sunSheen * 0.48 + grazingSheen * 0.135)
            * (1.0 + drainageThread * 0.22);
        color += vec3(0.14, 0.20, 0.21) * wetSheen;
    }

    // Same texel as the cover sample above; retains cool skylight in canopy shade.
    float terrainTransmission=canopyDappleTransmission(vWorld.xz,groundField.x);
    color *= mix(uCanopyAmbient, vec3(1.0), terrainTransmission);
    // The replacement mesh is also the cave floor: no coplanar rock-floor overlay.
    vec2 caveOffset=vWorld.xz-uCaveEntrance.xz;
    float caveDepth=dot(caveOffset,uCaveForward);
    float caveAcross=dot(caveOffset,vec2(uCaveForward.y,-uCaveForward.x));
    float caveWidth=2.65+.18*sin(caveDepth*.42+.3);
    float caveFloor=(1.0-smoothstep(caveWidth-.5,caveWidth+.3,
        abs(caveAcross-.22*sin(caveDepth*.31))))
        *smoothstep(-1.5,1.0,caveDepth)*(1.0-smoothstep(20.0,21.0,caveDepth));
    float caveGrain=valueNoise(vWorld.xz*4.0)*.65+valueNoise(vWorld.xz*17.0)*.35;
    vec3 caveStone=mix(vec3(.24,.25,.24),vec3(.34,.32,.28),caveGrain);
    caveStone*=mix(.85,.24,smoothstep(0.0,14.0,caveDepth));
    // Weathered stone continues onto the real replacement terrain around the portal,
    // avoiding a hard grey-shell/green-soil seam without adding a decal or draw.
    float caveAxis=abs(caveAcross-.22*sin(caveDepth*.31));
    float apronNoise=broad*.72+medium*.35;
    float caveApron=smoothstep(-5.5,-2.0,caveDepth)
        *(1.0-smoothstep(8.0,15.0,caveDepth))
        *smoothstep(caveWidth-.30+apronNoise*.22,caveWidth+.72+apronNoise*.22,caveAxis)
        *(1.0-smoothstep(6.1+apronNoise*.55,7.45+apronNoise*.55,caveAxis));
    vec3 apronStone=mix(vec3(.265,.27,.245),vec3(.405,.37,.30),
        clamp(caveGrain*.72+.18+medium*.18,0.0,1.0))*vGroundLight;
    color=mix(color,apronStone,caveApron*.86);
    color=mix(color,caveStone,caveFloor);
    color=applyCampfireLight(color,vWorld);
    float fog = smoothstep(260.0, 720.0, distanceToCamera);
    color = mix(color, islandAtmosphereFog, fog);
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body,
                                               const bool canopy = false)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\nprecision highp int;\n")
                    + environment::FogShaderDefinition
                    + (canopy ? CanopyLighting::Shader : "")
                    + (canopy ? IslandEnvironmentField::InlandShader : "")
                    + (canopy ? GroundCover::Shader() : std::string())
                    + (canopy ? TerrainSurfaceFrame::Shader : "")
                    + (canopy ? ForestFloorMaterial::Shader : "")
                    + (canopy ? CampfireLighting::Shader : "") + body;
            }
            return std::string("#version 330 core\n") + environment::FogShaderDefinition
                + (canopy ? CanopyLighting::Shader : "")
                + (canopy ? IslandEnvironmentField::InlandShader : "")
                + (canopy ? GroundCover::Shader() : std::string())
                + (canopy ? TerrainSurfaceFrame::Shader : "")
                + (canopy ? ForestFloorMaterial::Shader : "")
                + (canopy ? CampfireLighting::Shader : "") + body;
        }

        [[nodiscard]] CNA::Graphics::ShaderPackageEXT CreateShaderPackage()
        {
            using CNA::Graphics::ShaderCodeEXT;
            return CNA::Graphics::ShaderPackageEXT(
                {
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "terrain/terrain.es.vert.glsl", ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "terrain/terrain.es.frag.glsl", ShaderSource(true, FragmentBody, true)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "terrain/terrain.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "terrain/terrain.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody, true)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color BlendColor(const Color& from, const Color& to, const float amount)
        {
            const float t = std::clamp(amount, 0.0f, 1.0f);
            const auto channel = [t](const std::uint8_t a, const std::uint8_t b) {
                return static_cast<int>(std::lround(static_cast<float>(a)
                                                    + (static_cast<float>(b)
                                                       - static_cast<float>(a)) * t));
            };
            return Color(channel(from.getRProperty(), to.getRProperty()),
                         channel(from.getGProperty(), to.getGProperty()),
                         channel(from.getBProperty(), to.getBProperty()), 255);
        }

        [[nodiscard]] Color TerrainColor(const TerrainSample& sample, const float light)
        {
            const float height = sample.height;
            Color base;
            if (height < 1.4f) {
                const auto sand = CoastalSandColor(height);
                base = Color(static_cast<int>(std::lround(sand[0])),
                             static_cast<int>(std::lround(sand[1])),
                             static_cast<int>(std::lround(sand[2])), 255);
            } else if (height < 2.2f) {
                base = Color(220, 196, 131, 255);
            } else if (height < 4.5f) {
                base = BlendColor(Color(204, 186, 123, 255), Color(86, 145, 72, 255),
                                  (height - 2.2f) / 2.3f);
            } else if (height < 9.0f) {
                base = Color(79, 139, 70, 255);
            } else {
                base = BlendColor(Color(79, 139, 70, 255), Color(45, 101, 53, 255),
                                  (height - 9.0f) / 14.0f);
            }

            return Color(
                static_cast<std::uint8_t>(static_cast<float>(base.getRProperty()) * light),
                static_cast<std::uint8_t>(static_cast<float>(base.getGProperty()) * light),
                static_cast<std::uint8_t>(static_cast<float>(base.getBProperty()) * light),
                static_cast<std::uint8_t>(255));
        }
    }

    TerrainRenderer::TerrainRenderer(GraphicsDevice& device, const IslandTerrain& terrain,
                                     CanopyLighting& canopyLighting,const BunkerLayout& bunker)
        : device_(device), canopyLighting_(canopyLighting),bunker_(bunker),
          fallbackEffect_(std::make_unique<BasicEffect>(device))
    {
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setTextureEnabledProperty(false);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        fallbackEffect_->setFogStartProperty(260.0f);
        fallbackEffect_->setFogEndProperty(720.0f);

        if (device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            try {
                const CNA::Graphics::ShaderPackageEXT package = CreateShaderPackage();
                materialEffect_ = std::make_unique<ShaderEffect>(device_, package);
                if (materialEffect_->IsEffectValid()) {
                    materialReady_ = true;
                    std::cout << "island-simulator: terrain portable sand/soil material shader\n"
                              << "island-simulator: terrain near-field microvariation and "
                                 "procedural interior leaf litter\n"
                              << "island-simulator: terrain close sand granulation and "
                                 "shoreline-raked ripples\n"
                              << "island-simulator: terrain wet sand uses bounded "
                                 "view-dependent sun sheen\n"
                              << "island-simulator: terrain sparse close sand pebbles and "
                                 "shell flecks\n"
                              << "island-simulator: terrain fixed-sun shading from "
                                 "placed canopy volumes\n"
                              << "island-simulator: terrain authored primary footpath material\n"
                              << "island-simulator: terrain footpath narrow irregular wear\n"
                              << "island-simulator: terrain distinct authored east/west "
                                 "clearing materials\n"
                              << "island-simulator: east clearing uses a warm dry ground cue\n"
                              << ClearingFloorDiagnostic << '\n'
                              << ForestFloorDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(ClearingFloorDiagnostic);
                    MirrorRuntimeDiagnosticToPlatformLog(ForestFloorDiagnostic);
                    std::cout << SoilReliefDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(SoilReliefDiagnostic);
                    std::cout << FallenLeafDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(FallenLeafDiagnostic);
                    std::cout << CoastalSandDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(CoastalSandDiagnostic);
                    std::cout << SandMineralDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(SandMineralDiagnostic);
                    std::cout << WetSandBandDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(WetSandBandDiagnostic);
                    std::cout << WetSandDrainageDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(WetSandDrainageDiagnostic);
                    std::cout << ReefMacroDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(ReefMacroDiagnostic);
                    std::cout << SubstrateDistanceDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(SubstrateDistanceDiagnostic);
                    std::cout << AerialGroundScaleDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(AerialGroundScaleDiagnostic);
                    std::cout << AerialMaterialContrastDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(AerialMaterialContrastDiagnostic);
                    std::cout << SlopeLightDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(SlopeLightDiagnostic);
                    std::cout << StreamBankDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(StreamBankDiagnostic);
                    std::cout << CliffMaterialDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(CliffMaterialDiagnostic);
                    std::cout << CliffMacroProjectionDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(CliffMacroProjectionDiagnostic);
                    std::cout << RidgeSkyFillDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(RidgeSkyFillDiagnostic);
                    std::cout << RidgeOutcropDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(RidgeOutcropDiagnostic);
                    std::cout << CoastalCliffFillDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(CoastalCliffFillDiagnostic);
                } else {
                    std::cerr << "island-simulator: terrain material fallback: "
                              << materialEffect_->GetCompileErrorEXT() << '\n';
                    materialEffect_.reset();
                }
            } catch (const std::exception& exception) {
                std::cerr << "island-simulator: terrain material fallback: "
                          << exception.what() << '\n';
                materialEffect_.reset();
            }
        } else {
            std::cerr << "island-simulator: terrain material fallback: custom effects unavailable\n";
        }

        if (materialReady_) forestFloor_ = std::make_unique<ForestFloorMaterial>(device_);
        CreateMesh(terrain);
    }

    TerrainRenderer::~TerrainRenderer() = default;

    void TerrainRenderer::CreateMesh(const IslandTerrain& terrain)
    {
        const auto original=std::make_unique<RenderedTerrainSurface>(terrain);
        streamBed_=std::make_unique<StreamBedPatch>(StreamCourse(terrain),*original);
        cave_=std::make_unique<CaveLayout>(terrain);
        caveGround_=std::make_unique<CaveGroundPatch>(*cave_,*original);
        cells_.clear();
        cells_.reserve(TerrainMeshLayout::CellCount);
        std::vector<TerrainMaterialVertex> vertices;
        std::vector<std::uint16_t> indices;
        vertices.reserve(static_cast<std::size_t>(TerrainMeshLayout::Resolution
                                                  * TerrainMeshLayout::Resolution));
        indices.reserve(static_cast<std::size_t>(TerrainMeshLayout::QuadsPerAxis
                                                 * TerrainMeshLayout::QuadsPerAxis * 6));

        for (int gridRow = 0; gridRow < TerrainMeshLayout::Resolution; ++gridRow) {
            const float z = TerrainMeshLayout::Coordinate(gridRow);
            for (int gridColumn = 0; gridColumn < TerrainMeshLayout::Resolution;
                 ++gridColumn) {
                const float x = TerrainMeshLayout::Coordinate(gridColumn);
                const TerrainSample sample = terrain.Sample(x, z);
                const float light = TerrainDaylight(sample.normal);
                const Color color = TerrainColor(sample, light);
                vertices.push_back({{x, sample.height, z},
                                    {color.getRProperty(), color.getGProperty(), color.getBProperty(), 255},
                                    {sample.signedShoreDistance, sample.slopeRadians, light}});
            }
        }

        for (std::size_t cellIndex = 0; cellIndex < TerrainMeshLayout::CellCount; ++cellIndex) {
            const TerrainCellLayout layout = TerrainMeshLayout::Cell(cellIndex);
            float minimumHeight = std::numeric_limits<float>::infinity();
            float maximumHeight = -std::numeric_limits<float>::infinity();

            for (int localRow = 0; localRow <= layout.quadRows; ++localRow) {
                for (int localColumn = 0; localColumn <= layout.quadColumns; ++localColumn) {
                    const int gridRow = layout.firstGridRow + localRow;
                    const int gridColumn = layout.firstGridColumn + localColumn;
                    const std::size_t vertexIndex = static_cast<std::size_t>(
                        gridRow * TerrainMeshLayout::Resolution + gridColumn);
                    const float height = vertices[vertexIndex].position[1];
                    minimumHeight = std::min(minimumHeight, height);
                    maximumHeight = std::max(maximumHeight, height);
                }
            }

            RenderCell cell;
            cell.firstIndex = static_cast<int>(indices.size());
            for (int row = 0; row < layout.quadRows; ++row) {
                for (int column = 0; column < layout.quadColumns; ++column) {
                    const float midX=(TerrainMeshLayout::Coordinate(layout.firstGridColumn+column)
                        +TerrainMeshLayout::Coordinate(layout.firstGridColumn+column+1))*.5f;
                    const float midZ=(TerrainMeshLayout::Coordinate(layout.firstGridRow+row)
                        +TerrainMeshLayout::Coordinate(layout.firstGridRow+row+1))*.5f;
                    const float midY=terrain.Height(midX,midZ);
                    if(streamBed_->Contains(midX,midZ)||caveGround_->Contains(midX,midZ)
                        ||(!materialReady_&&bunker_.HidesTerrain(midX,midY,midZ)))continue;
                    const auto topLeft = static_cast<std::uint16_t>(
                        (layout.firstGridRow + row) * TerrainMeshLayout::Resolution
                        + layout.firstGridColumn + column);
                    const auto topRight = static_cast<std::uint16_t>(topLeft + 1U);
                    const auto bottomLeft = static_cast<std::uint16_t>(
                        topLeft + TerrainMeshLayout::Resolution);
                    const auto bottomRight = static_cast<std::uint16_t>(bottomLeft + 1U);
                    indices.insert(indices.end(), {topLeft, bottomLeft, topRight,
                                                   topRight, bottomLeft, bottomRight});
                }
            }

            cell.primitiveCount = (static_cast<int>(indices.size())-cell.firstIndex)/3;
            cell.bounds = BoundingBox(
                Vector3(layout.minimumX, minimumHeight - 0.1f, layout.minimumZ),
                Vector3(layout.maximumX, maximumHeight + 0.1f, layout.maximumZ));
            cells_.push_back(std::move(cell));
        }

        ValidateProceduralTriangleMeshOrThrow(vertices.size(), indices, "terrain");
        vertexCount_ = static_cast<int>(vertices.size());
        primitiveCount_ = static_cast<int>(indices.size() / 3U);
        if (TerrainDeclaration().getVertexStrideProperty() != static_cast<int>(sizeof(TerrainMaterialVertex))) {
            throw std::logic_error("Terrain vertex declaration does not match upload stride");
        }
        vertices_ = std::make_unique<VertexBuffer>(
            device_, TerrainDeclaration(),
            vertexCount_, BufferUsage::WriteOnly);
        vertices_->SetData(vertices.data(), vertexCount_);
        indices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(indices.size()),
            BufferUsage::WriteOnly);
        indices_->SetData(indices.data(), static_cast<int>(indices.size()));
        vertices.clear();
        Vector3 minimum(1000,1000,1000),maximum(-1000,-1000,-1000);
        for(const auto p:streamBed_->Vertices()) {
            auto sample=terrain.Sample(p.x,p.z);sample.height=p.y;
            const auto height=[&](float x,float z){return streamBed_->Contains(x,z)
                ?streamBed_->SampleHeight(x,z):original->Sample(x,z).height;};
            sample.normal=TerrainSurfaceFrame::Normalize({height(p.x-.1f,p.z)-height(p.x+.1f,p.z),
                .2f,height(p.x,p.z-.1f)-height(p.x,p.z+.1f)});
            sample.slopeRadians=std::acos(std::clamp(sample.normal.y,-1.0f,1.0f));
            const float light=TerrainDaylight(sample.normal);const auto color=TerrainColor(sample,light);
            vertices.push_back({{p.x,p.y,p.z},{color.getRProperty(),color.getGProperty(),color.getBProperty(),255},
                {sample.signedShoreDistance,sample.slopeRadians,light}});
            minimum=Vector3::Min(minimum,Vector3(p.x,p.y,p.z));
            maximum=Vector3::Max(maximum,Vector3(p.x,p.y,p.z));
        }
        const auto& streamIndices=streamBed_->Indices();
        ValidateProceduralTriangleMeshOrThrow(vertices.size(),streamIndices,"stream bed");
        streamVertexCount_=static_cast<int>(vertices.size());streamPrimitiveCount_=static_cast<int>(streamIndices.size()/3);
        streamBounds_=BoundingBox(minimum-Vector3(.1f,.1f,.1f),maximum+Vector3(.1f,.1f,.1f));
        streamVertices_=std::make_unique<VertexBuffer>(device_,TerrainDeclaration(),streamVertexCount_,BufferUsage::WriteOnly);
        streamVertices_->SetData(vertices.data(),streamVertexCount_);
        streamIndices_=std::make_unique<IndexBuffer>(device_,IndexElementSize::SixteenBits,
            static_cast<int>(streamIndices.size()),BufferUsage::WriteOnly);
        streamIndices_->SetData(streamIndices.data(),static_cast<int>(streamIndices.size()));
        std::cout << "island-simulator: stream bed replacement " << streamVertexCount_ << " vertices, "
            << streamPrimitiveCount_ << " triangles; original covered terrain removed\n";
        vertices.clear();minimum=Vector3(1000,1000,1000);maximum=Vector3(-1000,-1000,-1000);
        for(const auto p:caveGround_->Vertices()) {
            auto sample=terrain.Sample(p.x,p.z);sample.height=p.y;
            const auto height=[&](float x,float z){return caveGround_->Contains(x,z)
                ?caveGround_->SampleHeight(x,z):original->Sample(x,z).height;};
            sample.normal=TerrainSurfaceFrame::Normalize({height(p.x-.1f,p.z)-height(p.x+.1f,p.z),
                .2f,height(p.x,p.z-.1f)-height(p.x,p.z+.1f)});
            sample.slopeRadians=std::acos(std::clamp(sample.normal.y,-1.0f,1.0f));
            const float light=TerrainDaylight(sample.normal);const auto color=TerrainColor(sample,light);
            vertices.push_back({{p.x,p.y,p.z},{color.getRProperty(),color.getGProperty(),color.getBProperty(),255},
                {sample.signedShoreDistance,sample.slopeRadians,light}});
            minimum=Vector3::Min(minimum,Vector3(p.x,p.y,p.z));maximum=Vector3::Max(maximum,Vector3(p.x,p.y,p.z));
        }
        const auto caveIndices=caveGround_->Indices();
        ValidateProceduralTriangleMeshOrThrow(vertices.size(),caveIndices,"cave ground");
        caveVertexCount_=static_cast<int>(vertices.size());cavePrimitiveCount_=static_cast<int>(caveIndices.size()/3);
        caveBounds_=BoundingBox(minimum-Vector3(.1f,.1f,.1f),maximum+Vector3(.1f,.1f,.1f));
        caveVertices_=std::make_unique<VertexBuffer>(device_,TerrainDeclaration(),caveVertexCount_,BufferUsage::WriteOnly);
        caveVertices_->SetData(vertices.data(),caveVertexCount_);
        caveIndices_=std::make_unique<IndexBuffer>(device_,IndexElementSize::SixteenBits,
            static_cast<int>(caveIndices.size()),BufferUsage::WriteOnly);
        caveIndices_->SetData(caveIndices.data(),static_cast<int>(caveIndices.size()));
        std::cout<<"island-simulator: cave ground replacement "<<caveVertexCount_<<" vertices, "
            <<cavePrimitiveCount_<<" triangles; original covered terrain removed\n";
        const char* caveApron="island-simulator: cave terrain shoulders use an irregular rock apron around the open corridor";
        std::cout<<caveApron<<'\n';MirrorRuntimeDiagnosticToPlatformLog(caveApron);
    }

    void TerrainRenderer::DrawGeometry(const int firstIndex, const int primitiveCount)
    {
        device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_,
                                      firstIndex, primitiveCount);
    }

    void TerrainRenderer::Draw(const Matrix& view, const Matrix& projection,
                               const Vector3& cameraPosition,float seconds)
    {
        const BoundingFrustum frustum(view * projection);
        if (materialReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            materialEffect_->Apply();
            CampfireLighting::Bind(*materialEffect_,seconds);
            canopyLighting_.Bind(*materialEffect_);
            forestFloor_->Bind(*materialEffect_);
            materialEffect_->SetUniformMat4("uViewProjection", viewProjection);
            const auto entrance=cave_->Entrance(),forward=cave_->World(0,0,1);
            materialEffect_->SetUniformVec3("uCaveEntrance",entrance.x,entrance.y,entrance.z);
            materialEffect_->SetUniformVec2("uCaveForward",forward.x-entrance.x,forward.z-entrance.z);
            const auto bunkerEntrance=bunker_.Entrance();
            const auto bunkerAcross=bunker_.World(1,0,0);
            const auto bunkerDepth=bunker_.World(0,0,1);
            materialEffect_->SetUniformVec3("uBunkerEntrance",bunkerEntrance.x,
                                            bunkerEntrance.y,bunkerEntrance.z);
            materialEffect_->SetUniformVec4("uBunkerFrame",
                bunkerAcross.x-bunkerEntrance.x,bunkerAcross.z-bunkerEntrance.z,
                bunkerDepth.x-bunkerEntrance.x,bunkerDepth.z-bunkerEntrance.z);
            materialEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                            cameraPosition.Y, cameraPosition.Z);
            materialEffect_->SetUniformVec3(
                "uToSun", environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ);
            materialEffect_->SetUniformVec2("uSandWind", wind::DirectionX, wind::DirectionZ);
            materialEffect_->SetUniformVec4(
                "uPrimaryPathShape", VegetationLayout::PrimaryPathBaseX,
                VegetationLayout::PrimaryPathAmplitude,
                VegetationLayout::PrimaryPathZOffset,
                VegetationLayout::PrimaryPathFrequency);
            materialEffect_->SetUniformVec2(
                "uPrimaryPathZRange", VegetationLayout::PrimaryPathMinimumZ,
                VegetationLayout::PrimaryPathMaximumZ);
            materialEffect_->SetUniformVec3(
                "uEastClearing", VegetationLayout::EastClearing.x,
                VegetationLayout::EastClearing.z, VegetationLayout::EastClearing.radius);
            materialEffect_->SetUniformVec3(
                "uWestClearing", VegetationLayout::WestClearing.x,
                VegetationLayout::WestClearing.z, VegetationLayout::WestClearing.radius);
        } else {
            fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        std::array<bool, TerrainMeshLayout::CellCount> visible{};
        visibleCellCount_ = 0;
        for (std::size_t cellIndex = 0; cellIndex < cells_.size(); ++cellIndex) {
            visible[cellIndex] = frustum.Intersects(cells_[cellIndex].bounds);
            visibleCellCount_ += visible[cellIndex] ? 1 : 0;
        }

        const TerrainMeshLayout::CellRuns runs =
            TerrainMeshLayout::CoalesceVisibleCells(visible);
        drawCallCount_ = 0;
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(indices_.get());
        const auto drawRuns = [&]() {
            device_.SetVertexBuffer(vertices_.get());device_.setIndicesProperty(indices_.get());
            for (std::size_t runIndex = 0; runIndex < runs.count; ++runIndex) {
                const TerrainMeshLayout::CellRun& run = runs.values[runIndex];
                const RenderCell& firstCell = cells_[run.firstCell];
                int runPrimitiveCount = 0;
                for (std::size_t offset = 0; offset < run.cellCount; ++offset) {
                    runPrimitiveCount += cells_[run.firstCell + offset].primitiveCount;
                }
                DrawGeometry(firstCell.firstIndex, runPrimitiveCount);
                ++drawCallCount_;
            }
            if(frustum.Intersects(streamBounds_)) {
                device_.SetVertexBuffer(streamVertices_.get());device_.setIndicesProperty(streamIndices_.get());
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList,0,0,streamVertexCount_,0,streamPrimitiveCount_);
                ++drawCallCount_;
            }
            if(frustum.Intersects(caveBounds_)) {
                device_.SetVertexBuffer(caveVertices_.get());device_.setIndicesProperty(caveIndices_.get());
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList,0,0,caveVertexCount_,0,cavePrimitiveCount_);
                ++drawCallCount_;
            }
        };
        if (materialReady_) {
            drawRuns();
        } else {
            for (auto& pass : fallbackEffect_->getCurrentTechniqueProperty()
                                  ->getPassesProperty()) {
                pass.Apply();
                drawRuns();
            }
        }
    }
}
