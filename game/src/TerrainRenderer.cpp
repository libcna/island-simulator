#include "IslandSimulator/TerrainRenderer.hpp"
#include "IslandSimulator/CoastalMaterial.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/CanopyLighting.hpp"
#include "IslandSimulator/ForestFloorMaterial.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/TerrainMaterialVertex.hpp"
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
        constexpr std::string_view SubstrateDistanceDiagnostic =
            "island-simulator: terrain substrate persists at distance with footprint-filtered noise";
        constexpr std::string_view SlopeLightDiagnostic =
            "island-simulator: terrain palette overrides preserve smooth slope daylight; opaque 28-byte vertices";

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

uniform vec3 uCameraPosition;
uniform vec3 uToSun;
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
    float detailFade = 1.0 - smoothstep(95.0, 285.0, distanceToCamera);

    // Substrate is a world property: never fade humus, clearings or paths back to
    // the legacy green vertex palette as the camera retreats. Only unresolved
    // frequencies fade. Footprint is in metres/pixel, frequency in cells/metre.
    float broad = valueNoise(p * 0.060)
        * (1.0 - smoothstep(0.18, 0.50, groundPixelWidth * 0.060));
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
    float sand = (1.0 - smoothstep(7.0, 19.0, signedShoreDistance))
        * smoothstep(-6.0, -0.2, vWorld.y);
    float interior = smoothstep(8.0, 25.0, signedShoreDistance);
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
    float sandVariation = (broad * 0.190 + medium * 0.125 + fine * 0.026)
        * sand * detailFade;
    color *= vec3(1.0 + sandVariation * 0.92,
                  1.0 + sandVariation * 0.72,
                  1.0 + sandVariation * 0.43);
    color += vec3(0.043, 0.026, -0.011) * medium * sand * detailFade * vGroundLight;

    float closeSand = sand * (1.0 - smoothstep(28.0, 92.0, distanceToCamera));
    if (closeSand > 0.0) {
        float sandGranulation = valueNoise(p * 1.18 + vec2(4.7, -12.9));
        float grain = (sandGranulation * 0.064 + fine * 0.012) * closeSand;
        color *= vec3(1.0 + grain * 0.90,
                      1.0 + grain * 0.72,
                      1.0 + grain * 0.46);

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
        float meadow = smoothstep(-0.16, 0.17, meadowField + medium * 0.22)
            * smoothstep(0.45, 0.85, bedDepth)
            * (1.0 - smoothstep(1.8, 2.6, bedDepth));
        float reefZone = smoothstep(24.0, 36.0, offshore)
            * (1.0 - smoothstep(72.0, 88.0, offshore));
        float reefCrest = smoothstep(66.0, 73.0, offshore + meadowField * 12.0)
            * (1.0 - smoothstep(78.0, 86.0, offshore + meadowField * 12.0));
        float reefCover = smoothstep(-0.10, 0.19, reefField + meadowField * 0.38)
            * max(reefZone * 0.88, reefCrest);
        float habitatCoverage = submergedHabitat * (1.0 - sandChannel);
        vec3 grassBed = vec3(0.22, 0.265, 0.105)
            * (1.0 + meadowField * 0.28 + medium * 0.20) * vGroundLight;
        vec3 reefBed = mix(vec3(0.29, 0.255, 0.145), vec3(0.49, 0.405, 0.24),
                           clamp(0.5 + reefField + medium * 0.65, 0.0, 1.0))
            * vGroundLight;
        color = mix(color, grassBed, meadow * habitatCoverage * 0.86);
        color = mix(color, reefBed, reefCover * habitatCoverage);
    }

    float groundVariation = (broad * 0.185 + medium * 0.120 + fine * 0.024)
        * interior;
    color *= vec3(1.0 + groundVariation * 0.55,
                  1.0 + groundVariation,
                  1.0 + groundVariation * 0.48);
    float exposedSoil = smoothstep(0.16, 0.40, broad * 0.62 + medium * 0.38)
        * interior;
    color = mix(color, color * vec3(0.82, 0.76, 0.61), exposedSoil * 0.18);
    color = mix(color, color * vec3(0.82, 0.78, 0.68), steep * 0.24);

    // Break up the broad green inland floor with restrained, material-specific
    // humus and moss fields. These broad material fields also remain beyond 285 m;
    // close clods and individual fallen leaves retain their separate detail fades.
    float forestFloorMask = smoothstep(24.0, 54.0, signedShoreDistance)
        * (1.0 - steep)
        * (1.0 - max(eastClearingMask, westClearingMask) * 0.72);
    float forestMaterialField = broad * 0.70 + medium * 0.45 + fine * 0.035;
    // Shared with grass placement: sunlit, patch-rich ground carries a living turf layer,
    // shaded or patch-poor ground keeps exposed humus and fallen leaves.
    vec2 groundField = canopyGroundField(p);
    float turfAmount = groundCoverDensity(groundField.x, groundField.y,
        signedShoreDistance, slopeRadians) * 0.86 * (1.0 - steep);
    // Tropical understory grows through exposed humus; it is not a continuous green lawn.
    // Keep grass/moss islands, and leave the authored sunny clearing and beach palettes intact.
    color = mix(color, vec3(0.300, 0.223, 0.139) * vGroundLight, forestFloorMask * 0.82);
    float leafHumus = smoothstep(-0.015, 0.245, forestMaterialField)
        * forestFloorMask;
    float coolMoss = (1.0 - smoothstep(-0.245, -0.075, forestMaterialField))
        * forestFloorMask;
    color = mix(color, vec3(0.205, 0.151, 0.091) * vGroundLight, leafHumus * 0.54);
    color = mix(color, vec3(0.185, 0.265, 0.115) * vGroundLight, coolMoss * 0.64);

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
    float turfDryness = clamp(0.30 + broad * 0.85 + medium * 0.35
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
        float pathWidth = 1.52 + (broad + 0.5) * 0.64;
        float brokenLateralDistance = abs(lateralOffset)
            + medium * 0.48 + fine * 0.095;
        float pathEdgeWidth = max(0.52, groundPixelWidth);
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
            vec3 faceVector = cross(worldDx, worldDy);
            vec3 surfaceNormal = faceVector
                * inversesqrt(max(dot(faceVector, faceVector), 0.00000001));
            if (surfaceNormal.y < 0.0) surfaceNormal = -surfaceNormal;
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

    float wetHeight = vWorld.y + broad * 0.06 + medium * 0.025;
    float wetEdge = (1.0 - smoothstep(0.05, 0.42, wetHeight))
        * smoothstep(-0.12, 0.06, vWorld.y) * sand;
    color = mix(color, color * vec3(0.76, 0.81, 0.78), wetEdge * 0.42);
    if (wetEdge > 0.001) {
        vec3 faceVector = cross(worldDx, worldDy);
        vec3 wetNormal = faceVector
            * inversesqrt(max(dot(faceVector, faceVector), 0.00000001));
        if (wetNormal.y < 0.0) {
            wetNormal = -wetNormal;
        }
        vec3 toView = normalize(uCameraPosition - vWorld);
        vec3 halfVector = normalize(uToSun) + toView;
        vec3 halfDirection = halfVector
            * inversesqrt(max(dot(halfVector, halfVector), 0.00000001));
        float sunSheen = pow(max(dot(wetNormal, halfDirection), 0.0), 28.0);
        float grazingSheen = pow(1.0 - max(dot(wetNormal, toView), 0.0), 3.0);
        float wetSheen = wetEdge * (sunSheen * 0.42 + grazingSheen * 0.035);
        color += vec3(0.13, 0.17, 0.16) * wetSheen;
    }

    // Same texel as the cover sample above; retains cool skylight in canopy shade.
    color *= mix(uCanopyAmbient, vec3(1.0), groundField.x);
    float fog = smoothstep(260.0, 720.0, distanceToCamera);
    color = mix(color, vec3(0.53, 0.78, 0.88), fog);
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body,
                                               const bool canopy = false)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\nprecision highp int;\n")
                    + (canopy ? CanopyLighting::Shader : "")
                    + (canopy ? GroundCover::Shader() : std::string())
                    + (canopy ? ForestFloorMaterial::Shader : "") + body;
            }
            return std::string("#version 330 core\n") + (canopy ? CanopyLighting::Shader : "")
                + (canopy ? GroundCover::Shader() : std::string())
                + (canopy ? ForestFloorMaterial::Shader : "") + body;
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
                                     CanopyLighting& canopyLighting)
        : device_(device), canopyLighting_(canopyLighting),
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
                    std::cout << SubstrateDistanceDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(SubstrateDistanceDiagnostic);
                    std::cout << SlopeLightDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(SlopeLightDiagnostic);
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

            cell.primitiveCount = layout.quadColumns * layout.quadRows * 2;
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
    }

    void TerrainRenderer::DrawGeometry(const int firstIndex, const int primitiveCount)
    {
        device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_,
                                      firstIndex, primitiveCount);
    }

    void TerrainRenderer::Draw(const Matrix& view, const Matrix& projection,
                               const Vector3& cameraPosition)
    {
        const BoundingFrustum frustum(view * projection);
        if (materialReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            materialEffect_->Apply();
            canopyLighting_.Bind(*materialEffect_);
            forestFloor_->Bind(*materialEffect_);
            materialEffect_->SetUniformMat4("uViewProjection", viewProjection);
            materialEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                            cameraPosition.Y, cameraPosition.Z);
            materialEffect_->SetUniformVec3(
                "uToSun", environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ);
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
