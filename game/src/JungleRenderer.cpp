#include "IslandSimulator/JungleRenderer.hpp"
#include "IslandSimulator/CampfireLighting.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/CanopyVisibility.hpp"
#include "IslandSimulator/JungleWind.hpp"
#include "IslandSimulator/JungleMaterial.hpp"
#include "IslandSimulator/JungleGrounding.hpp"
#include "IslandSimulator/GroundLitter.hpp"
#include "IslandSimulator/LeafSurfaceMaterial.hpp"
#include "IslandSimulator/RenderedTerrainHeightMap.hpp"
#include "IslandSimulator/BarkMaterial.hpp"
#include "IslandSimulator/TrunkSurfaceMaterial.hpp"
#include "IslandSimulator/SurfaceNormals.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/LeafBladeProfile.hpp"
#include "IslandSimulator/UnderstoryLeafProfile.hpp"
#include "IslandSimulator/GroundBroadleaf.hpp"
#include "IslandSimulator/SplitGroundLeaf.hpp"
#include "IslandSimulator/BushProfile.hpp"
#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/CanopyBranchProfile.hpp"
#include "IslandSimulator/CanopyLeafPosture.hpp"
#include "IslandSimulator/CanopyLighting.hpp"
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
#include <sstream>
#include <stdexcept>
#include <string>
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
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector4.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float CanopyMaximumDistance = 475.0f;
        constexpr float StandingSnagNearDistance = 74.0f;
        constexpr float StandingSnagMaximumDistance = 310.0f;
        constexpr float UnderstoryNearDistance = 58.0f;
        constexpr float UnderstoryMaximumDistance = 128.0f;
        constexpr float BushNearDistance = 44.0f;
        constexpr float BushMaximumDistance = 102.0f;
        constexpr float GroundPlantNearDistance = 30.0f;
        constexpr float GroundPlantMaximumDistance = 72.0f;
        constexpr const char* CanopyLianaDiagnostic =
            "island-simulator: selected near canopy variants include hanging leafy lianas";
        constexpr const char* CanopyLayeringDiagnostic =
            "island-simulator: jungle near canopy uses irregular layered lower-crown edges";
        constexpr const char* CanopyBranchDiagnostic =
            "island-simulator: jungle near canopy uses irregular trunks and bent support branches";

        struct LeafSurface
        {
            Vector3 normal{};
            // Leaves: along/across/variation. Wood: cos(angle)/sin(angle)/axial metres.
            Vector4 coordinates{}; // Material tag: wood -1/other 0/live leaf 1/ground litter 2.
            Color woodAlbedo{}; // Custom shader only; stock vertices keep their historical baked shade.
        };

        struct PlantGpuVertex
        {
            float position[3];
            std::uint8_t color[4];
            float wind[2];
            float normal[3];
            float leaf[4];
        };
        static_assert(std::is_trivially_copyable_v<PlantGpuVertex>);
        static_assert(std::is_standard_layout_v<PlantGpuVertex> && sizeof(PlantGpuVertex) == 52);

        const VertexDeclaration& PlantDeclaration()
        {
            static const VertexDeclaration declaration{
                VertexElement(offsetof(PlantGpuVertex, position), VertexElementFormat::Vector3, VertexElementUsage::Position, 0),
                VertexElement(offsetof(PlantGpuVertex, color), VertexElementFormat::Color, VertexElementUsage::Color, 0),
                VertexElement(offsetof(PlantGpuVertex, wind), VertexElementFormat::Vector2, VertexElementUsage::TextureCoordinate, 0),
                VertexElement(offsetof(PlantGpuVertex, normal), VertexElementFormat::Vector3, VertexElementUsage::Normal, 0),
                VertexElement(offsetof(PlantGpuVertex, leaf), VertexElementFormat::Vector4, VertexElementUsage::TextureCoordinate, 5)};
            return declaration;
        }

        struct PlantGeometry
        {
            std::vector<VertexPositionColorTexture> vertices;
            std::vector<std::uint16_t> indices;
            std::vector<LeafSurface> leaves;
        };

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aWindData;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec4 aLeaf;
layout(location = 5) in vec4 aInstance0;
layout(location = 6) in vec4 aInstance1;
layout(location = 7) in vec4 aInstance2;
layout(location = 8) in vec4 aInstance3;

uniform mat4 uViewProjection;
uniform float uTime;

out vec3 vColor;
out vec3 vWorld;
out vec3 vLocal;
out float vGroundHeight;
out vec3 vLeafNormal;
out vec4 vLeaf;

void main()
{
    vec3 local = aPosition;
    float phase = aWindData.y * 6.28318530718;
    float broad = sin(uTime * 0.61 + phase + local.y * 0.075);
    float detail = sin(uTime * 1.07 + phase * 1.41 + local.x * 0.18 - local.z * 0.21);
    float sway = (broad * jungleBroadWind + detail * jungleDetailWind) * aWindData.x;
    local.x += sway;
    local.z += sway * jungleWindZ;
    local.y -= abs(sway) * jungleWindLowering * aWindData.x;

    mat4 instanceTransform = mat4(aInstance0, aInstance1, aInstance2, aInstance3);
    vec4 world = instanceTransform * vec4(local, 1.0);
    float groundHeight = aInstance3.y;
    if (aLeaf.w > 1.5) {
        // Dry litter is still. Preserve its scaled fold above the exact drawn terrain
        // at each vertex, not above the parent plant's horizontal root plane.
        groundHeight = grassTerrainHeight(world.xz);
        world.y = groundHeight + aPosition.y * length(aInstance1.xyz);
    }
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
    // Neighbouring trees of one form differ in weathering, not only in yaw and scale.
    if (aLeaf.w < -0.5) vColor *= 0.90 + 0.18 * tintSeed;
    vWorld = world.xyz;
    vLocal = aPosition;
    vGroundHeight = groundHeight;
    // Placements use uniform positive scale and yaw. No inverse-transpose is needed here.
    vLeafNormal = mat3(instanceTransform) * aNormal;
    vLeaf = aLeaf;
    // All vertices of a tagged wood primitive share the same exact integer seed. Reuse the
    // negative material tag; no new interpolator, instance attribute, or per-tree buffer.
    if (aLeaf.w < -0.5) vLeaf.w = -1.0 - floor(tintSeed * 1024.0);
    gl_Position = uViewProjection * world;
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;
in vec3 vLocal;
in float vGroundHeight;
in vec3 vLeafNormal;
in vec4 vLeaf;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;
uniform float uReceiveCanopyLight;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float fog = smoothstep(250.0, 660.0, distanceToCamera);
    vec3 faceVector = cross(dFdx(vWorld), dFdy(vWorld));
    float inverseFaceLength = inversesqrt(max(dot(faceVector, faceVector), 0.00000001));
    vec3 faceNormal = faceVector * inverseFaceLength;
    bool liveLeaf = vLeaf.w > 0.5 && vLeaf.w < 1.5;
    bool wood = vLeaf.w < -0.5;
    // Derivatives must be evaluated outside the per-primitive material branches.
    vec4 woodCoordinate = woodMaterialCoordinates(vLeaf.xyz);
    if (wood || liveLeaf) {
        faceNormal = vLeafNormal * inversesqrt(max(dot(vLeafNormal, vLeafNormal), 0.00000001));
    }
    float twoSidedDiffuse = abs(dot(faceNormal, normalize(uToSun)));
    float upwardFill = abs(faceNormal.y);
    float daylight = 0.78 + twoSidedDiffuse * 0.24 + upwardFill * 0.05;
    vec3 albedo = vColor;
    vec3 daylightColor = vec3(daylight);
    uint barkSeed = 0u;
    if (liveLeaf) {
        daylightColor = vec3(dot(leafDaylightTerms, vec3(1.0, twoSidedDiffuse, upwardFill)));
    } else if (wood) {
        barkSeed = uint(clamp(floor(-vLeaf.w - 1.0 + 0.5), 0.0, 1023.0));
        float aboveGround = vWorld.y - vGroundHeight;
        albedo = trunkSurfaceAlbedo(vColor, aboveGround, woodCoordinate, barkSeed);
        // Wood below and inside crowns receives the shared fixed-sun field in every draw.
        // Direct sun is attenuated; fill and sky light are cooled and reduced in shade.
        vec2 woodReceiver=canopyGroundReceiver(vWorld,vGroundHeight,uToSun);
        float transmission=canopyTransmission(woodReceiver);
        transmission=canopyDappleTransmission(woodReceiver,transmission);
        vec3 skyFill = mix(uCanopyAmbient, vec3(1.0), transmission);
        daylightColor = (woodDaylightTerms.x + woodDaylightTerms.z * max(faceNormal.y, 0.0)) * skyFill
            + woodDaylightTerms.y * max(dot(faceNormal, normalize(uToSun)), 0.0) * transmission;
        // Dense crowns can drive red/green bark channels almost to black even though the trunk
        // remains open to diffuse tropical sky. Preserve a restrained chromatic fill floor so
        // grain, moss and lichen remain readable without flattening sunlit faces.
        daylightColor = max(daylightColor, vec3(0.38, 0.40, 0.44));
        daylightColor *= trunkContactOcclusion(aboveGround);
    }
    vec3 litColor = albedo * daylightColor;
    // Derivatives must be evaluated outside the per-primitive material branch.
    vec2 leafUv = leafSurfaceUv(vLeaf.xyz);
    vec2 leafDx = dFdx(leafUv), leafDy = dFdy(leafUv);
    if (liveLeaf) {
        // Stable pigment belongs to the blade at all distances, not to a camera fade.
        litColor *= leafAgeTint(vLeaf.x, vLeaf.z);
        if (uLeafSurfaceEnabled > 0.5)
            litColor *= leafSurfaceModulation(leafUv, leafDx, leafDy);
    }
    vec2 leafFootprint = fwidth(vLeaf.xy);
    if (liveLeaf && distanceToCamera < 38.0) {
        float detailFade = 1.0 - smoothstep(16.0, 38.0, distanceToCamera);
        float across = abs(vLeaf.y);
        float midribWidth = max(leafFootprint.y, 0.008);
        float midrib = 1.0 - smoothstep(0.012, 0.012 + midribWidth * 1.5, across);
        float veinPhase = vLeaf.x * 11.0 - across * 2.8 + sin(vLeaf.x * 9.0 + vLeaf.z * 6.28) * 0.08;
        float veinFootprint = leafFootprint.x * 11.0 + leafFootprint.y * 2.8;
        float veinDistance = abs(fract(veinPhase + 0.5) - 0.5);
        float vein = (1.0 - smoothstep(0.025, 0.025 + max(veinFootprint, 0.025), veinDistance))
            * (1.0 - smoothstep(0.12, 0.45, veinFootprint));
        float bladeEdge = smoothstep(0.70, 1.0, across);
        float mottling = sin(vLeaf.x * 15.0 + sin(vLeaf.y * 7.0 + vLeaf.z * 9.0))
            * sin(vLeaf.x * 6.0 - vLeaf.y * 11.0 + vLeaf.z * 4.0);
        // Bound both sine phases in cycles/pixel, including modulation of the first phase.
        mottling *= materialDetailWeight(dot(leafFootprint, vec2(21.0,18.0)) / 6.28318530718);
        litColor *= 1.0 + detailFade * ((1.0 - uLeafSurfaceEnabled)
            * (midrib * 0.09 + vein * 0.045 + mottling * 0.025) - bladeEdge * 0.045);
        vec3 viewDirection = (uCameraPosition - vWorld) / max(distanceToCamera, 0.0001);
        vec3 facingNormal = dot(faceNormal, viewDirection) < 0.0 ? -faceNormal : faceNormal;
        vec3 halfVector = normalize(uToSun) + viewDirection;
        halfVector *= inversesqrt(max(dot(halfVector, halfVector), 0.000001));
        float wax = pow(max(dot(facingNormal, halfVector), 0.0), 24.0)
            * max(dot(facingNormal, normalize(uToSun)), 0.0);
        litColor += vec3(0.11, 0.12, 0.08) * wax * detailFade * (1.0 - bladeEdge * 0.35);
    }
    // Tagged wood is grey-brown by design; colour keys only identify untagged legacy bark.
    float barkColorMask = wood ? 1.0 : smoothstep(0.025, 0.095, vColor.r - vColor.g)
        * smoothstep(0.025, 0.105, vColor.g - vColor.b);
    float barkDetailFade = barkColorMask
        * (1.0 - smoothstep(38.0, 68.0, distanceToCamera));
    vec2 barkCoordinate = vec2(atan(vLocal.z, vLocal.x), vLocal.y);
    if (wood) {
        barkCoordinate = woodCoordinate.xy;
        barkDetailFade *= smoothstep(0.05, 0.20, dot(vLeaf.xy, vLeaf.xy));
    }
    float grainPhase = barkCoordinate.x * 5.0 + barkCoordinate.y * 0.42
        + sin(barkCoordinate.y * 1.31) * 0.32;
    float fissurePhase = barkCoordinate.x * 9.0 + barkCoordinate.y * 0.19
        + sin(barkCoordinate.y * 0.77) * 0.55;
    float grainFootprint = fwidth(grainPhase);
    float fissureFootprint = fwidth(fissurePhase);
    if (wood) {
        // Integer angular frequencies are periodic. Differentiate the circular pair to avoid
        // a false wide footprint on the atan branch cut; bound the axial phase modulation.
        grainFootprint = woodCoordinate.z * 5.0 + woodCoordinate.w * (0.42 + 1.31 * 0.32);
        fissureFootprint = woodCoordinate.z * 9.0 + woodCoordinate.w * (0.19 + 0.77 * 0.55);
    }
    float broadGrain = sin(grainPhase) * materialDetailWeight(grainFootprint / 6.28318530718);
    // abs(sin) repeats twice as often as sin; unresolved fissures converge to their mean.
    float fissureWave = abs(sin(fissurePhase));
    float fissure = mix(barkFissureTerms.z, smoothstep(barkFissureTerms.x, barkFissureTerms.y, fissureWave),
        materialDetailWeight(fissureFootprint / 3.14159265359));
    float barkResponse = 1.0 + broadGrain * 0.035 - fissure * 0.09;
    if (wood) barkResponse = barkMaterial(woodCoordinate, barkSeed);
    litColor *= mix(1.0, barkResponse, barkDetailFade);
    // Low layers receive the ground field; crown foliage must not shadow itself. Wood
    // already applied the same field to its direct and diffuse terms above.
    if (uReceiveCanopyLight > 0.5 && !wood) {
        litColor = applyCanopyLight(litColor, canopyGroundReceiver(vWorld, vGroundHeight, uToSun));
    }
    litColor=applyCampfireLight(litColor,vWorld);
    vec3 color = mix(litColor, vec3(0.53, 0.78, 0.88), fog);
    fragColor = vec4(color, 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body,
                                               const bool canopy = false)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\nprecision highp int;\n")
                    + (canopy ? CampfireLighting::Shader : "")
                    + (canopy ? std::string(CanopyLighting::Shader) + JungleMaterial::Shader() + BarkMaterial::Shader + TrunkSurface::Shader() + LeafSurfaceMaterial::Shader()
                              : JungleWind::ShaderConstants() + std::string(RenderedTerrainSurface::Shader)) + body;
            }
            return std::string("#version 330 core\n")
                + (canopy ? CampfireLighting::Shader : "")
                + (canopy ? std::string(CanopyLighting::Shader) + JungleMaterial::Shader() + BarkMaterial::Shader + TrunkSurface::Shader() + LeafSurfaceMaterial::Shader()
                          : JungleWind::ShaderConstants() + std::string(RenderedTerrainSurface::Shader)) + body;
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
                                  "jungle/jungle.es.frag.glsl", ShaderSource(true, FragmentBody, true)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "jungle/jungle.desktop.vert.glsl", ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "jungle/jungle.desktop.frag.glsl", ShaderSource(false, FragmentBody, true)),
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

        Vector4 WoodCoordinates(float angle, float along)
        {
            const auto c = JungleMaterial::WoodCoordinates(angle, along);
            return Vector4(c[0], c[1], c[2], -1);
        }

        void SetWoodAlbedo(PlantGeometry& geometry, const Color& albedo, float angle, float along)
        {
            geometry.leaves.resize(geometry.vertices.size());
            geometry.leaves.back() = {Vector3::Zero, WoodCoordinates(angle, along), albedo};
        }

        void FinishWoodNormals(PlantGeometry& geometry, std::size_t firstVertex,
                               std::size_t firstIndex, const std::vector<Vector3>& outwardDirections,
                               bool reverseWinding)
        {
            if (geometry.vertices.size() - firstVertex != outwardDirections.size())
                throw std::runtime_error("Wood surface reference count mismatch");
            std::vector<std::array<float, 3>> positions, outward;
            std::vector<std::uint16_t> indices;
            for (std::size_t i = firstVertex; i < geometry.vertices.size(); ++i) {
                const auto& p = geometry.vertices[i].Position;
                const auto& r = outwardDirections[i - firstVertex];
                positions.push_back({p.X, p.Y, p.Z});
                outward.push_back({r.X, r.Y, r.Z});
            }
            for (std::size_t i = firstIndex; i < geometry.indices.size(); i += 3) {
                if (reverseWinding) std::swap(geometry.indices[i+1], geometry.indices[i+2]);
                for (std::size_t corner = 0; corner < 3; ++corner) {
                    const auto index = geometry.indices[i+corner];
                    if (index < firstVertex || index >= geometry.vertices.size())
                        throw std::runtime_error("Wood surface index escapes its local mesh");
                    indices.push_back(static_cast<std::uint16_t>(index - firstVertex));
                }
            }
            std::vector<std::array<float, 3>> normals;
            try {
                normals = BuildOutwardSurfaceNormals(positions, indices, outward);
            } catch (const std::invalid_argument& error) {
                throw std::runtime_error("Canopy wood range starting at vertex "
                    + std::to_string(firstVertex) + " (" + std::to_string(positions.size())
                    + " vertices): " + error.what());
            }
            for (std::size_t i = 0; i < normals.size(); ++i)
                geometry.leaves[firstVertex+i].normal = Vector3(normals[i][0], normals[i][1], normals[i][2]);
        }

        [[nodiscard]] Color CanopyBarkColor(int variant);

        void AppendMangroveSupport(PlantGeometry& geometry, const Vector3& start,
                                   const Vector3& control, const Vector3& end,
                                   const float startRadius, const float endRadius,
                                   const float windWeight, const bool farMesh,
                                   const float phase)
        {
            const int rings = farMesh ? 3 : 5;
            const int sides = farMesh ? 5 : 8;
            const Color bark = CanopyBarkColor(MangroveCanopyVariant);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const auto firstIndex = geometry.indices.size();
            std::vector<Vector3> outwardDirections;
            float along = 0.0f;
            Vector3 previous = start;
            for (int ring = 0; ring < rings; ++ring) {
                const float t = static_cast<float>(ring) / static_cast<float>(rings - 1);
                const Vector3 centre = start * ((1.0f - t) * (1.0f - t))
                    + control * (2.0f * (1.0f - t) * t) + end * (t * t);
                const float aheadT = std::min(1.0f, t + .01f);
                const float behindT = std::max(0.0f, t - .01f);
                const auto point = [&](const float value) {
                    return start * ((1.0f - value) * (1.0f - value))
                        + control * (2.0f * (1.0f - value) * value) + end * (value * value);
                };
                const Vector3 axis = Vector3::Normalize(point(aheadT) - point(behindT));
                Vector3 right = Vector3::Cross(Vector3::Up, axis);
                if (right.LengthSquared() < 1.0e-6f) right = Vector3::Right;
                else right = Vector3::Normalize(right);
                const Vector3 up = Vector3::Normalize(Vector3::Cross(right, axis));
                if (ring > 0) along += Vector3::Distance(previous, centre);
                previous = centre;
                const float radius = std::lerp(startRadius, endRadius, t);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const Vector3 outward = right * std::cos(angle) + up * std::sin(angle);
                    geometry.vertices.emplace_back(centre + outward * radius,
                        ShadeColor(bark, .76f + .20f * std::max(0.0f, outward.Y)),
                        Vector2(windWeight * t, phase));
                    SetWoodAlbedo(geometry, bark, angle, along);
                    outwardDirections.push_back(outward);
                }
            }
            for (int ring = 0; ring + 1 < rings; ++ring) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(base + ring * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(base + ring * sides + next);
                    const auto upper = static_cast<std::uint16_t>(lower + sides);
                    const auto upperNext = static_cast<std::uint16_t>(lowerNext + sides);
                    geometry.indices.insert(geometry.indices.end(),
                        {lower, upper, lowerNext, lowerNext, upper, upperNext});
                }
            }
            FinishWoodNormals(geometry, base, firstIndex, outwardDirections, false);
        }

        void AppendMangroveRootsAndStems(PlantGeometry& geometry, const bool farMesh)
        {
            const int rootCount = farMesh ? 5 : 8;
            for (int root = 0; root < rootCount; ++root) {
                const float angle = .31f + static_cast<float>(root)
                    * 2.0f * std::numbers::pi_v<float> / static_cast<float>(rootCount);
                const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
                const Vector3 side(-direction.Z, 0.0f, direction.X);
                const float length = 1.18f + .18f * std::sin(root * 2.17f);
                AppendMangroveSupport(geometry, direction * length + Vector3::Up * .035f,
                    direction * (.72f + .08f * std::cos(root * 1.7f))
                        + side * (.12f * std::sin(root * 2.9f)) + Vector3::Up * .48f,
                    direction * .18f + Vector3::Up * (1.82f + .18f * (root % 3)),
                    .13f, .105f, 0.0f, farMesh, .11f * static_cast<float>(root));
            }
            const int stemCount = farMesh ? 2 : 3;
            for (int stem = 0; stem < stemCount; ++stem) {
                const float angle = 1.1f + static_cast<float>(stem)
                    * 2.0f * std::numbers::pi_v<float> / 3.0f;
                const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
                AppendMangroveSupport(geometry, direction * .30f + Vector3::Up * .04f,
                    direction * (.48f + .08f * stem) + Vector3::Up * (2.55f + .14f * stem),
                    direction * (.72f + .11f * stem) + Vector3::Up * (5.65f + .16f * stem),
                    .20f, .105f, .52f, farMesh, .23f + .19f * stem);
            }
        }

        // Neutral grey-brown tropical bark, from smooth pale fig-like trunks to darker fissured
        // ones. Saturated orange-brown read as painted wood under the fixed afternoon sun.
        [[nodiscard]] Color CanopyBarkColor(const int variant)
        {
            constexpr std::array<std::array<std::uint8_t, 3>, CanopyFormCount> barks{{
                {112, 91, 68}, {142, 134, 118}, {88, 72, 57}, {126, 101, 75},
                {101, 86, 69}, {151, 141, 122}, {82, 69, 58}, {128, 94, 68},
                // Salt-worn pale bark retains warm mineral colour under the compact colony's shade.
                {166, 154, 132}}};
            const auto& bark = barks.at(static_cast<std::size_t>(variant));
            return Color(bark[0], bark[1], bark[2], static_cast<std::uint8_t>(255));
        }

        void AppendCanopyButtresses(PlantGeometry& geometry, const int variant)
        {
            const int buttressCount = 4 + variant % 2;
            const Color bark = CanopyBarkColor(variant);
            // Rounded ridges that rise out of the flared trunk and decay into the soil. A single
            // wedge read as a flat paper fin; five columns give it a convex, root-like section.
            constexpr int Stations = 5;
            constexpr std::array<float, 5> Columns{-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
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
                const float innerDistance = 0.26f;
                // The maximum scaled tip stays inside the player-blocked trunk footprint for
                // every authored canopy scale, so the visual flare does not create a snag cue.
                const float outerDistance = 0.58f + lengthVariation * 0.09f;
                const float crestHeight = 0.74f + lengthVariation * 0.26f;
                const float innerWidth = 0.13f + widthVariation * 0.05f;
                const float outerWidth = 0.026f + widthVariation * 0.018f;
                const float sweep = (angleVariation < 0.0f ? -1.0f : 1.0f) * 0.045f;
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                const auto firstIndex = geometry.indices.size();
                const Vector2 still(0.0f, static_cast<float>(buttress) * 0.17f);
                std::vector<Vector3> outwardDirections;
                float along = 0.0f;
                Vector3 previousCrest;
                for (int station = 0; station < Stations; ++station) {
                    const float s = static_cast<float>(station) / static_cast<float>(Stations - 1);
                    const Vector3 centre = direction * std::lerp(innerDistance, outerDistance, s)
                        + side * (sweep * std::sin(std::numbers::pi_v<float> * s));
                    const float crest = crestHeight * std::pow(1.0f - s, 1.25f) + 0.022f * s;
                    const float halfWidth = std::lerp(innerWidth, outerWidth, s);
                    const Vector3 crestPoint = centre + Vector3::Up * crest;
                    if (station > 0) along += Vector3::Distance(previousCrest, crestPoint);
                    previousCrest = crestPoint;
                    for (const float column : Columns) {
                        // Feet stay just above the rendered root plane; the section is convex.
                        const float lift = std::pow(1.0f - column * column, 0.7f);
                        geometry.vertices.emplace_back(
                            centre + side * (column * halfWidth)
                                + Vector3::Up * std::max(0.016f, crest * lift),
                            ShadeColor(bark, 0.78f + 0.20f * lift + 0.06f * column), still);
                        SetWoodAlbedo(geometry, bark,
                            column * 2.0f * std::numbers::pi_v<float> / 3.0f, along);
                        outwardDirections.push_back(side * column
                            + Vector3::Up * (1.0f - std::abs(column) * 0.8f) + direction * 0.15f);
                    }
                }
                constexpr auto columns = static_cast<std::uint16_t>(Columns.size());
                for (int station = 0; station + 1 < Stations; ++station) {
                    for (std::uint16_t column = 0; column + 1 < columns; ++column) {
                        const auto a = static_cast<std::uint16_t>(base + station * columns + column);
                        const auto b = static_cast<std::uint16_t>(a + columns);
                        const auto c = static_cast<std::uint16_t>(a + 1U);
                        const auto d = static_cast<std::uint16_t>(b + 1U);
                        geometry.indices.insert(geometry.indices.end(), {a, c, b, b, c, d});
                    }
                }
                // Open underneath and partly inside the trunk; references give the outward side.
                FinishWoodNormals(geometry, base, firstIndex, outwardDirections, false);
            }
        }

        void AppendCanopyTrunk(PlantGeometry& geometry, const int variant, const bool farMesh)
        {
            const int sides = farMesh ? 5 : 12;
            const auto profile = MakeCanopyTrunkProfile(variant);
            const float height = profile.height;
            // Both tiers retain the straight lower collision zone and the bend's start knot.
            // Near rings concentrate on the root flare. Irregularity is a smooth function of
            // height and angle, never of ring index, so ring boundaries do not form creases.
            const auto ringHeights = CanopyTrunkRingHeights(variant,farMesh);
            const Color bark = CanopyBarkColor(variant);
            const std::uint16_t base = static_cast<std::uint16_t>(geometry.vertices.size());
            const auto firstIndex = geometry.indices.size();
            std::vector<Vector3> outwardDirections;

            for (const float localHeight : ringHeights) {
                const float t = localHeight / height;
                const float pigment = JungleMaterial::TrunkPigment(t);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const auto position = CanopyTrunkVertex(variant,localHeight,angle,farMesh);
                    const float directional = 0.68f + 0.30f
                        * std::max(0.0f, std::cos(angle) * 0.74f + std::sin(angle) * 0.28f);
                    geometry.vertices.emplace_back(
                        Vector3(position[0],position[1],position[2]),
                        ShadeColor(bark, directional * pigment),
                        Vector2(t * t * 0.08f, static_cast<float>(variant) * 0.17f));
                    SetWoodAlbedo(geometry, ShadeColor(bark, pigment),
                        angle, localHeight);
                    outwardDirections.emplace_back(std::cos(angle), 0.0f, std::sin(angle));
                }
            }
            const int rings = static_cast<int>(ringHeights.size()) - 1;
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
            FinishWoodNormals(geometry, base, firstIndex, outwardDirections, false);
        }

        void AppendStandingSnagSection(PlantGeometry& geometry, const Vector3& start,
                                       const Vector3& end, const Vector3& bow,
                                       const float startRadius, const float endRadius,
                                       const Color& bark, const bool farMesh,
                                       const float phase, const bool jaggedTip)
        {
            const int rings = farMesh ? 4 : 7;
            const int sides = farMesh ? 6 : 10;
            const Vector3 axis = Vector3::Normalize(end - start);
            Vector3 right = Vector3::Cross(Vector3::Up, axis);
            if (right.LengthSquared() < 1.0e-6f) right = Vector3::Right;
            else right = Vector3::Normalize(right);
            const Vector3 aroundUp = Vector3::Normalize(Vector3::Cross(right, axis));
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const auto firstIndex = geometry.indices.size();
            std::vector<Vector3> outwardDirections;
            float along = 0.0f;
            Vector3 previous = start;
            for (int ring = 0; ring < rings; ++ring) {
                const float t = static_cast<float>(ring) / static_cast<float>(rings - 1);
                const Vector3 centre = start + (end - start) * t
                    + bow * std::sin(t * std::numbers::pi_v<float>);
                if (ring > 0) along += Vector3::Distance(previous, centre);
                previous = centre;
                const float rootFlare = 1.0f + .27f * std::pow(1.0f - t, 5.0f);
                const float radius = std::lerp(startRadius, endRadius, t) * rootFlare;
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const Vector3 outward = right * std::cos(angle)
                        + aroundUp * std::sin(angle);
                    const float split = jaggedTip && ring + 1 == rings
                        ? .075f * std::sin(angle * 3.0f + phase * 7.0f)
                            + .035f * std::cos(angle * 5.0f - phase * 4.0f)
                        : 0.0f;
                    geometry.vertices.emplace_back(
                        centre + outward * radius + axis * split,
                        ShadeColor(bark, .72f + .22f * std::max(0.0f, outward.Y)
                            + .05f * std::cos(angle - .4f)),
                        Vector2(0.0f, phase));
                    SetWoodAlbedo(geometry, bark, angle, along + split);
                    outwardDirections.push_back(outward);
                }
            }
            for (int ring = 0; ring + 1 < rings; ++ring) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(base + ring * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(base + ring * sides + next);
                    const auto upper = static_cast<std::uint16_t>(lower + sides);
                    const auto upperNext = static_cast<std::uint16_t>(lowerNext + sides);
                    geometry.indices.insert(geometry.indices.end(),
                        {lower, upper, lowerNext, lowerNext, upper, upperNext});
                }
            }
            FinishWoodNormals(geometry, base, firstIndex, outwardDirections, false);

            // A pale, uneven fracture plane keeps the silhouette recognisably storm-broken
            // instead of reading as a pole whose crown failed to render.
            const Color fracture(171, 157, 128, 255);
            const auto capCentre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(end - axis * .025f, fracture, Vector2(0.0f, phase));
            SetWoodAlbedo(geometry, fracture, 0.0f, along);
            geometry.leaves.back().normal = axis;
            for (int side = 0; side < sides; ++side) {
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                const Vector3 outward = right * std::cos(angle) + aroundUp * std::sin(angle);
                const float split = jaggedTip
                    ? .075f * std::sin(angle * 3.0f + phase * 7.0f)
                        + .035f * std::cos(angle * 5.0f - phase * 4.0f)
                    : 0.0f;
                geometry.vertices.emplace_back(
                    end + outward * endRadius + axis * split,
                    ShadeColor(fracture, .90f + .08f * std::cos(angle)),
                    Vector2(0.0f, phase));
                SetWoodAlbedo(geometry, fracture, angle, along + split);
                geometry.leaves.back().normal = axis;
            }
            for (int side = 0; side < sides; ++side) {
                const auto current = static_cast<std::uint16_t>(capCentre + 1 + side);
                const auto next = static_cast<std::uint16_t>(
                    capCentre + 1 + (side + 1) % sides);
                geometry.indices.insert(geometry.indices.end(), {capCentre, next, current});
            }
        }

        [[nodiscard]] PlantGeometry CreateStandingSnagGeometry(const int variant,
                                                               const bool farMesh)
        {
            PlantGeometry geometry;
            geometry.vertices.reserve(farMesh ? 150U : 330U);
            geometry.indices.reserve(farMesh ? 500U : 1050U);
            // Standing dead wood is sun-bleached and desaturated; it must separate from the
            // almost-black live trunks even under dense canopy transmission.
            const Color bark = variant == 0 ? Color(139, 130, 112, 255)
                                             : Color(114, 106, 92, 255);
            const float height = variant == 0 ? 10.6f : 8.8f;
            AppendStandingSnagSection(geometry, Vector3::Zero,
                Vector3(.20f, height, -.16f),
                Vector3(variant == 0 ? .23f : -.30f, 0.0f,
                        variant == 0 ? .14f : .24f),
                variant == 0 ? .52f : .58f, variant == 0 ? .28f : .30f,
                bark, farMesh, .13f + .31f * variant, true);
            AppendStandingSnagSection(geometry,
                Vector3(.08f, variant == 0 ? 5.0f : 4.35f, -.06f),
                Vector3(variant == 0 ? 2.05f : -1.75f,
                        variant == 0 ? 7.15f : 6.25f,
                        variant == 0 ? .72f : 1.15f),
                Vector3(0.0f, .12f, variant == 0 ? .18f : -.16f),
                .25f, .12f, bark, farMesh, .51f + .11f * variant, true);
            if (!farMesh || variant == 1) {
                AppendStandingSnagSection(geometry,
                    Vector3(.14f, variant == 0 ? 7.15f : 5.85f, -.10f),
                    Vector3(variant == 0 ? -1.23f : 1.34f,
                            variant == 0 ? 8.38f : 7.35f,
                            variant == 0 ? 1.32f : -.86f),
                    Vector3(variant == 0 ? -.10f : .14f, .08f, .08f),
                    .20f, .10f, bark, farMesh, .73f + .09f * variant, true);
            }
            if (!farMesh) {
                AppendStandingSnagSection(geometry,
                    Vector3(.04f, variant == 0 ? 3.25f : 3.55f, -.03f),
                    Vector3(variant == 0 ? -.72f : .82f,
                            variant == 0 ? 4.04f : 4.38f,
                            variant == 0 ? -.82f : .56f),
                    Vector3(.05f, .04f, variant == 0 ? -.08f : .09f),
                    .16f, .078f, bark, false, .34f + .17f * variant, true);
            }
            return geometry;
        }

        void AppendLeafCluster(PlantGeometry& geometry, const Vector3& centre,
                               const Vector3& radius, const Color& baseColor,
                               const float windPhase, const bool farMesh,
                               const bool roundedFar = false)
        {
            const int rings = farMesh ? (roundedFar ? 3 : 2) : 4;
            const int slices = farMesh ? 6 : 9;
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int ring = 0; ring <= rings; ++ring) {
                const float ringT = static_cast<float>(ring) / static_cast<float>(rings);
                const float latitude = -std::numbers::pi_v<float> * 0.5f
                    + ringT * std::numbers::pi_v<float>;
                const float horizontal = std::cos(latitude);
                const float layerPhase = windPhase * 2.0f * std::numbers::pi_v<float>
                    + ringT * 3.71f;
                const Vector3 layerOffset = farMesh && !roundedFar ? Vector3::Zero
                    : Vector3(std::sin(layerPhase) * radius.X * 0.045f,
                              0.0f,
                              std::cos(layerPhase * 0.83f) * radius.Z * 0.045f);
                for (int slice = 0; slice < slices; ++slice) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(slice) / static_cast<float>(slices);
                    const Vector3 direction(horizontal * std::cos(angle), std::sin(latitude),
                                            horizontal * std::sin(angle));
                    const float contour = farMesh && !roundedFar ? 1.0f
                        : (0.91f + 0.07f
                               * std::cos(angle * 3.0f + windPhase * 6.2831853f)
                           + 0.035f * std::sin(angle * 5.0f - windPhase * 4.1f))
                            * (0.95f + 0.055f
                               * std::sin(ringT * 4.37f + angle * 2.0f + layerPhase));
                    const float light = 0.70f + 0.29f * std::max(
                        0.0f, direction.X * 0.52f + direction.Y * 0.75f - direction.Z * 0.20f);
                    geometry.vertices.emplace_back(
                        centre + layerOffset
                            + Vector3(direction.X * radius.X * contour,
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

        void AppendCanopyBranch(PlantGeometry& geometry, const CanopyBranchProfile& branch,
                                const Color& baseColor)
        {
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const auto firstIndex = geometry.indices.size();
            std::vector<Vector3> outwardDirections;
            for (std::size_t i=0;i<branch.vertices.size();++i) {
                const auto& vertex=branch.vertices[i];
                const auto& p=vertex.position;
                geometry.vertices.emplace_back(Vector3(p[0],p[1],p[2]),
                    ShadeColor(baseColor,0.76f+0.06f*static_cast<float>(i%6)),
                    Vector2(vertex.weight,vertex.phase));
                SetWoodAlbedo(geometry,baseColor,vertex.angle,vertex.along);
                outwardDirections.emplace_back(vertex.outward[0],vertex.outward[1],vertex.outward[2]);
            }
            for (const auto& triangle:branch.triangles)
                for (const auto index:triangle)
                    geometry.indices.push_back(static_cast<std::uint16_t>(base+index));
            FinishWoodNormals(geometry,base,firstIndex,outwardDirections,false);
        }

        void AppendPointedLeaf(PlantGeometry& geometry, const Vector3& root,
                               const float angle, const float length, const float width,
                               const float droop, const Color& baseColor,
                               const float windPhase, const float ridgeHeight = 0.16f,
                               const float pitch = 0.0f, const float roll = 0.0f)
        {
            const auto blade = PointedLeafPositions(angle,length,width,droop,ridgeHeight,pitch,roll);
            const auto position = [&](std::size_t i) {
                return root + Vector3(blade[i][0],blade[i][1],blade[i][2]);
            };
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(root, ShadeColor(baseColor, 0.78f),
                                           Vector2(0.55f, windPhase));
            geometry.vertices.emplace_back(position(1),
                                           ShadeColor(baseColor, 0.91f),
                                           Vector2(0.78f, windPhase));
            geometry.vertices.emplace_back(position(2), ShadeColor(baseColor, 1.10f),
                                           Vector2(0.80f, windPhase));
            geometry.vertices.emplace_back(position(3),
                                           ShadeColor(baseColor, 0.84f),
                                           Vector2(0.78f, windPhase));
            geometry.vertices.emplace_back(position(4), ShadeColor(baseColor, 0.96f),
                                           Vector2(0.90f, windPhase));
            geometry.leaves.resize(geometry.vertices.size());
            const std::array<Vector2, 5> leafCoordinates{{{0, 0}, {0.48f, -1},
                                                         {0.48f, 0}, {0.48f, 1}, {1, 0}}};
            for (std::size_t i = 0; i < leafCoordinates.size(); ++i) {
                geometry.leaves[base + i].coordinates = Vector4(
                    leafCoordinates[i].X, leafCoordinates[i].Y, windPhase, 1.0f);
            }
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

        void AppendCanopyBlade(PlantGeometry& geometry, const Vector3& root,
                               float angle, float length, float width, float droop,
                               const Color& color, float phase, float ridge, float pitch, float roll)
        {
            const auto blade = CanopyBladePositions(angle,length,width,droop,ridge,pitch,roll);
            constexpr std::array<float,7> shades{0.78f,0.91f,1.10f,0.84f,0.90f,0.85f,0.96f};
            constexpr std::array<float,7> wind{0.55f,0.72f,0.80f,0.72f,0.85f,0.85f,0.90f};
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (std::size_t i = 0; i < blade.size(); ++i) {
                geometry.vertices.emplace_back(root + Vector3(blade[i][0],blade[i][1],blade[i][2]),
                    ShadeColor(color,shades[i]),Vector2(wind[i],phase));
            }
            geometry.leaves.resize(geometry.vertices.size());
            for (std::size_t i = 0; i < blade.size(); ++i) {
                geometry.leaves[base+i].coordinates = Vector4(
                    CanopyBladeCoordinates[i][0],CanopyBladeCoordinates[i][1],phase,1);
            }
            for (auto index : CanopyBladeIndices)
                geometry.indices.push_back(static_cast<std::uint16_t>(base+index));
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
            geometry.vertices.reserve(farMesh ? 1100U : 5200U);
            geometry.indices.reserve(farMesh ? 3200U : 13200U);
            if (variant == MangroveCanopyVariant) {
                AppendMangroveRootsAndStems(geometry, farMesh);
            } else if (!farMesh) {
                AppendCanopyButtresses(geometry, variant);
            }
            AppendCanopyTrunk(geometry, variant, farMesh);

            constexpr int clusterCount = 8;
            const std::array<Color, CanopyFormCount> foliageColors{
                Color(34,104,48), Color(42,123,52), Color(34,114,48), Color(42,131,52),
                Color(55,105,45), Color(42,98,59), Color(59,112,42), Color(42,110,51),
                Color(37,94,48)};
            const Color leafBase = foliageColors.at(static_cast<std::size_t>(variant));
            for (int cluster = 0; cluster < clusterCount; ++cluster) {
                const auto profile = MakeCanopyCrownProfile(variant, cluster);
                const float angle = profile.angle;
                const Vector3 centre(profile.x, profile.y, profile.z);
                const float width = profile.width;
                const float phase = profile.phase;
                // Primary crown supports still occupy visible pixels at the LOD boundary.
                // Share their exact geometry/material/wind instead of dropping all branches.
                if (cluster > 0) {
                    AppendCanopyBranch(geometry, MakeCanopyBranchProfile(variant,cluster),
                                       ShadeColor(CanopyBarkColor(variant), 0.86f));
                }
                const float crownHeight = profile.height;
                // A small opaque core represents unresolved inner foliage. Individual folded
                // leaves cover the outer volume instead of painting detail onto a solid ball.
                // Retain the same eight lobe anchors at distance. Merging pairs into oversized
                // cores filled near-tier sky gaps and caused a visible solid-mass LOD pop.
                constexpr float coreWidthScale = 0.66f;
                AppendLeafCluster(geometry, centre,
                                  Vector3(width * coreWidthScale,
                                          crownHeight * 0.60f,
                                          width * 0.91f * coreWidthScale),
                                  cluster % 3 == 0 ? ShadeColor(leafBase, 1.10f) : leafBase,
                                  phase, farMesh, farMesh);
                {
                    // Retain source identities on the golden-angle shell. Fewer shoots fund
                    // curved blade outlines without adding triangles or alpha overdraw.
                    constexpr int SprigCount = CanopySprigCount;
                    for (int sprig = 0; sprig < SprigCount; ++sprig) {
                        if (!RetainCanopySprig(sprig,farMesh)) continue;
                        const float shellY = -1.0f + 2.0f
                            * (static_cast<float>(sprig) + 0.5f) / SprigCount;
                        const float shellRadius = std::sqrt(std::max(0.0f, 1.0f - shellY * shellY));
                        const float sprigAngle = phase * 6.2831853f + sprig * 2.39996323f;
                        const Vector3 outward(std::cos(sprigAngle), 0.0f, std::sin(sprigAngle));
                        const Vector3 sprigRoot = centre
                            + Vector3(outward.X, 0.0f, outward.Z * 0.91f)
                                * (width * 0.66f * shellRadius)
                            + Vector3::Up * (crownHeight * shellY * 0.82f);
                        for (int leaf = 0; leaf < CanopySprigLeafCount(cluster,sprig); ++leaf) {
                            const float leafVariation = static_cast<float>(
                                (sprig * 5 + leaf * 3 + variant + cluster) % 7) / 6.0f;
                            const float leafAngle = sprigAngle + (leaf - 1) * 0.83f;
                            const auto posture = MakeCanopyLeafPosture(variant,cluster,sprig,leaf);
                            AppendCanopyBlade(geometry,
                                sprigRoot + outward * (static_cast<float>(leaf) * 0.055f),
                                leafAngle, (0.68f + leafVariation * 0.30f)*posture.lengthScale,
                                (0.19f + leafVariation * 0.065f)*posture.widthScale,
                                0.13f + 0.16f * (1.0f - leafVariation),
                                ShadeColor(leafBase, 0.83f + leafVariation * 0.25f
                                    + shellY * 0.09f),
                                std::fmod(phase + sprig * 0.071f, 1.0f), 0.075f, posture.pitch, posture.roll);
                        }
                    }
                    const float leafAngle = angle
                        + (cluster % 2 == 0 ? -0.32f : 0.39f);
                    const Vector3 leafDirection(std::cos(leafAngle), 0.0f,
                                                std::sin(leafAngle));
                    const Vector3 leafRoot = centre + leafDirection * (width * 0.72f)
                        - Vector3::Up * (0.12f + 0.05f * static_cast<float>(cluster % 3));
                    const auto edgePosture = MakeCanopyLeafPosture(variant,cluster,CanopySprigCount,0);
                    AppendCanopyBlade(
                        geometry, leafRoot, leafAngle,
                        (1.48f + 0.11f * static_cast<float>((cluster + variant) % 4))*edgePosture.lengthScale,
                        (0.42f + 0.04f * static_cast<float>((cluster * 2 + variant) % 3))*edgePosture.widthScale,
                        0.42f + 0.08f * static_cast<float>((cluster + variant) % 3),
                        cluster % 2 == 0 ? ShadeColor(leafBase, 1.03f)
                                         : ShadeColor(leafBase, 0.91f),
                        phase, 0.16f, edgePosture.pitch, edgePosture.roll);
                    if ((cluster + variant) % 2 == 0) {
                        const float lowerLeafAngle = angle
                            + (cluster % 3 == 0 ? 0.58f : -0.51f);
                        const Vector3 lowerLeafDirection(std::cos(lowerLeafAngle), 0.0f,
                                                         std::sin(lowerLeafAngle));
                        const Vector3 lowerLeafRoot = centre
                            + lowerLeafDirection * (width * 0.60f)
                            - Vector3::Up
                                * (0.42f + 0.07f * static_cast<float>(cluster % 3));
                        const auto lowerPosture = MakeCanopyLeafPosture(variant,cluster,CanopySprigCount+1,0);
                        AppendCanopyBlade(
                            geometry, lowerLeafRoot, lowerLeafAngle,
                            (1.08f + 0.10f
                                * static_cast<float>((cluster * 2 + variant) % 3))*lowerPosture.lengthScale,
                            (0.32f + 0.035f
                                * static_cast<float>((cluster + variant) % 3))*lowerPosture.widthScale,
                            0.68f + 0.08f
                                * static_cast<float>((cluster * 3 + variant) % 3),
                            cluster % 3 == 0 ? ShadeColor(leafBase, 0.88f)
                                             : ShadeColor(leafBase, 0.97f),
                            std::fmod(phase + 0.071f, 1.0f), 0.11f, lowerPosture.pitch, lowerPosture.roll);
                    }
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
                         const float phase, const int segments,
                         const UnderstoryLeafProfile* profile = nullptr)
        {
            const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const bool detailed = segments > 2;
            constexpr std::array<float, 5> NearColumns = {-1.0f, -0.06f, 0.0f, 0.06f, 1.0f};
            const auto& detailedColumns = profile && profile->shape != UnderstoryBladeShape::Narrow
                ? BroadleafColumns : NearColumns;
            const int columns = detailed ? 5 : 2;
            for (int segment = 0; segment <= segments; ++segment) {
                const float t = static_cast<float>(segment) / static_cast<float>(segments);
                for (int column = 0; column < columns; ++column) {
                    const float across = detailed ? detailedColumns[static_cast<std::size_t>(column)]
                        : (column == 0 ? -1.0f : 1.0f);
                    const auto point = profile ? SampleUnderstoryLeaf(*profile, t, across)
                        : SampleLeafBlade(t, across, length, width, lift, phase);
                    const float midrib = detailed && column == 2 ? 0.04f : 0.0f;
                    const float sideShade = across < 0.0f ? 0.94f : 1.0f;
                    const Color color = ShadeColor(baseColor, (0.80f + 0.14f * t + midrib) * sideShade);
                    geometry.vertices.emplace_back(direction * point.along + side * point.across
                        + Vector3::Up * point.height, color, Vector2(t * t, phase));
                    const auto normal = profile ? UnderstoryLeafNormal(*profile, t, across)
                        : SampleLeafBladeNormal(t, across, length, width, lift, phase);
                    geometry.leaves.resize(geometry.vertices.size());
                    geometry.leaves.back() = {side * normal.across + Vector3::Up * normal.height
                        + direction * normal.along, Vector4(t, across, phase, 1.0f)};
                }
            }
            for (int segment = 0; segment < segments; ++segment) {
                for (int column = 0; column + 1 < columns; ++column) {
                    const auto a = static_cast<std::uint16_t>(base + segment * columns + column);
                    const auto b = static_cast<std::uint16_t>(a + 1U);
                    const auto c = static_cast<std::uint16_t>(a + columns);
                    const auto d = static_cast<std::uint16_t>(c + 1U);
                    geometry.indices.insert(geometry.indices.end(), {a, c, b, b, c, d});
                }
            }
        }

        void AppendFernFrond(PlantGeometry& geometry, const UnderstoryLeafProfile& profile,
                             const Color& color, const bool farMesh)
        {
            const Vector3 direction(std::cos(profile.angle),0,std::sin(profile.angle));
            const Vector3 side(-direction.Z,0,direction.X);
            const auto centre = [&](float t) {
                const auto p = SampleUnderstoryLeaf(profile,t,0);
                return direction*p.along+side*p.across+Vector3::Up*p.height;
            };
            // A centre vertex at every attachment gives stem and leaflet exactly the same
            // position/phase/weight under the existing shader, including its position-based gust.
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            for (float t : FernRachisKnots) {
                const float width = 0.009f*(1-0.80f*t);
                for (int column = -1; column <= 1; ++column) {
                    geometry.vertices.emplace_back(centre(t)+side*(column*width),
                        ShadeColor(color,column == 0 ? 0.86f : 0.70f),Vector2(t*t,profile.phase));
                }
            }
            for (int segment = 0; segment+1 < static_cast<int>(FernRachisKnots.size()); ++segment) {
                for (int column = 0; column < 2; ++column) {
                    const auto a = static_cast<std::uint16_t>(base+segment*3+column);
                    const auto b = static_cast<std::uint16_t>(a+1);
                    const auto c = static_cast<std::uint16_t>(a+3);
                    const auto d = static_cast<std::uint16_t>(c+1);
                    geometry.indices.insert(geometry.indices.end(),{a,c,b,b,c,d});
                }
            }
            constexpr std::array<float,5> along{0,0.48f,0.48f,0.48f,1};
            for (int pair = 0; pair < FernPairCount; ++pair) {
                if (!RetainFernPair(pair,farMesh)) continue;
                for (int sign : {-1,1}) {
                    const auto p = FernLeaflet(profile,pair,sign);
                    const auto first = geometry.vertices.size();
                    AppendPointedLeaf(geometry,centre(p.t),p.angle,p.length,p.width,p.droop,
                        ShadeColor(color,0.89f+0.08f*p.t+(sign == 1 ? 0.025f : 0)),
                        profile.phase,p.width*0.15f,p.pitch,p.roll);
                    for (std::size_t i = 0; i < along.size(); ++i) {
                        const float weight = FernWindWeight(p.t,along[i]);
                        geometry.vertices[first+i].TextureCoordinate = Vector2(weight,profile.phase);
                    }
                }
            }
        }

        [[nodiscard]] PlantGeometry CreateUnderstoryGeometry(const int variant,
                                                             const bool farMesh)
        {
            PlantGeometry geometry;
            const int leafCount = UnderstoryLeafCount(variant, farMesh);
            const int segments = farMesh ? 2 : 8;
            const int columns = farMesh ? 2 : 5;
            const bool broadLeaf = variant == 1 || variant == 3;
            const int fernPairs = farMesh ? (FernPairCount+1)/2 : FernPairCount;
            const auto perLeafVertices = broadLeaf ? (segments+1)*columns
                : static_cast<int>(FernRachisKnots.size())*3+fernPairs*2*5;
            const auto perLeafTriangles = broadLeaf ? segments*(columns-1)*2
                : (static_cast<int>(FernRachisKnots.size())-1)*4+fernPairs*2*4;
            geometry.vertices.reserve(static_cast<std::size_t>(leafCount*perLeafVertices));
            geometry.indices.reserve(static_cast<std::size_t>(leafCount*perLeafTriangles*3));
            const Color color = broadLeaf
                ? Color(42 + variant * 3, 126 + variant * 4, 58, 255)
                : Color(35, 111 + variant * 5, 49 + variant * 2, 255);
            for (int leaf = 0; leaf < leafCount; ++leaf) {
                const auto profile = UnderstoryLeaf(variant, leaf, farMesh);
                if (broadLeaf) {
                    AppendFrond(geometry, profile.angle, profile.length, profile.width, profile.lift,
                        ShadeColor(color, profile.pigment), profile.phase, segments, &profile);
                } else {
                    AppendFernFrond(geometry,profile,ShadeColor(color,profile.pigment),farMesh);
                }
            }
            return geometry;
        }

        [[nodiscard]] PlantGeometry CreateBushGeometry(const int variant, const bool farMesh)
        {
            PlantGeometry geometry;
            const Color baseColor = variant == 0 ? Color(43,122,52,255)
                : (variant == 1 ? Color(52,134,59,255) : Color(36,111,52,255));
            const auto vector=[](const BushPoint& p) { return Vector3(p[0],p[1],p[2]); };
            const auto leafCoordinates=BushLeafCoordinates(farMesh);
            const auto leafIndices=BushLeafIndices(farMesh);
            for (int shootIndex=0;shootIndex<BushShootCount(variant);++shootIndex) {
                const auto shoot=MakeBushShoot(variant,shootIndex);
                const int sides=farMesh ? 3 : 5;
                const auto stemBase=static_cast<std::uint16_t>(geometry.vertices.size());
                const auto firstIndex=geometry.indices.size();
                std::vector<Vector3> outwardDirections;
                for (const float t:BushShootKnots) {
                    const Vector3 centre=vector(SampleBushShoot(shoot,t));
                    const Vector3 axis=Vector3::Normalize(vector(SampleBushShoot(shoot,std::min(1.0f,t+0.001f)))
                        -vector(SampleBushShoot(shoot,std::max(0.0f,t-0.001f))));
                    // This shoot always has positive vertical tangent. One fixed horizontal
                    // reference avoids a ring twist when a changing tangent crosses a threshold.
                    const Vector3 reference(std::cos(shoot.angle),0,std::sin(shoot.angle));
                    const Vector3 right=Vector3::Normalize(Vector3::Cross(reference,axis));
                    const Vector3 up=Vector3::Normalize(Vector3::Cross(axis,right));
                    const float radius=0.023f*(1-0.75f*t);
                    for (int side=0;side<sides;++side) {
                        const float angle=2*std::numbers::pi_v<float>*side/sides;
                        const Vector3 outward=right*std::cos(angle)+up*std::sin(angle);
                        const Color wood(105,82,48,255);
                        geometry.vertices.emplace_back(centre+outward*radius,ShadeColor(wood,0.92f),
                            Vector2(BushShootWind(t),shoot.phase));
                        SetWoodAlbedo(geometry,wood,angle,shoot.height*t);
                        outwardDirections.push_back(outward);
                    }
                }
                for (int ring=0;ring<5;++ring) for (int side=0;side<sides;++side) {
                    const auto a=static_cast<std::uint16_t>(stemBase+ring*sides+side);
                    const auto b=static_cast<std::uint16_t>(stemBase+ring*sides+(side+1)%sides);
                    const auto c=static_cast<std::uint16_t>(a+sides),d=static_cast<std::uint16_t>(b+sides);
                    geometry.indices.insert(geometry.indices.end(),{a,c,b,b,c,d});
                }
                FinishWoodNormals(geometry,stemBase,firstIndex,outwardDirections,true);

                for (int leafIndex=0;leafIndex<BushLeavesPerShoot;++leafIndex) {
                    const auto leaf=MakeBushLeaf(variant,shootIndex,leafIndex);
                    const auto base=static_cast<std::uint16_t>(geometry.vertices.size());
                    for (const auto& coordinate:leafCoordinates) {
                        const float t=coordinate[0],side=coordinate[1];
                        const auto p=SampleBushLeaf(leaf,t,side),n=BushLeafNormal(leaf,t,side);
                        geometry.vertices.emplace_back(vector(p),ShadeColor(baseColor,leaf.pigment*(0.92f+0.08f*t)),
                            Vector2(BushLeafWind(leaf,t),leaf.phase));
                        geometry.leaves.resize(geometry.vertices.size());
                        geometry.leaves.back()={vector(n),Vector4(t,side,leaf.phase,1)};
                    }
                    for (auto index:leafIndices) geometry.indices.push_back(static_cast<std::uint16_t>(base+index));
                }
            }
            return geometry;
        }

        void AppendUprightGroundLeaf(PlantGeometry& geometry, const GroundBroadleaf& leaf,
                                     const Color& baseColor, const bool farMesh)
        {
            const float angle=leaf.angle, height=leaf.height, reach=leaf.reach;
            const float windPhase=leaf.phase;
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

            const auto leafBase = static_cast<std::uint16_t>(geometry.vertices.size());
            const int segments=farMesh ? 2 : 6, columns=farMesh ? 3 : 5;
            for (int row=0; row<=segments; ++row) {
                const float t=static_cast<float>(row)/segments;
                for (int column=0; column<columns; ++column) {
                    const float across=-1+2.0f*column/(columns-1);
                    const auto p=SampleGroundBroadleaf(leaf,t,across);
                    const auto n=GroundBroadleafNormal(leaf,t,across);
                    const float midrib=std::pow(1.0f-std::abs(across),7.0f);
                    const float margin=std::pow(std::abs(across),4.0f);
                    geometry.vertices.emplace_back(direction*p.along+side*p.across+Vector3::Up*p.height,
                        ShadeColor(baseColor,0.94f+0.05f*t+0.10f*midrib-0.08f*margin),
                        Vector2(0.48f+0.44f*t,windPhase));
                    geometry.leaves.resize(geometry.vertices.size());
                    geometry.leaves.back()={side*n.across+Vector3::Up*n.height+direction*n.along,
                        Vector4(t,across,windPhase,1.0f)};
                }
            }
            for (int row=0; row<segments; ++row) for (int col=0; col<columns-1; ++col) {
                const auto a=static_cast<std::uint16_t>(leafBase+row*columns+col);
                const auto b=static_cast<std::uint16_t>(a+1);
                const auto c=static_cast<std::uint16_t>(a+columns);
                const auto d=static_cast<std::uint16_t>(c+1);
                geometry.indices.insert(geometry.indices.end(),{a,c,b,b,c,d});
            }
        }

        void AppendSplitGroundLeaf(PlantGeometry& geometry, const SplitGroundLeaf& leaf,
                                   const Color& baseColor, const bool farMesh)
        {
            const Vector3 direction(std::cos(leaf.angle),0,std::sin(leaf.angle));
            const Vector3 side(-direction.Z,0,direction.X);
            const auto base=static_cast<std::uint16_t>(geometry.vertices.size());
            const int rows=SplitGroundLeafRows(farMesh),columns=SplitGroundLeafColumns(farMesh);
            for (int row=0;row<rows;++row) {
                const float t=SplitGroundLeafRow(row,farMesh);
                for (int column=0;column<columns;++column) {
                    const float across=-1+2.0f*column/(columns-1);
                    const auto p=SampleSplitGroundLeaf(leaf,t,across);
                    const auto n=SplitGroundLeafNormal(leaf,t,across);
                    const float midrib=std::pow(1.0f-std::abs(across),7.0f);
                    const float margin=std::pow(std::abs(across),4.0f);
                    geometry.vertices.emplace_back(direction*p.along+side*p.across+Vector3::Up*p.height,
                        ShadeColor(baseColor,0.92f+0.06f*t+0.09f*midrib-0.07f*margin),
                        Vector2(t*t,leaf.phase));
                    geometry.leaves.resize(geometry.vertices.size());
                    geometry.leaves.back()={side*n.across+Vector3::Up*n.height+direction*n.along,
                        Vector4(t,across,leaf.phase,1.0f)};
                }
            }
            for (const auto index:SplitGroundLeafIndices(farMesh))
                geometry.indices.push_back(static_cast<std::uint16_t>(base+index));
        }

        void AppendFallenGroundLeaf(PlantGeometry& geometry, const int leaf, const Color& baseColor)
        {
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const auto points=MakeGroundLitterLeaf(leaf);
            for (std::size_t i=0;i<points.size();++i) {
                geometry.vertices.emplace_back(Vector3(points[i][0],points[i][1],points[i][2]),
                    ShadeColor(baseColor,GroundLitterShade[i]),Vector2(0,0));
            }
            geometry.leaves.resize(geometry.vertices.size());
            for (std::size_t i=0;i<points.size();++i)
                geometry.leaves[base+i].coordinates = Vector4(0,0,0,2);
            for (const auto index:GroundLitterIndices)
                geometry.indices.push_back(static_cast<std::uint16_t>(base+index));
        }

        [[nodiscard]] PlantGeometry CreateGroundPlantGeometry(const int variant,
                                                              const bool farMesh)
        {
            PlantGeometry geometry;
            const bool uprightBroadleaf = variant == 1;
            const bool splitLeaf = variant == 2;
            const int leafCount = uprightBroadleaf ? (farMesh ? 4 : 6)
                : (splitLeaf ? (farMesh ? 4 : 5) : (farMesh ? 4 : 7));
            const int segments = farMesh ? 2 : 4;
            const int verticesPerLeaf = uprightBroadleaf ? (farMesh ? 13 : 39)
                : (splitLeaf ? SplitGroundLeafRows(farMesh)*SplitGroundLeafColumns(farMesh)
                    : (segments + 1) * (farMesh ? 2 : 5));
            const int indicesPerLeaf = uprightBroadleaf ? (farMesh ? 30 : 150)
                : (splitLeaf ? (SplitGroundLeafRows(farMesh)-1)*(SplitGroundLeafColumns(farMesh)-1)*6
                    : segments * (farMesh ? 1 : 4) * 6);
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
                    const auto profile=GroundBroadleafForTier(leaf,farMesh);
                    AppendUprightGroundLeaf(geometry,profile,ShadeColor(color,profile.pigment),farMesh);
                } else if (splitLeaf) {
                    const auto profile=SplitGroundLeafForTier(leaf,farMesh);
                    AppendSplitGroundLeaf(geometry,profile,ShadeColor(color,profile.pigment),farMesh);
                } else {
                    AppendFrond(geometry, angle, length, width, lift,
                                leafColor, phase, segments);
                }
            }
            if (!farMesh && variant == 0) {
                // Keep the extra forest-floor breakup sparse by attaching it to only one of the
                // three deterministic ground-plant variants. It rides the existing instance,
                // buffer, culling, and draw path instead of adding a separate litter batch.
                for (int leaf = 0; leaf < GroundLitterCount; ++leaf) {
                    const Color litterColor = leaf == 1
                        ? Color(103, 72, 29, 255)
                        : Color(128, 87 + leaf * 7, 31, 255);
                    AppendFallenGroundLeaf(geometry, leaf, litterColor);
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

    JungleRenderer::JungleRenderer(GraphicsDevice& device, const IslandTerrain& terrain,
                                   const JungleLayout& layout,
                                   CanopyLighting& canopyLighting,
                                   RenderedTerrainHeightMap& terrainHeightMap,
                                   const float maximumDistanceScale)
        : device_(device), canopyLighting_(canopyLighting), terrainHeightMap_(terrainHeightMap),
          maximumDistanceScale_(maximumDistanceScale)
    {
        if (!std::isfinite(maximumDistanceScale_) || maximumDistanceScale_ <= 0.0f
            || maximumDistanceScale_ > 1.0f) {
            throw std::invalid_argument("Jungle maximum distance scale must be in (0, 1]");
        }
        instancingSupported_ = device_.SupportsCapability(CNA::GraphicsCapability::Instancing);
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
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

        if (customShaderReady_) leafMaterial_ = std::make_unique<LeafSurfaceMaterial>(device_);
        CreateMeshes();
        CreateCells(terrain, layout);
        constexpr const char* rootDiagnostic =
            "island-simulator: jungle root origins and culling bounds follow rendered terrain triangles";
        std::cout << rootDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(rootDiagnostic);
        if (customShaderReady_) {
            constexpr const char* litterContactDiagnostic =
                "island-simulator: fallen leaves conform to rendered triangles using the shared grass height texture";
            std::cout << litterContactDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(litterContactDiagnostic);
        }
        constexpr const char* cullingDiagnostic =
            "island-simulator: canopy per-tree frustum boxes contain both LODs and wind";
        std::cout << cullingDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(cullingDiagnostic);
        std::cout << "island-simulator: jungle " << totalCanopyCount_ << " canopy trees, "
                  << totalUnderstoryCount_ << " understory plants, " << totalBushCount_
                  << " low bushes, "
                  << (instancingSupported_ ? "hardware instancing" : "per-plant fallback")
                  << '\n';
        std::cout << "island-simulator: jungle ground layer " << totalGroundPlantCount_
                  << " small plants\n";
        const std::string snagDiagnostic = "island-simulator: forest age structure includes "
            + std::to_string(totalStandingSnagCount_)
            + " standing dead/broken trunks with fracture caps, branch stubs and two-tier LOD";
        std::cout << snagDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(snagDiagnostic);
        constexpr const char* understoryProfileDiagnostic =
            "island-simulator: understory leaf identities share asymmetric posture and curled profiles across LOD";
        std::cout << understoryProfileDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(understoryProfileDiagnostic);
        constexpr const char* fernDiagnostic =
            "island-simulator: pinnate understory shares rachis attachment knots and retained leaflet wind across LOD";
        std::cout << fernDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(fernDiagnostic);
        constexpr const char* broadleafDiagnostic =
            "island-simulator: broadleaf paddle and lance laminae share cupped profiles and asymmetric margins across LOD";
        std::cout << broadleafDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(broadleafDiagnostic);
        constexpr const char* groundBroadleafDiagnostic =
            "island-simulator: juvenile ground broadleaf uses layered arching laminae, readable midribs, and retained near/far leaf identities";
        std::cout << groundBroadleafDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(groundBroadleafDiagnostic);
        constexpr const char* splitGroundDiagnostic =
            "island-simulator: split ground leaves use layered asymmetric lobes, readable midribs, shared LOD knots, and root-anchored wind";
        std::cout << splitGroundDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(splitGroundDiagnostic);
        constexpr const char* bushDiagnostic =
            "island-simulator: shrubs use supported shoots and curved individual leaves with shared LOD identities and complete leaf outlines";
        std::cout << bushDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(bushDiagnostic);
        constexpr const char* bushFormDiagnostic =
            "island-simulator: shrubs use early radial spread, crooked shoots, cupped leaf pitch, and wider pigment variation";
        std::cout << bushFormDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(bushFormDiagnostic);
        constexpr const char* materialDiagnostic =
            "island-simulator: leaf daylight contrast and stable tip pigment; footprint-filtered leaf and bark detail";
        std::cout << materialDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(materialDiagnostic);
        constexpr const char* woodDiagnostic =
            "island-simulator: canopy wood separates albedo from baked shade with validated outward normals and one-sided daylight";
        std::cout << woodDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(woodDiagnostic);
        constexpr const char* barkDiagnostic =
            "island-simulator: canopy bark follows local wood axes with seam-safe circular footprint filtering";
        std::cout << barkDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(barkDiagnostic);
        constexpr const char* barkVariationDiagnostic =
            "island-simulator: canopy bark has instance-stable warped grain and interrupted filtered fissures";
        std::cout << barkVariationDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(barkVariationDiagnostic);
        constexpr const char* trunkSurfaceDiagnostic =
            "island-simulator: canopy trunks use root flare, rounded buttress ridges and grey-brown bark with moss, lichen, contact occlusion and canopy shade";
        std::cout << trunkSurfaceDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(trunkSurfaceDiagnostic);
        constexpr const char* trunkPigmentDiagnostic =
            "island-simulator: canopy trunk pigment follows normalized height across LOD tiers";
        std::cout << trunkPigmentDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(trunkPigmentDiagnostic);
        constexpr const char* trunkPaletteDiagnostic =
            "island-simulator: canopy bark palette separates pale, warm, and dark fissured tree forms";
        std::cout << trunkPaletteDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(trunkPaletteDiagnostic);
        constexpr const char* trunkShadeDiagnostic =
            "island-simulator: canopy trunks retain diffuse sky fill and readable bark detail in deep crown shade";
        std::cout << trunkShadeDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(trunkShadeDiagnostic);
        constexpr const char* canopyPostureDiagnostic =
            "island-simulator: canopy blades use spatial pitch and roll with identical retained LOD poses";
        std::cout << canopyPostureDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(canopyPostureDiagnostic);
        constexpr const char* canopyOutlineDiagnostic =
            "island-simulator: curved canopy blades retain source identities within original triangle budgets";
        std::cout << canopyOutlineDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(canopyOutlineDiagnostic);
        int addedNearTriangles = 0, addedFarTriangles = 0;
        for (std::size_t i = layout.Bushes().size() - layout.ClearingBushCount(); i < layout.Bushes().size(); ++i) {
            addedNearTriangles += bushNearMeshes_[layout.Bushes()[i].variant].primitiveCount;
            addedFarTriangles += bushFarMeshes_[layout.Bushes()[i].variant].primitiveCount;
        }
        for (std::size_t i = layout.GroundPlants().size() - layout.ClearingGroundPlantCount(); i < layout.GroundPlants().size(); ++i) {
            addedNearTriangles += groundPlantNearMeshes_[layout.GroundPlants()[i].variant].primitiveCount;
            addedFarTriangles += groundPlantFarMeshes_[layout.GroundPlants()[i].variant].primitiveCount;
        }
        const std::string colonies = "island-simulator: clearing-edge colonies respect full route footprints; bushes="
            + std::to_string(layout.ClearingBushCount()) + " ground="
            + std::to_string(layout.ClearingGroundPlantCount()) + " all-near-triangles="
            + std::to_string(addedNearTriangles) + " all-far-triangles=" + std::to_string(addedFarTriangles);
        std::cout << colonies << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(colonies);
        int forestNearTriangles=0,forestFarTriangles=0;
        for (const auto& colony:layout.ForestColonies()) {
            for (std::size_t index=colony.firstPlant;index<colony.firstPlant+colony.plantCount;++index) {
                const auto variant=layout.GroundPlants()[index].variant;
                forestNearTriangles+=groundPlantNearMeshes_[variant].primitiveCount;
                forestFarTriangles+=groundPlantFarMeshes_[variant].primitiveCount;
            }
        }
        const std::string forestDiagnostic="island-simulator: forest-floor colonies retain route footprints; groups="
            +std::to_string(layout.ForestColonies().size())+" plants="+std::to_string(layout.ForestGroundPlantCount())
            +" all-near-triangles="+std::to_string(forestNearTriangles)+" all-far-triangles="
            +std::to_string(forestFarTriangles);
        std::cout << forestDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(forestDiagnostic);
        std::cout << "island-simulator: stream-bank mixed fern/broadleaf plants "
                  << layout.StreamBankGroundPlantCount() << ", shared moisture field\n";
        std::cout << "island-simulator: localized mangrove colony " << layout.MangroveCount()
                  << " trees, low crowns, multiple stems, stilt roots, canopy variant "
                  << MangroveCanopyVariant << '\n';
        const auto juvenile = std::ranges::find_if(layout.Canopy(), [](const auto& tree) {
            return tree.variant != MangroveCanopyVariant
                && tree.scale < JungleLayout::CanopyMinimumScale;
        });
        const std::string juvenilePosition = juvenile == layout.Canopy().end() ? "none"
            : std::to_string(static_cast<int>(std::lround(juvenile->x * 1000.0f))) + ':'
                + std::to_string(static_cast<int>(std::lround(juvenile->z * 1000.0f)));
        const std::string juvenileDiagnostic = "island-simulator: canopy age structure includes "
            + std::to_string(layout.JuvenileCanopyCount())
            + " deterministic 3-6m saplings beside established stands; first="
            + juvenilePosition;
        std::cout << juvenileDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(juvenileDiagnostic);
        constexpr std::string_view mangroveBarkDiagnostic =
            "island-simulator: mangrove salt-worn bark uses a pale warm palette above shared damp-base moss";
        std::cout << mangroveBarkDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(mangroveBarkDiagnostic);
        constexpr const char* crownStructureDiagnostic =
            "island-simulator: eight structural crown forms share eight LOD lobe anchors and supported trunk bends";
        std::cout << crownStructureDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(crownStructureDiagnostic);
        if (customShaderReady_) {
            std::cout << "island-simulator: jungle portable wind shader with canopy/understory/bush/ground LOD\n";
            constexpr const char* leafMaterialDiagnostic =
                "island-simulator: jungle leaves use smooth normals, filtered veins and restrained wax shading";
            std::cout << leafMaterialDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(leafMaterialDiagnostic);
            constexpr const char* canopyReceiverDiagnostic =
                "island-simulator: jungle ground layers receive shared canopy shade; crowns excluded";
            std::cout << canopyReceiverDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(canopyReceiverDiagnostic);
            std::cout << "island-simulator: jungle near canopy tapered support branches and pointed leaf silhouettes\n";
            std::cout << CanopyLayeringDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(CanopyLayeringDiagnostic);
            std::cout << CanopyBranchDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(CanopyBranchDiagnostic);
            std::cout << CanopyLianaDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(CanopyLianaDiagnostic);
            std::cout << "island-simulator: jungle near canopy irregular buttress roots ground trunk bases\n";
            std::cout << "island-simulator: jungle near canopy procedural bark fissures\n";
            std::cout << "island-simulator: jungle near bushes with pointed leaf edge breakup\n";
            std::cout << "island-simulator: near understory uses curved five-column blades with raised midribs\n";
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

    CanopyInspectionCamera JungleRenderer::SetLodInspection(const CanopyLodInspection& inspection)
    {
        int matches = 0;
        CanopyInspectionCamera camera{};
        for (const auto& cell : cells_) for (const auto& tree : cell.canopy) {
            const auto& instance = tree.draw;
            if (inspection.Matches(instance.x, instance.z)) {
                ++matches;
                camera = MakeCanopyInspectionCamera(instance.x, instance.z,
                    std::sqrt(instance.nearLodDistanceSquared),
                    {tree.bounds.Min.X, tree.bounds.Min.Y, tree.bounds.Min.Z},
                    {tree.bounds.Max.X, tree.bounds.Max.Y, tree.bounds.Max.Z});
                std::cout << "island-simulator: canopy inspection target variant="
                          << static_cast<int>(instance.variant) << " threshold="
                          << std::sqrt(instance.nearLodDistanceSquared) << '\n';
            }
        }
        if (matches != 1) throw std::invalid_argument("Canopy inspection target must match exactly one tree");
        lodInspection_ = inspection;
        std::cout << "island-simulator: TEMPORARY SOFTWARE DIAGNOSTIC canopy-lod target="
                  << inspection.xMillimetres << ':' << inspection.zMillimetres
                  << " tier=" << (inspection.near ? "near" : "far")
                  << " frozen-simulation=0 shader-time=1\n";
        return camera;
    }

    void JungleRenderer::WriteVegetationLodDiagnostics(std::ostream& output) const
    {
        output << " canopy " << visibleCanopyNearCount_ << ' ' << visibleCanopyFarCount_ << " trees ";
        bool first = true;
        const auto write = [&](const auto& batches, char tier) {
            for (const auto& batch : batches) for (const auto& transform : batch) {
                if (!first) output << ',';
                first = false;
                // Authored grid sites are metres apart. Millimetres give stable compact IDs,
                // and the consumer rejects duplicates instead of silently merging two plants.
                output << std::lround(transform.M41 * 1000.0f) << ':'
                       << std::lround(transform.M43 * 1000.0f) << ':' << tier;
            }
        };
        write(visibleCanopyNear_, 'N');
        write(visibleCanopyFar_, 'F');
        if (first) output << "none";
        output << " shrubs " << visibleBushNearCount_ << ' ' << visibleBushFarCount_ << " bushes ";
        first = true;
        write(visibleBushNear_, 'N');
        write(visibleBushFar_, 'F');
        if (first) output << "none";
    }

    void JungleRenderer::CreateMeshes()
    {
        std::size_t customVertexBytes = 0;
        const auto upload = [this, &customVertexBytes](const PlantGeometry& geometry, PlantMesh& mesh,
                                   const char* label) {
            ValidateProceduralTriangleMeshOrThrow(
                geometry.vertices.size(), geometry.indices, label);
            mesh.vertexCount = static_cast<int>(geometry.vertices.size());
            mesh.primitiveCount = static_cast<int>(geometry.indices.size() / 3U);
            for (const auto& vertex : geometry.vertices) {
                if (!std::isfinite(vertex.Position.X) || !std::isfinite(vertex.Position.Y)
                    || !std::isfinite(vertex.Position.Z) || !std::isfinite(vertex.TextureCoordinate.X)
                    || !std::isfinite(vertex.TextureCoordinate.Y)
                    || vertex.TextureCoordinate.X < 0.0f || vertex.TextureCoordinate.X > 1.0f) {
                    throw std::runtime_error(std::string(label) + " contains invalid position/wind data");
                }
                mesh.horizontalRadius = std::max(mesh.horizontalRadius,
                    std::hypot(vertex.Position.X, vertex.Position.Z));
                mesh.minimumY = std::min(mesh.minimumY, vertex.Position.Y);
                mesh.maximumY = std::max(mesh.maximumY, vertex.Position.Y);
            }
            // VertexBody's maximum displacement is below 0.15 m horizontally / 0.006 m vertically.
            mesh.horizontalRadius += JungleWind::HorizontalMargin;
            mesh.minimumY -= JungleWind::VerticalMargin;
            mesh.maximumY += JungleWind::VerticalMargin;
            if (customShaderReady_) {
                std::vector<Vector3> faceNormals(geometry.vertices.size(), Vector3::Zero);
                for (std::size_t i = 0; i < geometry.indices.size(); i += 3) {
                    const auto a = geometry.indices[i], b = geometry.indices[i + 1], c = geometry.indices[i + 2];
                    const auto normal = Vector3::Cross(geometry.vertices[b].Position - geometry.vertices[a].Position,
                                                      geometry.vertices[c].Position - geometry.vertices[a].Position);
                    faceNormals[a] += normal;
                    faceNormals[b] += normal;
                    faceNormals[c] += normal;
                }
                std::vector<PlantGpuVertex> packed;
                packed.reserve(geometry.vertices.size());
                for (std::size_t i = 0; i < geometry.vertices.size(); ++i) {
                    const auto& vertex = geometry.vertices[i];
                    const LeafSurface surface = i < geometry.leaves.size() ? geometry.leaves[i] : LeafSurface{};
                    Vector3 normal = surface.normal.LengthSquared() > 1.0e-12f ? surface.normal : faceNormals[i];
                    normal = normal.LengthSquared() > 1.0e-12f ? Vector3::Normalize(normal) : Vector3::Up;
                    const auto& leaf = surface.coordinates;
                    if (!std::isfinite(normal.X) || !std::isfinite(normal.Y) || !std::isfinite(normal.Z)
                        || !std::isfinite(leaf.X) || !std::isfinite(leaf.Y) || !std::isfinite(leaf.Z)
                        || (leaf.W != -1.0f && leaf.W != 0.0f && leaf.W != 1.0f && leaf.W != 2.0f)
                        || (leaf.W == 2.0f && vertex.TextureCoordinate.X != 0.0f)
                        || (leaf.W == -1.0f
                            ? !JungleMaterial::ValidWoodCoordinates(leaf.X, leaf.Y, leaf.Z)
                            : (leaf.X < 0.0f || leaf.X > 1.0f || std::abs(leaf.Y) > 1.0f))) {
                        throw std::runtime_error(std::string(label) + " contains invalid plant surface data");
                    }
                    const auto& color = leaf.W < 0 ? surface.woodAlbedo : vertex.Color;
                    packed.push_back({{vertex.Position.X, vertex.Position.Y, vertex.Position.Z},
                        {color.getRProperty(), color.getGProperty(), color.getBProperty(), color.getAProperty()},
                        {vertex.TextureCoordinate.X, vertex.TextureCoordinate.Y}, {normal.X, normal.Y, normal.Z},
                        {leaf.X, leaf.Y, leaf.Z, leaf.W}});
                }
                mesh.vertices = std::make_unique<VertexBuffer>(device_, PlantDeclaration(), mesh.vertexCount, BufferUsage::WriteOnly);
                mesh.vertices->SetData(packed.data(), mesh.vertexCount);
                customVertexBytes += packed.size() * sizeof(PlantGpuVertex);
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
        for (std::size_t variant = 0; variant < canopyNearMeshes_.size(); ++variant) {
            upload(CreateCanopyGeometry(static_cast<int>(variant), false),
                   canopyNearMeshes_[variant], "canopy near LOD");
            upload(CreateCanopyGeometry(static_cast<int>(variant), true),
                   canopyFarMeshes_[variant], "canopy far LOD");
            std::ostringstream diagnostic;
            diagnostic << "island-simulator: canopy mesh variant=" << variant
                      << " near-vertices=" << canopyNearMeshes_[variant].vertexCount
                      << " near-triangles=" << canopyNearMeshes_[variant].primitiveCount
                      << " far-vertices=" << canopyFarMeshes_[variant].vertexCount
                      << " far-triangles=" << canopyFarMeshes_[variant].primitiveCount;
            std::cout << diagnostic.str() << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic.str());
        }
        for (std::size_t variant = 0; variant < standingSnagNearMeshes_.size(); ++variant) {
            upload(CreateStandingSnagGeometry(static_cast<int>(variant), false),
                   standingSnagNearMeshes_[variant], "standing snag near LOD");
            upload(CreateStandingSnagGeometry(static_cast<int>(variant), true),
                   standingSnagFarMeshes_[variant], "standing snag far LOD");
            std::ostringstream diagnostic;
            diagnostic << "island-simulator: standing snag variant=" << variant
                       << " near-vertices=" << standingSnagNearMeshes_[variant].vertexCount
                       << " near-triangles=" << standingSnagNearMeshes_[variant].primitiveCount
                       << " far-vertices=" << standingSnagFarMeshes_[variant].vertexCount
                       << " far-triangles=" << standingSnagFarMeshes_[variant].primitiveCount;
            std::cout << diagnostic.str() << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic.str());
        }
        for (std::size_t variant = 0; variant < understoryNearMeshes_.size(); ++variant) {
            upload(CreateUnderstoryGeometry(static_cast<int>(variant), false),
                   understoryNearMeshes_[variant], "understory near LOD");
            upload(CreateUnderstoryGeometry(static_cast<int>(variant), true),
                   understoryFarMeshes_[variant], "understory far LOD");
            std::cout << "island-simulator: understory variant=" << variant
                      << " near-vertices=" << understoryNearMeshes_[variant].vertexCount
                      << " near-triangles=" << understoryNearMeshes_[variant].primitiveCount
                      << " far-vertices=" << understoryFarMeshes_[variant].vertexCount
                      << " far-triangles=" << understoryFarMeshes_[variant].primitiveCount << '\n';
        }
        const auto footprintGeometry = [](PlantGeometry geometry, float radius) {
            for (const auto& vertex : geometry.vertices) {
                if (std::hypot(vertex.Position.X, vertex.Position.Z) + JungleWind::HorizontalMargin > radius)
                    throw std::runtime_error("Jungle mesh exceeds clearing placement footprint including wind");
            }
            return geometry;
        };
        for (std::size_t variant = 0; variant < bushNearMeshes_.size(); ++variant) {
            upload(footprintGeometry(CreateBushGeometry(static_cast<int>(variant), false), JungleLayout::BushFootprintRadius),
                   bushNearMeshes_[variant], "bush near LOD");
            upload(footprintGeometry(CreateBushGeometry(static_cast<int>(variant), true), JungleLayout::BushFootprintRadius),
                   bushFarMeshes_[variant], "bush far LOD");
            std::cout << "island-simulator: bush variant=" << variant
                      << " near-vertices=" << bushNearMeshes_[variant].vertexCount
                      << " near-triangles=" << bushNearMeshes_[variant].primitiveCount
                      << " far-vertices=" << bushFarMeshes_[variant].vertexCount
                      << " far-triangles=" << bushFarMeshes_[variant].primitiveCount << '\n';
        }
        for (std::size_t variant = 0; variant < groundPlantNearMeshes_.size(); ++variant) {
            upload(footprintGeometry(CreateGroundPlantGeometry(static_cast<int>(variant), false), JungleLayout::GroundPlantFootprintRadius),
                   groundPlantNearMeshes_[variant], "ground-plant near LOD");
            upload(footprintGeometry(CreateGroundPlantGeometry(static_cast<int>(variant), true), JungleLayout::GroundPlantFootprintRadius),
                   groundPlantFarMeshes_[variant], "ground-plant far LOD");
            std::cout << "island-simulator: ground variant=" << variant
                      << " near-vertices=" << groundPlantNearMeshes_[variant].vertexCount
                      << " near-triangles=" << groundPlantNearMeshes_[variant].primitiveCount
                      << " far-vertices=" << groundPlantFarMeshes_[variant].vertexCount
                      << " far-triangles=" << groundPlantFarMeshes_[variant].primitiveCount << '\n';
        }
        if (customShaderReady_) {
            std::cout << "island-simulator: jungle static vertex payload " << customVertexBytes
                      << " bytes, 52-byte leaf layout, 9 attributes with instances\n";
        }
    }

    void JungleRenderer::CreateCells(const IslandTerrain& terrain, const JungleLayout& layout)
    {
        // Startup-only CPU cache; avoid a large Web stack allocation. No new GPU
        // texture, per-frame height query or shader-side deformation is required.
        const auto surface = std::make_unique<RenderedTerrainSurface>(terrain);
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
        const auto extendMeshBounds = [&extendBounds](JungleCell& cell,
            const float x, const float y, const float z, const float scale,
            const PlantMesh& nearMesh, const PlantMesh& farMesh) {
            const float radius = std::max(nearMesh.horizontalRadius, farMesh.horizontalRadius) * scale;
            const float minimumY = std::min(nearMesh.minimumY, farMesh.minimumY) * scale;
            const float maximumY = std::max(nearMesh.maximumY, farMesh.maximumY) * scale;
            extendBounds(cell, Vector3(x - radius, y + minimumY, z - radius),
                         Vector3(x + radius, y + maximumY, z + radius));
        };
        for (const CanopyPlacement& source : layout.Canopy()) {
            const auto tree = GroundJunglePlacement(source, *surface);
            JungleCell& cell = cells_[CellIndex(tree.x, tree.z)];
            const float nearLodDistance = CanopyNearLodDistance(tree.yawRadians);
            const auto& nearMesh = canopyNearMeshes_[tree.variant];
            const auto& farMesh = canopyFarMeshes_[tree.variant];
            const auto bounds = CanopyInstanceBounds(
                std::max(nearMesh.horizontalRadius, farMesh.horizontalRadius),
                std::min(nearMesh.minimumY, farMesh.minimumY),
                std::max(nearMesh.maximumY, farMesh.maximumY),
                Vector3(tree.x, tree.y, tree.z), tree.scale);
            cell.canopy.push_back({{Matrix::CreateScale(tree.scale)
                                       * Matrix::CreateRotationY(tree.yawRadians)
                                       * Matrix::CreateTranslation(tree.x, tree.y, tree.z),
                                   tree.variant, tree.x, tree.z,
                                   nearLodDistance * nearLodDistance}, bounds});
            extendBounds(cell, bounds.Min, bounds.Max);
            ++totalCanopyCount_;
        }
        for (const StandingSnagPlacement& source : layout.StandingSnags()) {
            const auto snag = GroundJunglePlacement(source, *surface);
            JungleCell& cell = cells_[CellIndex(snag.x, snag.z)];
            cell.standingSnags.push_back({Matrix::CreateScale(snag.scale)
                                              * Matrix::CreateRotationY(snag.yawRadians)
                                              * Matrix::CreateTranslation(snag.x, snag.y, snag.z),
                                          snag.variant, snag.x, snag.z,
                                          StandingSnagNearDistance * StandingSnagNearDistance});
            extendMeshBounds(cell, snag.x, snag.y, snag.z, snag.scale,
                             standingSnagNearMeshes_[snag.variant],
                             standingSnagFarMeshes_[snag.variant]);
            ++totalStandingSnagCount_;
        }
        for (const UnderstoryPlacement& source : layout.Understory()) {
            const auto plant = GroundJunglePlacement(source, *surface);
            JungleCell& cell = cells_[CellIndex(plant.x, plant.z)];
            cell.understory.push_back({Matrix::CreateScale(plant.scale)
                                           * Matrix::CreateRotationY(plant.yawRadians)
                                           * Matrix::CreateTranslation(plant.x, plant.y, plant.z),
                                       plant.variant, plant.x, plant.z,
                                       UnderstoryNearDistance * UnderstoryNearDistance});
            extendMeshBounds(cell, plant.x, plant.y, plant.z, plant.scale,
                             understoryNearMeshes_[plant.variant], understoryFarMeshes_[plant.variant]);
            ++totalUnderstoryCount_;
        }
        for (const BushPlacement& source : layout.Bushes()) {
            const auto bush = GroundJunglePlacement(source, *surface);
            JungleCell& cell = cells_[CellIndex(bush.x, bush.z)];
            cell.bushes.push_back({Matrix::CreateScale(bush.scale)
                                       * Matrix::CreateRotationY(bush.yawRadians)
                                       * Matrix::CreateTranslation(bush.x, bush.y, bush.z),
                                   bush.variant, bush.x, bush.z,
                                   BushNearDistance * BushNearDistance});
            extendMeshBounds(cell, bush.x, bush.y, bush.z, bush.scale,
                             bushNearMeshes_[bush.variant], bushFarMeshes_[bush.variant]);
            ++totalBushCount_;
        }
        for (const GroundPlantPlacement& source : layout.GroundPlants()) {
            const auto plant = GroundJunglePlacement(source, *surface);
            JungleCell& cell = cells_[CellIndex(plant.x, plant.z)];
            cell.groundPlants.push_back({Matrix::CreateScale(plant.scale)
                                             * Matrix::CreateRotationY(plant.yawRadians)
                                             * Matrix::CreateTranslation(
                                                 plant.x, plant.y, plant.z),
                                         plant.variant, plant.x, plant.z,
                                         GroundPlantNearDistance * GroundPlantNearDistance});
            extendMeshBounds(cell, plant.x, plant.y, plant.z, plant.scale,
                             groundPlantNearMeshes_[plant.variant], groundPlantFarMeshes_[plant.variant]);
            if (customShaderReady_ && plant.variant == 0) {
                const float radius = GroundLitterRadius * plant.scale;
                const auto height = surface->HeightRange(plant.x-radius,plant.z-radius,
                                                         plant.x+radius,plant.z+radius);
                extendBounds(cell,Vector3(plant.x-radius,height[0]-0.0001f,plant.z-radius),
                    Vector3(plant.x+radius,height[1]+0.052f*plant.scale+0.0001f,plant.z+radius));
            }
            ++totalGroundPlantCount_;
        }
        for (auto& transforms : visibleCanopyNear_) transforms.reserve(layout.Canopy().size());
        for (auto& transforms : visibleCanopyFar_) transforms.reserve(layout.Canopy().size());
        for (auto& transforms : visibleStandingSnagNear_)
            transforms.reserve(layout.StandingSnags().size());
        for (auto& transforms : visibleStandingSnagFar_)
            transforms.reserve(layout.StandingSnags().size());
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
            ValidateInstancedDrawRangeOrThrow(
                mesh.instanceCapacity, 0, count, "jungle instance");
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
                              const Vector3& cameraPosition, const float timeSeconds,
                              std::span<const MonkeyState> monkeys)
    {
        for (auto& transforms : visibleCanopyNear_) transforms.clear();
        for (auto& transforms : visibleCanopyFar_) transforms.clear();
        for (auto& transforms : visibleStandingSnagNear_) transforms.clear();
        for (auto& transforms : visibleStandingSnagFar_) transforms.clear();
        for (auto& transforms : visibleUnderstoryNear_) transforms.clear();
        for (auto& transforms : visibleUnderstoryFar_) transforms.clear();
        for (auto& transforms : visibleBushNear_) transforms.clear();
        for (auto& transforms : visibleBushFar_) transforms.clear();
        for (auto& transforms : visibleGroundPlantNear_) transforms.clear();
        for (auto& transforms : visibleGroundPlantFar_) transforms.clear();
        visibleCanopyCount_ = 0;
        visibleStandingSnagCount_ = 0;
        canopyFrustumRejectedCount_ = 0;
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

        int inspectionSubmissions = 0;
        const BoundingFrustum frustum(view * projection);
        const float canopyMaximumDistance = CanopyMaximumDistance * maximumDistanceScale_;
        const float standingSnagMaximumDistance = StandingSnagMaximumDistance
            * maximumDistanceScale_;
        const float understoryMaximumDistance = UnderstoryMaximumDistance * maximumDistanceScale_;
        const float bushMaximumDistance = BushMaximumDistance * maximumDistanceScale_;
        const float groundPlantMaximumDistance = GroundPlantMaximumDistance * maximumDistanceScale_;
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
            if (cellDistanceSquared <= canopyMaximumDistance * canopyMaximumDistance) {
                for (const CanopyInstance& tree : cell.canopy) {
                    const DrawInstance& instance = tree.draw;
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > canopyMaximumDistance * canopyMaximumDistance) {
                        continue;
                    }
                    if (frustum.Contains(tree.bounds) == ContainmentType::Disjoint) {
                        ++canopyFrustumRejectedCount_;
                        continue;
                    }
                    bool nearLod = instanceDistanceSquared <= instance.nearLodDistanceSquared;
                    if (lodInspection_ && lodInspection_->Matches(instance.x, instance.z)) {
                        nearLod = lodInspection_->near;
                        ++inspectionSubmissions;
                    }
                    // Supporting geometry must not change beneath planted bark grips.
                    for (const auto& monkey:monkeys)
                        if ((monkey.treeExcursion || monkey.treeReleasing || monkey.treePreparing)
                            && std::hypot(monkey.selectedTree.x-instance.x,monkey.selectedTree.z-instance.z)<1e-4f)
                            nearLod=true;
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
            if (cellDistanceSquared
                <= standingSnagMaximumDistance * standingSnagMaximumDistance) {
                for (const DrawInstance& instance : cell.standingSnags) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > standingSnagMaximumDistance * standingSnagMaximumDistance) continue;
                    const bool nearLod = instanceDistanceSquared
                        <= instance.nearLodDistanceSquared;
                    (nearLod ? visibleStandingSnagNear_ : visibleStandingSnagFar_)
                        [instance.variant].push_back(instance.transform);
                    ++visibleStandingSnagCount_;
                    cellVisible = true;
                }
            }
            if (cellDistanceSquared <= understoryMaximumDistance * understoryMaximumDistance) {
                for (const DrawInstance& instance : cell.understory) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > understoryMaximumDistance * understoryMaximumDistance) {
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
            if (cellDistanceSquared <= bushMaximumDistance * bushMaximumDistance) {
                for (const DrawInstance& instance : cell.bushes) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared > bushMaximumDistance * bushMaximumDistance) {
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
            if (cellDistanceSquared <= groundPlantMaximumDistance * groundPlantMaximumDistance) {
                for (const DrawInstance& instance : cell.groundPlants) {
                    const float instanceDeltaX = instance.x - cameraPosition.X;
                    const float instanceDeltaZ = instance.z - cameraPosition.Z;
                    const float instanceDistanceSquared = instanceDeltaX * instanceDeltaX
                        + instanceDeltaZ * instanceDeltaZ;
                    if (instanceDistanceSquared
                        > groundPlantMaximumDistance * groundPlantMaximumDistance) {
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

        if (lodInspection_ && inspectionSubmissions != 1) {
            throw std::runtime_error("Canopy inspection target was not submitted by this camera");
        }
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        if (customShaderReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            customEffect_->Apply();
            canopyLighting_.Bind(*customEffect_);
            terrainHeightMap_.Bind(*customEffect_);
            leafMaterial_->Bind(*customEffect_);
            customEffect_->SetUniformFloat("uReceiveCanopyLight", 0.0f);
            customEffect_->SetUniformMat4("uViewProjection", viewProjection);
            customEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                          cameraPosition.Y, cameraPosition.Z);
            customEffect_->SetUniformVec3(
                "uToSun", environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ);
            customEffect_->SetUniformFloat("uTime", timeSeconds);
            CampfireLighting::Bind(*customEffect_,timeSeconds);
        } else {
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        for (std::size_t variant = 0; variant < canopyNearMeshes_.size(); ++variant) {
            DrawInstances(canopyNearMeshes_[variant], visibleCanopyNear_[variant]);
            DrawInstances(canopyFarMeshes_[variant], visibleCanopyFar_[variant]);
        }
        if (customShaderReady_) customEffect_->SetUniformFloat("uReceiveCanopyLight", 1.0f);
        for (std::size_t variant = 0; variant < standingSnagNearMeshes_.size(); ++variant) {
            DrawInstances(standingSnagNearMeshes_[variant], visibleStandingSnagNear_[variant]);
            DrawInstances(standingSnagFarMeshes_[variant], visibleStandingSnagFar_[variant]);
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
