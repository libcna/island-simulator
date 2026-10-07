#include "IslandSimulator/JungleRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numbers>
#include <string>

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
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
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
        constexpr float CanopyNearMinimumDistance = 105.0f;
        constexpr float CanopyNearDistanceRange = 24.0f;
        constexpr float CanopyMaximumDistance = 475.0f;
        constexpr float UnderstoryNearDistance = 58.0f;
        constexpr float UnderstoryMaximumDistance = 128.0f;
        constexpr float BushNearDistance = 44.0f;
        constexpr float BushMaximumDistance = 102.0f;
        constexpr float GroundPlantNearDistance = 30.0f;
        constexpr float GroundPlantMaximumDistance = 72.0f;
        constexpr const char* CanopyLianaDiagnostic =
            "island-simulator: selected near canopy variants include hanging leafy lianas";

        struct PlantGeometry
        {
            std::vector<VertexPositionColorTexture> vertices;
            std::vector<std::uint16_t> indices;
        };

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aWindData;
layout(location = 3) in vec4 aInstance0;
layout(location = 4) in vec4 aInstance1;
layout(location = 5) in vec4 aInstance2;
layout(location = 6) in vec4 aInstance3;

uniform mat4 uViewProjection;
uniform float uTime;

out vec3 vColor;
out vec3 vWorld;
out vec3 vLocal;

void main()
{
    vec3 local = aPosition;
    float phase = aWindData.y * 6.28318530718;
    float broad = sin(uTime * 0.61 + phase + local.y * 0.075);
    float detail = sin(uTime * 1.07 + phase * 1.41 + local.x * 0.18 - local.z * 0.21);
    float sway = (broad * 0.105 + detail * 0.028) * aWindData.x;
    local.x += sway;
    local.z += sway * 0.43;
    local.y -= abs(sway) * 0.035 * aWindData.x;

    mat4 instanceTransform = mat4(aInstance0, aInstance1, aInstance2, aInstance3);
    vec4 world = instanceTransform * vec4(local, 1.0);
    // Scale and yaw already vary deterministically per placement. Recombine their transform
    // coefficients into a stable tint seed so neighbouring copies do not share one flat green,
    // while near/far LODs retain exactly the same instance identity.
    float tintSeed = fract(abs(aInstance0.x * 1.37 + aInstance0.z * 2.17
        + aInstance2.x * 3.11 + aInstance2.z * 4.03));
    float signedTint = tintSeed - 0.5;
    float foliageMask = smoothstep(0.035, 0.12, aColor.g - aColor.r);
    vec3 foliageTint = vec3(1.0 + signedTint * 0.10,
                            1.0 + signedTint * 0.15,
                            1.0 - signedTint * 0.06);
    vColor = aColor.rgb * mix(vec3(1.0), foliageTint, foliageMask);
    vWorld = world.xyz;
    vLocal = aPosition;
    gl_Position = uViewProjection * world;
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;
in vec3 vLocal;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float fog = smoothstep(250.0, 660.0, distanceToCamera);
    vec3 faceVector = cross(dFdx(vWorld), dFdy(vWorld));
    float inverseFaceLength = inversesqrt(max(dot(faceVector, faceVector), 0.00000001));
    vec3 faceNormal = faceVector * inverseFaceLength;
    float twoSidedDiffuse = abs(dot(faceNormal, normalize(uToSun)));
    float upwardFill = abs(faceNormal.y);
    float daylight = 0.78 + twoSidedDiffuse * 0.24 + upwardFill * 0.05;
    vec3 litColor = vColor * daylight;
    float barkColorMask = smoothstep(0.025, 0.095, vColor.r - vColor.g)
        * smoothstep(0.025, 0.105, vColor.g - vColor.b);
    float barkDetailFade = barkColorMask
        * (1.0 - smoothstep(38.0, 68.0, distanceToCamera));
    float barkAngle = atan(vLocal.z, vLocal.x);
    float broadGrain = sin(barkAngle * 5.0 + vLocal.y * 0.42
        + sin(vLocal.y * 1.31) * 0.32);
    float fissureWave = abs(sin(barkAngle * 9.0 + vLocal.y * 0.19
        + sin(vLocal.y * 0.77) * 0.55));
    float fissure = smoothstep(0.82, 0.97, fissureWave);
    litColor *= 1.0 + barkDetailFade * (broadGrain * 0.035 - fissure * 0.09);
    vec3 color = mix(litColor, vec3(0.53, 0.78, 0.88), fog);
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
                                  "jungle/jungle.es.vert.glsl", ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "jungle/jungle.es.frag.glsl", ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "jungle/jungle.desktop.vert.glsl", ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "jungle/jungle.desktop.frag.glsl", ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color ShadeColor(const Color& color, const float factor)
        {
            const auto channel = [factor](const std::uint8_t value) {
                return static_cast<std::uint8_t>(std::lround(std::clamp(
                    static_cast<float>(value) * factor, 0.0f, 255.0f)));
            };
            return Color(channel(color.getRProperty()), channel(color.getGProperty()),
                         channel(color.getBProperty()), color.getAProperty());
        }

        void AppendCanopyButtresses(PlantGeometry& geometry, const int variant)
        {
            const int buttressCount = 4 + variant % 2;
            const Color bark(104 + variant * 4, 79 + variant * 2, 48, 255);
            for (int buttress = 0; buttress < buttressCount; ++buttress) {
                const float rootIndex = static_cast<float>(buttress + 1);
                const float angleVariation = std::sin(
                    rootIndex * 2.41f + static_cast<float>(variant) * 0.83f);
                const float lengthVariation = 0.5f + 0.5f * std::sin(
                    rootIndex * 3.73f - static_cast<float>(variant) * 1.17f);
                const float widthVariation = 0.5f + 0.5f * std::sin(
                    rootIndex * 5.19f + static_cast<float>(variant) * 0.61f);
                const float angle = static_cast<float>(buttress) * 2.0f
                    * std::numbers::pi_v<float> / static_cast<float>(buttressCount)
                    + static_cast<float>(variant) * 0.37f + angleVariation * 0.11f;
                const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
                const Vector3 side(-direction.Z, 0.0f, direction.X);
                const float innerWidth = 0.13f + widthVariation * 0.05f;
                const float outerWidth = 0.035f + widthVariation * 0.025f;
                const float crestHeight = 0.58f + lengthVariation * 0.24f;
                const Vector3 innerCentre = direction * 0.25f;
                // The maximum scaled tip stays inside the player-blocked trunk footprint for
                // every authored canopy scale, so the visual flare does not create a snag cue.
                const Vector3 outerCentre = direction * (0.58f + lengthVariation * 0.09f);
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                const Vector2 still(0.0f, static_cast<float>(buttress) * 0.17f);
                geometry.vertices.emplace_back(
                    innerCentre - side * innerWidth + Vector3(0.0f, 0.025f, 0.0f),
                    ShadeColor(bark, 0.80f), still);
                geometry.vertices.emplace_back(
                    innerCentre + side * innerWidth + Vector3(0.0f, 0.025f, 0.0f),
                    ShadeColor(bark, 0.96f), still);
                geometry.vertices.emplace_back(
                    innerCentre + direction * 0.015f + Vector3(0.0f, crestHeight, 0.0f),
                    ShadeColor(bark, 1.08f), still);
                geometry.vertices.emplace_back(
                    outerCentre - side * outerWidth + Vector3(0.0f, 0.018f, 0.0f),
                    ShadeColor(bark, 0.74f), still);
                geometry.vertices.emplace_back(
                    outerCentre + side * outerWidth + Vector3(0.0f, 0.018f, 0.0f),
                    ShadeColor(bark, 0.88f), still);
                geometry.vertices.emplace_back(
                    outerCentre - direction * 0.025f + Vector3(0.0f, 0.060f, 0.0f),
                    ShadeColor(bark, 0.93f), still);
                geometry.indices.insert(geometry.indices.end(), {
                    base, static_cast<std::uint16_t>(base + 3U),
                    static_cast<std::uint16_t>(base + 2U),
                    static_cast<std::uint16_t>(base + 2U),
                    static_cast<std::uint16_t>(base + 3U),
                    static_cast<std::uint16_t>(base + 5U),
                    static_cast<std::uint16_t>(base + 2U),
                    static_cast<std::uint16_t>(base + 5U),
                    static_cast<std::uint16_t>(base + 1U),
                    static_cast<std::uint16_t>(base + 1U),
                    static_cast<std::uint16_t>(base + 5U),
                    static_cast<std::uint16_t>(base + 4U),
                    base, static_cast<std::uint16_t>(base + 2U),
                    static_cast<std::uint16_t>(base + 1U),
                    static_cast<std::uint16_t>(base + 3U),
                    static_cast<std::uint16_t>(base + 4U),
                    static_cast<std::uint16_t>(base + 5U)});
            }
        }

        void AppendCanopyTrunk(PlantGeometry& geometry, const int variant, const bool farMesh,
                               float& crownX, float& crownY, float& crownZ)
        {
            const int rings = farMesh ? 4 : 7;
            const int sides = farMesh ? 5 : 8;
            const float height = 8.6f + static_cast<float>(variant) * 0.82f;
            const float leanAngle = 0.63f + static_cast<float>(variant) * 1.49f;
            const float lean = 0.22f + static_cast<float>((variant * 3) % 4) * 0.13f;
            const std::uint16_t base = static_cast<std::uint16_t>(geometry.vertices.size());

            for (int ring = 0; ring <= rings; ++ring) {
                const float t = static_cast<float>(ring) / static_cast<float>(rings);
                const float bend = lean * t * t;
                const float centreX = std::cos(leanAngle) * bend;
                const float centreZ = std::sin(leanAngle) * bend;
                const float radius = 0.43f + (0.23f - 0.43f) * t;
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float directional = 0.68f + 0.30f
                        * std::max(0.0f, std::cos(angle) * 0.74f + std::sin(angle) * 0.28f);
                    const Color bark(104 + variant * 4, 79 + variant * 2, 48, 255);
                    geometry.vertices.emplace_back(
                        Vector3(centreX + std::cos(angle) * radius, height * t,
                                centreZ + std::sin(angle) * radius),
                        ShadeColor(bark, directional * (ring % 2 == 0 ? 1.04f : 0.94f)),
                        Vector2(t * t * 0.08f, static_cast<float>(variant) * 0.17f));
                }
            }
            for (int ring = 0; ring < rings; ++ring) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(base + ring * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(base + ring * sides + next);
                    const auto upper = static_cast<std::uint16_t>(base + (ring + 1) * sides + side);
                    const auto upperNext = static_cast<std::uint16_t>(base + (ring + 1) * sides + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }
            crownX = std::cos(leanAngle) * lean;
            crownY = height;
            crownZ = std::sin(leanAngle) * lean;
        }

        void AppendLeafCluster(PlantGeometry& geometry, const Vector3& centre,
                               const Vector3& radius, const Color& baseColor,
                               const float windPhase, const bool farMesh,
                               const bool roundedFar = false)
        {
            const int rings = farMesh ? (roundedFar ? 3 : 2) : 4;
            const int slices = farMesh ? (roundedFar ? 7 : 6) : 9;
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int ring = 0; ring <= rings; ++ring) {
                const float ringT = static_cast<float>(ring) / static_cast<float>(rings);
                const float latitude = -std::numbers::pi_v<float> * 0.5f
                    + ringT * std::numbers::pi_v<float>;
                const float horizontal = std::cos(latitude);
                for (int slice = 0; slice < slices; ++slice) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(slice) / static_cast<float>(slices);
                    const Vector3 direction(horizontal * std::cos(angle), std::sin(latitude),
                                            horizontal * std::sin(angle));
                    const float contour = farMesh ? 1.0f
                        : 0.91f + 0.07f * std::cos(angle * 3.0f + windPhase * 6.2831853f)
                            + 0.035f * std::sin(angle * 5.0f - windPhase * 4.1f);
                    const float light = 0.70f + 0.29f * std::max(
                        0.0f, direction.X * 0.52f + direction.Y * 0.75f - direction.Z * 0.20f);
                    geometry.vertices.emplace_back(
                        centre + Vector3(direction.X * radius.X * contour,
                                         direction.Y * radius.Y,
                                         direction.Z * radius.Z * contour),
                        ShadeColor(baseColor, light), Vector2(0.76f, windPhase));
                }
            }
            for (int ring = 0; ring < rings; ++ring) {
                for (int slice = 0; slice < slices; ++slice) {
                    const int next = (slice + 1) % slices;
                    const auto lower = static_cast<std::uint16_t>(base + ring * slices + slice);
                    const auto lowerNext = static_cast<std::uint16_t>(base + ring * slices + next);
                    const auto upper = static_cast<std::uint16_t>(base + (ring + 1) * slices + slice);
                    const auto upperNext = static_cast<std::uint16_t>(base + (ring + 1) * slices + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }
        }

        void AppendCanopyBranch(PlantGeometry& geometry, const Vector3& start,
                                const Vector3& end, const Color& baseColor,
                                const float windPhase)
        {
            constexpr int sides = 4;
            const Vector3 axis = Vector3::Normalize(end - start);
            const Vector3 reference = std::fabs(axis.Y) < 0.88f
                ? Vector3::Up : Vector3::Right;
            const Vector3 right = Vector3::Normalize(Vector3::Cross(reference, axis));
            const Vector3 up = Vector3::Normalize(Vector3::Cross(axis, right));
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int endpoint = 0; endpoint < 2; ++endpoint) {
                const Vector3 centre = endpoint == 0 ? start : end;
                const float radius = endpoint == 0 ? 0.17f : 0.085f;
                const float windWeight = endpoint == 0 ? 0.12f : 0.58f;
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const Vector3 outward = right * std::cos(angle) + up * std::sin(angle);
                    geometry.vertices.emplace_back(
                        centre + outward * radius,
                        ShadeColor(baseColor, 0.76f + 0.10f * static_cast<float>(side)),
                        Vector2(windWeight, windPhase));
                }
            }
            for (int side = 0; side < sides; ++side) {
                const int next = (side + 1) % sides;
                const auto lower = static_cast<std::uint16_t>(base + side);
                const auto lowerNext = static_cast<std::uint16_t>(base + next);
                const auto upper = static_cast<std::uint16_t>(base + sides + side);
                const auto upperNext = static_cast<std::uint16_t>(base + sides + next);
                geometry.indices.insert(geometry.indices.end(),
                                        {lower, upper, lowerNext,
                                         lowerNext, upper, upperNext});
            }
        }

        void AppendPointedLeaf(PlantGeometry& geometry, const Vector3& root,
                               const float angle, const float length, const float width,
                               const float droop, const Color& baseColor,
                               const float windPhase, const float ridgeHeight = 0.16f)
        {
            const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const Vector3 shoulder = root + direction * (length * 0.48f);
            const Vector3 ridge = shoulder
                + Vector3(0.0f, ridgeHeight - droop * 0.18f, 0.0f);
            const Vector3 tip = root + direction * length - Vector3::Up * droop;
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(root, ShadeColor(baseColor, 0.78f),
                                           Vector2(0.55f, windPhase));
            geometry.vertices.emplace_back(shoulder - side * width,
                                           ShadeColor(baseColor, 0.91f),
                                           Vector2(0.78f, windPhase));
            geometry.vertices.emplace_back(ridge, ShadeColor(baseColor, 1.10f),
                                           Vector2(0.80f, windPhase));
            geometry.vertices.emplace_back(shoulder + side * width,
                                           ShadeColor(baseColor, 0.84f),
                                           Vector2(0.78f, windPhase));
            geometry.vertices.emplace_back(tip, ShadeColor(baseColor, 0.96f),
                                           Vector2(0.90f, windPhase));
            geometry.indices.insert(geometry.indices.end(), {
                base, static_cast<std::uint16_t>(base + 1U),
                static_cast<std::uint16_t>(base + 2U),
                base, static_cast<std::uint16_t>(base + 2U),
                static_cast<std::uint16_t>(base + 3U),
                static_cast<std::uint16_t>(base + 1U),
                static_cast<std::uint16_t>(base + 4U),
                static_cast<std::uint16_t>(base + 2U),
                static_cast<std::uint16_t>(base + 2U),
                static_cast<std::uint16_t>(base + 4U),
                static_cast<std::uint16_t>(base + 3U)});
        }

        void AppendCanopyLiana(PlantGeometry& geometry, const Vector3& anchor,
                               const int variant, const float directionAngle,
                               const float length, const float windPhase)
        {
            constexpr int Segments = 6;
            constexpr int Sides = 3;
            const Vector3 direction(std::cos(directionAngle), 0.0f,
                                    std::sin(directionAngle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const auto centreAt = [&](const float t) {
                const float outward = std::sin(t * std::numbers::pi_v<float>) * 0.16f
                    + t * t * 0.09f;
                const float curl = std::sin(
                    t * 2.0f * std::numbers::pi_v<float>
                    + static_cast<float>(variant) * 0.71f) * 0.055f;
                return anchor - Vector3::Up * (length * t)
                    + direction * outward + side * curl;
            };

            const Color vineColor(57 + variant * 2, 78 + variant * 3, 34, 255);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int segment = 0; segment <= Segments; ++segment) {
                const float t = static_cast<float>(segment) / static_cast<float>(Segments);
                const Vector3 centre = centreAt(t);
                const float radius = 0.038f - t * 0.016f;
                const float windWeight = 0.58f + t * 0.32f;
                for (int sideIndex = 0; sideIndex < Sides; ++sideIndex) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(sideIndex) / static_cast<float>(Sides);
                    const Vector3 outward(std::cos(angle) * radius, 0.0f,
                                          std::sin(angle) * radius);
                    geometry.vertices.emplace_back(
                        centre + outward,
                        ShadeColor(vineColor, 0.80f + 0.10f * static_cast<float>(sideIndex)),
                        Vector2(windWeight, windPhase));
                }
            }
            for (int segment = 0; segment < Segments; ++segment) {
                for (int sideIndex = 0; sideIndex < Sides; ++sideIndex) {
                    const int next = (sideIndex + 1) % Sides;
                    const auto upper = static_cast<std::uint16_t>(
                        base + segment * Sides + sideIndex);
                    const auto upperNext = static_cast<std::uint16_t>(
                        base + segment * Sides + next);
                    const auto lower = static_cast<std::uint16_t>(
                        base + (segment + 1) * Sides + sideIndex);
                    const auto lowerNext = static_cast<std::uint16_t>(
                        base + (segment + 1) * Sides + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {upper, lower, upperNext,
                                             upperNext, lower, lowerNext});
                }
            }

            const Color leafColor(42, 112 + variant * 4, 47, 255);
            AppendPointedLeaf(
                geometry, centreAt(0.43f), directionAngle + 1.06f,
                0.58f, 0.16f, 0.25f, ShadeColor(leafColor, 1.04f), windPhase, 0.075f);
            AppendPointedLeaf(
                geometry, centreAt(0.70f), directionAngle - 1.18f,
                0.49f, 0.14f, 0.30f, ShadeColor(leafColor, 0.91f), windPhase, 0.065f);
        }

        [[nodiscard]] PlantGeometry CreateCanopyGeometry(const int variant, const bool farMesh)
        {
            PlantGeometry geometry;
            geometry.vertices.reserve(farMesh ? 150U : 600U);
            geometry.indices.reserve(farMesh ? 700U : 2600U);
            float crownX = 0.0f;
            float crownY = 0.0f;
            float crownZ = 0.0f;
            if (!farMesh) {
                AppendCanopyButtresses(geometry, variant);
            }
            AppendCanopyTrunk(geometry, variant, farMesh, crownX, crownY, crownZ);

            const int clusterCount = farMesh ? 4 : 8;
            const Color leafBase = variant % 2 == 0
                ? Color(34, 104 + variant * 5, 48, 255)
                : Color(42, 119 + variant * 4, 52, 255);
            for (int cluster = 0; cluster < clusterCount; ++cluster) {
                const float angle = static_cast<float>(variant) * 0.57f
                    + static_cast<float>(cluster) * 2.39996323f;
                const float radial = cluster == 0 ? 0.0f
                    : (farMesh ? 1.4f + 0.34f * static_cast<float>((cluster + variant) % 4)
                               : 1.75f + 0.38f * static_cast<float>((cluster + variant) % 4));
                const float vertical = static_cast<float>((cluster * 3 + variant) % 5) * 0.38f;
                const Vector3 centre(crownX + std::cos(angle) * radial,
                                     crownY - 0.35f + vertical,
                                     crownZ + std::sin(angle) * radial);
                const float width = farMesh
                    ? 2.65f + 0.22f * static_cast<float>((cluster + variant) % 3)
                    : 2.35f + 0.18f * static_cast<float>((cluster + variant) % 3);
                const float phase = std::fmod(static_cast<float>(cluster) * 0.139f
                                              + static_cast<float>(variant) * 0.23f, 1.0f);
                if (!farMesh && cluster > 0) {
                    const Vector3 branchStart(crownX + std::cos(angle) * 0.12f,
                                              crownY - 1.20f,
                                              crownZ + std::sin(angle) * 0.12f);
                    const Vector3 branchEnd = centre - Vector3::Up * 0.62f
                        - Vector3(std::cos(angle) * 0.48f, 0.0f,
                                  std::sin(angle) * 0.48f);
                    AppendCanopyBranch(geometry, branchStart, branchEnd,
                                       Color(78 + variant * 3, 62 + variant * 2, 38, 255),
                                       phase);
                }
                AppendLeafCluster(geometry, centre,
                                  Vector3(width,
                                          farMesh ? 1.65f + 0.13f * variant
                                                  : 1.38f + 0.10f * variant,
                                          width * 0.91f),
                                  cluster % 3 == 0 ? ShadeColor(leafBase, 1.10f) : leafBase,
                                  phase, farMesh, farMesh);
                if (!farMesh) {
                    const float leafAngle = angle
                        + (cluster % 2 == 0 ? -0.32f : 0.39f);
                    const Vector3 leafDirection(std::cos(leafAngle), 0.0f,
                                                std::sin(leafAngle));
                    const Vector3 leafRoot = centre + leafDirection * (width * 0.72f)
                        - Vector3::Up * (0.12f + 0.05f * static_cast<float>(cluster % 3));
                    AppendPointedLeaf(
                        geometry, leafRoot, leafAngle,
                        1.48f + 0.11f * static_cast<float>((cluster + variant) % 4),
                        0.42f + 0.04f * static_cast<float>((cluster * 2 + variant) % 3),
                        0.42f + 0.08f * static_cast<float>((cluster + variant) % 3),
                        cluster % 2 == 0 ? ShadeColor(leafBase, 1.03f)
                                         : ShadeColor(leafBase, 0.91f),
                        phase);
                }
                const int lianaCluster = variant == 1 ? 5 : 2;
                if (!farMesh && (variant == 1 || variant == 3)
                    && cluster == lianaCluster) {
                    AppendCanopyLiana(
                        geometry,
                        centre - Vector3::Up
                            * (1.04f + 0.08f * static_cast<float>(variant)),
                        variant, angle + (variant == 1 ? 0.34f : -0.41f),
                        2.45f + 0.24f * static_cast<float>(variant), phase);
                }
            }
            return geometry;
        }

        void AppendFrond(PlantGeometry& geometry, const float angle, const float length,
                         const float width, const float lift, const Color& baseColor,
                         const float phase, const int segments)
        {
            const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int segment = 0; segment <= segments; ++segment) {
                const float t = static_cast<float>(segment) / static_cast<float>(segments);
                const float taper = segment == segments
                    ? 0.025f : std::pow(std::sin(t * std::numbers::pi_v<float>), 0.55f);
                const Vector3 centre = direction * (length * t)
                    + Vector3(0.0f, lift * std::sin(t * std::numbers::pi_v<float>)
                                      - 0.22f * length * t * t, 0.0f);
                const Color color = ShadeColor(baseColor, 0.76f + 0.24f * t);
                geometry.vertices.emplace_back(centre - side * (width * taper), color,
                                               Vector2(t * t, phase));
                geometry.vertices.emplace_back(centre + side * (width * taper), color,
                                               Vector2(t * t, phase));
            }
            for (int segment = 0; segment < segments; ++segment) {
                const auto left = static_cast<std::uint16_t>(base + segment * 2);
                const auto right = static_cast<std::uint16_t>(left + 1U);
                const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                geometry.indices.insert(geometry.indices.end(),
                                        {left, nextLeft, right,
                                         right, nextLeft, nextRight});
            }
        }

        [[nodiscard]] PlantGeometry CreateUnderstoryGeometry(const int variant,
                                                             const bool farMesh)
        {
            PlantGeometry geometry;
            const int leafCount = farMesh ? 5 : 8 + variant;
            const int segments = farMesh ? 2 : 5;
            geometry.vertices.reserve(static_cast<std::size_t>(leafCount * (segments + 1) * 2));
            geometry.indices.reserve(static_cast<std::size_t>(leafCount * segments * 6));
            const bool broadLeaf = variant == 1 || variant == 3;
            const Color color = broadLeaf
                ? Color(42 + variant * 3, 126 + variant * 4, 58, 255)
                : Color(35, 111 + variant * 5, 49 + variant * 2, 255);
            for (int leaf = 0; leaf < leafCount; ++leaf) {
                const float angle = static_cast<float>(variant) * 0.49f
                    + 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(leaf) / static_cast<float>(leafCount);
                const float variation = 0.88f
                    + 0.16f * static_cast<float>((leaf * 5 + variant) % 5) / 4.0f;
                const float length = (broadLeaf ? 2.05f : 1.72f) * variation;
                const float width = (broadLeaf ? 0.48f : 0.25f) * variation;
                const float lift = broadLeaf ? 1.08f : 0.74f;
                const float phase = std::fmod(static_cast<float>(leaf) / leafCount
                                              + static_cast<float>(variant) * 0.19f, 1.0f);
                AppendFrond(geometry, angle, length, width, lift, color, phase, segments);
            }
            return geometry;
        }

        [[nodiscard]] PlantGeometry CreateBushGeometry(const int variant, const bool farMesh)
        {
            PlantGeometry geometry;
            const int clusterCount = farMesh ? 2 : 4 + variant;
            const Color baseColor = variant == 0 ? Color(38, 112, 48, 255)
                : (variant == 1 ? Color(48, 126, 55, 255)
                                : Color(31, 101, 48, 255));
            for (int cluster = 0; cluster < clusterCount; ++cluster) {
                const float angle = static_cast<float>(variant) * 0.61f
                    + static_cast<float>(cluster) * 2.39996323f;
                const float radial = cluster == 0 ? 0.0f
                    : 0.40f + 0.11f * static_cast<float>((cluster + variant) % 3);
                const float height = 0.39f
                    + 0.08f * static_cast<float>((cluster * 2 + variant) % 3);
                const float width = 0.57f
                    + 0.07f * static_cast<float>((cluster + variant) % 3);
                const float phase = std::fmod(static_cast<float>(cluster) * 0.173f
                                              + static_cast<float>(variant) * 0.21f, 1.0f);
                AppendLeafCluster(
                    geometry,
                    Vector3(std::cos(angle) * radial, height, std::sin(angle) * radial),
                    Vector3(width, 0.35f + 0.045f * static_cast<float>(variant),
                            width * 0.88f),
                    cluster % 3 == 0 ? ShadeColor(baseColor, 1.08f) : baseColor,
                    phase, farMesh);
                if (!farMesh) {
                    const Vector3 leafCentre(std::cos(angle) * radial,
                                             height + 0.14f,
                                             std::sin(angle) * radial);
                    for (int leaf = 0; leaf < 2; ++leaf) {
                        const float leafAngle = angle
                            + (leaf == 0 ? -0.46f : 0.34f)
                            + 0.07f * static_cast<float>((cluster + variant) % 3);
                        const Vector3 direction(std::cos(leafAngle), 0.0f,
                                                std::sin(leafAngle));
                        const Vector3 root = leafCentre + direction * 0.06f;
                        AppendPointedLeaf(
                            geometry, root, leafAngle,
                            0.88f + 0.08f * static_cast<float>(
                                (cluster + leaf + variant) % 3),
                            0.21f + 0.03f * static_cast<float>(
                                (cluster * 2 + leaf + variant) % 3),
                            0.09f + 0.025f * static_cast<float>(
                                (cluster + leaf + variant) % 3),
                            leaf == 0 ? ShadeColor(baseColor, 1.08f)
                                      : ShadeColor(baseColor, 0.91f),
                            std::fmod(phase + static_cast<float>(leaf) * 0.083f, 1.0f),
                            0.08f);
                    }
                }
            }
            return geometry;
        }

        void AppendUprightGroundLeaf(PlantGeometry& geometry, const float angle,
                                     const float height, const float reach,
                                     const float width, const Color& baseColor,
                                     const float windPhase)
        {
            const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const Vector3 root = direction * 0.035f;
            const Vector3 petioleEnd = direction * (reach * 0.28f)
                + Vector3::Up * (height * 0.31f);
            const float petioleWidth = 0.018f;
            const auto stemBase = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(root - side * petioleWidth,
                                           ShadeColor(baseColor, 0.62f),
                                           Vector2(0.08f, windPhase));
            geometry.vertices.emplace_back(root + side * petioleWidth,
                                           ShadeColor(baseColor, 0.72f),
                                           Vector2(0.08f, windPhase));
            geometry.vertices.emplace_back(petioleEnd - side * petioleWidth,
                                           ShadeColor(baseColor, 0.70f),
                                           Vector2(0.48f, windPhase));
            geometry.vertices.emplace_back(petioleEnd + side * petioleWidth,
                                           ShadeColor(baseColor, 0.80f),
                                           Vector2(0.48f, windPhase));
            geometry.indices.insert(geometry.indices.end(), {
                stemBase, static_cast<std::uint16_t>(stemBase + 2U),
                static_cast<std::uint16_t>(stemBase + 1U),
                static_cast<std::uint16_t>(stemBase + 1U),
                static_cast<std::uint16_t>(stemBase + 2U),
                static_cast<std::uint16_t>(stemBase + 3U)});

            const Vector3 shoulder = direction * (reach * 0.64f)
                + Vector3::Up * (height * 0.64f);
            const Vector3 ridge = shoulder + Vector3::Up * 0.045f;
            const Vector3 tip = direction * reach + Vector3::Up * height;
            const auto leafBase = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(petioleEnd, ShadeColor(baseColor, 0.76f),
                                           Vector2(0.48f, windPhase));
            geometry.vertices.emplace_back(shoulder - side * width,
                                           ShadeColor(baseColor, 0.90f),
                                           Vector2(0.76f, windPhase));
            geometry.vertices.emplace_back(ridge, ShadeColor(baseColor, 1.10f),
                                           Vector2(0.80f, windPhase));
            geometry.vertices.emplace_back(shoulder + side * width,
                                           ShadeColor(baseColor, 0.84f),
                                           Vector2(0.76f, windPhase));
            geometry.vertices.emplace_back(tip, ShadeColor(baseColor, 0.98f),
                                           Vector2(0.92f, windPhase));
            geometry.indices.insert(geometry.indices.end(), {
                leafBase, static_cast<std::uint16_t>(leafBase + 1U),
                static_cast<std::uint16_t>(leafBase + 2U),
                leafBase, static_cast<std::uint16_t>(leafBase + 2U),
                static_cast<std::uint16_t>(leafBase + 3U),
                static_cast<std::uint16_t>(leafBase + 1U),
                static_cast<std::uint16_t>(leafBase + 4U),
                static_cast<std::uint16_t>(leafBase + 2U),
                static_cast<std::uint16_t>(leafBase + 2U),
                static_cast<std::uint16_t>(leafBase + 4U),
                static_cast<std::uint16_t>(leafBase + 3U)});
        }

        void AppendSplitGroundLeaf(PlantGeometry& geometry, const float angle,
                                   const float length, const float width,
                                   const float lift, const Color& baseColor,
                                   const float windPhase)
        {
            const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const auto centre = [direction, length, lift](const float t) {
                return direction * (length * t)
                    + Vector3::Up * (lift * std::sin(t * std::numbers::pi_v<float>)
                                     - 0.10f * length * t * t);
            };

            const float petioleWidth = 0.017f;
            const Vector3 petioleEnd = centre(0.24f);
            const auto petioleBase = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(-side * petioleWidth,
                                           ShadeColor(baseColor, 0.60f),
                                           Vector2(0.04f, windPhase));
            geometry.vertices.emplace_back(side * petioleWidth,
                                           ShadeColor(baseColor, 0.70f),
                                           Vector2(0.04f, windPhase));
            geometry.vertices.emplace_back(petioleEnd - side * petioleWidth,
                                           ShadeColor(baseColor, 0.68f),
                                           Vector2(0.34f, windPhase));
            geometry.vertices.emplace_back(petioleEnd + side * petioleWidth,
                                           ShadeColor(baseColor, 0.78f),
                                           Vector2(0.34f, windPhase));
            geometry.indices.insert(geometry.indices.end(), {
                petioleBase, static_cast<std::uint16_t>(petioleBase + 2U),
                static_cast<std::uint16_t>(petioleBase + 1U),
                static_cast<std::uint16_t>(petioleBase + 1U),
                static_cast<std::uint16_t>(petioleBase + 2U),
                static_cast<std::uint16_t>(petioleBase + 3U)});

            constexpr int spineSegments = 4;
            constexpr float bladeStart = 0.22f;
            const float spineHalfWidth = width * 0.19f;
            const auto spineBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int segment = 0; segment <= spineSegments; ++segment) {
                const float t = bladeStart + (1.0f - bladeStart)
                    * static_cast<float>(segment) / static_cast<float>(spineSegments);
                const float taper = 1.0f - 0.72f * std::pow(t, 2.2f);
                const Vector3 point = centre(t) + Vector3::Up * (0.026f * std::sin(t * 3.1f));
                const Color color = ShadeColor(baseColor, 0.79f + 0.19f * t);
                geometry.vertices.emplace_back(point - side * (spineHalfWidth * taper), color,
                                               Vector2(t * t, windPhase));
                geometry.vertices.emplace_back(point + side * (spineHalfWidth * taper),
                                               ShadeColor(color, 0.94f),
                                               Vector2(t * t, windPhase));
            }
            for (int segment = 0; segment < spineSegments; ++segment) {
                const auto left = static_cast<std::uint16_t>(spineBase + segment * 2);
                const auto right = static_cast<std::uint16_t>(left + 1U);
                const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                geometry.indices.insert(geometry.indices.end(), {
                    left, nextLeft, right, right, nextLeft, nextRight});
            }

            constexpr int lobePairs = 3;
            for (int pair = 0; pair < lobePairs; ++pair) {
                const float pairT = 0.39f + static_cast<float>(pair) * 0.205f;
                const float halfSpan = 0.065f - static_cast<float>(pair) * 0.005f;
                const float extent = width * (1.0f - static_cast<float>(pair) * 0.17f);
                for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                    const float sideSign = sideIndex == 0 ? -1.0f : 1.0f;
                    const Vector3 innerStart = centre(pairT - halfSpan)
                        + side * (sideSign * spineHalfWidth);
                    const Vector3 innerMiddle = centre(pairT)
                        + side * (sideSign * spineHalfWidth * 0.72f)
                        + Vector3::Up * 0.040f;
                    const Vector3 innerEnd = centre(pairT + halfSpan)
                        + side * (sideSign * spineHalfWidth * 0.84f);
                    const Vector3 outerShoulder = centre(pairT - 0.025f)
                        + side * (sideSign * extent * 0.64f)
                        + Vector3::Up * 0.028f;
                    const Vector3 outerTip = centre(pairT + 0.028f)
                        + side * (sideSign * extent);
                    const auto lobeBase = static_cast<std::uint16_t>(geometry.vertices.size());
                    const float windWeight = pairT * pairT;
                    const Color lobeColor = sideIndex == 0
                        ? ShadeColor(baseColor, 0.91f + static_cast<float>(pair) * 0.045f)
                        : ShadeColor(baseColor, 1.02f - static_cast<float>(pair) * 0.035f);
                    geometry.vertices.emplace_back(innerStart, ShadeColor(lobeColor, 0.82f),
                                                   Vector2(windWeight, windPhase));
                    geometry.vertices.emplace_back(outerShoulder, lobeColor,
                                                   Vector2(windWeight, windPhase));
                    geometry.vertices.emplace_back(innerMiddle, ShadeColor(lobeColor, 1.10f),
                                                   Vector2(windWeight, windPhase));
                    geometry.vertices.emplace_back(outerTip, ShadeColor(lobeColor, 0.95f),
                                                   Vector2(windWeight, windPhase));
                    geometry.vertices.emplace_back(innerEnd, ShadeColor(lobeColor, 0.86f),
                                                   Vector2(windWeight, windPhase));
                    geometry.indices.insert(geometry.indices.end(), {
                        lobeBase, static_cast<std::uint16_t>(lobeBase + 1U),
                        static_cast<std::uint16_t>(lobeBase + 2U),
                        static_cast<std::uint16_t>(lobeBase + 2U),
                        static_cast<std::uint16_t>(lobeBase + 1U),
                        static_cast<std::uint16_t>(lobeBase + 3U),
                        static_cast<std::uint16_t>(lobeBase + 2U),
                        static_cast<std::uint16_t>(lobeBase + 3U),
                        static_cast<std::uint16_t>(lobeBase + 4U)});
                }
            }
        }

        void AppendFallenGroundLeaf(PlantGeometry& geometry, const float angle,
                                    const float radialOffset, const float length,
                                    const float width, const Color& baseColor,
                                    const float foldDirection)
        {
            const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const Vector3 root = direction * radialOffset + Vector3::Up * 0.021f;
            const Vector3 shoulder = root + direction * (length * 0.47f);
            const Vector3 ridge = shoulder + side * (width * foldDirection * 0.10f)
                + Vector3::Up * 0.031f;
            const Vector3 tip = root + direction * length + Vector3::Up * 0.008f;
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const Vector2 still(0.0f, 0.0f);
            geometry.vertices.emplace_back(root, ShadeColor(baseColor, 0.72f), still);
            geometry.vertices.emplace_back(shoulder - side * width,
                                           ShadeColor(baseColor, 0.83f), still);
            geometry.vertices.emplace_back(ridge, ShadeColor(baseColor, 1.08f), still);
            geometry.vertices.emplace_back(shoulder + side * width,
                                           ShadeColor(baseColor, 0.67f), still);
            geometry.vertices.emplace_back(tip, ShadeColor(baseColor, 0.78f), still);
            geometry.indices.insert(geometry.indices.end(), {
                base, static_cast<std::uint16_t>(base + 1U),
                static_cast<std::uint16_t>(base + 2U),
                base, static_cast<std::uint16_t>(base + 2U),
                static_cast<std::uint16_t>(base + 3U),
                static_cast<std::uint16_t>(base + 1U),
                static_cast<std::uint16_t>(base + 4U),
                static_cast<std::uint16_t>(base + 2U),
                static_cast<std::uint16_t>(base + 2U),
                static_cast<std::uint16_t>(base + 4U),
                static_cast<std::uint16_t>(base + 3U)});
        }

        [[nodiscard]] PlantGeometry CreateGroundPlantGeometry(const int variant,
                                                              const bool farMesh)
        {
            PlantGeometry geometry;
            const bool uprightBroadleaf = !farMesh && variant == 1;
            const bool splitLeaf = !farMesh && variant == 2;
            const int leafCount = uprightBroadleaf ? 6 : (splitLeaf ? 5 : (farMesh ? 4 : 7));
            const int segments = farMesh ? 2 : 4;
            const int verticesPerLeaf = uprightBroadleaf ? 9
                : (splitLeaf ? 44 : (segments + 1) * 2);
            const int indicesPerLeaf = uprightBroadleaf ? 18
                : (splitLeaf ? 84 : segments * 6);
            geometry.vertices.reserve(static_cast<std::size_t>(leafCount * verticesPerLeaf));
            geometry.indices.reserve(static_cast<std::size_t>(leafCount * indicesPerLeaf));
            const Color color = variant == 0 ? Color(54, 132, 55, 255)
                : (variant == 1 ? Color(45, 119, 48, 255)
                                : Color(65, 139, 62, 255));
            for (int leaf = 0; leaf < leafCount; ++leaf) {
                const float angle = static_cast<float>(variant) * 0.73f
                    + 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(leaf) / static_cast<float>(leafCount);
                const float variation = 0.86f
                    + 0.18f * static_cast<float>((leaf * 3 + variant) % 5) / 4.0f;
                const float length = (0.62f + 0.06f * static_cast<float>(variant)) * variation;
                const float width = (0.13f + 0.025f * static_cast<float>(variant)) * variation;
                const float lift = 0.43f + 0.055f * static_cast<float>(variant);
                const float phase = std::fmod(static_cast<float>(leaf) / leafCount
                                              + static_cast<float>(variant) * 0.27f, 1.0f);
                const Color leafColor = leaf % 3 == 0
                    ? ShadeColor(color, 1.10f) : color;
                if (uprightBroadleaf) {
                    AppendUprightGroundLeaf(
                        geometry, angle,
                        0.57f + 0.055f * static_cast<float>((leaf * 2 + variant) % 3),
                        0.44f + 0.055f * static_cast<float>((leaf + variant) % 3),
                        0.25f + 0.035f * static_cast<float>((leaf * 4 + variant) % 3),
                        leafColor, phase);
                } else if (splitLeaf) {
                    AppendSplitGroundLeaf(
                        geometry, angle,
                        0.68f + 0.045f * static_cast<float>((leaf + variant) % 3),
                        0.27f + 0.022f * static_cast<float>((leaf * 2 + variant) % 3),
                        0.49f + 0.035f * static_cast<float>((leaf * 3 + variant) % 2),
                        leafColor, phase);
                } else {
                    AppendFrond(geometry, angle, length, width, lift,
                                leafColor, phase, segments);
                }
            }
            if (!farMesh && variant == 0) {
                // Keep the extra forest-floor breakup sparse by attaching it to only one of the
                // three deterministic ground-plant variants. It rides the existing instance,
                // buffer, culling, and draw path instead of adding a separate litter batch.
                constexpr int fallenLeafCount = 3;
                for (int leaf = 0; leaf < fallenLeafCount; ++leaf) {
                    const float angle = 0.38f + static_cast<float>(leaf) * 2.17f;
                    const float length = 0.31f
                        + 0.045f * static_cast<float>((leaf * 2 + 1) % 3);
                    const float width = 0.082f
                        + 0.012f * static_cast<float>((leaf + 2) % 3);
                    const Color litterColor = leaf == 1
                        ? Color(103, 72, 29, 255)
                        : Color(128, 87 + leaf * 7, 31, 255);
                    AppendFallenGroundLeaf(
                        geometry, angle, 0.18f + static_cast<float>(leaf) * 0.075f,
                        length, width, litterColor, leaf % 2 == 0 ? -1.0f : 1.0f);
                }
            }
            return geometry;
        }

        [[nodiscard]] std::size_t CellIndex(const float x, const float z) noexcept
        {
            constexpr float cellWidth = 2.0f * TerrainMeshLayout::Extent
                / static_cast<float>(TerrainMeshLayout::CellsPerAxis);
            const int column = std::clamp(static_cast<int>(
                std::floor((x + TerrainMeshLayout::Extent) / cellWidth)),
                0, TerrainMeshLayout::CellsPerAxis - 1);
            const int row = std::clamp(static_cast<int>(
                std::floor((z + TerrainMeshLayout::Extent) / cellWidth)),
                0, TerrainMeshLayout::CellsPerAxis - 1);
            return static_cast<std::size_t>(row * TerrainMeshLayout::CellsPerAxis + column);
        }
    }

    const VertexDeclaration& JungleRenderer::InstanceDeclaration()
    {
        static const VertexDeclaration declaration{
            VertexElement(0, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 1),
            VertexElement(16, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 2),
            VertexElement(32, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 3),
            VertexElement(48, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 4)};
        return declaration;
    }

    JungleRenderer::JungleRenderer(GraphicsDevice& device, const JungleLayout& layout)
        : device_(device)
    {
        instancingSupported_ = device_.SupportsCapability(CNA::GraphicsCapability::Instancing);
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        fallbackEffect_->setFogStartProperty(250.0f);
        fallbackEffect_->setFogEndProperty(660.0f);

        if (!instancingSupported_) {
            fallbackReason_ = "the active renderer does not support hardware instancing";
        } else if (!device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            fallbackReason_ = "the active renderer does not support custom effects";
        } else {
            try {
                customEffect_ = std::make_unique<ShaderEffect>(device_, CreateShaderPackage());
                if (customEffect_->IsEffectValid()) {
                    customShaderReady_ = true;
                } else {
                    fallbackReason_ = "the jungle shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable jungle shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMeshes();
        CreateCells(layout);
        std::cout << "island-simulator: jungle " << totalCanopyCount_ << " canopy trees, "
                  << totalUnderstoryCount_ << " understory plants, " << totalBushCount_
                  << " low bushes, "
                  << (instancingSupported_ ? "hardware instancing" : "per-plant fallback")
                  << '\n';
        std::cout << "island-simulator: jungle ground layer " << totalGroundPlantCount_
                  << " small plants\n";
        if (customShaderReady_) {
            std::cout << "island-simulator: jungle portable wind shader with canopy/understory/bush/ground LOD\n";
            std::cout << "island-simulator: jungle near canopy tapered support branches and pointed leaf silhouettes\n";
            std::cout << CanopyLianaDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(CanopyLianaDiagnostic);
            std::cout << "island-simulator: jungle near canopy irregular buttress roots ground trunk bases\n";
            std::cout << "island-simulator: jungle near canopy procedural bark fissures\n";
            std::cout << "island-simulator: jungle near bushes with pointed leaf edge breakup\n";
            std::cout << "island-simulator: jungle near ground plants include upright and split-leaf silhouettes\n";
            constexpr const char* litterDiagnostic =
                "island-simulator: jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes";
            std::cout << litterDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(litterDiagnostic);
            std::cout << "island-simulator: jungle fixed-sun two-sided geometric facet lighting\n";
            constexpr const char* tintDiagnostic =
                "island-simulator: jungle per-instance foliage tint reuses existing transforms";
            std::cout << tintDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(tintDiagnostic);
        } else {
            std::cerr << "island-simulator: jungle stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    JungleRenderer::~JungleRenderer() = default;

    void JungleRenderer::CreateMeshes()
    {
        const auto upload = [this](const PlantGeometry& geometry, PlantMesh& mesh,
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
        for (std::size_t variant = 0; variant < canopyNearMeshes_.size(); ++variant) {
            upload(CreateCanopyGeometry(static_cast<int>(variant), false),
                   canopyNearMeshes_[variant], "canopy near LOD");
            upload(CreateCanopyGeometry(static_cast<int>(variant), true),
                   canopyFarMeshes_[variant], "canopy far LOD");
        }
        for (std::size_t variant = 0; variant < understoryNearMeshes_.size(); ++variant) {
            upload(CreateUnderstoryGeometry(static_cast<int>(variant), false),
                   understoryNearMeshes_[variant], "understory near LOD");
            upload(CreateUnderstoryGeometry(static_cast<int>(variant), true),
                   understoryFarMeshes_[variant], "understory far LOD");
        }
        for (std::size_t variant = 0; variant < bushNearMeshes_.size(); ++variant) {
            upload(CreateBushGeometry(static_cast<int>(variant), false),
                   bushNearMeshes_[variant], "bush near LOD");
            upload(CreateBushGeometry(static_cast<int>(variant), true),
                   bushFarMeshes_[variant], "bush far LOD");
        }
        for (std::size_t variant = 0; variant < groundPlantNearMeshes_.size(); ++variant) {
            upload(CreateGroundPlantGeometry(static_cast<int>(variant), false),
                   groundPlantNearMeshes_[variant], "ground-plant near LOD");
            upload(CreateGroundPlantGeometry(static_cast<int>(variant), true),
                   groundPlantFarMeshes_[variant], "ground-plant far LOD");
        }
    }

    void JungleRenderer::CreateCells(const JungleLayout& layout)
    {
        const auto extendBounds = [](JungleCell& cell, const Vector3& minimum,
                                     const Vector3& maximum) {
            if (!cell.occupied) {
                cell.bounds = BoundingBox(minimum, maximum);
                cell.occupied = true;
            } else {
                cell.bounds = BoundingBox(Vector3::Min(cell.bounds.Min, minimum),
                                          Vector3::Max(cell.bounds.Max, maximum));
            }
        };
        for (const CanopyPlacement& tree : layout.Canopy()) {
            JungleCell& cell = cells_[CellIndex(tree.x, tree.z)];
            const float normalizedYaw = tree.yawRadians
                / (2.0f * std::numbers::pi_v<float>);
            const float nearLodDistance = CanopyNearMinimumDistance
                + CanopyNearDistanceRange * normalizedYaw;
            cell.canopy.push_back({Matrix::CreateScale(tree.scale)
                                       * Matrix::CreateRotationY(tree.yawRadians)
                                       * Matrix::CreateTranslation(tree.x, tree.y, tree.z),
                                   tree.variant, tree.x, tree.z,
                                   nearLodDistance * nearLodDistance});
            const float radius = 6.2f * tree.scale;
            extendBounds(cell, Vector3(tree.x - radius, tree.y - 0.1f, tree.z - radius),
                         Vector3(tree.x + radius, tree.y + 16.5f * tree.scale, tree.z + radius));
            ++totalCanopyCount_;
        }
        for (const UnderstoryPlacement& plant : layout.Understory()) {
            JungleCell& cell = cells_[CellIndex(plant.x, plant.z)];
            cell.understory.push_back({Matrix::CreateScale(plant.scale)
                                           * Matrix::CreateRotationY(plant.yawRadians)
                                           * Matrix::CreateTranslation(plant.x, plant.y, plant.z),
                                       plant.variant, plant.x, plant.z,
                                       UnderstoryNearDistance * UnderstoryNearDistance});
            const float radius = 2.2f * plant.scale;
            extendBounds(cell, Vector3(plant.x - radius, plant.y - 0.05f, plant.z - radius),
                         Vector3(plant.x + radius, plant.y + 2.2f * plant.scale, plant.z + radius));
            ++totalUnderstoryCount_;
        }
        for (const BushPlacement& bush : layout.Bushes()) {
            JungleCell& cell = cells_[CellIndex(bush.x, bush.z)];
            cell.bushes.push_back({Matrix::CreateScale(bush.scale)
                                       * Matrix::CreateRotationY(bush.yawRadians)
                                       * Matrix::CreateTranslation(bush.x, bush.y, bush.z),
                                   bush.variant, bush.x, bush.z,
                                   BushNearDistance * BushNearDistance});
            const float radius = 1.35f * bush.scale;
            extendBounds(cell, Vector3(bush.x - radius, bush.y - 0.03f, bush.z - radius),
                         Vector3(bush.x + radius, bush.y + 1.2f * bush.scale,
                                 bush.z + radius));
            ++totalBushCount_;
        }
        for (const GroundPlantPlacement& plant : layout.GroundPlants()) {
            JungleCell& cell = cells_[CellIndex(plant.x, plant.z)];
            cell.groundPlants.push_back({Matrix::CreateScale(plant.scale)
                                             * Matrix::CreateRotationY(plant.yawRadians)
                                             * Matrix::CreateTranslation(
                                                 plant.x, plant.y, plant.z),
                                         plant.variant, plant.x, plant.z,
                                         GroundPlantNearDistance * GroundPlantNearDistance});
            const float radius = 0.78f * plant.scale;
            extendBounds(cell, Vector3(plant.x - radius, plant.y - 0.02f, plant.z - radius),
                         Vector3(plant.x + radius, plant.y + 0.82f * plant.scale,
                                 plant.z + radius));
            ++totalGroundPlantCount_;
        }
        for (auto& transforms : visibleCanopyNear_) transforms.reserve(layout.Canopy().size());
        for (auto& transforms : visibleCanopyFar_) transforms.reserve(layout.Canopy().size());
        for (auto& transforms : visibleUnderstoryNear_) transforms.reserve(layout.Understory().size());
        for (auto& transforms : visibleUnderstoryFar_) transforms.reserve(layout.Understory().size());
        for (auto& transforms : visibleBushNear_) transforms.reserve(layout.Bushes().size());
        for (auto& transforms : visibleBushFar_) transforms.reserve(layout.Bushes().size());
        for (auto& transforms : visibleGroundPlantNear_)
            transforms.reserve(layout.GroundPlants().size());
        for (auto& transforms : visibleGroundPlantFar_)
            transforms.reserve(layout.GroundPlants().size());
    }

    void JungleRenderer::DrawInstances(PlantMesh& mesh, const std::vector<Matrix>& transforms)
    {
        if (transforms.empty()) return;
        device_.setIndicesProperty(mesh.indices.get());
        if (instancingSupported_) {
            const int count = static_cast<int>(transforms.size());
            if (mesh.instances == nullptr || count > mesh.instanceCapacity) {
                int capacity = std::max(16, mesh.instanceCapacity);
                while (capacity < count) capacity *= 2;
                mesh.instances = std::make_unique<DynamicVertexBuffer>(
                    device_, InstanceDeclaration(), capacity, BufferUsage::WriteOnly);
                mesh.instanceCapacity = capacity;
            }
            mesh.instances->SetDataRaw(transforms.data(), count, static_cast<int>(sizeof(Matrix)));
            device_.SetVertexBuffers({VertexBufferBinding(mesh.vertices.get(), 0, 0),
                                      VertexBufferBinding(mesh.instances.get(), 0, 1)});
            if (!customShaderReady_) {
                fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
                fallbackEffect_->Apply();
            }
            device_.DrawInstancedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                            mesh.vertexCount, 0, mesh.primitiveCount, count);
            ++drawCallCount_;
            return;
        }
        device_.SetVertexBuffer(mesh.vertices.get());
        for (const Matrix& transform : transforms) {
            fallbackEffect_->setWorldProperty(transform);
            fallbackEffect_->Apply();
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                          mesh.vertexCount, 0, mesh.primitiveCount);
            ++drawCallCount_;
        }
    }

    void JungleRenderer::Draw(const Matrix& view, const Matrix& projection,
                              const Vector3& cameraPosition, const float timeSeconds)
    {
        for (auto& transforms : visibleCanopyNear_) transforms.clear();
        for (auto& transforms : visibleCanopyFar_) transforms.clear();
        for (auto& transforms : visibleUnderstoryNear_) transforms.clear();
        for (auto& transforms : visibleUnderstoryFar_) transforms.clear();
        for (auto& transforms : visibleBushNear_) transforms.clear();
        for (auto& transforms : visibleBushFar_) transforms.clear();
        for (auto& transforms : visibleGroundPlantNear_) transforms.clear();
        for (auto& transforms : visibleGroundPlantFar_) transforms.clear();
        visibleCanopyCount_ = 0;
        visibleCanopyNearCount_ = 0;
        visibleCanopyFarCount_ = 0;
        visibleUnderstoryCount_ = 0;
        visibleUnderstoryNearCount_ = 0;
        visibleUnderstoryFarCount_ = 0;
        visibleBushCount_ = 0;
        visibleBushNearCount_ = 0;
        visibleBushFarCount_ = 0;
        visibleGroundPlantCount_ = 0;
        visibleGroundPlantNearCount_ = 0;
        visibleGroundPlantFarCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        const BoundingFrustum frustum(view * projection);
        for (const JungleCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const float cellDeltaX = std::max(
                {cell.bounds.Min.X - cameraPosition.X, 0.0f,
                 cameraPosition.X - cell.bounds.Max.X});
            const float cellDeltaZ = std::max(
                {cell.bounds.Min.Z - cameraPosition.Z, 0.0f,
                 cameraPosition.Z - cell.bounds.Max.Z});
            const float cellDistanceSquared = cellDeltaX * cellDeltaX
                + cellDeltaZ * cellDeltaZ;
            bool cellVisible = false;
            if (cellDistanceSquared <= CanopyMaximumDistance * CanopyMaximumDistance) {
                for (const DrawInstance& instance : cell.canopy) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > CanopyMaximumDistance * CanopyMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= instance.nearLodDistanceSquared;
                    (nearLod ? visibleCanopyNear_ : visibleCanopyFar_)[instance.variant]
                        .push_back(instance.transform);
                    if (nearLod) {
                        ++visibleCanopyNearCount_;
                    } else {
                        ++visibleCanopyFarCount_;
                    }
                    ++visibleCanopyCount_;
                    cellVisible = true;
                }
            }
            if (cellDistanceSquared <= UnderstoryMaximumDistance * UnderstoryMaximumDistance) {
                for (const DrawInstance& instance : cell.understory) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > UnderstoryMaximumDistance * UnderstoryMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= instance.nearLodDistanceSquared;
                    (nearLod ? visibleUnderstoryNear_ : visibleUnderstoryFar_)[instance.variant]
                        .push_back(instance.transform);
                    if (nearLod) {
                        ++visibleUnderstoryNearCount_;
                    } else {
                        ++visibleUnderstoryFarCount_;
                    }
                    ++visibleUnderstoryCount_;
                    cellVisible = true;
                }
            }
            if (cellDistanceSquared <= BushMaximumDistance * BushMaximumDistance) {
                for (const DrawInstance& instance : cell.bushes) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared > BushMaximumDistance * BushMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= instance.nearLodDistanceSquared;
                    (nearLod ? visibleBushNear_ : visibleBushFar_)[instance.variant]
                        .push_back(instance.transform);
                    if (nearLod) {
                        ++visibleBushNearCount_;
                    } else {
                        ++visibleBushFarCount_;
                    }
                    ++visibleBushCount_;
                    cellVisible = true;
                }
            }
            if (cellDistanceSquared <= GroundPlantMaximumDistance * GroundPlantMaximumDistance) {
                for (const DrawInstance& instance : cell.groundPlants) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > GroundPlantMaximumDistance * GroundPlantMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= instance.nearLodDistanceSquared;
                    (nearLod ? visibleGroundPlantNear_ : visibleGroundPlantFar_)[instance.variant]
                        .push_back(instance.transform);
                    if (nearLod) {
                        ++visibleGroundPlantNearCount_;
                    } else {
                        ++visibleGroundPlantFarCount_;
                    }
                    ++visibleGroundPlantCount_;
                    cellVisible = true;
                }
            }
            if (cellVisible) ++visibleCellCount_;
        }

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
            customEffect_->SetUniformFloat("uTime", timeSeconds);
        } else {
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        for (std::size_t variant = 0; variant < canopyNearMeshes_.size(); ++variant) {
            DrawInstances(canopyNearMeshes_[variant], visibleCanopyNear_[variant]);
            DrawInstances(canopyFarMeshes_[variant], visibleCanopyFar_[variant]);
        }
        for (std::size_t variant = 0; variant < understoryNearMeshes_.size(); ++variant) {
            DrawInstances(understoryNearMeshes_[variant], visibleUnderstoryNear_[variant]);
            DrawInstances(understoryFarMeshes_[variant], visibleUnderstoryFar_[variant]);
        }
        for (std::size_t variant = 0; variant < bushNearMeshes_.size(); ++variant) {
            DrawInstances(bushNearMeshes_[variant], visibleBushNear_[variant]);
            DrawInstances(bushFarMeshes_[variant], visibleBushFar_[variant]);
        }
        for (std::size_t variant = 0; variant < groundPlantNearMeshes_.size(); ++variant) {
            DrawInstances(groundPlantNearMeshes_[variant], visibleGroundPlantNear_[variant]);
            DrawInstances(groundPlantFarMeshes_[variant], visibleGroundPlantFar_[variant]);
        }
        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
