#include "IslandSimulator/NaturalPropRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/RockSurface.hpp"
#include "IslandSimulator/SurfaceNormals.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numbers>
#include <string>
#include <string_view>
#include <type_traits>

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
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float RockNearDistance = 92.0f;
        constexpr float RockMaximumDistance = 285.0f;
        constexpr float LogNearDistance = 72.0f;
        constexpr float LogMaximumDistance = 175.0f;
        constexpr float DriftwoodNearDistance = 58.0f;
        constexpr float DriftwoodMaximumDistance = 145.0f;
        constexpr float BeachDetailMaximumDistance = 52.0f;
        constexpr float FallenFrondMaximumDistance = 76.0f;
        constexpr float ReefHabitatNearDistance = 46.0f;
        constexpr float ReefHabitatMaximumDistance = 138.0f;
        constexpr std::string_view RockFacetDiagnostic =
            "island-simulator: rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering";
        constexpr std::string_view RockTintDiagnostic =
            "island-simulator: rock per-instance tint reuses existing transforms";
        constexpr std::string_view FallenLogEndDiagnostic =
            "island-simulator: near fallen logs use irregular broken end contours";
        constexpr std::string_view FallenLogCrackDiagnostic =
            "island-simulator: near fallen-log end grain uses restrained radial cracks";
        constexpr std::string_view DriftwoodBranchDiagnostic =
            "island-simulator: all near driftwood variants use tapered three-dimensional branches";
        constexpr std::string_view DriftwoodEndDiagnostic =
            "island-simulator: driftwood end caps match irregular body rings";
        constexpr std::string_view FallenFrondDiagnostic =
            "island-simulator: fallen fronds use curved asymmetric leaflet silhouettes";
        constexpr std::string_view FallenFrondFoldDiagnostic =
            "island-simulator: fallen frond leaflets use geometric central folds";
        constexpr std::string_view StaghornCoralDiagnostic =
            "island-simulator: branching reef form uses dense upright staghorn stems, offset side forks, and small growing tips";
        constexpr int FallenFrondMaterialDetail = 248;

        struct PropGeometry
        {
            std::vector<VertexPositionColor> vertices;
            std::vector<std::uint16_t> indices;
            std::vector<Vector3> normals;
            std::vector<Color> albedos;
        };

        struct PropGpuVertex
        {
            float position[3];
            std::uint8_t color[4];
            float normal[3];
        };
        static_assert(std::is_trivially_copyable_v<PropGpuVertex>);
        static_assert(std::is_standard_layout_v<PropGpuVertex> && sizeof(PropGpuVertex) == 28);

        const VertexDeclaration& PropDeclaration()
        {
            static const VertexDeclaration declaration{
                VertexElement(offsetof(PropGpuVertex, position), VertexElementFormat::Vector3, VertexElementUsage::Position, 0),
                VertexElement(offsetof(PropGpuVertex, color), VertexElementFormat::Color, VertexElementUsage::Color, 0),
                VertexElement(offsetof(PropGpuVertex, normal), VertexElementFormat::Vector3, VertexElementUsage::Normal, 0)};
            return declaration;
        }

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aInstance0;
layout(location = 4) in vec4 aInstance1;
layout(location = 5) in vec4 aInstance2;
layout(location = 6) in vec4 aInstance3;

uniform mat4 uViewProjection;

out vec3 vColor;
out vec3 vLocal;
out vec3 vWorld;
out float vMaterialDetail;
out vec3 vNormal;

void main()
{
    mat4 instanceTransform = mat4(aInstance0, aInstance1, aInstance2, aInstance3);
    vec4 world = instanceTransform * vec4(aPosition, 1.0);
    // Neutral stone is identifiable from the existing vertex colour, including the far LOD.
    // Reuse scale/yaw coefficients as a stable per-placement seed so copies of one rock mesh
    // gain restrained warm/cool variation without another instance attribute or draw.
    float tintSeed = fract(abs(aInstance0.x * 1.73 + aInstance0.z * 2.41
        + aInstance2.x * 3.29 + aInstance2.z * 4.17));
    float signedTint = tintSeed - 0.5;
    float colorMaximum = max(aColor.r, max(aColor.g, aColor.b));
    float colorMinimum = min(aColor.r, min(aColor.g, aColor.b));
    float relativeChroma = (colorMaximum - colorMinimum) / max(colorMaximum, 0.0001);
    float stoneMask = 1.0 - smoothstep(0.28, 0.34, relativeChroma);
    vec3 stoneTint = vec3(1.0 + signedTint * 0.12,
                          1.0 + signedTint * 0.055,
                          1.0 - signedTint * 0.10);
    vColor = aColor.rgb * mix(vec3(1.0), stoneTint, stoneMask);
    vLocal = aPosition;
    vWorld = world.xyz;
    vMaterialDetail = aColor.a;
    // Inverse transpose for orthogonal scale/yaw columns; no per-fragment matrix inverse.
    vNormal = aInstance0.xyz * (aNormal.x / max(dot(aInstance0.xyz, aInstance0.xyz), 0.000001))
        + aInstance1.xyz * (aNormal.y / max(dot(aInstance1.xyz, aInstance1.xyz), 0.000001))
        + aInstance2.xyz * (aNormal.z / max(dot(aInstance2.xyz, aInstance2.xyz), 0.000001));
    vNormal *= inversesqrt(max(dot(vNormal,vNormal), 0.000001));
    gl_Position = uViewProjection * world;
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vLocal;
in vec3 vWorld;
in float vMaterialDetail;
in vec3 vNormal;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;

out vec4 fragColor;

float stoneHash(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float stoneNoise(vec3 p)
{
    vec3 cell = floor(p);
    vec3 t = fract(p);
    t = t*t*(3.0-2.0*t);
    return mix(mix(mix(stoneHash(cell), stoneHash(cell+vec3(1,0,0)), t.x),
                   mix(stoneHash(cell+vec3(0,1,0)), stoneHash(cell+vec3(1,1,0)), t.x), t.y),
               mix(mix(stoneHash(cell+vec3(0,0,1)), stoneHash(cell+vec3(1,0,1)), t.x),
                   mix(stoneHash(cell+vec3(0,1,1)), stoneHash(cell+vec3(1,1,1)), t.x), t.y), t.z);
}

float filteredStoneNoise(vec3 p, float footprint)
{
    return mix(stoneNoise(p), 0.5, smoothstep(0.22, 0.65, footprint));
}

void main()
{
    // Derivatives must execute outside distance/material branches. The warped grain uses a
    // conservative Jacobian bound (5.5 + sqrt(3)*1.4*sqrt(3)*1.5*0.85 < 11).
    float worldFootprint = max(length(dFdx(vWorld)), length(dFdy(vWorld)));
    vec3 faceVector = cross(dFdx(vWorld), dFdy(vWorld));
    float distanceToCamera = length(vWorld - uCameraPosition);
    float fog = smoothstep(235.0, 610.0, distanceToCamera);
    vec3 materialColor = vColor;
    if (vMaterialDetail < 0.125) {
        vec3 normal = vNormal * inversesqrt(max(dot(vNormal,vNormal), 0.000001));
        float direct = max(dot(normal, normalize(uToSun)), 0.0);
        float sky = max(normal.y, 0.0);
        float depth = max(-vWorld.y, 0.0);
        vec3 underwater = mix(vec3(1.0), vec3(0.70,0.86,0.82),
                              smoothstep(0.4,6.5,depth)*0.34);
        materialColor = vColor * underwater * (0.56 + 0.34*direct + 0.14*sky);
    } else if (vMaterialDetail < 0.375) {
        vec3 normal = vNormal * inversesqrt(max(dot(vNormal,vNormal), 0.000001));
        float direct = max(dot(normal, normalize(uToSun)), 0.0);
        float sky = max(normal.y, 0.0);
        float broad = filteredStoneNoise(vWorld * 0.85, worldFootprint * 0.85);
        float nearDetail = 1.0 - smoothstep(55.0, 90.0, distanceToCamera);
        float grain = 0.5;
        float pores = 0.5;
        if (nearDetail > 0.0) {
            grain = filteredStoneNoise(vWorld * 5.5 + vec3(broad * 1.4), worldFootprint * 11.0);
            pores = filteredStoneNoise(vWorld * 28.0, worldFootprint * 28.0);
        }
        float mineral = smoothstep(0.56,0.78,grain);
        vec3 albedo = vColor * (0.86 + 0.25 * broad);
        albedo = mix(albedo, albedo * vec3(1.17,1.13,1.05), mineral * 0.55 * nearDetail);
        albedo *= 1.0 + (pores-0.5) * 0.22 * nearDetail;
        float lichen = smoothstep(0.58,0.78,broad) * smoothstep(-0.1,0.6,normal.y)
            * smoothstep(0.25,1.0,vWorld.y);
        albedo = mix(albedo, albedo * vec3(0.81,1.02,0.78), lichen * 0.3);
        float wetBand = 1.0 - smoothstep(-0.10, 0.26 + 0.10*grain, vWorld.y);
        albedo *= mix(vec3(1.0), vec3(0.64,0.71,0.73), wetBand);
        materialColor = albedo * (0.55 + 0.43*direct + 0.13*sky);
    } else if (vMaterialDetail < 0.625) {
        float barkAngle = atan(vLocal.y, vLocal.z);
        float coarseGroove = 0.5 + 0.5 * sin(
            barkAngle * 8.0 + vLocal.x * 3.5 + sin(vLocal.x * 12.0) * 0.45);
        float fissure = 1.0 - smoothstep(0.02, 0.24, abs(sin(
            barkAngle * 11.0 + sin(vLocal.x * 13.0) * 0.55)));
        vec3 detailedColor = vColor
            * (0.94 + 0.09 * coarseGroove) * (1.0 - 0.15 * fissure);
        materialColor = mix(vColor, detailedColor,
                            1.0 - smoothstep(42.0, 70.0, distanceToCamera));
    } else if (vMaterialDetail < 0.875) {
        float cutRadius = length(vLocal.yz);
        float cutAngle = atan(vLocal.y, vLocal.z);
        float growthRing = 0.5 + 0.5 * sin(
            cutRadius * 19.0 + sin(cutAngle * 3.0) * 0.38
            + sin(cutAngle * 7.0) * 0.14);
        vec3 detailedColor = materialColor
            * (0.94 + 0.08 * smoothstep(0.18, 0.82, growthRing));
        float crackA = smoothstep(0.9970, 0.9996, cos(
            cutAngle - 0.48 - sin(cutRadius * 7.0) * 0.040));
        float crackB = smoothstep(0.9974, 0.9997, cos(
            cutAngle + 1.16 - sin(cutRadius * 8.5) * 0.032));
        float crackC = smoothstep(0.9977, 0.99975, cos(
            cutAngle - 2.54 + sin(cutRadius * 6.2) * 0.028));
        float crackReach = smoothstep(0.16, 0.34, cutRadius)
            * (0.46 + 0.54 * smoothstep(0.38, 0.86, cutRadius));
        float radialCrack = max(crackA, max(crackB, crackC)) * crackReach;
        detailedColor *= 1.0 - 0.085 * radialCrack;
        materialColor = mix(vColor, detailedColor,
                            1.0 - smoothstep(42.0, 70.0, distanceToCamera));
    } else if (vMaterialDetail < 0.9375) {
        float woodAngle = atan(vLocal.y, vLocal.z);
        float weathering = 0.5 + 0.5 * sin(
            woodAngle * 6.0 + sin(vLocal.x * 13.0) * 0.62);
        float dryCrease = 1.0 - smoothstep(0.03, 0.21, abs(sin(
            woodAngle * 9.0 + sin(vLocal.x * 8.0) * 0.48)));
        vec3 detailedColor = vColor
            * (0.96 + 0.06 * weathering) * (1.0 - 0.07 * dryCrease);
        materialColor = mix(vColor, detailedColor,
                            1.0 - smoothstep(30.0, 56.0, distanceToCamera));
    } else if (vMaterialDetail < 0.99) {
        float inverseFaceLength = inversesqrt(max(dot(faceVector, faceVector), 0.00000001));
        vec3 faceNormal = faceVector * inverseFaceLength;
        float twoSidedDiffuse = abs(dot(faceNormal, uToSun));
        float upwardFill = abs(faceNormal.y);
        materialColor *= 0.86 + twoSidedDiffuse * 0.16 + upwardFill * 0.04;
    }
    // Wood albedo is not prelit. Reuse the stone stream's scale-correct normal transform,
    // including far wood (material 255); zero-normal decorative/contact meshes stay unchanged.
    if (vMaterialDetail >= 0.375 && dot(vNormal, vNormal) > 0.000001) {
        vec3 normal = normalize(vNormal);
        float direct = max(dot(normal, normalize(uToSun)), 0.0);
        materialColor *= 0.55 + 0.43*direct + 0.13*max(normal.y, 0.0);
    }
    vec3 color = mix(materialColor, vec3(0.53, 0.78, 0.88), fog);
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
                                  CNA::ShaderStageEXT::Vertex, "main", "props/props.es.vert.glsl",
                                  ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main", "props/props.es.frag.glsl",
                                  ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main", "props/props.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main", "props/props.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody)),
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

        void AppendContactDisk(PropGeometry& geometry, const int sides)
        {
            const auto centre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(Vector3(0.0f, 0.055f, 0.0f),
                                           Color(38, 63, 37, 255));
            for (int side = 0; side < sides; ++side) {
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                geometry.vertices.emplace_back(
                    Vector3(std::cos(angle) * 0.76f, 0.055f, std::sin(angle) * 0.76f),
                    Color(38, 63, 37, 255));
            }
            for (int side = 0; side < sides; ++side) {
                geometry.indices.insert(geometry.indices.end(),
                                        {centre,
                                         static_cast<std::uint16_t>(centre + 1U + side),
                                         static_cast<std::uint16_t>(
                                             centre + 1U + (side + 1) % sides)});
            }
        }

        [[nodiscard]] PropGeometry CreateRockGeometry(const int variant, const bool farMesh)
        {
            PropGeometry geometry;
            const int slices = farMesh ? 7 : 24;
            AppendContactDisk(geometry, slices);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.normals.resize(base, Vector3::Zero);
            for (const auto& vertex : geometry.vertices) geometry.albedos.push_back(vertex.Color);
            const int materialDetail = farMesh ? 255 : 64;
            const Color rockBase = variant == 0 ? Color(112, 113, 98, materialDetail)
                : (variant == 1 ? Color(126, 115, 92, materialDetail)
                   : (variant == 2 ? Color(98, 116, 104, materialDetail)
                                   : Color(124, 128, 116, materialDetail)));
            const Color albedo(rockBase.getRProperty(), rockBase.getGProperty(),
                               rockBase.getBProperty(), static_cast<std::uint8_t>(64));
            const auto surface = BuildRockSurface(variant, farMesh);
            for (const auto& vertex : surface.vertices) {
                const Vector3 normal(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
                const float light = 0.72f + 0.30f * std::max(0.0f,
                    normal.X * 0.54f + normal.Y * 0.77f - normal.Z * 0.18f);
                geometry.vertices.emplace_back(Vector3(vertex.position[0], vertex.position[1], vertex.position[2]),
                                                ShadeColor(rockBase, light));
                geometry.normals.push_back(normal);
                geometry.albedos.push_back(albedo);
            }
            for (const auto index : surface.indices) geometry.indices.push_back(base + index);
            return geometry;
        }

        void BuildWoodNormals(PropGeometry& geometry)
        {
            std::vector<std::array<float, 3>> positions;
            positions.reserve(geometry.vertices.size());
            for (const auto& vertex : geometry.vertices) {
                positions.push_back({vertex.Position.X, vertex.Position.Y, vertex.Position.Z});
            }
            for (const auto& normal : BuildSurfaceNormals(positions, geometry.indices)) {
                geometry.normals.emplace_back(normal[0], normal[1], normal[2]);
            }
        }

        void AppendReefTube(PropGeometry& geometry, const Vector3& start,
                            const Vector3& end, const float baseRadius,
                            const float tipRadius, const int sides, const Color& color)
        {
            const Vector3 axis = Vector3::Normalize(end - start);
            Vector3 right = Vector3::Cross(Vector3::Up, axis);
            if (right.LengthSquared() < 1.0e-6f) right = Vector3::Right;
            else right = Vector3::Normalize(right);
            const Vector3 around = Vector3::Normalize(Vector3::Cross(right, axis));
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int ring = 0; ring < 2; ++ring) {
                const Vector3 centre = ring == 0 ? start : end;
                const float radius = ring == 0 ? baseRadius : tipRadius;
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    geometry.vertices.emplace_back(centre + radius
                        * (right * std::cos(angle) + around * std::sin(angle)), color);
                    geometry.albedos.push_back(color);
                }
            }
            for (int side = 0; side < sides; ++side) {
                const int next = (side + 1) % sides;
                const auto lower = static_cast<std::uint16_t>(base + side);
                const auto lowerNext = static_cast<std::uint16_t>(base + next);
                const auto upper = static_cast<std::uint16_t>(base + sides + side);
                const auto upperNext = static_cast<std::uint16_t>(base + sides + next);
                geometry.indices.insert(geometry.indices.end(),
                    {lower, upper, lowerNext, lowerNext, upper, upperNext});
            }
        }

        void AppendReefPlate(PropGeometry& geometry, const Vector3& centre,
                             const float radius, const int sides, const float phase,
                             const Color& color)
        {
            const float halfThickness = .055f;
            const auto topCentre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(centre + Vector3::Up * halfThickness, color);
            geometry.albedos.push_back(color);
            const auto bottomCentre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(centre - Vector3::Up * halfThickness, color);
            geometry.albedos.push_back(color);
            const auto rim = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int layer = 0; layer < 2; ++layer) {
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float irregular = radius * (1.0f + .10f
                        * std::sin(angle * 3.0f + phase) + .055f * std::sin(angle * 5.0f));
                    geometry.vertices.emplace_back(centre + Vector3(std::cos(angle) * irregular,
                        (layer == 0 ? halfThickness : -halfThickness)
                            + .025f * std::sin(angle * 2.0f + phase),
                        std::sin(angle) * irregular), color);
                    geometry.albedos.push_back(color);
                }
            }
            for (int side = 0; side < sides; ++side) {
                const int next = (side + 1) % sides;
                const auto top = static_cast<std::uint16_t>(rim + side);
                const auto topNext = static_cast<std::uint16_t>(rim + next);
                const auto bottom = static_cast<std::uint16_t>(rim + sides + side);
                const auto bottomNext = static_cast<std::uint16_t>(rim + sides + next);
                geometry.indices.insert(geometry.indices.end(),
                    {topCentre, topNext, top, bottomCentre, bottom, bottomNext,
                     top, topNext, bottom, bottom, topNext, bottomNext});
            }
        }

        void AppendReefMound(PropGeometry& geometry, const Vector3& centre,
                             const float radius, const float height, const int rings,
                             const int slices, const float phase, const Color& color)
        {
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int ring = 0; ring < rings; ++ring) {
                const float t = static_cast<float>(ring) / static_cast<float>(rings);
                const float latitude = t * std::numbers::pi_v<float> * .5f;
                for (int slice = 0; slice < slices; ++slice) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(slice) / static_cast<float>(slices);
                    const float radial = radius * std::cos(latitude)
                        * (1.0f + .075f * std::sin(angle * 5.0f + phase));
                    geometry.vertices.emplace_back(centre + Vector3(std::cos(angle) * radial,
                        .025f + height * std::sin(latitude), std::sin(angle) * radial), color);
                    geometry.albedos.push_back(color);
                }
            }
            const auto top = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(centre + Vector3::Up * (height + .025f), color);
            geometry.albedos.push_back(color);
            for (int ring = 0; ring + 1 < rings; ++ring) {
                for (int slice = 0; slice < slices; ++slice) {
                    const int next = (slice + 1) % slices;
                    const auto a = static_cast<std::uint16_t>(base + ring*slices + slice);
                    const auto b = static_cast<std::uint16_t>(base + (ring+1)*slices + slice);
                    const auto c = static_cast<std::uint16_t>(base + ring*slices + next);
                    const auto d = static_cast<std::uint16_t>(base + (ring+1)*slices + next);
                    geometry.indices.insert(geometry.indices.end(), {a,b,c,c,b,d});
                }
            }
            const int lastRing = rings - 1;
            for (int slice = 0; slice < slices; ++slice) {
                geometry.indices.insert(geometry.indices.end(),
                    {static_cast<std::uint16_t>(base + lastRing*slices + slice), top,
                     static_cast<std::uint16_t>(base + lastRing*slices + (slice+1)%slices)});
            }
        }

        [[nodiscard]] PropGeometry CreateReefHabitatGeometry(const int variant,
                                                              const bool farMesh)
        {
            if (variant == 3) {
                PropGeometry rock = CreateRockGeometry(2, farMesh);
                const Color underwaterRock(79, 94, 84, 64);
                for (auto& vertex : rock.vertices) vertex.Color = underwaterRock;
                std::fill(rock.albedos.begin(), rock.albedos.end(), underwaterRock);
                return rock;
            }

            PropGeometry geometry;
            const int sides = farMesh ? 5 : 7;
            if (variant == 0) {
                const Color coral(202, 126, 101, 16);
                const Color growingTip = ShadeColor(coral, 1.12f);
                AppendReefMound(geometry, Vector3(0,-.04f,0), .31f, .20f,
                    farMesh ? 2 : 3, farMesh ? 6 : 9, .4f, ShadeColor(coral, .82f));
                const int branches = farMesh ? 5 : 11;
                for (int branch = 0; branch < branches; ++branch) {
                    const float angle = .31f + static_cast<float>(branch) * 2.39996f;
                    const float startRadius = .055f
                        + .022f * static_cast<float>(branch % 3);
                    const Vector3 start(std::cos(angle) * startRadius, .08f,
                                        std::sin(angle) * startRadius);
                    const float reach = .22f
                        + .055f * static_cast<float>((branch * 2) % 5);
                    const float height = .66f
                        + .085f * static_cast<float>((branch * 3) % 6);
                    const Vector3 end(std::cos(angle) * reach, height,
                                      std::sin(angle) * reach);
                    AppendReefTube(geometry, start, end, .074f, .026f, sides, coral);
                    if (!farMesh) {
                        const Vector3 direction = Vector3::Normalize(end - start);
                        const Vector3 fork = end - direction
                            * (.20f + .025f * static_cast<float>(branch % 3));
                        const float split = branch % 2 == 0 ? .52f : -.52f;
                        const Vector3 sideTip = fork + Vector3(
                            std::cos(angle + split) * .15f,
                            .22f + .025f * static_cast<float>(branch % 2),
                            std::sin(angle + split) * .15f);
                        AppendReefTube(geometry, fork, sideTip, .035f, .018f, 6, coral);
                        AppendReefMound(geometry, sideTip - Vector3::Up * .014f,
                            .030f, .045f, 2, 6, angle, growingTip);
                    }
                    AppendReefMound(geometry, end - Vector3::Up * .016f,
                        farMesh ? .035f : .033f, farMesh ? .052f : .050f,
                        2, farMesh ? 5 : 6, angle + .7f, growingTip);
                }
            } else if (variant == 1) {
                const Color coral(183, 113, 111, 16);
                const int rings = farMesh ? 2 : 4;
                const int slices = farMesh ? 7 : 11;
                AppendReefMound(geometry, Vector3(-.16f,0,.02f), .56f, .67f,
                    rings, slices, .3f, coral);
                AppendReefMound(geometry, Vector3(.30f,0,.10f), .43f, .51f,
                    rings, slices, 1.7f, ShadeColor(coral, .94f));
                if (!farMesh) AppendReefMound(geometry, Vector3(.05f,0,-.35f),
                    .36f, .45f, 3, 9, 2.8f, ShadeColor(coral, 1.06f));
            } else {
                const Color coral(196, 153, 112, 16);
                AppendReefTube(geometry, Vector3(0,-.04f,0), Vector3(0,.58f,0),
                    .14f, .10f, sides, coral);
                AppendReefPlate(geometry, Vector3(-.10f,.37f,.06f), .43f,
                    farMesh ? 7 : 11, .8f, ShadeColor(coral, .92f));
                AppendReefPlate(geometry, Vector3(.11f,.68f,-.05f), farMesh ? .62f : .70f,
                    farMesh ? 8 : 13, 2.1f, coral);
                if (!farMesh) AppendReefPlate(geometry, Vector3(-.20f,.91f,.12f),
                    .48f, 11, 3.4f, ShadeColor(coral, 1.07f));
            }
            BuildWoodNormals(geometry);
            return geometry;
        }

        [[nodiscard]] PropGeometry CreateLogGeometry(const int variant, const bool farMesh)
        {
            PropGeometry geometry;
            const int sections = farMesh ? 2 : 5;
            const int sides = farMesh ? 6 : 9;
            geometry.vertices.reserve(static_cast<std::size_t>((sections + 1) * sides + sides * 2 + 5));
            geometry.indices.reserve(static_cast<std::size_t>(sections * sides * 6 + sides * 6 + 12));
            const int barkMaterial = farMesh ? 255 : 128;
            const Color bark = variant == 0 ? Color(101, 69, 42, barkMaterial)
                : (variant == 1 ? Color(87, 67, 45, barkMaterial)
                                : Color(112, 77, 44, barkMaterial));
            const auto ringRadius = [variant](const int section, const int side,
                                               const int sideCount) {
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sideCount);
                return 0.92f + 0.07f
                    * std::sin(angle * (2.0f + static_cast<float>(variant))
                               + static_cast<float>(section) * 1.7f);
            };
            const auto brokenEndOffset = [farMesh, variant, sections, sides](
                                             const int section, const int side) {
                if (farMesh || (section != 0 && section != sections)) return 0.0f;
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                const float endPhase = section == 0 ? 0.73f : 2.19f;
                const float broad = 0.5f + 0.5f * std::sin(
                    angle * (2.0f + static_cast<float>(variant)) + endPhase);
                const float splinter = 0.5f + 0.5f * std::sin(
                    angle * 5.0f - static_cast<float>(variant) * 0.91f - endPhase);
                return 0.012f + broad * 0.048f + splinter * 0.018f;
            };
            const auto sideBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int section = 0; section <= sections; ++section) {
                const float t = static_cast<float>(section) / static_cast<float>(sections);
                const float x = t - 0.5f;
                const float bend = std::sin(t * std::numbers::pi_v<float>)
                    * (0.035f + 0.012f * variant);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float ring = ringRadius(section, side, sides);
                    const float endDirection = section == 0 ? -1.0f
                        : (section == sections ? 1.0f : 0.0f);
                    const float brokenX = x
                        + endDirection * brokenEndOffset(section, side);
                    const float light = 0.60f + 0.36f
                        * std::max(0.0f, std::sin(angle) * 0.78f - std::cos(angle) * 0.22f);
                    geometry.vertices.emplace_back(
                        Vector3(brokenX, std::sin(angle) * ring + bend,
                                std::cos(angle) * ring),
                        ShadeColor(bark, light));
                    geometry.albedos.push_back(bark);
                }
            }
            for (int section = 0; section < sections; ++section) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(sideBase + section * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(sideBase + section * sides + next);
                    const auto upper = static_cast<std::uint16_t>(sideBase + (section + 1) * sides + side);
                    const auto upperNext = static_cast<std::uint16_t>(sideBase + (section + 1) * sides + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }

            const Color cut(151 + variant * 4, 112 + variant * 3, 66,
                            farMesh ? 255 : 192);
            for (int end = 0; end < 2; ++end) {
                const int endSection = end == 0 ? 0 : sections;
                const float endDirection = end == 0 ? -1.0f : 1.0f;
                const float x = endDirection * (farMesh ? 0.505f : 0.53f);
                const auto centre = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(Vector3(x, 0.0f, 0.0f), cut);
                geometry.albedos.push_back(cut);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float cutX = farMesh ? x
                        : endDirection * 0.5f
                            + endDirection * brokenEndOffset(endSection, side);
                    const float cutRadius = farMesh ? 0.86f
                        : ringRadius(endSection, side, sides);
                    geometry.vertices.emplace_back(
                        Vector3(cutX, std::sin(angle) * cutRadius,
                                std::cos(angle) * cutRadius),
                        ShadeColor(cut, 0.82f + 0.16f * std::max(0.0f, std::sin(angle))));
                    geometry.albedos.push_back(cut);
                }
                for (int side = 0; side < sides; ++side) {
                    const auto first = static_cast<std::uint16_t>(centre + 1U + side);
                    const auto next = static_cast<std::uint16_t>(
                        centre + 1U + (side + 1) % sides);
                    if (end == 0) geometry.indices.insert(geometry.indices.end(), {centre, first, next});
                    else geometry.indices.insert(geometry.indices.end(), {centre, next, first});
                }
            }

            BuildWoodNormals(geometry);
            // Cap rims have independent vertices; their normal must face away from the log.
            const std::size_t caps = (sections + 1) * sides;
            for (int end = 0; end < 2; ++end) {
                for (int vertex = 0; vertex <= sides; ++vertex) {
                    if (geometry.normals[caps + end*(sides+1) + vertex].X * (end == 0 ? -1 : 1) <= 0) {
                        throw std::runtime_error("Log cap normal faces inward");
                    }
                }
            }

            const auto mossBase = static_cast<std::uint16_t>(geometry.vertices.size());
            const Color moss(54, 94 + variant * 5, 43, 255);
            geometry.vertices.emplace_back(Vector3(-0.42f, 0.87f, -0.28f), moss);
            geometry.vertices.emplace_back(Vector3(0.42f, 0.87f, -0.28f), moss);
            geometry.vertices.emplace_back(Vector3(-0.42f, 0.87f, 0.28f), ShadeColor(moss, 1.12f));
            geometry.vertices.emplace_back(Vector3(0.42f, 0.87f, 0.28f), ShadeColor(moss, 1.12f));
            geometry.indices.insert(geometry.indices.end(),
                                    {mossBase, static_cast<std::uint16_t>(mossBase + 1U),
                                     static_cast<std::uint16_t>(mossBase + 2U),
                                     static_cast<std::uint16_t>(mossBase + 2U),
                                     static_cast<std::uint16_t>(mossBase + 1U),
                                     static_cast<std::uint16_t>(mossBase + 3U)});

            const auto contactBase = static_cast<std::uint16_t>(geometry.vertices.size());
            const Color contact(37, 57, 34, 255);
            geometry.vertices.emplace_back(Vector3(-0.47f, -0.76f, -0.72f), contact);
            geometry.vertices.emplace_back(Vector3(0.47f, -0.76f, -0.72f), contact);
            geometry.vertices.emplace_back(Vector3(-0.47f, -0.76f, 0.72f), contact);
            geometry.vertices.emplace_back(Vector3(0.47f, -0.76f, 0.72f), contact);
            geometry.indices.insert(geometry.indices.end(),
                                    {contactBase, static_cast<std::uint16_t>(contactBase + 2U),
                                     static_cast<std::uint16_t>(contactBase + 1U),
                                     static_cast<std::uint16_t>(contactBase + 1U),
                                     static_cast<std::uint16_t>(contactBase + 2U),
                                     static_cast<std::uint16_t>(contactBase + 3U)});
            while (geometry.albedos.size() < geometry.vertices.size()) {
                geometry.albedos.push_back(geometry.vertices[geometry.albedos.size()].Color);
                geometry.normals.push_back(Vector3::Zero);
            }
            return geometry;
        }

        [[nodiscard]] PropGeometry CreateDriftwoodGeometry(const int variant,
                                                           const bool farMesh)
        {
            PropGeometry geometry;
            const int sections = farMesh ? 2 : 4;
            const int sides = farMesh ? 5 : 7;
            const Color woodBase = variant == 0 ? Color(157, 137, 103, 255)
                : (variant == 1 ? Color(171, 148, 109, 255)
                                : Color(142, 126, 98, 255));
            const Color wood(static_cast<int>(woodBase.getRProperty()),
                             static_cast<int>(woodBase.getGProperty()),
                             static_cast<int>(woodBase.getBProperty()),
                             farMesh ? 255 : 224);
            const auto bendOffset = [variant](const float t) {
                return Vector3(
                    0.0f,
                    std::sin(t * std::numbers::pi_v<float>)
                        * (0.08f + 0.025f * static_cast<float>(variant)),
                    std::sin(t * 2.0f * std::numbers::pi_v<float>
                             + static_cast<float>(variant)) * 0.055f);
            };
            const auto ringRadius = [variant](const float angle, const int section) {
                return 0.82f + 0.12f
                    * std::sin(angle * (2.0f + static_cast<float>(variant))
                               + static_cast<float>(section) * 1.9f);
            };
            const auto sideBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int section = 0; section <= sections; ++section) {
                const float t = static_cast<float>(section) / static_cast<float>(sections);
                const float x = t - 0.5f;
                const Vector3 bend = bendOffset(t);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float ring = ringRadius(angle, section);
                    const float light = 0.72f + 0.27f
                        * std::max(0.0f, std::sin(angle) * 0.86f
                                            - std::cos(angle) * 0.18f);
                    geometry.vertices.emplace_back(
                        Vector3(x, std::sin(angle) * ring + bend.Y,
                                std::cos(angle) * ring + bend.Z),
                        ShadeColor(wood, light));
                    geometry.albedos.push_back(wood);
                }
            }
            for (int section = 0; section < sections; ++section) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(
                        sideBase + section * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(
                        sideBase + section * sides + next);
                    const auto upper = static_cast<std::uint16_t>(
                        sideBase + (section + 1) * sides + side);
                    const auto upperNext = static_cast<std::uint16_t>(
                        sideBase + (section + 1) * sides + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }

            const Color endColor = ShadeColor(woodBase, 1.08f);
            for (int end = 0; end < 2; ++end) {
                const int section = end == 0 ? 0 : sections;
                const float t = static_cast<float>(section) / static_cast<float>(sections);
                const float x = t - 0.5f;
                const Vector3 bend = bendOffset(t);
                const auto centre = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(Vector3(x, bend.Y, bend.Z), endColor);
                geometry.albedos.push_back(endColor);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float ring = ringRadius(angle, section);
                    geometry.vertices.emplace_back(
                        Vector3(x, std::sin(angle) * ring + bend.Y,
                                std::cos(angle) * ring + bend.Z), endColor);
                    geometry.albedos.push_back(endColor);
                }
                for (int side = 0; side < sides; ++side) {
                    const auto first = static_cast<std::uint16_t>(centre + 1U + side);
                    const auto next = static_cast<std::uint16_t>(
                        centre + 1U + (side + 1) % sides);
                    if (end == 0) {
                        geometry.indices.insert(geometry.indices.end(), {centre, first, next});
                    } else {
                        geometry.indices.insert(geometry.indices.end(), {centre, next, first});
                    }
                }
            }

            if (!farMesh) {
                const auto appendTaperedLimb = [&geometry, &wood](
                    const std::array<Vector3, 3>& path, const float baseHalfWidth,
                    const float baseRadius, const float brightness) {
                    constexpr int ringCount = 3;
                    constexpr int sides = 6;
                    const auto limbBase = static_cast<std::uint16_t>(geometry.vertices.size());
                    for (int ring = 0; ring < ringCount; ++ring) {
                        const Vector3 tangent = ring == 0 ? path[1] - path[0]
                            : (ring == 1 ? path[2] - path[0] : path[2] - path[1]);
                        const float yzLength = std::hypot(tangent.Y, tangent.Z);
                        const Vector3 transverse = yzLength > 0.0001f
                            ? Vector3(0.0f, -tangent.Z / yzLength,
                                      tangent.Y / yzLength)
                            : Vector3(0.0f, 0.0f, 1.0f);
                        const float taper = ring == 0 ? 1.0f : (ring == 1 ? 0.62f : 0.22f);
                        for (int side = 0; side < sides; ++side) {
                            const float angle = 2.0f * std::numbers::pi_v<float>
                                * static_cast<float>(side) / static_cast<float>(sides);
                            const Vector3 offset(
                                std::cos(angle) * baseHalfWidth * taper, 0.0f, 0.0f);
                            const float light = brightness * (0.78f + 0.22f
                                * std::max(0.0f, std::sin(angle)));
                            geometry.vertices.emplace_back(
                                path[ring] + offset
                                    + transverse * (std::sin(angle) * baseRadius * taper),
                                ShadeColor(wood, light));
                            geometry.albedos.push_back(ShadeColor(wood, brightness));
                        }
                    }
                    for (int ring = 0; ring < ringCount - 1; ++ring) {
                        for (int side = 0; side < sides; ++side) {
                            const int next = (side + 1) % sides;
                            const auto lower = static_cast<std::uint16_t>(
                                limbBase + ring * sides + side);
                            const auto lowerNext = static_cast<std::uint16_t>(
                                limbBase + ring * sides + next);
                            const auto upper = static_cast<std::uint16_t>(
                                limbBase + (ring + 1) * sides + side);
                            const auto upperNext = static_cast<std::uint16_t>(
                                limbBase + (ring + 1) * sides + next);
                            geometry.indices.insert(geometry.indices.end(),
                                                    {lower, upper, lowerNext,
                                                     lowerNext, upper, upperNext});
                        }
                    }
                    const Vector3 tipDirection = path[2] - path[1];
                    const float tipYzLength = std::hypot(tipDirection.Y, tipDirection.Z);
                    const Vector3 tip = tipYzLength > 0.0001f
                        ? path[2] + Vector3(0.0f, tipDirection.Y / tipYzLength * 0.055f,
                                            tipDirection.Z / tipYzLength * 0.055f)
                        : path[2];
                    const auto tipIndex = static_cast<std::uint16_t>(geometry.vertices.size());
                    geometry.vertices.emplace_back(tip, ShadeColor(wood, brightness * 1.05f));
                    geometry.albedos.push_back(ShadeColor(wood, brightness * 1.05f));
                    const auto lastRing = static_cast<std::uint16_t>(
                        limbBase + (ringCount - 1) * sides);
                    for (int side = 0; side < sides; ++side) {
                        geometry.indices.insert(
                            geometry.indices.end(),
                            {static_cast<std::uint16_t>(lastRing + side), tipIndex,
                             static_cast<std::uint16_t>(lastRing + (side + 1) % sides)});
                    }
                };

                if (variant == 2) {
                    appendTaperedLimb(
                        {Vector3(-0.20f, 0.35f, 0.28f),
                         Vector3(-0.28f, 1.78f, 0.42f),
                         Vector3(-0.42f, 3.34f, 0.62f)},
                        0.026f, 0.30f, 0.96f);
                    appendTaperedLimb(
                        {Vector3(0.16f, 0.32f, -0.26f),
                         Vector3(0.25f, 1.54f, -0.40f),
                         Vector3(0.42f, 2.84f, -0.58f)},
                        0.023f, 0.28f, 0.90f);
                    appendTaperedLimb(
                        {Vector3(-0.29f, 1.68f, 0.41f),
                         Vector3(-0.13f, 2.55f, 0.54f),
                         Vector3(0.02f, 3.36f, 0.72f)},
                        0.018f, 0.22f, 1.02f);
                } else {
                    appendTaperedLimb(
                        {Vector3(-0.18f + 0.04f * static_cast<float>(variant),
                                 0.36f, 0.34f),
                         Vector3(-0.13f, 1.06f, 0.91f),
                         Vector3(-0.06f, 1.90f + 0.15f * static_cast<float>(variant),
                                 1.47f)},
                        0.020f, 0.23f, 0.95f);
                    if (variant == 1) {
                        appendTaperedLimb(
                            {Vector3(0.21f, 0.34f, -0.31f),
                             Vector3(0.18f, 1.03f, -0.84f),
                             Vector3(0.09f, 2.18f, -1.49f)},
                            0.018f, 0.21f, 0.90f);
                    }
                }
            }
            BuildWoodNormals(geometry);
            const std::size_t caps = (sections + 1) * sides;
            for (int end = 0; end < 2; ++end) {
                for (int vertex = 0; vertex <= sides; ++vertex) {
                    if (geometry.normals[caps + end*(sides+1) + vertex].X * (end == 0 ? -1 : 1) < 0.99f) {
                        throw std::runtime_error("Driftwood cap normal faces inward");
                    }
                }
            }
            return geometry;
        }

        void AppendBeachPebble(PropGeometry& geometry, const float centreX,
                               const float centreZ, const float radiusX,
                               const float radiusZ, const float height,
                               const float rotation, const Color& baseColor,
                               const bool scalloped)
        {
            constexpr int sides = 7;
            const Color contact = ShadeColor(baseColor, 0.48f);
            const auto contactCentre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(Vector3(centreX, 0.006f, centreZ), contact);
            for (int side = 0; side < sides; ++side) {
                const float angle = rotation + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                geometry.vertices.emplace_back(
                    Vector3(centreX + std::cos(angle) * radiusX * 1.12f,
                            0.006f,
                            centreZ + std::sin(angle) * radiusZ * 1.12f),
                    contact);
            }
            for (int side = 0; side < sides; ++side) {
                geometry.indices.insert(
                    geometry.indices.end(),
                    {contactCentre,
                     static_cast<std::uint16_t>(contactCentre + 1U + side),
                     static_cast<std::uint16_t>(contactCentre + 1U + (side + 1) % sides)});
            }

            const auto top = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(
                Vector3(centreX - std::sin(rotation) * radiusZ * 0.12f,
                        height, centreZ + std::cos(rotation) * radiusX * 0.12f),
                ShadeColor(baseColor, 1.16f));
            for (int side = 0; side < sides; ++side) {
                const float angle = rotation + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                const float ridge = scalloped && side % 2 == 0 ? 1.0f : 0.82f;
                const float light = 0.70f + 0.26f
                    * std::max(0.0f, std::sin(angle) * 0.82f - std::cos(angle) * 0.18f);
                geometry.vertices.emplace_back(
                    Vector3(centreX + std::cos(angle) * radiusX * ridge,
                            0.012f,
                            centreZ + std::sin(angle) * radiusZ * ridge),
                    ShadeColor(baseColor, light));
            }
            for (int side = 0; side < sides; ++side) {
                geometry.indices.insert(
                    geometry.indices.end(),
                    {top,
                     static_cast<std::uint16_t>(top + 1U + side),
                     static_cast<std::uint16_t>(top + 1U + (side + 1) % sides)});
            }
        }

        void AppendBeachWrackStrand(PropGeometry& geometry, const float angle,
                                    const float length, const float phase,
                                    const Color& baseColor)
        {
            constexpr int sections = 5;
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const float forwardX = std::cos(angle);
            const float forwardZ = std::sin(angle);
            const float sideX = -forwardZ;
            const float sideZ = forwardX;
            for (int section = 0; section <= sections; ++section) {
                const float t = static_cast<float>(section)
                    / static_cast<float>(sections);
                const float envelope = std::sin(t * std::numbers::pi_v<float>);
                const float along = (t - 0.5f) * length;
                const float bend = std::sin(t * std::numbers::pi_v<float> * 2.0f + phase)
                    * (0.018f + envelope * 0.038f);
                const float halfWidth = 0.012f + envelope * 0.016f;
                const float centreX = forwardX * along + sideX * bend;
                const float centreZ = forwardZ * along + sideZ * bend;
                const float height = 0.016f + envelope * 0.014f;
                geometry.vertices.emplace_back(
                    Vector3(centreX - sideX * halfWidth, height,
                            centreZ - sideZ * halfWidth),
                    ShadeColor(baseColor, 0.78f + t * 0.12f));
                geometry.vertices.emplace_back(
                    Vector3(centreX + sideX * halfWidth, height + 0.003f,
                            centreZ + sideZ * halfWidth),
                    ShadeColor(baseColor, 1.04f - t * 0.08f));
            }
            for (int section = 0; section < sections; ++section) {
                const auto left = static_cast<std::uint16_t>(base + section * 2);
                const auto right = static_cast<std::uint16_t>(left + 1U);
                const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                geometry.indices.insert(geometry.indices.end(),
                                        {left, nextLeft, right,
                                         right, nextLeft, nextRight});
            }
        }

        [[nodiscard]] PropGeometry CreateBeachDetailGeometry(const int variant)
        {
            PropGeometry geometry;
            geometry.vertices.reserve(64U);
            geometry.indices.reserve(168U);
            const auto appendFlatQuad = [&geometry](const float minimumX,
                                                    const float minimumZ,
                                                    const float maximumX,
                                                    const float maximumZ,
                                                    const float skew,
                                                    const Color& color) {
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(
                    Vector3(minimumX - skew, 0.008f, minimumZ), color);
                geometry.vertices.emplace_back(
                    Vector3(maximumX - skew, 0.008f, minimumZ), color);
                geometry.vertices.emplace_back(
                    Vector3(minimumX + skew, 0.008f, maximumZ), color);
                geometry.vertices.emplace_back(
                    Vector3(maximumX + skew, 0.008f, maximumZ), color);
                geometry.indices.insert(geometry.indices.end(),
                                        {base, static_cast<std::uint16_t>(base + 2U),
                                         static_cast<std::uint16_t>(base + 1U),
                                         static_cast<std::uint16_t>(base + 1U),
                                         static_cast<std::uint16_t>(base + 2U),
                                         static_cast<std::uint16_t>(base + 3U)});
            };
            if (variant == 4) {
                const Color compressedSand(150, 127, 83, 255);
                const Color flipperSand(165, 139, 91, 255);
                appendFlatQuad(-0.045f, -0.70f, 0.045f, 0.70f, 0.0f,
                               compressedSand);
                for (int sweep = 0; sweep < 4; ++sweep) {
                    const float z = -0.56f + static_cast<float>(sweep) * 0.36f;
                    const float alternate = sweep % 2 == 0 ? 1.0f : -1.0f;
                    appendFlatQuad(-0.58f, z - 0.055f, -0.16f, z + 0.055f,
                                   -0.075f * alternate, flipperSand);
                    appendFlatQuad(0.16f, z - 0.055f, 0.58f, z + 0.055f,
                                   0.075f * alternate, flipperSand);
                }
                return geometry;
            }
            if (variant == 5) {
                constexpr int sides = 14;
                const Color innerSand(170, 143, 94, 255);
                const Color outerSand(202, 173, 116, 255);
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float uneven = 1.0f + 0.08f * std::sin(angle * 5.0f + 0.4f);
                    geometry.vertices.emplace_back(
                        Vector3(std::cos(angle) * 0.32f,
                                0.007f, std::sin(angle) * 0.25f), innerSand);
                    geometry.vertices.emplace_back(
                        Vector3(std::cos(angle) * 0.72f * uneven,
                                0.014f + 0.010f * (side % 3),
                                std::sin(angle) * 0.57f * uneven), outerSand);
                }
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto inner = static_cast<std::uint16_t>(base + side * 2);
                    const auto outer = static_cast<std::uint16_t>(inner + 1U);
                    const auto innerNext = static_cast<std::uint16_t>(base + next * 2);
                    const auto outerNext = static_cast<std::uint16_t>(innerNext + 1U);
                    geometry.indices.insert(geometry.indices.end(),
                                            {inner, innerNext, outer,
                                             outer, innerNext, outerNext});
                }
                appendFlatQuad(-0.31f, -0.24f, 0.31f, 0.24f, 0.0f, innerSand);
                return geometry;
            }
            if (variant == 3) {
                const std::array<Color, 4> wrackColors = {
                    Color(104, 92, 46, 255),
                    Color(124, 104, 51, 255),
                    Color(85, 91, 43, 255),
                    Color(139, 111, 57, 255),
                };
                for (int strand = 0; strand < 4; ++strand) {
                    const float strandPhase = 0.52f + static_cast<float>(strand) * 1.17f;
                    AppendBeachWrackStrand(
                        geometry, strandPhase, 0.46f + 0.065f * static_cast<float>(strand),
                        0.41f + static_cast<float>(strand) * 0.83f,
                        wrackColors[static_cast<std::size_t>(strand)]);
                }
                AppendBeachPebble(geometry, -0.10f, 0.05f, 0.026f, 0.022f,
                                  0.024f, 0.3f, Color(118, 99, 48, 255), false);
                AppendBeachPebble(geometry, 0.13f, -0.08f, 0.022f, 0.019f,
                                  0.021f, -0.6f, Color(91, 89, 42, 255), false);
                return geometry;
            }
            const float phase = static_cast<float>(variant) * 0.73f;
            const std::array<Color, 4> colors = {
                Color(216, 195, 153, 255),
                Color(190, 132, 103, 255),
                Color(146, 145, 126, 255),
                Color(229, 214, 178, 255),
            };
            for (int item = 0; item < 4; ++item) {
                const float angle = phase + static_cast<float>(item) * 1.83f;
                const float distance = item == 0 ? 0.08f
                    : 0.19f + 0.055f * static_cast<float>((item + variant) % 3);
                const float size = 0.075f
                    + 0.018f * static_cast<float>((item * 3 + variant) % 4);
                const bool shell = (item + variant) % 3 == 0;
                AppendBeachPebble(
                    geometry, std::cos(angle) * distance, std::sin(angle) * distance,
                    size * (shell ? 1.28f : 1.0f),
                    size * (shell ? 0.82f : 0.92f),
                    size * (shell ? 0.66f : 0.52f), angle * 0.61f,
                    colors[static_cast<std::size_t>((item + variant) % colors.size())], shell);
            }
            return geometry;
        }

        [[nodiscard]] PropGeometry CreateFallenFrondGeometry(const int variant)
        {
            PropGeometry geometry;
            geometry.vertices.reserve(136U);
            geometry.indices.reserve(228U);
            const Color rachis = variant == 2
                ? Color(105, 76, 39, FallenFrondMaterialDetail)
                : Color(102, 112, 48, FallenFrondMaterialDetail);
            const Color leaf = variant == 0
                ? Color(52, 113, 55, FallenFrondMaterialDetail)
                : (variant == 1
                       ? Color(91, 112, 49, FallenFrondMaterialDetail)
                       : Color(133, 100, 48, FallenFrondMaterialDetail));
            constexpr int stemSections = 9;
            const auto stemBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int section = 0; section <= stemSections; ++section) {
                const float t = static_cast<float>(section)
                    / static_cast<float>(stemSections);
                const float x = t - 0.5f;
                const float arch = std::sin(t * std::numbers::pi_v<float>);
                const float curve = arch * 0.032f;
                const float sideways = std::sin(
                    t * std::numbers::pi_v<float> * 2.0f + variant * 0.7f)
                    * (0.012f + arch * 0.010f);
                const float halfWidth = 0.014f - t * 0.006f;
                geometry.vertices.emplace_back(
                    Vector3(x, 0.018f + curve, sideways - halfWidth),
                    ShadeColor(rachis, 0.82f + t * 0.18f));
                geometry.vertices.emplace_back(
                    Vector3(x, 0.018f + curve, sideways + halfWidth),
                    ShadeColor(rachis, 1.05f + t * 0.10f));
            }
            for (int section = 0; section < stemSections; ++section) {
                const auto left = static_cast<std::uint16_t>(stemBase + section * 2);
                const auto right = static_cast<std::uint16_t>(left + 1U);
                const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                geometry.indices.insert(geometry.indices.end(),
                                        {left, nextLeft, right,
                                         right, nextLeft, nextRight});
            }

            constexpr int leafletPairs = 14;
            for (int pair = 0; pair < leafletPairs; ++pair) {
                const float t = 0.055f + static_cast<float>(pair)
                    / static_cast<float>(leafletPairs - 1) * 0.855f;
                const float x = t - 0.5f;
                const float envelope = 0.055f
                    + std::sin(t * std::numbers::pi_v<float>) * 0.155f;
                const float centreHeight = 0.020f
                    + std::sin(t * std::numbers::pi_v<float>) * 0.032f;
                const float centreSideways = std::sin(
                    t * std::numbers::pi_v<float> * 2.0f + variant * 0.7f)
                    * (0.012f
                       + std::sin(t * std::numbers::pi_v<float>) * 0.010f);
                for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                    const float side = sideIndex == 0 ? -1.0f : 1.0f;
                    const float alternate = ((pair + sideIndex + variant) % 2 == 0)
                        ? 0.018f : -0.012f;
                    const float shapeVariation = std::sin(
                        static_cast<float>(pair) * 1.71f
                        + static_cast<float>(sideIndex) * 2.13f
                        + static_cast<float>(variant) * 0.83f);
                    const float sideScale = 0.91f + shapeVariation * 0.10f;
                    const float sweep = 0.038f + t * 0.052f
                        + 0.006f * static_cast<float>(variant)
                        + shapeVariation * 0.009f;
                    const float tipDrop = 0.008f
                        + std::abs(shapeVariation) * 0.010f;
                    const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                    const float ageVariation = 0.94f + shapeVariation * 0.07f;
                    const Color inner = ShadeColor(
                        leaf, (0.80f + 0.020f * pair) * ageVariation);
                    const Color outer = ShadeColor(
                        leaf, (0.92f + 0.014f * pair) * ageVariation);
                    geometry.vertices.emplace_back(
                        Vector3(x, centreHeight + 0.006f,
                                centreSideways + side * 0.010f), inner);
                    geometry.vertices.emplace_back(
                        Vector3(x - 0.028f, centreHeight + 0.001f,
                                centreSideways + side * envelope * 0.23f),
                        ShadeColor(inner, 0.92f));
                    geometry.vertices.emplace_back(
                        Vector3(x + sweep + alternate,
                                centreHeight + 0.010f - tipDrop,
                                centreSideways + side * envelope * sideScale),
                        ShadeColor(outer, 1.05f));
                    geometry.vertices.emplace_back(
                        Vector3(x + 0.040f, centreHeight + 0.002f,
                                centreSideways + side * envelope * 0.24f), outer);
                    geometry.indices.insert(
                        geometry.indices.end(),
                        {base, static_cast<std::uint16_t>(base + 1U),
                         static_cast<std::uint16_t>(base + 2U),
                         base, static_cast<std::uint16_t>(base + 2U),
                         static_cast<std::uint16_t>(base + 3U)});
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

    const VertexDeclaration& NaturalPropRenderer::InstanceDeclaration()
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

    NaturalPropRenderer::NaturalPropRenderer(GraphicsDevice& device,
                                             const NaturalPropLayout& layout)
        : device_(device)
    {
        instancingSupported_ = device_.SupportsCapability(CNA::GraphicsCapability::Instancing);
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        fallbackEffect_->setFogStartProperty(235.0f);
        fallbackEffect_->setFogEndProperty(610.0f);

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
                    fallbackReason_ = "the natural-prop shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable natural-prop shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMeshes();
        CreateCells(layout);
        const int clearingRockCount = static_cast<int>(std::count_if(
            layout.Rocks().begin(), layout.Rocks().end(),
            [](const RockPlacement& rock) { return rock.clearingLandmark; }));
        const int clearingLogCount = static_cast<int>(std::count_if(
            layout.FallenLogs().begin(), layout.FallenLogs().end(),
            [](const FallenLogPlacement& log) { return log.clearingLandmark; }));
        const int coveDriftwoodCount = static_cast<int>(std::count_if(
            layout.Driftwood().begin(), layout.Driftwood().end(),
            [](const DriftwoodPlacement& wood) { return wood.coveLandmark; }));
        std::cout << "island-simulator: natural props " << totalRockCount_ << " rocks, "
                  << totalReefRockCount_ << " reef outcrops, "
                  << totalReefHabitatCount_ << " submerged reef habitat instances, "
                  << totalLogCount_ << " fallen logs, " << totalDriftwoodCount_
                  << " driftwood pieces, " << totalFallenFrondCount_
                  << " fallen palm fronds, "
                  << (instancingSupported_ ? "hardware instancing" : "per-prop fallback") << '\n';
        std::cout << "island-simulator: clearing landmarks " << clearingRockCount
                  << " rocks and " << clearingLogCount
                  << (clearingLogCount == 1 ? " fallen log" : " fallen logs")
                  << ", shared authored placement\n";
        std::cout << "island-simulator: cove landmark " << coveDriftwoodCount
                  << " driftwood pieces, shared authored shoreline profile\n";
        std::cout << "island-simulator: stream-bank fallen logs "
                  << layout.StreamBankLogCount() << ", darker existing bark variant\n";
        std::cout << "island-simulator: stream-bank stones "
                  << layout.StreamBankRockCount()
                  << ", non-solid existing rock variants beyond the carved bank\n";
        std::cout << "island-simulator: beach detail " << totalBeachDetailCount_
                  << " shell/pebble/wrack clusters, near-field culled\n";
        std::cout << "island-simulator: dry wrack keeps its dedicated beach-detail variant\n";
        std::cout << "island-simulator: two turtle nesting traces use paired flipper tracks "
                     "and disturbed dry-sand nests\n";
        std::cout << "island-simulator: prevailing-wind shelter adds "
                  << NaturalPropLayout::WindSortedBayDriftwoodCount << " driftwood and "
                  << NaturalPropLayout::WindSortedBayDetailCount
                  << " shell/wrack placements to the leeward bay\n";
        if (customShaderReady_) {
            std::cout << "island-simulator: natural-prop portable shader with near/far LOD\n";
            std::cout << "island-simulator: near rocks use filtered mineral grain and restrained lichen patches\n";
            std::cout << "island-simulator: near low rocks use a fixed-sea-level wet material band\n";
            std::cout << RockFacetDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(RockFacetDiagnostic);
            std::cout << RockTintDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(RockTintDiagnostic);
            std::cout << "island-simulator: near fallen logs use procedural bark grooves and cut-end growth rings\n";
            constexpr std::string_view woodLighting =
                "island-simulator: wood uses outward shared normals and scale-correct fixed-sun lighting";
            std::cout << woodLighting << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(woodLighting);
            std::cout << FallenLogEndDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(FallenLogEndDiagnostic);
            std::cout << FallenLogCrackDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(FallenLogCrackDiagnostic);
            std::cout << "island-simulator: near driftwood uses faded sun-bleached longitudinal grain\n";
            std::cout << "island-simulator: near driftwood variant 2 uses three tapered fork limbs\n";
            std::cout << DriftwoodBranchDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(DriftwoodBranchDiagnostic);
            std::cout << DriftwoodEndDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(DriftwoodEndDiagnostic);
            std::cout << FallenFrondDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(FallenFrondDiagnostic);
            std::cout << FallenFrondFoldDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(FallenFrondFoldDiagnostic);
            constexpr std::string_view reefDiagnostic =
                "island-simulator: spatial reef uses branching, rounded, plate coral and underwater-rock LOD forms";
            std::cout << reefDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(reefDiagnostic);
            std::cout << StaghornCoralDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(StaghornCoralDiagnostic);
        } else {
            std::cerr << "island-simulator: natural-prop stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    NaturalPropRenderer::~NaturalPropRenderer() = default;

    void NaturalPropRenderer::CreateMeshes()
    {
        std::size_t uploadedVertices = 0;
        const auto upload = [this, &uploadedVertices](const PropGeometry& geometry, PropMesh& mesh,
                                   const char* label) {
            ValidateProceduralTriangleMeshOrThrow(
                geometry.vertices.size(), geometry.indices, label);
            mesh.vertexCount = static_cast<int>(geometry.vertices.size());
            mesh.primitiveCount = static_cast<int>(geometry.indices.size() / 3U);
            uploadedVertices += geometry.vertices.size();
            if (customShaderReady_) {
                if ((!geometry.normals.empty() && geometry.normals.size() != geometry.vertices.size())
                    || (!geometry.albedos.empty() && geometry.albedos.size() != geometry.vertices.size())) {
                    throw std::runtime_error("Prop normal/albedo count does not match uploaded geometry");
                }
                std::vector<PropGpuVertex> vertices;
                vertices.reserve(geometry.vertices.size());
                for (std::size_t index = 0; index < geometry.vertices.size(); ++index) {
                    const auto& source = geometry.vertices[index];
                    const auto normal = geometry.normals.empty() ? Vector3::Zero : geometry.normals[index];
                    const Color color = geometry.albedos.empty() ? source.Color : geometry.albedos[index];
                    for (float value : {source.Position.X, source.Position.Y, source.Position.Z,
                                       normal.X, normal.Y, normal.Z}) {
                        if (!std::isfinite(value)) throw std::runtime_error("Non-finite natural-prop upload");
                    }
                    vertices.push_back({{source.Position.X, source.Position.Y, source.Position.Z},
                        {color.getRProperty(),color.getGProperty(),color.getBProperty(),color.getAProperty()},
                        {normal.X,normal.Y,normal.Z}});
                }
                mesh.vertices = std::make_unique<VertexBuffer>(device_, PropDeclaration(), mesh.vertexCount, BufferUsage::WriteOnly);
                mesh.vertices->SetData(vertices.data(), mesh.vertexCount);
            } else {
                mesh.vertices = std::make_unique<VertexBuffer>(device_, VertexPositionColor::getVertexDeclarationStatic(),
                                                               mesh.vertexCount, BufferUsage::WriteOnly);
                mesh.vertices->SetData(geometry.vertices.data(), mesh.vertexCount);
            }
            mesh.indices = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits, static_cast<int>(geometry.indices.size()),
                BufferUsage::WriteOnly);
            mesh.indices->SetData(geometry.indices.data(), static_cast<int>(geometry.indices.size()));
        };
        for (std::size_t variant = 0; variant < rockNearMeshes_.size(); ++variant) {
            upload(CreateRockGeometry(static_cast<int>(variant), false), rockNearMeshes_[variant],
                   "rock near LOD");
            upload(CreateRockGeometry(static_cast<int>(variant), true), rockFarMeshes_[variant],
                   "rock far LOD");
        }
        for (std::size_t variant = 0; variant < logNearMeshes_.size(); ++variant) {
            upload(CreateLogGeometry(static_cast<int>(variant), false), logNearMeshes_[variant],
                   "log near LOD");
            upload(CreateLogGeometry(static_cast<int>(variant), true), logFarMeshes_[variant],
                   "log far LOD");
        }
        for (std::size_t variant = 0; variant < driftwoodNearMeshes_.size(); ++variant) {
            upload(CreateDriftwoodGeometry(static_cast<int>(variant), false),
                   driftwoodNearMeshes_[variant], "driftwood near LOD");
            upload(CreateDriftwoodGeometry(static_cast<int>(variant), true),
                   driftwoodFarMeshes_[variant], "driftwood far LOD");
        }
        for (std::size_t variant = 0; variant < beachDetailMeshes_.size(); ++variant) {
            upload(CreateBeachDetailGeometry(static_cast<int>(variant)),
                   beachDetailMeshes_[variant], "beach-detail");
        }
        for (std::size_t variant = 0; variant < fallenFrondMeshes_.size(); ++variant) {
            upload(CreateFallenFrondGeometry(static_cast<int>(variant)),
                   fallenFrondMeshes_[variant], "fallen-frond");
        }
        for (std::size_t variant = 0; variant < reefHabitatNearMeshes_.size(); ++variant) {
            upload(CreateReefHabitatGeometry(static_cast<int>(variant), false),
                   reefHabitatNearMeshes_[variant], "reef habitat near LOD");
            upload(CreateReefHabitatGeometry(static_cast<int>(variant), true),
                   reefHabitatFarMeshes_[variant], "reef habitat far LOD");
        }
        std::cout << "island-simulator: prop mesh inventory " << uploadedVertices << " vertices, "
                  << uploadedVertices * (customShaderReady_ ? sizeof(PropGpuVertex) : sizeof(VertexPositionColor))
                  << " vertex bytes\n";
    }

    void NaturalPropRenderer::CreateCells(const NaturalPropLayout& layout)
    {
        const auto extendBounds = [](PropCell& cell, const Vector3& minimum,
                                     const Vector3& maximum) {
            if (!cell.occupied) {
                cell.bounds = BoundingBox(minimum, maximum);
                cell.occupied = true;
            } else {
                cell.bounds = BoundingBox(Vector3::Min(cell.bounds.Min, minimum),
                                          Vector3::Max(cell.bounds.Max, maximum));
            }
        };
        for (const RockPlacement& rock : layout.Rocks()) {
            PropCell& cell = cells_[CellIndex(rock.x, rock.z)];
            cell.rocks.push_back({Matrix::CreateScale(rock.scaleX, rock.scaleY, rock.scaleZ)
                                      * Matrix::CreateRotationY(rock.yawRadians)
                                      * Matrix::CreateTranslation(rock.x, rock.y, rock.z),
                                  rock.variant, rock.x, rock.z, rock.offshore});
            const float radius = std::max(rock.scaleX, rock.scaleZ);
            extendBounds(cell, Vector3(rock.x - radius, rock.y - 0.1f, rock.z - radius),
                         Vector3(rock.x + radius, rock.y + rock.scaleY * RockSurfaceMaximumHeight, rock.z + radius));
            ++totalRockCount_;
            if (rock.offshore) ++totalReefRockCount_;
        }
        for (const FallenLogPlacement& log : layout.FallenLogs()) {
            PropCell& cell = cells_[CellIndex(log.x, log.z)];
            cell.logs.push_back({Matrix::CreateScale(log.length, log.radius, log.radius)
                                     * Matrix::CreateRotationY(WoodModelYaw(log.yawRadians))
                                     * Matrix::CreateTranslation(log.x, log.y, log.z),
                                 log.variant, log.x, log.z});
            const float boundRadius = log.length * 0.55f;
            extendBounds(cell, Vector3(log.x - boundRadius, log.y - log.radius, log.z - boundRadius),
                         Vector3(log.x + boundRadius, log.y + log.radius, log.z + boundRadius));
            ++totalLogCount_;
        }
        for (const DriftwoodPlacement& wood : layout.Driftwood()) {
            PropCell& cell = cells_[CellIndex(wood.x, wood.z)];
            cell.driftwood.push_back({Matrix::CreateScale(wood.length, wood.radius, wood.radius)
                                          * Matrix::CreateRotationY(WoodModelYaw(wood.yawRadians))
                                          * Matrix::CreateTranslation(wood.x, wood.y, wood.z),
                                      wood.variant, wood.x, wood.z});
            const float boundRadius = wood.length * 0.56f;
            extendBounds(cell,
                         Vector3(wood.x - boundRadius, wood.y - wood.radius,
                                 wood.z - boundRadius),
                         Vector3(wood.x + boundRadius, wood.y + wood.radius * 3.6f,
                                 wood.z + boundRadius));
            ++totalDriftwoodCount_;
        }
        for (const BeachDetailPlacement& detail : layout.BeachDetails()) {
            PropCell& cell = cells_[CellIndex(detail.x, detail.z)];
            cell.beachDetails.push_back({
                Matrix::CreateScale(detail.scale)
                    * Matrix::CreateRotationY(detail.yawRadians)
                    * Matrix::CreateTranslation(detail.x, detail.y, detail.z),
                detail.variant, detail.x, detail.z});
            const float radius = detail.scale * (detail.turtleTrace ? 0.82f : 0.58f);
            extendBounds(cell,
                         Vector3(detail.x - radius, detail.y - 0.02f,
                                 detail.z - radius),
                         Vector3(detail.x + radius, detail.y + detail.scale * 0.13f,
                                 detail.z + radius));
            ++totalBeachDetailCount_;
        }
        for (const FallenFrondPlacement& frond : layout.FallenFronds()) {
            PropCell& cell = cells_[CellIndex(frond.x, frond.z)];
            cell.fallenFronds.push_back({
                Matrix::CreateScale(frond.scale)
                    * Matrix::CreateRotationY(frond.yawRadians)
                    * Matrix::CreateTranslation(frond.x, frond.y, frond.z),
                frond.variant, frond.x, frond.z});
            const float radius = frond.scale * 0.58f;
            extendBounds(cell,
                         Vector3(frond.x - radius, frond.y - 0.02f,
                                 frond.z - radius),
                         Vector3(frond.x + radius, frond.y + frond.scale * 0.12f,
                                 frond.z + radius));
            ++totalFallenFrondCount_;
        }
        for (const ReefHabitatPlacement& habitat : layout.ReefHabitat()) {
            PropCell& cell = cells_[CellIndex(habitat.x, habitat.z)];
            cell.reefHabitat.push_back({
                Matrix::CreateScale(habitat.scale)
                    * Matrix::CreateRotationY(habitat.yawRadians)
                    * Matrix::CreateTranslation(habitat.x, habitat.y, habitat.z),
                habitat.variant, habitat.x, habitat.z});
            const float radius = habitat.scale * .82f;
            extendBounds(cell, Vector3(habitat.x - radius, habitat.y - .08f,
                                       habitat.z - radius),
                         Vector3(habitat.x + radius, habitat.y + habitat.scale * 1.05f,
                                 habitat.z + radius));
            ++totalReefHabitatCount_;
        }
        for (auto& transforms : visibleRockNear_) transforms.reserve(layout.Rocks().size());
        for (auto& transforms : visibleRockFar_) transforms.reserve(layout.Rocks().size());
        for (auto& transforms : visibleLogNear_) transforms.reserve(layout.FallenLogs().size());
        for (auto& transforms : visibleLogFar_) transforms.reserve(layout.FallenLogs().size());
        for (auto& transforms : visibleDriftwoodNear_) {
            transforms.reserve(layout.Driftwood().size());
        }
        for (auto& transforms : visibleDriftwoodFar_) {
            transforms.reserve(layout.Driftwood().size());
        }
        for (auto& transforms : visibleBeachDetails_) {
            transforms.reserve(layout.BeachDetails().size());
        }
        for (auto& transforms : visibleFallenFronds_) {
            transforms.reserve(layout.FallenFronds().size());
        }
        for (auto& transforms : visibleReefHabitatNear_) {
            transforms.reserve(layout.ReefHabitat().size());
        }
        for (auto& transforms : visibleReefHabitatFar_) {
            transforms.reserve(layout.ReefHabitat().size());
        }
    }

    void NaturalPropRenderer::DrawInstances(PropMesh& mesh,
                                            const std::vector<Matrix>& transforms)
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
            ValidateInstancedDrawRangeOrThrow(
                mesh.instanceCapacity, 0, count, "natural-prop instance");
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

    void NaturalPropRenderer::Draw(const Matrix& view, const Matrix& projection,
                                   const Vector3& cameraPosition)
    {
        for (auto& transforms : visibleRockNear_) transforms.clear();
        for (auto& transforms : visibleRockFar_) transforms.clear();
        for (auto& transforms : visibleLogNear_) transforms.clear();
        for (auto& transforms : visibleLogFar_) transforms.clear();
        for (auto& transforms : visibleDriftwoodNear_) transforms.clear();
        for (auto& transforms : visibleDriftwoodFar_) transforms.clear();
        for (auto& transforms : visibleBeachDetails_) transforms.clear();
        for (auto& transforms : visibleFallenFronds_) transforms.clear();
        for (auto& transforms : visibleReefHabitatNear_) transforms.clear();
        for (auto& transforms : visibleReefHabitatFar_) transforms.clear();
        visibleRockCount_ = 0;
        visibleReefRockCount_ = 0;
        visibleLogCount_ = 0;
        visibleDriftwoodCount_ = 0;
        visibleDriftwoodNearCount_ = 0;
        visibleDriftwoodFarCount_ = 0;
        visibleBeachDetailCount_ = 0;
        visibleFallenFrondCount_ = 0;
        visibleReefHabitatCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        const BoundingFrustum frustum(view * projection);
        for (const PropCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const float closestX = std::clamp(cameraPosition.X,
                                              cell.bounds.Min.X, cell.bounds.Max.X);
            const float closestZ = std::clamp(cameraPosition.Z,
                                              cell.bounds.Min.Z, cell.bounds.Max.Z);
            const float deltaX = closestX - cameraPosition.X;
            const float deltaZ = closestZ - cameraPosition.Z;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            bool cellVisible = false;
            if (distanceSquared <= RockMaximumDistance * RockMaximumDistance) {
                for (const DrawInstance& instance : cell.rocks) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > RockMaximumDistance * RockMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= RockNearDistance * RockNearDistance;
                    (nearLod ? visibleRockNear_ : visibleRockFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleRockCount_;
                    if (instance.offshore) ++visibleReefRockCount_;
                    cellVisible = true;
                }
            }
            if (distanceSquared <= LogMaximumDistance * LogMaximumDistance) {
                for (const DrawInstance& instance : cell.logs) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared > LogMaximumDistance * LogMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= LogNearDistance * LogNearDistance;
                    (nearLod ? visibleLogNear_ : visibleLogFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleLogCount_;
                    cellVisible = true;
                }
            }
            if (distanceSquared <= DriftwoodMaximumDistance * DriftwoodMaximumDistance) {
                for (const DrawInstance& instance : cell.driftwood) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > DriftwoodMaximumDistance * DriftwoodMaximumDistance) {
                        continue;
                    }
                    const bool nearLod = instanceDistanceSquared
                        <= DriftwoodNearDistance * DriftwoodNearDistance;
                    (nearLod ? visibleDriftwoodNear_ : visibleDriftwoodFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleDriftwoodCount_;
                    if (nearLod) ++visibleDriftwoodNearCount_;
                    else ++visibleDriftwoodFarCount_;
                    cellVisible = true;
                }
            }
            for (const DrawInstance& instance : cell.beachDetails) {
                const float detailDeltaX = instance.x - cameraPosition.X;
                const float detailDeltaZ = instance.z - cameraPosition.Z;
                if (detailDeltaX * detailDeltaX + detailDeltaZ * detailDeltaZ
                    > BeachDetailMaximumDistance * BeachDetailMaximumDistance) {
                    continue;
                }
                visibleBeachDetails_[instance.variant].push_back(instance.transform);
                ++visibleBeachDetailCount_;
                cellVisible = true;
            }
            for (const DrawInstance& instance : cell.fallenFronds) {
                const float frondDeltaX = instance.x - cameraPosition.X;
                const float frondDeltaZ = instance.z - cameraPosition.Z;
                if (frondDeltaX * frondDeltaX + frondDeltaZ * frondDeltaZ
                    > FallenFrondMaximumDistance * FallenFrondMaximumDistance) {
                    continue;
                }
                visibleFallenFronds_[instance.variant].push_back(instance.transform);
                ++visibleFallenFrondCount_;
                cellVisible = true;
            }
            if (distanceSquared <= ReefHabitatMaximumDistance * ReefHabitatMaximumDistance) {
                for (const DrawInstance& instance : cell.reefHabitat) {
                    const float reefDeltaX = instance.x - cameraPosition.X;
                    const float reefDeltaZ = instance.z - cameraPosition.Z;
                    const float reefDistanceSquared = reefDeltaX * reefDeltaX
                        + reefDeltaZ * reefDeltaZ;
                    if (reefDistanceSquared
                        > ReefHabitatMaximumDistance * ReefHabitatMaximumDistance) continue;
                    const bool nearLod = reefDistanceSquared
                        <= ReefHabitatNearDistance * ReefHabitatNearDistance;
                    (nearLod ? visibleReefHabitatNear_ : visibleReefHabitatFar_)
                        [instance.variant].push_back(instance.transform);
                    ++visibleReefHabitatCount_;
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
        } else {
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        for (std::size_t variant = 0; variant < rockNearMeshes_.size(); ++variant) {
            DrawInstances(rockNearMeshes_[variant], visibleRockNear_[variant]);
            DrawInstances(rockFarMeshes_[variant], visibleRockFar_[variant]);
        }
        for (std::size_t variant = 0; variant < logNearMeshes_.size(); ++variant) {
            DrawInstances(logNearMeshes_[variant], visibleLogNear_[variant]);
            DrawInstances(logFarMeshes_[variant], visibleLogFar_[variant]);
        }
        for (std::size_t variant = 0; variant < driftwoodNearMeshes_.size(); ++variant) {
            DrawInstances(driftwoodNearMeshes_[variant], visibleDriftwoodNear_[variant]);
            DrawInstances(driftwoodFarMeshes_[variant], visibleDriftwoodFar_[variant]);
        }
        for (std::size_t variant = 0; variant < beachDetailMeshes_.size(); ++variant) {
            DrawInstances(beachDetailMeshes_[variant], visibleBeachDetails_[variant]);
        }
        for (std::size_t variant = 0; variant < fallenFrondMeshes_.size(); ++variant) {
            DrawInstances(fallenFrondMeshes_[variant], visibleFallenFronds_[variant]);
        }
        for (std::size_t variant = 0; variant < reefHabitatNearMeshes_.size(); ++variant) {
            DrawInstances(reefHabitatNearMeshes_[variant], visibleReefHabitatNear_[variant]);
            DrawInstances(reefHabitatFarMeshes_[variant], visibleReefHabitatFar_[variant]);
        }
        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
