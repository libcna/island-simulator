#include "IslandSimulator/GrassRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GrassLod.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numbers>
#include <string>
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
        constexpr float LayoutExtent = 300.0f;
        constexpr float FarDistance = 135.0f;
        constexpr int NearBladeCount = 34;
        constexpr int MiddleBladeCount = 10;
        constexpr int FarBladeCount = 5;
        constexpr int NearBladeLevels = 5;
        constexpr int MiddleBladeLevels = 3;
        constexpr int FarBladeLevels = 3;
        constexpr float GoldenAngle = 2.39996323f;

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

        struct ClusterGeometry
        {
            std::vector<VertexPositionColorTexture> vertices;
            std::vector<std::uint16_t> indices;
        };

        enum class GrassMeshTier
        {
            Near,
            Middle,
            Far
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
layout(location = 3) in vec4 aPlacement;
layout(location = 4) in vec4 aVariation;
layout(location = 5) in vec2 aSlope;
layout(location = 6) in float aBiomeTint;

uniform mat4 uViewProjection;
uniform float uTime;
uniform vec2 uPlayerPosition;
uniform float uPlayerInteractionStrength;

out vec3 vColor;
out vec3 vWorld;
out vec3 vBladeNormal;
out float vBladeHeight;
out float vBiomeTint;

void main()
{
    float normalY = sqrt(max(0.001, 1.0 - dot(aSlope, aSlope)));
    vec3 surfaceNormal = normalize(vec3(aSlope.x, normalY, aSlope.y));
    vec3 tangentX = normalize(vec3(surfaceNormal.y, -surfaceNormal.x, 0.0));
    vec3 tangentZ = normalize(cross(tangentX, surfaceNormal));
    float yawCos = cos(aPlacement.w);
    float yawSin = sin(aPlacement.w);
    float localX = aPosition.x * yawCos - aPosition.z * yawSin;
    float localZ = aPosition.x * yawSin + aPosition.z * yawCos;
    float bladeRandom = fract(sin(aWindData.y * 91.73 + aVariation.w * 37.21 + 0.618)
                              * 43758.5453);
    float bladeStature = mix(0.84, 1.16, bladeRandom);
    vec3 world = aPlacement.xyz
        + tangentX * (localX * aVariation.y)
        + surfaceNormal * (aPosition.y * aVariation.x * bladeStature)
        + tangentZ * (localZ * aVariation.y);

    float heightWeight = pow(clamp(aWindData.x, 0.0, 1.0), 1.55);
    vec2 playerOffset = world.xz - uPlayerPosition;
    float playerDistanceSquared = dot(playerOffset, playerOffset);
    float interactionStrength = clamp(uPlayerInteractionStrength, 0.0, 1.0);
    float interactionRadiusSquared = mix(0.3025, 1.8225, interactionStrength);
    if (playerDistanceSquared < interactionRadiusSquared) {
        float interaction = (1.0 - smoothstep(0.09, interactionRadiusSquared,
                                               playerDistanceSquared))
            * heightWeight;
        vec2 awayFromPlayer = playerOffset
            * inversesqrt(max(playerDistanceSquared, 0.01));
        float pushDistance = mix(0.12, 0.42, interactionStrength);
        float lowering = mix(0.08, 0.28, interactionStrength);
        world.xz += awayFromPlayer * interaction * pushDistance * aVariation.x;
        world.y -= interaction * lowering * aVariation.x;
    }

    float phase = (aWindData.y + aVariation.z) * 6.28318530718;
    float broad = sin(uTime * 0.86 + phase + world.x * 0.031 + world.z * 0.024);
    float detail = sin(uTime * 1.37 + phase * 1.73 - world.x * 0.071 + world.z * 0.063);
    float sway = (broad * 0.085 + detail * 0.026) * heightWeight * aVariation.x
        * mix(0.92, 1.08, bladeRandom);
    world.x += sway;
    world.z += sway * 0.46;
    world.y -= abs(sway) * 0.055 * heightWeight;

    float bladeFacing = aColor.a * 6.28318530718;
    float bladeNormalX = cos(bladeFacing) * yawCos - sin(bladeFacing) * yawSin;
    float bladeNormalZ = cos(bladeFacing) * yawSin + sin(bladeFacing) * yawCos;
    vBladeNormal = normalize(tangentX * bladeNormalX
                             + tangentZ * bladeNormalZ
                             + surfaceNormal * 0.10);
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
    vBiomeTint = aBiomeTint;
    gl_Position = uViewProjection * vec4(world, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;
in vec3 vBladeNormal;
in float vBladeHeight;
in float vBiomeTint;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float groundBlend = smoothstep(82.0, 125.0, distanceToCamera);
    vec3 normal = normalize(vBladeNormal);
    vec3 toSun = normalize(uToSun);
    vec3 toCamera = normalize(uCameraPosition - vWorld);
    float twoSidedDiffuse = abs(dot(normal, toSun));
    float rootLight = mix(0.80, 1.0, smoothstep(0.0, 0.72, vBladeHeight));
    float edgeLight = pow(1.0 - abs(dot(normal, toCamera)), 2.0) * 0.06;
    float transmission = pow(max(dot(-toCamera, toSun), 0.0), 5.0)
        * smoothstep(0.25, 0.92, vBladeHeight) * 0.10;
    float light = rootLight * (0.78 + twoSidedDiffuse * 0.30 + edgeLight + transmission);
    vec3 litColor = vColor * light;
    float coolWeight = clamp((0.5 - vBiomeTint) * 2.0, 0.0, 1.0);
    float warmWeight = clamp((vBiomeTint - 0.5) * 2.0, 0.0, 1.0);
    vec3 groundColor = mix(vec3(0.255, 0.465, 0.255),
                           vec3(0.205, 0.405, 0.235), coolWeight);
    groundColor = mix(groundColor, vec3(0.335, 0.455, 0.185), warmWeight);
    vec3 color = mix(litColor, groundColor, groundBlend);
    fragColor = vec4(color, 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\n") + body;
            }
            return std::string("#version 330 core\n") + body;
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
                                  "grass/grass.es.frag.glsl", ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "grass/grass.desktop.vert.glsl", ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "grass/grass.desktop.frag.glsl", ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] std::size_t CellIndex(const float x, const float z) noexcept
        {
            constexpr float cellWidth = 2.0f * LayoutExtent
                / static_cast<float>(GrassRenderer::CellsPerAxis);
            const int column = std::clamp(static_cast<int>(
                std::floor((x + LayoutExtent) / cellWidth)),
                0, GrassRenderer::CellsPerAxis - 1);
            const int row = std::clamp(static_cast<int>(
                std::floor((z + LayoutExtent) / cellWidth)),
                0, GrassRenderer::CellsPerAxis - 1);
            return static_cast<std::size_t>(row * GrassRenderer::CellsPerAxis + column);
        }

        [[nodiscard]] std::uint8_t EncodedBladeFacing(const float facing)
        {
            const float turns = facing / (2.0f * std::numbers::pi_v<float>);
            const float normalizedFacing = turns - std::floor(turns);
            const int encodedFacing = static_cast<int>(std::lround(normalizedFacing * 255.0f));
            return static_cast<std::uint8_t>(std::clamp(encodedFacing, 0, 255));
        }

        [[nodiscard]] Color BladeColor(const int blade, const float heightFraction,
                                       const bool rightSide, const float facing)
        {
            const float bladeShift = static_cast<float>((blade * 17) % 9) / 8.0f - 0.5f;
            const bool reverseFold = (blade & 1) != 0;
            const float foldLight = rightSide == reverseFold ? 0.94f : 1.06f;
            const float light = (0.78f + heightFraction * 0.22f) * foldLight;
            const bool sunCured = ((blade * 5 + 2) % 11) < 2;
            const float curedProfile = sunCured
                ? 0.08f + 0.60f * heightFraction * heightFraction
                : 0.0f;
            const float redCure = 1.0f + curedProfile * 0.75f;
            const float greenCure = 1.0f + curedProfile * 0.06f;
            const float blueCure = 1.0f - curedProfile * 0.82f;
            const int red = static_cast<int>(std::lround(
                (40.0f + bladeShift * 9.0f) * light * redCure));
            const int green = static_cast<int>(std::lround(
                (124.0f + bladeShift * 19.0f) * light * greenCure));
            const int blue = static_cast<int>(std::lround(
                (38.0f + bladeShift * 8.0f) * light * blueCure));
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
        }

        void AppendBlade(ClusterGeometry& geometry, const int blade, const int bladeCount,
                         const GrassMeshTier tier, const int bladeLevels)
        {
            const bool middleMesh = tier == GrassMeshTier::Middle;
            const bool farMesh = tier == GrassMeshTier::Far;
            const bool vFold = HasNearVFold(tier, blade);
            const bool juvenileBlade = tier == GrassMeshTier::Near && blade % 6 == 5;
            const float normalizedRadius = std::sqrt(
                (static_cast<float>(blade) + 0.45f) / static_cast<float>(bladeCount));
            const float angle = GoldenAngle * static_cast<float>(blade)
                + (farMesh ? 0.51f : (middleMesh ? 0.27f : 0.0f));
            const float clusterRadius = farMesh ? 0.82f : (middleMesh ? 0.93f : 1.02f);
            const float baseX = std::cos(angle) * normalizedRadius * clusterRadius;
            const float baseZ = std::sin(angle) * normalizedRadius * clusterRadius;
            const float facing = angle * 1.71f + static_cast<float>(blade % 4) * 0.43f;
            const Vector3 direction(std::cos(facing), 0.0f, std::sin(facing));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const float individual = 0.87f
                + 0.19f * static_cast<float>((blade * 5 + bladeCount) % 7) / 6.0f;
            const float juvenileHeight = juvenileBlade
                ? 0.56f + 0.06f * static_cast<float>(blade % 3) : 1.0f;
            const float height = (farMesh ? 0.60f
                : (middleMesh ? 0.48f + 0.035f * static_cast<float>(blade % 5)
                              : 0.40f + 0.045f * static_cast<float>(blade % 6)))
                * individual * juvenileHeight;
            const float width = (farMesh ? 0.11f
                : (middleMesh ? 0.055f + 0.005f * static_cast<float>(blade % 4)
                              : 0.023f + 0.004f * static_cast<float>(blade % 5)))
                * (juvenileBlade ? 0.74f : 1.0f);
            const float lean = (farMesh ? 0.12f
                : (middleMesh ? 0.09f + 0.012f * static_cast<float>(blade % 5)
                              : 0.07f + 0.018f * static_cast<float>(blade % 6)))
                * (juvenileBlade ? 0.62f : 1.0f);
            const float phase = std::fmod(static_cast<float>(blade) * 0.173f, 1.0f);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());

            for (int level = 0; level < bladeLevels; ++level) {
                const float t = static_cast<float>(level) / static_cast<float>(bladeLevels - 1);
                const float taper = level == bladeLevels - 1
                    ? 0.018f
                    : 0.58f + 0.42f * std::sin(t * std::numbers::pi_v<float>);
                const Vector3 centre(baseX, 0.0f, baseZ);
                const Vector3 curved = centre + Vector3(0.0f, 1.0f, 0.0f) * (height * t)
                    + direction * (lean * t * t)
                    + side * (0.012f * std::sin(t * std::numbers::pi_v<float>)
                              * (blade % 3 == 0 ? -1.0f : 1.0f));
                const Vector3 halfWidth = side * (width * taper);
                const Vector2 windData(t, phase);
                if (vFold) {
                    const float foldAngle = 0.42f
                        + 0.045f * static_cast<float>(blade % 4);
                    const Vector3 ridge = curved + direction * (width * taper * 0.48f);
                    geometry.vertices.emplace_back(
                        curved - halfWidth,
                        BladeColor(blade, t, false, facing - foldAngle), windData);
                    geometry.vertices.emplace_back(
                        ridge, BladeColor(blade, t, (blade & 1) != 0, facing), windData);
                    geometry.vertices.emplace_back(
                        curved + halfWidth,
                        BladeColor(blade, t, true, facing + foldAngle), windData);
                } else {
                    geometry.vertices.emplace_back(
                        curved - halfWidth,
                        BladeColor(blade, t, false, facing), windData);
                    geometry.vertices.emplace_back(
                        curved + halfWidth,
                        BladeColor(blade, t, true, facing), windData);
                }
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
                const Vector3 tip = Vector3(baseX, height, baseZ) + direction * lean;
                AppendNearSeedHead(geometry, blade, tip, direction, side, phase, facing);
            }
        }

        [[nodiscard]] ClusterGeometry CreateClusterGeometry(const int bladeCount,
                                                            const GrassMeshTier tier,
                                                            const int bladeLevels)
        {
            ClusterGeometry geometry;
            int seedHeadCount = 0;
            int foldedBladeCount = 0;
            for (int blade = 0; blade < bladeCount; ++blade) {
                seedHeadCount += HasNearSeedHead(tier, blade) ? 1 : 0;
                foldedBladeCount += HasNearVFold(tier, blade) ? 1 : 0;
            }
            geometry.vertices.reserve(static_cast<std::size_t>(
                (bladeCount * 2 + foldedBladeCount) * bladeLevels
                + seedHeadCount * 22));
            geometry.indices.reserve(static_cast<std::size_t>(
                (bladeCount * 6 + foldedBladeCount * 6) * (bladeLevels - 1)
                + seedHeadCount * 24));
            for (int blade = 0; blade < bladeCount; ++blade) {
                AppendBlade(geometry, blade, bladeCount, tier, bladeLevels);
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
                                 const GrassLayout& layout)
        : device_(device)
    {
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.255f, 0.465f, 0.255f));
        fallbackEffect_->setFogStartProperty(82.0f);
        fallbackEffect_->setFogEndProperty(125.0f);

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
                      << "island-simulator: selected near grass blades use a geometric V-fold "
                         "ridge\n"
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
                "island-simulator: selected near grass blades use a geometric V-fold ridge");
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
            mesh.vertexCount = static_cast<int>(geometry.vertices.size());
            mesh.primitiveCount = static_cast<int>(geometry.indices.size() / 3U);
            mesh.vertices = std::make_unique<VertexBuffer>(
                device_, VertexPositionColorTexture::getVertexDeclarationStatic(),
                mesh.vertexCount, BufferUsage::WriteOnly);
            mesh.vertices->SetData(geometry.vertices.data(), mesh.vertexCount);
            mesh.indices = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits, static_cast<int>(geometry.indices.size()),
                BufferUsage::WriteOnly);
            mesh.indices->SetData(geometry.indices.data(), static_cast<int>(geometry.indices.size()));
        };
        upload(CreateClusterGeometry(
            NearBladeCount, GrassMeshTier::Near, NearBladeLevels), nearMesh_, "grass near LOD");
        upload(CreateClusterGeometry(
            MiddleBladeCount, GrassMeshTier::Middle, MiddleBladeLevels), middleMesh_,
            "grass middle LOD");
        upload(CreateClusterGeometry(
            FarBladeCount, GrassMeshTier::Far, FarBladeLevels), farMesh_, "grass far LOD");
    }

    void GrassRenderer::CreateCells(const IslandTerrain& terrain, const GrassLayout& layout)
    {
        std::array<std::vector<CellBuildEntry>, CellCount> entries;
        std::uint32_t placementIndex = 0U;
        for (const GrassPlacement& patch : layout.Patches()) {
            const TerrainSample sample = terrain.Sample(patch.x, patch.z);
            const std::size_t index = CellIndex(patch.x, patch.z);
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
            const Vector3 minimum(patch.x - 1.4f, patch.y - 0.08f, patch.z - 1.4f);
            const Vector3 maximum(patch.x + 1.4f, patch.y + 1.35f, patch.z + 1.4f);
            if (!cell.occupied) {
                cell.bounds = BoundingBox(minimum, maximum);
                cell.occupied = true;
            } else {
                cell.bounds = BoundingBox(Vector3::Min(cell.bounds.Min, minimum),
                                          Vector3::Max(cell.bounds.Max, maximum));
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
            customEffect_->Apply();
            customEffect_->SetUniformMat4("uViewProjection", viewProjection);
            customEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                          cameraPosition.Y, cameraPosition.Z);
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
        } else {
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        const BoundingFrustum frustum(view * projection);
        for (const GrassCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const Vector3 centre = (cell.bounds.Min + cell.bounds.Max) * 0.5f;
            const float deltaX = centre.X - cameraPosition.X;
            const float deltaZ = centre.Z - cameraPosition.Z;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            if (distanceSquared > FarDistance * FarDistance) {
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
