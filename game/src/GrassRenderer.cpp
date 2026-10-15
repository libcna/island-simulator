#include "IslandSimulator/GrassRenderer.hpp"
#include "IslandSimulator/CampfireLighting.hpp"
#include "IslandSimulator/GpuDiagnostics.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/CanopyLighting.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GrassBladeProfile.hpp"
#include "IslandSimulator/GrassMotionBounds.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/RenderedTerrainHeightMap.hpp"
#include "IslandSimulator/GrassLod.hpp"
#include "IslandSimulator/GrassMaterial.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/SurfaceNormals.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBufferBinding.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElement.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int NearBladeCount = NearGrassBladeCount;
        constexpr int MiddleBladeCount = MiddleGrassBladeCount;
        constexpr int FarBladeCount = FarGrassBladeCount;
        constexpr int NearBladeLevels = 5;
        constexpr int MiddleBladeLevels = 3;
        constexpr int FarBladeLevels = 3;
        constexpr const char* NaturalBladeDiagnostic =
            "island-simulator: grass near blades use asymmetric edges and occasional bent tips";
        constexpr const char* PlantedBladeDiagnostic =
            "island-simulator: close grass uses broadened planted bases and lower-blade curvature";
        constexpr const char* MaterialContinuityDiagnostic =
            "island-simulator: grass retains foliage colour at distance with normalized-height basal tint";
        constexpr const char* SurfaceMaterialDiagnostic =
            "island-simulator: grass pigment uses separate height occlusion and filtered longitudinal veins";
        constexpr const char* SharedLodDiagnostic =
            "island-simulator: grass LOD proxies share seven tuft roots, profiles and wind phases";
        constexpr const char* DistanceFadeDiagnostic =
            "island-simulator: grass anchor-distance contraction uses nearest-bound cell culling";
        constexpr const char* FootprintVariationDiagnostic =
            "island-simulator: grass instances use area-preserving asymmetric tuft footprints";

        struct GrassInstanceData
        {
            float x;
            float y;
            float z;
            float yaw;
            float heightScale;
            float spreadScale;
            float windPhase;
            float colorVariation;
            float normalX;
            float normalZ;
            float biomeTint;
        };

        static_assert(sizeof(GrassInstanceData) == 44U);

        struct GrassSurfaceVertex
        {
            std::array<float, 3> position;
            std::array<std::uint8_t, 4> color;
            std::array<float, 2> wind;
            std::array<float, 3> normal;
            float across;
            std::array<float,2> root;
        };
        static_assert(std::is_trivially_copyable_v<GrassSurfaceVertex>);
        static_assert(std::is_standard_layout_v<GrassSurfaceVertex> && sizeof(GrassSurfaceVertex) == 48);

        const VertexDeclaration& GrassSurfaceDeclaration()
        {
            static const VertexDeclaration declaration{
                VertexElement(offsetof(GrassSurfaceVertex, position), VertexElementFormat::Vector3,
                              VertexElementUsage::Position, 0),
                VertexElement(offsetof(GrassSurfaceVertex, color), VertexElementFormat::Color,
                              VertexElementUsage::Color, 0),
                VertexElement(offsetof(GrassSurfaceVertex, wind), VertexElementFormat::Vector2,
                              VertexElementUsage::TextureCoordinate, 0),
                VertexElement(offsetof(GrassSurfaceVertex, normal), VertexElementFormat::Vector3,
                              VertexElementUsage::Normal, 0),
                VertexElement(offsetof(GrassSurfaceVertex, across), VertexElementFormat::Single,
                              VertexElementUsage::TextureCoordinate, 5),
                VertexElement(offsetof(GrassSurfaceVertex, root), VertexElementFormat::Vector2,
                              VertexElementUsage::TextureCoordinate, 6)};
            return declaration;
        }

        struct ClusterGeometry
        {
            std::vector<VertexPositionColorTexture> vertices;
            std::vector<std::uint16_t> indices;
            std::vector<float> materialHeights;
            std::vector<float> bladeAcross; // [-1,1] on blades; 2 on seed heads.
            std::vector<std::array<float,2>> bladeRoots;
        };

        [[nodiscard]] constexpr bool HasNearSeedHead(const GrassMeshTier tier,
                                                     const int blade) noexcept
        {
            return tier == GrassMeshTier::Near && ((blade * 7 + 3) % 17) < 2;
        }

        [[nodiscard]] constexpr bool HasNearVFold(const GrassMeshTier tier,
                                                  const int blade) noexcept
        {
            return tier == GrassMeshTier::Near && blade % 3 == 0;
        }

        struct CellBuildEntry
        {
            GrassInstanceData instance;
            Matrix fallbackTransform;
            std::uint32_t lodOrder = 0U;
        };

        [[nodiscard]] std::uint32_t GrassLodOrder(std::uint32_t index) noexcept
        {
            index ^= index >> 16U;
            index *= 0x7feb352dU;
            index ^= index >> 15U;
            index *= 0x846ca68bU;
            return index ^ (index >> 16U);
        }

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aWindData;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in float aBladeAcross;
layout(location = 5) in vec2 aBladeRoot;
layout(location = 6) in vec4 aPlacement;
layout(location = 7) in vec4 aVariation;
layout(location = 8) in vec2 aSlope;
layout(location = 9) in float aBiomeTint;

uniform mat4 uViewProjection;
uniform float uTime;
uniform vec3 uCameraPosition;
uniform vec2 uGrassFadeRange;
uniform vec2 uPlayerPosition;
uniform float uPlayerInteractionStrength;

out vec3 vColor;
out vec3 vWorld;
out vec3 vBladeNormal;
out float vBladeHeight;
out float vGroundHeight;
out float vMaterialHeight;
out vec2 vBladeSurface;

void main()
{
    float normalY = sqrt(max(grassNormalFloor, 1.0 - dot(aSlope, aSlope)));
    vec3 surfaceNormal = normalize(vec3(aSlope.x, normalY, aSlope.y));
    vec3 tangentX = normalize(vec3(surfaceNormal.y, -surfaceNormal.x, 0.0));
    vec3 tangentZ = normalize(cross(tangentX, surfaceNormal));
    float yawCos = cos(aPlacement.w);
    float yawSin = sin(aPlacement.w);
    float footprintHash = fract(sin(dot(aBladeRoot, vec2(12.9898, 78.233))
                                    + aVariation.w * 37.719) * 43758.5453);
    float footprintAngle = (aVariation.z + footprintHash * 0.38196601125)
        * 6.28318530718;
    vec2 footprintAxis = vec2(cos(footprintAngle), sin(footprintAngle));
    vec2 footprintPerpendicular = vec2(-footprintAxis.y, footprintAxis.x);
    float footprintAxisScale = mix(grassMinimumFootprintAxisScale,
                                   grassMaximumFootprintAxisScale, footprintHash);
    float footprintPerpendicularScale = 1.0 / footprintAxisScale;
    vec2 localPlanar = footprintAxis
            * dot(aPosition.xz, footprintAxis) * footprintAxisScale
        + footprintPerpendicular
            * dot(aPosition.xz, footprintPerpendicular) * footprintPerpendicularScale;
    float localX = localPlanar.x * yawCos - localPlanar.y * yawSin;
    float localZ = localPlanar.x * yawSin + localPlanar.y * yawCos;
    float bladeRandom = fract(sin(aWindData.y * 91.73 + aVariation.w * 37.21 + 0.618)
                              * 43758.5453);
    float bladeStature = mix(grassMinimumStature, grassMaximumStature, bladeRandom);
    float visibility = 1.0 - smoothstep(uGrassFadeRange.x, uGrassFadeRange.y,
                                       length(aPlacement.xz - uCameraPosition.xz));
    vec3 base = aPlacement.xyz
        + tangentX * (localX * aVariation.y * visibility)
        + tangentZ * (localZ * aVariation.y * visibility);
    // Drape the undeformed footprint onto the same triangles the terrain actually draws.
    // Root vertices have zero local height/interaction/wind, so they remain planted exactly.
    base.y = grassTerrainHeight(base.xz);
    vec3 world = base + surfaceNormal * (aPosition.y * aVariation.x * bladeStature * visibility);

    float heightWeight = pow(clamp(aWindData.x, 0.0, 1.0), grassHeightExponent);
    vec2 playerOffset = world.xz - uPlayerPosition;
    float playerDistanceSquared = dot(playerOffset, playerOffset);
    float interactionStrength = clamp(uPlayerInteractionStrength, 0.0, 1.0);
    float interactionRadiusSquared = mix(0.3025, 1.8225, interactionStrength);
    if (playerDistanceSquared < interactionRadiusSquared) {
        float interaction = (1.0 - smoothstep(0.09, interactionRadiusSquared,
                                               playerDistanceSquared))
            * heightWeight * visibility;
        vec2 awayFromPlayer = playerOffset
            * inversesqrt(max(playerDistanceSquared, 0.01));
        float pushDistance = mix(grassMinimumPush, grassMaximumPush, interactionStrength);
        float lowering = mix(grassMinimumLowering, grassMaximumLowering, interactionStrength);
        world.xz += awayFromPlayer * interaction * pushDistance * aVariation.x;
        world.y -= interaction * lowering * aVariation.x;
    }

    float phase = (aWindData.y + aVariation.z) * 6.28318530718;
    float broad = sin(uTime * 0.86 + phase + world.x * 0.031 + world.z * 0.024);
    float detail = sin(uTime * 1.37 + phase * 1.73 - world.x * 0.071 + world.z * 0.063);
    float sway = (broad * grassBroadWind + detail * grassDetailWind) * heightWeight * aVariation.x
        * mix(grassMinimumWindGain, grassMaximumWindGain, bladeRandom) * visibility;
    world.x += sway;
    world.z += sway * grassWindZ;
    world.y -= abs(sway) * grassWindLowering * heightWeight;

    // Inverse transpose of the rest-shape scale, yaw and terrain basis. Shared mesh normals
    // follow curvature/folds instead of assigning the same artificial tilt to every level.
    // Wind/player deformation of normals is still approximate; positions retain full motion.
    vec2 localPlanarNormal = footprintAxis
            * dot(aNormal.xz, footprintAxis) / footprintAxisScale
        + footprintPerpendicular
            * dot(aNormal.xz, footprintPerpendicular) / footprintPerpendicularScale;
    vec3 scaledNormal = vec3(localPlanarNormal.x / aVariation.y,
                            aNormal.y / (aVariation.x * bladeStature),
                            localPlanarNormal.y / aVariation.y);
    float bladeNormalX = scaledNormal.x * yawCos - scaledNormal.z * yawSin;
    float bladeNormalZ = scaledNormal.x * yawSin + scaledNormal.z * yawCos;
    vBladeNormal = normalize(tangentX * bladeNormalX + tangentZ * bladeNormalZ
                             + surfaceNormal * scaledNormal.y);
    float coolWeight = clamp((0.5 - aBiomeTint) * 2.0, 0.0, 1.0);
    float warmWeight = clamp((aBiomeTint - 0.5) * 2.0, 0.0, 1.0);
    vec3 biomeColor = mix(vec3(1.0), vec3(0.82, 0.98, 0.91), coolWeight);
    biomeColor = mix(biomeColor, vec3(1.19, 1.045, 0.73), warmWeight);
    float colorShift = (aVariation.w - 0.5) * 0.12 + (bladeRandom - 0.5) * 0.075;
    vColor = aColor.rgb * biomeColor
        * vec3(1.0 + colorShift * 0.55,
               1.0 + colorShift,
               1.0 + colorShift * 0.42);
    vWorld = world;
    vBladeHeight = clamp(aWindData.x, 0.0, 1.0);
    vGroundHeight = base.y;
    vMaterialHeight = aColor.a;
    vBladeSurface = vec2(aBladeAcross, aWindData.y);
    gl_Position = uViewProjection * vec4(world, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;
in vec3 vBladeNormal;
in float vBladeHeight;
in float vGroundHeight;
in float vMaterialHeight;
in vec2 vBladeSurface;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    vec3 normal = normalize(vBladeNormal);
    vec3 toSun = normalize(uToSun);
    vec3 toCamera = normalize(uCameraPosition - vWorld);
    float twoSidedDiffuse = abs(dot(normal, toSun));
    float rootLight = mix(grassRootOcclusion, 1.0, smoothstep(0.0, 1.0, vMaterialHeight));
    float edgeLight = pow(1.0 - abs(dot(normal, toCamera)), 2.0) * 0.06;
    float transmission = pow(max(dot(-toCamera, toSun), 0.0), 5.0)
        * smoothstep(0.25, 0.92, vBladeHeight) * 0.10;
    float light = rootLight * (0.78 + twoSidedDiffuse * 0.30 + edgeLight + transmission);
    // Leaves stay leaves at every LOD. The visible terrain supplies the substrate between
    // blades; a synthetic green "ground" fade would overwrite both material and lighting.
    float basalWeight = 1.0 - smoothstep(0.0, grassBasalEnd, vMaterialHeight);
    // Evaluate derivatives before the seed-head branch. Unresolved veins and then the
    // wider midrib/edge converge to neutral modulation instead of shimmering at distance.
    float footprint = fwidth(vBladeSurface.x);
    float resolved = 1.0 - smoothstep(0.12, 0.5, footprint);
    float veinResolved = 1.0 - smoothstep(0.2, 0.5, footprint * grassGrainCycles);
    float surface = 1.0;
    if (abs(vBladeSurface.x) < 1.5) {
        float veins = cos((vBladeSurface.x * grassGrainCycles + vBladeSurface.y) * 6.28318530718);
        float midrib = 1.0 - smoothstep(0.015, 0.13, abs(vBladeSurface.x));
        float edge = smoothstep(0.7, 1.0, abs(vBladeSurface.x));
        surface += veins * grassGrainStrength * veinResolved
            + (midrib * grassMidribStrength - edge * grassEdgeStrength) * resolved;
    }
    vec3 color = vColor * (vec3(1.0) + grassBasalShift * basalWeight) * light * surface;
    // Map elevated blades back along the fixed sun ray to the ground transmission field.
    vec2 receiver = canopyGroundReceiver(vWorld, vGroundHeight, uToSun);
    color = applyCanopyLight(color, receiver);
    color = applyCampfireLight(color,vWorld);
    color = mix(color, islandAtmosphereFog, smoothstep(260.0, 720.0, distanceToCamera));
    fragColor = vec4(color, 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body,
                                               const bool canopy = false)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\n")
                    + environment::FogShaderDefinition
                    + GrassMotion::ShaderConstants()
                    + GrassFootprintShaderConstants()
                    + GrassMaterialShaderConstants()
                    + (canopy ? CanopyLighting::Shader : RenderedTerrainSurface::Shader)
                    + (canopy ? CampfireLighting::Shader : "") + body;
            }
            return std::string("#version 330 core\n") + environment::FogShaderDefinition
                + GrassMotion::ShaderConstants()
                + GrassFootprintShaderConstants()
                + GrassMaterialShaderConstants()
                + (canopy ? CanopyLighting::Shader : RenderedTerrainSurface::Shader)
                + (canopy ? CampfireLighting::Shader : "") + body;
        }

        [[nodiscard]] CNA::Graphics::ShaderPackageEXT CreateShaderPackage()
        {
            using CNA::Graphics::ShaderCodeEXT;
            return CNA::Graphics::ShaderPackageEXT(
                {
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "grass/grass.es.vert.glsl", ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "grass/grass.es.frag.glsl", ShaderSource(true, FragmentBody, true)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "grass/grass.desktop.vert.glsl", ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "grass/grass.desktop.frag.glsl", ShaderSource(false, FragmentBody, true)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] std::uint8_t EncodedBladeFacing(const float facing)
        {
            const float turns = facing / (2.0f * std::numbers::pi_v<float>);
            const float normalizedFacing = turns - std::floor(turns);
            const int encodedFacing = static_cast<int>(std::lround(normalizedFacing * 255.0f));
            return static_cast<std::uint8_t>(std::clamp(encodedFacing, 0, 255));
        }

        [[nodiscard]] Color BladeColor(const int blade, const float heightFraction,
                                       const bool rightSide, const float facing, const bool shaderLit)
        {
            const float bladeShift = static_cast<float>((blade * 17) % 9) / 8.0f - 0.5f;
            const bool reverseFold = (blade & 1) != 0;
            const float foldLight = rightSide == reverseFold ? 0.94f : 1.06f;
            // Unlit BasicEffect keeps its legacy shading. Custom shading derives fold
            // illumination from actual normals; its vertex RGB is pigment only.
            const float light = shaderLit ? 1.0f : (0.78f + heightFraction * 0.22f) * foldLight;
            const bool sunCured = ((blade * 5 + 2) % 11) < 2;
            const float curedProfile = sunCured
                ? 0.08f + 0.60f * heightFraction * heightFraction
                : 0.0f;
            const float redCure = 1.0f + curedProfile * 0.75f;
            const float greenCure = 1.0f + curedProfile * 0.06f;
            const float blueCure = 1.0f - curedProfile * 0.82f;
            // Stable pigment families keep a whole field from reading as one artificial green.
            const int family = (blade * 5 + 2) % 7;
            const float baseRed = family < 2 ? 80.0f : (family < 5 ? 70.0f : 62.0f);
            const float baseGreen = family < 2 ? 122.0f : (family < 5 ? 119.0f : 111.0f);
            const float baseBlue = family < 2 ? 39.0f : (family < 5 ? 44.0f : 48.0f);
            // Sunlit tropical grasses are yellow-green, not emerald; keep per-blade spread.
            const int red = static_cast<int>(std::lround(
                (baseRed + bladeShift * 12.0f) * light * redCure));
            const int green = static_cast<int>(std::lround(
                (baseGreen + bladeShift * 18.0f) * light * greenCure));
            const int blue = static_cast<int>(std::lround(
                (baseBlue + bladeShift * 8.0f) * light * blueCure));
            return Color(static_cast<std::uint8_t>(std::clamp(red, 0, 255)),
                         static_cast<std::uint8_t>(std::clamp(green, 0, 255)),
                         static_cast<std::uint8_t>(std::clamp(blue, 0, 255)),
                         EncodedBladeFacing(facing));
        }

        [[nodiscard]] Color SeedHeadColor(const int blade, const float facing)
        {
            const bool sunCured = (blade & 1) != 0;
            const int variation = (blade * 11) % 13 - 6;
            const int red = (sunCured ? 116 : 72) + variation;
            const int green = (sunCured ? 126 : 132) + variation;
            const int blue = (sunCured ? 48 : 42) + variation / 2;
            return Color(static_cast<std::uint8_t>(std::clamp(red, 0, 255)),
                         static_cast<std::uint8_t>(std::clamp(green, 0, 255)),
                         static_cast<std::uint8_t>(std::clamp(blue, 0, 255)),
                         EncodedBladeFacing(facing));
        }

        void AppendNearSeedHead(ClusterGeometry& geometry, const int blade,
                                const Vector3& tip, const Vector3& direction,
                                const Vector3& side, const float phase, const float facing)
        {
            const Vector3 up(0.0f, 1.0f, 0.0f);
            const Vector3 stemTop = tip + up * 0.082f + direction * 0.014f;
            const Vector3 stemHalfWidth = side * 0.0036f;
            const Vector2 windData(1.0f, phase);
            const Color color = SeedHeadColor(blade, facing);
            const auto stemBase = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(tip - stemHalfWidth, color, windData);
            geometry.vertices.emplace_back(tip + stemHalfWidth, color, windData);
            geometry.vertices.emplace_back(stemTop - stemHalfWidth * 0.55f, color, windData);
            geometry.vertices.emplace_back(stemTop + stemHalfWidth * 0.55f, color, windData);
            geometry.indices.insert(geometry.indices.end(),
                                    {stemBase, static_cast<std::uint16_t>(stemBase + 2U),
                                     static_cast<std::uint16_t>(stemBase + 1U),
                                     static_cast<std::uint16_t>(stemBase + 1U),
                                     static_cast<std::uint16_t>(stemBase + 2U),
                                     static_cast<std::uint16_t>(stemBase + 3U)});

            for (int spikelet = 0; spikelet < 6; ++spikelet) {
                const float level = static_cast<float>(spikelet);
                const float sign = (spikelet & 1) == 0 ? -1.0f : 1.0f;
                const Vector3 root = tip + up * (0.026f + level * 0.009f)
                    + direction * (level * 0.002f);
                const float reach = 0.011f + 0.002f * static_cast<float>(spikelet % 3);
                const Vector3 outer = root + side * (sign * reach) + up * 0.004f;
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(root, color, windData);
                geometry.vertices.emplace_back(outer - up * 0.0035f, color, windData);
                geometry.vertices.emplace_back(outer + up * 0.0045f, color, windData);
                geometry.indices.insert(geometry.indices.end(),
                                        {base, static_cast<std::uint16_t>(base + 1U),
                                         static_cast<std::uint16_t>(base + 2U)});
            }
            geometry.materialHeights.resize(geometry.vertices.size(), 1.0f);
            geometry.bladeAcross.resize(geometry.vertices.size(), 2.0f);
        }

        void AppendBlade(ClusterGeometry& geometry, const int blade,
                         const GrassMeshTier tier, const int bladeLevels, const bool shaderLit)
        {
            const GrassLodBlade profile = GrassBladeForTier(tier, blade);
            const GrassBladePlacement& placement = profile.placement;
            const GrassBladeShape& shape = profile.shape;
            const int sourceBlade = profile.sourceBlade;
            const bool vFold = shape.folded;
            const float baseX = placement.x;
            const float baseZ = placement.z;
            const float facing = placement.facing;
            const Vector3 direction(std::cos(facing), 0.0f, std::sin(facing));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const float phase = placement.windPhase;
            const auto pointAt = [&](const float t, const float across) {
                const auto p = SampleGrassBlade(shape,t,across);
                return Vector3(baseX,p.height,baseZ) + side*p.across + direction*p.along;
            };
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());

            for (int level = 0; level < bladeLevels; ++level) {
                const float t = static_cast<float>(level) / static_cast<float>(bladeLevels - 1);
                const Vector2 windData(t, phase);
                if (vFold) {
                    const float foldAngle = 0.42f
                        + 0.045f * static_cast<float>(blade % 4);
                    geometry.vertices.emplace_back(
                        pointAt(t,-1),
                        BladeColor(sourceBlade, t, false, facing - foldAngle, shaderLit), windData);
                    geometry.vertices.emplace_back(
                        pointAt(t,0), BladeColor(sourceBlade, t, (sourceBlade & 1) != 0, facing, shaderLit), windData);
                    geometry.vertices.emplace_back(
                        pointAt(t,1),
                        BladeColor(sourceBlade, t, true, facing + foldAngle, shaderLit), windData);
                    geometry.bladeAcross.insert(geometry.bladeAcross.end(), {-1, 0, 1});
                } else {
                    geometry.vertices.emplace_back(
                        pointAt(t,-1),
                        BladeColor(sourceBlade, t, false, facing, shaderLit), windData);
                    geometry.vertices.emplace_back(
                        pointAt(t,1),
                        BladeColor(sourceBlade, t, true, facing, shaderLit), windData);
                    geometry.bladeAcross.insert(geometry.bladeAcross.end(), {-1, 1});
                }
                geometry.materialHeights.resize(geometry.vertices.size(), t);
            }

            if (vFold) {
                for (int level = 0; level < bladeLevels - 1; ++level) {
                    const auto left = static_cast<std::uint16_t>(base + level * 3);
                    const auto ridge = static_cast<std::uint16_t>(left + 1U);
                    const auto right = static_cast<std::uint16_t>(left + 2U);
                    const auto nextLeft = static_cast<std::uint16_t>(left + 3U);
                    const auto nextRidge = static_cast<std::uint16_t>(left + 4U);
                    const auto nextRight = static_cast<std::uint16_t>(left + 5U);
                    geometry.indices.insert(geometry.indices.end(), {
                        left, nextLeft, ridge, ridge, nextLeft, nextRidge,
                        ridge, nextRidge, right, right, nextRidge, nextRight});
                }
            } else {
                for (int level = 0; level < bladeLevels - 1; ++level) {
                    const auto left = static_cast<std::uint16_t>(base + level * 2);
                    const auto right = static_cast<std::uint16_t>(left + 1U);
                    const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                    const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                    geometry.indices.insert(geometry.indices.end(), {
                        left, nextLeft, right, right, nextLeft, nextRight});
                }
            }

            if (HasNearSeedHead(tier, blade)) {
                // Use the same sampled terminal centre/ridge as the blade, not its nominal tip.
                const Vector3 tip = pointAt(1,0);
                AppendNearSeedHead(geometry, blade, tip, direction, side, phase, facing);
            }
            geometry.bladeRoots.resize(geometry.vertices.size(),{baseX,baseZ});
        }

        void AppendTurfBlade(ClusterGeometry& geometry, const int blade, const bool shaderLit)
        {
            const auto turf = NearGrassTurfBlade(blade);
            const auto& placement = turf.placement;
            const Vector3 direction(std::cos(placement.facing), 0.0f, std::sin(placement.facing));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int level = 0; level < 3; ++level) {
                const float t = level * 0.5f;
                // A shared pointed tip avoids a nearly zero-width terminal quad on short blades.
                const int edgeCount = level == 2 ? 1 : 2;
                for (int edge = 0; edge < edgeCount; ++edge) {
                    const auto point = SampleGrassBlade(turf.shape, t,
                        level == 2 ? 0.0f : (edge == 0 ? -1.0f : 1.0f));
                    const bool dry = blade % 4 == 0;
                    const float variation = 0.88f + 0.04f * (blade % 5);
                    const float light = (shaderLit ? 1.0f : 0.76f + 0.24f * t) * variation;
                    const Color color(
                        static_cast<std::uint8_t>((dry ? 116.0f : 66.0f) * light),
                        static_cast<std::uint8_t>((dry ? 126.0f : 138.0f) * light),
                        static_cast<std::uint8_t>((dry ? 51.0f : 42.0f) * light),
                        EncodedBladeFacing(placement.facing));
                    geometry.vertices.emplace_back(
                        Vector3(placement.x, point.height, placement.z)
                            + side * point.across + direction * point.along,
                        color, Vector2(t * turf.shape.height / 0.65f, placement.windPhase));
                    geometry.bladeAcross.push_back(level == 2 ? 0.0f : (edge == 0 ? -1.0f : 1.0f));
                }
                geometry.materialHeights.resize(geometry.vertices.size(), t);
            }
            for (const auto offset : {0, 2, 1, 1, 2, 3, 2, 4, 3}) {
                geometry.indices.push_back(static_cast<std::uint16_t>(base + offset));
            }
            geometry.bladeRoots.resize(geometry.vertices.size(),{placement.x,placement.z});
        }

        [[nodiscard]] ClusterGeometry CreateClusterGeometry(const int bladeCount,
                                                            const GrassMeshTier tier,
                                                            const int bladeLevels, const bool shaderLit)
        {
            ClusterGeometry geometry;
            const int turfBlades = tier == GrassMeshTier::Near ? NearGrassTurfBladeCount : 0;
            int seedHeadCount = 0;
            int foldedBladeCount = 0;
            for (int blade = 0; blade < bladeCount; ++blade) {
                seedHeadCount += HasNearSeedHead(tier, blade) ? 1 : 0;
                foldedBladeCount += HasNearVFold(tier, blade) ? 1 : 0;
            }
            geometry.vertices.reserve(static_cast<std::size_t>(
                (bladeCount * 2 + foldedBladeCount) * bladeLevels
                + seedHeadCount * 22 + turfBlades * 5));
            geometry.indices.reserve(static_cast<std::size_t>(
                (bladeCount * 6 + foldedBladeCount * 6) * (bladeLevels - 1)
                + seedHeadCount * 24 + turfBlades * 9));
            geometry.materialHeights.reserve(geometry.vertices.capacity());
            geometry.bladeAcross.reserve(geometry.vertices.capacity());
            for (int blade = 0; blade < bladeCount; ++blade) {
                AppendBlade(geometry, blade, tier, bladeLevels, shaderLit);
            }
            for (int blade = 0; blade < turfBlades; ++blade) {
                AppendTurfBlade(geometry, blade, shaderLit);
            }
            return geometry;
        }
    }

    const VertexDeclaration& GrassRenderer::InstanceDeclaration()
    {
        static const VertexDeclaration declaration{
            VertexElement(0, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 1),
            VertexElement(16, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 2),
            VertexElement(32, VertexElementFormat::Vector2,
                          VertexElementUsage::TextureCoordinate, 3),
            VertexElement(40, VertexElementFormat::Single,
                          VertexElementUsage::TextureCoordinate, 4)};
        return declaration;
    }

    GrassRenderer::GrassRenderer(GraphicsDevice& device, const IslandTerrain& terrain,
                                 const GrassLayout& layout,
                                 CanopyLighting& canopyLighting, RenderedTerrainHeightMap& terrainHeightMap,
                                 const float maximumDrawDistance)
        : device_(device), canopyLighting_(canopyLighting), terrainHeightMap_(terrainHeightMap),
          maximumDrawDistance_(maximumDrawDistance)
    {
        if (!std::isfinite(maximumDrawDistance_) || maximumDrawDistance_ <= 0.0f) {
            throw std::invalid_argument("Grass maximum draw distance must be finite and positive");
        }
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        fallbackEffect_->setFogStartProperty(260.0f);
        fallbackEffect_->setFogEndProperty(720.0f);

        if (!device_.SupportsCapability(CNA::GraphicsCapability::Instancing)) {
            fallbackReason_ = "the active renderer does not support hardware instancing";
        } else if (!device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            fallbackReason_ = "the active renderer does not support custom effects";
        } else {
            try {
                const CNA::Graphics::ShaderPackageEXT package = CreateShaderPackage();
                customEffect_ = std::make_unique<ShaderEffect>(device_, package);
                if (customEffect_->IsEffectValid()) {
                    customShaderReady_ = true;
                } else {
                    fallbackReason_ = "the grass shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable grass shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMeshes();
        CreateCells(terrain, layout);
        if (customShaderReady_) {
            std::cout << "island-simulator: grass " << totalPatchCount_
                      << " patches in " << CellCount
                      << " cells, portable instanced wind shader\n"
                      << "island-simulator: grass player interaction integrated into the "
                         "instanced vertex shader\n"
                      << "island-simulator: grass five-level near curvature with portable "
                         "two-sided daylight shading\n"
                      << "island-simulator: grass near clusters use 42 blades for denser "
                         "first-person cover\n"
                      << "island-simulator: selected near grass blades use a geometric V-fold "
                         "ridge\n"
                      << NaturalBladeDiagnostic << '\n'
                      << PlantedBladeDiagnostic << '\n'
                      << MaterialContinuityDiagnostic << '\n'
                      << SurfaceMaterialDiagnostic << '\n'
                      << SharedLodDiagnostic << '\n'
                      << DistanceFadeDiagnostic << '\n'
                      << FootprintVariationDiagnostic << '\n'
                      << "island-simulator: grass shared rest-shape normals follow curvature and instance scale\n"
                      << "island-simulator: grass progressive near taper and seed roots share the sampled blade profile\n"
                      << "island-simulator: grass near blades form seven unequal fanned tufts with coherent wind\n"
                      << "island-simulator: grass near mesh adds 48 low curved turf blades with height-scaled motion\n"
                      << "island-simulator: grass culling bounds cover measured meshes, slope and full shared motion limits\n"
                      << "island-simulator: grass biome tint follows coast, path, and authored "
                         "clearings\n"
                      << "island-simulator: grass sparse sun-cured tip variation in shared "
                         "vertex colours\n"
                      << "island-simulator: grass per-instance blade stature with near juvenile "
                         "undergrowth\n"
                      << "island-simulator: grass sparse near seed heads share blade wind and "
                         "instancing\n"
                      << "island-simulator: grass three-tier per-patch LOD transitions "
                      << GrassLod::NearOnlyDistance << '-'
                      << GrassLod::MiddleOnlyStartDistance << '/'
                      << GrassLod::MiddleOnlyEndDistance << '-'
                      << GrassLod::FarOnlyDistance << " metres\n";
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass biome tint follows coast, path, and authored clearings");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass near clusters use 42 blades for denser first-person cover");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: selected near grass blades use a geometric V-fold ridge");
            MirrorRuntimeDiagnosticToPlatformLog(NaturalBladeDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(PlantedBladeDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(MaterialContinuityDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(SurfaceMaterialDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(SharedLodDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(DistanceFadeDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(FootprintVariationDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass shared rest-shape normals follow curvature and instance scale");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass progressive near taper and seed roots share the sampled blade profile");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass near blades form seven unequal fanned tufts with coherent wind");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass near mesh adds 48 low curved turf blades with height-scaled motion");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass culling bounds cover measured meshes, slope and full shared motion limits");
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: grass sparse near seed heads share blade wind and instancing");
        } else {
            std::cerr << "island-simulator: grass " << totalPatchCount_
                      << " patches, stock-effect fallback: " << fallbackReason_ << '\n';
        }
    }

    GrassRenderer::~GrassRenderer() = default;

    void GrassRenderer::CreateMeshes()
    {
        const auto upload = [this](const ClusterGeometry& geometry, GrassMesh& mesh,
                                   const char* label) {
            ValidateProceduralTriangleMeshOrThrow(
                geometry.vertices.size(), geometry.indices, label);
            if (geometry.materialHeights.size() != geometry.vertices.size()
                || geometry.bladeAcross.size() != geometry.vertices.size()
                || geometry.bladeRoots.size() != geometry.vertices.size()) {
                throw std::logic_error(std::string(label) + ": missing grass material coordinates");
            }
            for (float across : geometry.bladeAcross) {
                if (!std::isfinite(across) || (std::abs(across) > 1 && across != 2)) {
                    throw std::logic_error(std::string(label) + ": invalid blade or seed material tag");
                }
            }
            for(const auto& root:geometry.bladeRoots) {
                if(!std::isfinite(root[0])||!std::isfinite(root[1]))
                    throw std::logic_error(std::string(label)+": invalid grass blade root");
            }
            for (const auto& vertex : geometry.vertices) {
                const auto& p = vertex.Position;
                if (!std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z) || p.Y < 0) {
                    throw std::runtime_error(std::string(label) + ": invalid above-ground mesh bound");
                }
                meshRadius_ = std::max(meshRadius_, std::hypot(p.X,p.Z));
                meshHeight_ = std::max(meshHeight_,p.Y);
            }
            mesh.vertexCount = static_cast<int>(geometry.vertices.size());
            mesh.primitiveCount = static_cast<int>(geometry.indices.size() / 3U);
            if (customShaderReady_) {
                std::vector<std::array<float, 3>> positions;
                positions.reserve(geometry.vertices.size());
                for (const auto& vertex : geometry.vertices) {
                    positions.push_back({vertex.Position.X, vertex.Position.Y, vertex.Position.Z});
                }
                const auto normals = BuildSurfaceNormals(positions, geometry.indices);
                std::vector<GrassSurfaceVertex> vertices;
                vertices.reserve(geometry.vertices.size());
                for (std::size_t i = 0; i < geometry.vertices.size(); ++i) {
                    const auto& vertex = geometry.vertices[i];
                    vertices.push_back({positions[i],
                        {vertex.Color.getRProperty(), vertex.Color.getGProperty(), vertex.Color.getBProperty(),
                         EncodeGrassMaterialHeight(geometry.materialHeights[i])},
                        {vertex.TextureCoordinate.X, vertex.TextureCoordinate.Y}, normals[i],
                        geometry.bladeAcross[i],geometry.bladeRoots[i]});
                }
                mesh.vertices = std::make_unique<VertexBuffer>(device_, GrassSurfaceDeclaration(),
                    mesh.vertexCount, BufferUsage::WriteOnly);
                mesh.vertices->SetData(vertices.data(), mesh.vertexCount);
            } else {
                mesh.vertices = std::make_unique<VertexBuffer>(
                    device_, VertexPositionColorTexture::getVertexDeclarationStatic(),
                    mesh.vertexCount, BufferUsage::WriteOnly);
                mesh.vertices->SetData(geometry.vertices.data(), mesh.vertexCount);
            }
            mesh.indices = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits, static_cast<int>(geometry.indices.size()),
                BufferUsage::WriteOnly);
            mesh.indices->SetData(geometry.indices.data(), static_cast<int>(geometry.indices.size()));
        };
        upload(CreateClusterGeometry(
            NearBladeCount, GrassMeshTier::Near, NearBladeLevels, customShaderReady_), nearMesh_, "grass near LOD");
        upload(CreateClusterGeometry(
            MiddleBladeCount, GrassMeshTier::Middle, MiddleBladeLevels, customShaderReady_), middleMesh_,
            "grass middle LOD");
        upload(CreateClusterGeometry(
            FarBladeCount, GrassMeshTier::Far, FarBladeLevels, customShaderReady_), farMesh_, "grass far LOD");
        const std::string inventory = "island-simulator: grass mesh vertices/triangles near="
            + std::to_string(nearMesh_.vertexCount) + "/" + std::to_string(nearMesh_.primitiveCount)
            + " middle=" + std::to_string(middleMesh_.vertexCount) + "/"
            + std::to_string(middleMesh_.primitiveCount) + " far="
            + std::to_string(farMesh_.vertexCount) + "/" + std::to_string(farMesh_.primitiveCount);
        std::cout << inventory << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(inventory);
        std::cout << "island-simulator: measured grass mesh radius " << meshRadius_
                  << " m, height " << meshHeight_ << " m across all LODs and seed stems\n";
    }

    void GrassRenderer::CreateCells(const IslandTerrain& terrain, const GrassLayout& layout)
    {
        const auto surface=std::make_unique<RenderedTerrainSurface>(terrain);
        if(customShaderReady_) {
            constexpr const char* diagnostic="island-simulator: grass roots sample RGB24 rendered terrain triangles before wind";
            std::cout << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        }
        std::array<std::vector<CellBuildEntry>, CellCount> entries;
        std::uint32_t placementIndex = 0U;
        for (const GrassPlacement& patch : layout.Patches()) {
            const TerrainSample sample = terrain.Sample(patch.x, patch.z);
            const std::size_t index = GrassVisibility::CellIndex(patch.x, patch.z);
            entries[index].push_back(CellBuildEntry{
                GrassInstanceData{
                    patch.x, patch.y, patch.z, patch.yawRadians,
                    patch.scale, patch.spreadScale, patch.windPhase, patch.colorVariation,
                    sample.normal.x, sample.normal.z, patch.biomeTint},
                Matrix::CreateScale(patch.spreadScale, patch.scale, patch.spreadScale)
                    * Matrix::CreateRotationY(patch.yawRadians)
                    * Matrix::CreateTranslation(patch.x, patch.y, patch.z),
                GrassLodOrder(placementIndex)});
            ++placementIndex;

            GrassCell& cell = cells_[index];
            ++cell.patchCount;
            ++totalPatchCount_;
            const auto bounds = GrassMotion::CalculateBounds(meshRadius_,meshHeight_,patch.scale,
                patch.spreadScale,sample.normal.x,sample.normal.z);
            const float footprint=meshRadius_*patch.spreadScale;
            const auto surfaceRange=surface->HeightRange(patch.x-footprint,patch.z-footprint,
                                                        patch.x+footprint,patch.z+footprint);
            // Retain the old fallback envelope and conservatively include all draped bases.
            // Bounds already include motion and a 1 cm margin, larger than RGB24 rounding error.
            const Vector3 minimum(patch.x-bounds.horizontalRadius,
                                  std::min(patch.y,surfaceRange[0])+bounds.minimumY,
                                  patch.z-bounds.horizontalRadius);
            const Vector3 maximum(patch.x+bounds.horizontalRadius,
                                  std::max(patch.y,surfaceRange[1])+bounds.maximumY,
                                  patch.z+bounds.horizontalRadius);
            const Vector3 anchor(patch.x,patch.y,patch.z);
            if (!cell.occupied) {
                cell.bounds = BoundingBox(minimum, maximum);
                cell.placementBounds = BoundingBox(anchor,anchor);
                cell.occupied = true;
            } else {
                cell.bounds = BoundingBox(Vector3::Min(cell.bounds.Min, minimum),
                                          Vector3::Max(cell.bounds.Max, maximum));
                cell.placementBounds = BoundingBox(Vector3::Min(cell.placementBounds.Min,anchor),
                                                   Vector3::Max(cell.placementBounds.Max,anchor));
            }
        }

        for (std::size_t index = 0; index < cells_.size(); ++index) {
            if (entries[index].empty()) {
                continue;
            }
            std::ranges::sort(entries[index], {}, &CellBuildEntry::lodOrder);
            std::vector<GrassInstanceData> instances;
            instances.reserve(entries[index].size());
            GrassCell& cell = cells_[index];
            cell.fallbackTransforms.reserve(entries[index].size());
            for (const CellBuildEntry& entry : entries[index]) {
                instances.push_back(entry.instance);
                cell.fallbackTransforms.push_back(entry.fallbackTransform);
            }
            cells_[index].instances = std::make_unique<VertexBuffer>(
                device_, InstanceDeclaration(), static_cast<int>(instances.size()),
                BufferUsage::WriteOnly);
            cells_[index].instances->SetDataRaw(instances.data(),
                                                static_cast<int>(instances.size()),
                                                static_cast<int>(sizeof(GrassInstanceData)));
        }
    }

    void GrassRenderer::DrawInstancedMesh(const GrassMesh& mesh, const GrassCell& cell,
                                          const int instanceOffset, const int instanceCount)
    {
        if (instanceCount <= 0) {
            return;
        }
        ValidateInstancedDrawRangeOrThrow(
            cell.patchCount, instanceOffset, instanceCount, "grass instance");
        GpuDiagnosticScope drawScope("grass-instanced-draw");
        device_.SetVertexBuffers({VertexBufferBinding(mesh.vertices.get(), 0, 0),
                                  VertexBufferBinding(cell.instances.get(), instanceOffset, 1)});
        device_.setIndicesProperty(mesh.indices.get());
        device_.DrawInstancedPrimitives(PrimitiveType::TriangleList, 0, 0, mesh.vertexCount,
                                        0, mesh.primitiveCount, instanceCount);
        ++drawCallCount_;
    }

    void GrassRenderer::DrawFallbackMesh(const GrassMesh& mesh, const GrassCell& cell,
                                         const int instanceOffset, const int instanceCount)
    {
        device_.SetVertexBuffer(mesh.vertices.get());
        device_.setIndicesProperty(mesh.indices.get());
        for (int index = instanceOffset; index < instanceOffset + instanceCount; ++index) {
            const Matrix& transform = cell.fallbackTransforms[static_cast<std::size_t>(index)];
            fallbackEffect_->setWorldProperty(transform);
            fallbackEffect_->getCurrentTechniqueProperty()->getPassesProperty()[0]->Apply();
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, mesh.vertexCount,
                                          0, mesh.primitiveCount);
            ++drawCallCount_;
        }
    }

    void GrassRenderer::Draw(const Matrix& view, const Matrix& projection,
                             const Vector3& cameraPosition, const Vector3& playerPosition,
                             const float playerInteractionStrength, const float timeSeconds)
    {
        visiblePatchCount_ = 0;
        visibleNearPatchCount_ = 0;
        visibleMiddlePatchCount_ = 0;
        visibleFarPatchCount_ = 0;
        mixedLodCellCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);

        if (customShaderReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            {
                GpuDiagnosticScope applyScope("grass-effect-apply");
                customEffect_->Apply();
            }
            GpuDiagnosticScope uploadScope("grass-uniform-upload");
            canopyLighting_.Bind(*customEffect_);
            terrainHeightMap_.Bind(*customEffect_);
            customEffect_->SetUniformMat4("uViewProjection", viewProjection);
            customEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                          cameraPosition.Y, cameraPosition.Z);
            customEffect_->SetUniformVec2("uGrassFadeRange",
                GrassVisibility::FadeStart(maximumDrawDistance_), maximumDrawDistance_);
            customEffect_->SetUniformVec3(
                "uToSun", environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ);
            customEffect_->SetUniformVec2(
                "uPlayerPosition",
                std::isfinite(playerPosition.X) ? playerPosition.X : cameraPosition.X,
                std::isfinite(playerPosition.Z) ? playerPosition.Z : cameraPosition.Z);
            customEffect_->SetUniformFloat(
                "uPlayerInteractionStrength",
                std::isfinite(playerInteractionStrength)
                    ? std::clamp(playerInteractionStrength, 0.0f, 1.0f) : 0.0f);
            customEffect_->SetUniformFloat("uTime", timeSeconds);
            CampfireLighting::Bind(*customEffect_,timeSeconds);
        } else {
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        const BoundingFrustum frustum(view * projection);
        for (const GrassCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            // Keep mesh-tier selection unchanged. Shader fading is per anchor, so reject a
            // cell only once every anchor is beyond range, not when its centre crosses it.
            const Vector3 centre = (cell.placementBounds.Min + cell.placementBounds.Max) * 0.5f;
            const float deltaX = centre.X - cameraPosition.X;
            const float deltaZ = centre.Z - cameraPosition.Z;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            const float cullingDistanceSquared = customShaderReady_
                ? GrassVisibility::NearestDistanceSquared(cameraPosition.X, cameraPosition.Z,
                    cell.placementBounds.Min.X, cell.placementBounds.Min.Z,
                    cell.placementBounds.Max.X, cell.placementBounds.Max.Z)
                : distanceSquared; // Degraded BasicEffect path has no vertex-distance fade.
            if (cullingDistanceSquared > maximumDrawDistance_ * maximumDrawDistance_) {
                continue;
            }
            ++visibleCellCount_;
            visiblePatchCount_ += cell.patchCount;
            const GrassLodSplit split = GrassLod::Split(
                cell.patchCount, std::sqrt(distanceSquared));
            visibleNearPatchCount_ += split.nearCount;
            visibleMiddlePatchCount_ += split.middleCount;
            visibleFarPatchCount_ += split.farCount;
            const int activeLodCount = (split.nearCount > 0 ? 1 : 0)
                + (split.middleCount > 0 ? 1 : 0) + (split.farCount > 0 ? 1 : 0);
            if (activeLodCount > 1) {
                ++mixedLodCellCount_;
            }
            if (customShaderReady_) {
                DrawInstancedMesh(nearMesh_, cell, 0, split.nearCount);
                DrawInstancedMesh(
                    middleMesh_, cell, split.nearCount, split.middleCount);
                DrawInstancedMesh(farMesh_, cell,
                                  split.nearCount + split.middleCount, split.farCount);
            } else {
                DrawFallbackMesh(nearMesh_, cell, 0, split.nearCount);
                DrawFallbackMesh(
                    middleMesh_, cell, split.nearCount, split.middleCount);
                DrawFallbackMesh(farMesh_, cell,
                                 split.nearCount + split.middleCount, split.farCount);
            }
        }

        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
