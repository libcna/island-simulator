#include "IslandSimulator/OceanRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/CoastalMaterial.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/OceanMeshLayout.hpp"
#include "IslandSimulator/OceanWaveSpectrum.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
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
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr const char* GrazingReflectionDiagnostic =
            "island-simulator: ocean grazing reflection bands reuse analytic slope field";
        constexpr const char* SurfaceSpectrumDiagnostic =
            "island-simulator: ocean shared nine-wave spectrum uses filtered short-wave normals";
        constexpr const char* CoastalOpticsDiagnostic =
            "island-simulator: ocean shallow attenuation uses interpolated bed depth and contact fade";
        constexpr const char* SurfaceContrastDiagnostic =
            "island-simulator: ocean bounded grazing contrast and depth-limited filtered breaker foam";
        constexpr const char* WindPacketDiagnostic =
            "island-simulator: ocean short waves use phased wind packets with analytic envelope gradients";
        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aWaterData;

uniform mat4 uViewProjection;
uniform float uTime;

out vec3 vWorld;
out vec3 vVertexTint;
out float vBedHeight;
out float vShoreDistance;

void main()
{
    vec3 world = aPosition;
    world.y += oceanHeight(world.xz, uTime);
    vWorld = world;
    vVertexTint = aColor.rgb;
    vBedHeight = aWaterData.x;
    vShoreDistance = aWaterData.y;
    gl_Position = uViewProjection * vec4(world, 1.0);
    // CNA projection matrices follow XNA's [0, 1] depth convention. Custom GLSL uses GL's
    // [-1, 1] clip depth, so perform the same remap as CNA's stock effects.
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vWorld;
in vec3 vVertexTint;
in float vBedHeight;
in float vShoreDistance;

uniform vec3 uCameraPosition;
uniform vec3 uToSun;
uniform float uTime;
uniform vec2 uPlayerWaterPosition;
uniform float uPlayerRippleIntensity;
uniform float uPlayerRipplePhase;
uniform float uPlayerSplashIntensity;
uniform float uPlayerSplashPhase;
uniform vec4 uCoastalOptics;
uniform vec4 uSurfaceAppearance;

out vec4 fragColor;

float playerRippleBand(float distanceToPlayer, float phase)
{
    float radius = mix(0.34, 1.85, phase);
    float width = mix(0.018, 0.050, phase);
    float band = 1.0 - smoothstep(width, width + 0.035,
                                  abs(distanceToPlayer - radius));
    float envelope = sin(phase * 3.14159265);
    return band * envelope * envelope;
}

float playerSplashBand(vec2 playerOffset, float phase)
{
    float distanceToPlayer = length(playerOffset);
    float radius = mix(0.18, 2.65, phase);
    float width = mix(0.15, 0.045, phase);
    float leading = 1.0 - smoothstep(width, width + 0.045,
                                     abs(distanceToPlayer - radius));
    float trailingRadius = radius * 0.72;
    float trailing = 1.0 - smoothstep(width * 0.72, width + 0.055,
                                      abs(distanceToPlayer - trailingRadius));
    float angle = atan(playerOffset.y, playerOffset.x);
    float brokenCrest = 0.70 + 0.30 * sin(angle * 7.0 + phase * 13.0 + uTime * 0.7);
    float envelope = smoothstep(0.0, 0.08, phase)
        * (1.0 - smoothstep(0.72, 1.0, phase));
    return max(leading, trailing * 0.52) * brokenCrest * envelope;
}

void main()
{
    float waterDepth = max(vWorld.y - vBedHeight, 0.0);
    float vShallow = exp(-waterDepth * uCoastalOptics.y);
    float vShoreFoam = exp(-abs(vShoreDistance) / uSurfaceAppearance.w)
        * (1.0 - smoothstep(uSurfaceAppearance.y, uSurfaceAppearance.z, waterDepth));
    float contactCoverage = smoothstep(0.0, uCoastalOptics.z, waterDepth);
    vec4 waveSlopes = oceanSlopes(vWorld.xz, uTime, dFdx(vWorld.xz), dFdy(vWorld.xz));
    vec2 slope = waveSlopes.xy;
    vec2 surfaceSlope = slope + waveSlopes.zw;
    vec3 normal = normalize(vec3(-surfaceSlope.x, 1.0, -surfaceSlope.y));
    float distanceToCamera = length(vWorld - uCameraPosition);
    vec3 toEye = (uCameraPosition - vWorld) / max(distanceToCamera, 0.0001);
    vec3 toSun = normalize(uToSun);
    vec3 halfwayDirection = (toEye + toSun) / max(length(toEye + toSun), 0.0001);

    // Use the broad surface for integrated Fresnel coverage. Applying the nonlinear alpha
    // floor to each tiny shading ripple turns the few analytic carriers into opaque stripes.
    // Complete packet normals still drive reflected direction and sunlight glints.
    vec3 coverageNormal = normalize(vec3(-slope.x, 1.0, -slope.y));
    float viewFacing = clamp(dot(coverageNormal, toEye), 0.0, 1.0);
    float fresnel = 0.025 + 0.975 * pow(1.0 - viewFacing, 5.0);
    vec3 reflected = reflect(-toEye, normal);
    float horizon = pow(1.0 - clamp(reflected.y, 0.0, 1.0), 1.7);
    vec3 reflectedSky = mix(vec3(0.22, 0.60, 0.82), vec3(0.72, 0.89, 0.95), horizon);

    vec3 deepWater = vec3(0.015, 0.245, 0.385);
    vec3 lagoonWater = vec3(0.070, 0.625, 0.610);
    vec3 water = mix(deepWater, lagoonWater, clamp(vShallow, 0.0, 1.0));
    water *= mix(vec3(0.92, 0.97, 1.02), vVertexTint * 1.65, 0.12);
    water += vec3(0.015, 0.070, 0.075) * max(dot(normal, toSun), 0.0);
    water = mix(water, reflectedSky, fresnel * 0.72);

    // Evaluate derivatives before player/material branches; unresolved colour waves fade to
    // their mean instead of producing screen-space stripes at grazing angles.
    float phaseA = dot(vWorld.xz, vec2(0.48, 0.19)) - uTime * 0.82;
    float phaseB = dot(vWorld.xz, vec2(-0.27, 0.57)) - uTime * 0.61 + 1.8;
    float continuityPhase = dot(vWorld.xz, vec2(0.91, -0.43)) + uTime * 0.36;
    float weightA = oceanWaveFilter(max(abs(dFdx(phaseA)), abs(dFdy(phaseA))));
    float weightB = oceanWaveFilter(max(abs(dFdx(phaseB)), abs(dFdy(phaseB))));
    float continuityWeight = oceanWaveFilter(max(abs(dFdx(continuityPhase)), abs(dFdy(continuityPhase))));
    float crossingWaves = 0.50 + 0.27 * sin(phaseA) * weightA + 0.23 * sin(phaseB) * weightB;
    float waveDetailFade = 1.0 - smoothstep(135.0, 420.0, distanceToCamera);
    vec2 planarEye = toEye.xz;
    vec2 planarEyeDirection = planarEye / max(length(planarEye), 0.0001);
    float glancingSlope = dot(surfaceSlope, planarEyeDirection);
    float reflectionRipple = glancingSlope / (uSurfaceAppearance.x + abs(glancingSlope));
    float grazingWeight = smoothstep(0.16, 0.86, 1.0 - viewFacing)
        * waveDetailFade * mix(0.58, 1.0, clamp(vShallow, 0.0, 1.0));
    water += vec3(0.032, 0.076, 0.090) * reflectionRipple * grazingWeight;
    float subsurfaceVisibility = (1.0 - fresnel)
        * mix(0.42, 1.0, clamp(vShallow, 0.0, 1.0));
    float subsurfaceWave = (crossingWaves - 0.50)
        * waveDetailFade * subsurfaceVisibility;
    water += vec3(0.045, 0.135, 0.114) * subsurfaceWave;

    float shallowRidgeLight = smoothstep(0.010, 0.023, length(slope))
        * waveDetailFade * (1.0 - fresnel)
        * smoothstep(0.35, 0.92, clamp(vShallow, 0.0, 1.0));
    water += vec3(0.018, 0.064, 0.054) * shallowRidgeLight;

    vec2 planarSunDirection = normalize(toSun.xz);
    float sunwardSlope = dot(slope, planarSunDirection);
    float directionalCrest = smoothstep(0.002, 0.009, sunwardSlope)
        * (1.0 - smoothstep(0.012, 0.021, sunwardSlope));
    directionalCrest *= (1.0 - smoothstep(85.0, 245.0, distanceToCamera))
        * (1.0 - fresnel)
        * mix(0.38, 1.0, clamp(vShallow, 0.0, 1.0));
    water += vec3(0.030, 0.074, 0.061) * directionalCrest;

    float sunGlint = pow(max(dot(normal, halfwayDirection), 0.0), 210.0);
    sunGlint += pow(max(dot(normal, halfwayDirection), 0.0), 42.0) * 0.055;
    water += vec3(1.00, 0.91, 0.68) * sunGlint * 2.4;

    float foam = clamp(vShoreFoam, 0.0, 1.0)
        * smoothstep(0.47, 0.79, crossingWaves + length(slope) * 4.2);
    float foamContinuity = 0.5 + 0.5 * sin(continuityPhase) * continuityWeight;
    foam *= 0.16 + 0.84 * foamContinuity * foamContinuity;
    if (uPlayerRippleIntensity > 0.001) {
        vec2 playerOffset = vWorld.xz - uPlayerWaterPosition;
        float distanceToPlayer = length(playerOffset);
        float firstPhase = fract(uPlayerRipplePhase);
        float secondPhase = fract(uPlayerRipplePhase + 0.5);
        float playerRipple = max(playerRippleBand(distanceToPlayer, firstPhase),
                                 playerRippleBand(distanceToPlayer, secondPhase));
        float spatialVariation = 0.72 + 0.28 * sin(
            dot(playerOffset, vec2(4.7, -3.9)) + uTime * 0.54
            + uPlayerRipplePhase * 7.0);
        playerRipple *= clamp(uPlayerRippleIntensity, 0.0, 1.0)
            * mix(0.58, 1.0, clamp(vShallow, 0.0, 1.0)) * spatialVariation;
        foam = max(foam, playerRipple * 0.24);
    }
    if (uPlayerSplashIntensity > 0.001) {
        vec2 playerOffset = vWorld.xz - uPlayerWaterPosition;
        float splash = playerSplashBand(playerOffset, clamp(uPlayerSplashPhase, 0.0, 1.0));
        splash *= clamp(uPlayerSplashIntensity, 0.0, 1.0)
            * mix(0.72, 1.0, clamp(vShallow, 0.0, 1.0));
        foam = max(foam, splash * 0.58);
    }
    foam = clamp(foam, 0.0, 0.86);
    water = mix(water, vec3(0.82, 0.94, 0.91), foam);

    float fog = smoothstep(300.0, 760.0, distanceToCamera);
    water = mix(water, vec3(0.53, 0.78, 0.88), fog);

    float alpha = uCoastalOptics.w * (1.0 - exp(-waterDepth * uCoastalOptics.x));
    // Grazing sky reflection should not inherit the clear lagoon's downward-view transparency.
    alpha = max(alpha, fresnel * 0.92);
    alpha = mix(alpha, 0.95, foam);
    alpha = mix(alpha, 0.96, fog);
    alpha *= contactCoverage;
    // BlendState::AlphaBlend uses premultiplied colour, matching XNA semantics.
    fragColor = vec4(water * alpha, alpha);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const std::string& body)
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
                                  "ocean/ocean.es.vert.glsl",
                                  ShaderSource(true, OceanDisplacementGlsl() + VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "ocean/ocean.es.frag.glsl",
                                  ShaderSource(true, OceanSlopesGlsl() + FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "ocean/ocean.desktop.vert.glsl",
                                  ShaderSource(false, OceanDisplacementGlsl() + VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "ocean/ocean.desktop.frag.glsl",
                                  ShaderSource(false, OceanSlopesGlsl() + FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color WaterVertexColor(const float shallow)
        {
            const auto mixChannel = [shallow](const int deep, const int lagoon) {
                return static_cast<std::uint8_t>(std::lround(
                    static_cast<float>(deep) + static_cast<float>(lagoon - deep) * shallow));
            };
            const std::uint8_t alpha = mixChannel(232, 142);
            return Color(mixChannel(18, 62), mixChannel(105, 178), mixChannel(157, 174), alpha);
        }
    }

    OceanRenderer::OceanRenderer(GraphicsDevice& device, const IslandTerrain& terrain,
                                 const int quadsPerAxis)
        : device_(device), quadsPerAxis_(quadsPerAxis)
    {
        if (!OceanMeshLayout::IsPortable(quadsPerAxis_)) {
            throw std::invalid_argument(
                "Ocean grid must be positive and addressable by unsigned 16-bit indices");
        }
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        fallbackEffect_->setFogStartProperty(300.0f);
        fallbackEffect_->setFogEndProperty(760.0f);

        if (!device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            fallbackReason_ = "the active renderer does not support custom effects";
        } else {
            try {
                const CNA::Graphics::ShaderPackageEXT package = CreateShaderPackage();
                customEffect_ = std::make_unique<ShaderEffect>(device_, package);
                if (customEffect_->IsEffectValid()) {
                    customShaderReady_ = true;
                } else {
                    fallbackReason_ = "the ocean shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable ocean shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMesh(terrain);
        if (customShaderReady_) {
            std::cout << "island-simulator: ocean portable custom shader, " << vertexCount_
                      << " vertices, " << primitiveCount_ << " triangles\n"
                      << "island-simulator: ocean reused crossing-wave subsurface colour\n"
                      << "island-simulator: ocean shallow ridge light reuses animated slope field\n"
                      << "island-simulator: ocean sunward ripple crests reuse analytic slope field\n"
                      << GrazingReflectionDiagnostic << '\n'
                      << SurfaceSpectrumDiagnostic << '\n'
                      << CoastalOpticsDiagnostic << '\n'
                      << SurfaceContrastDiagnostic << '\n'
                      << WindPacketDiagnostic << '\n'
                      << "island-simulator: ocean shoreline foam uses broken continuity mask\n";
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: ocean sunward ripple crests reuse analytic slope field");
            MirrorRuntimeDiagnosticToPlatformLog(GrazingReflectionDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(SurfaceSpectrumDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(CoastalOpticsDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(SurfaceContrastDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(WindPacketDiagnostic);
        } else {
            std::cerr << "island-simulator: ocean stock-effect fallback: " << fallbackReason_
                      << '\n';
        }
    }

    OceanRenderer::~OceanRenderer() = default;

    void OceanRenderer::CreateMesh(const IslandTerrain& terrain)
    {
        std::vector<VertexPositionColorTexture> vertices;
        std::vector<std::uint16_t> indices;
        vertices.reserve(OceanMeshLayout::VertexCountFor(quadsPerAxis_));
        indices.reserve(OceanMeshLayout::TriangleCountFor(quadsPerAxis_) * 3U);

        const int verticesPerAxis = quadsPerAxis_ + 1;
        for (int row = 0; row < verticesPerAxis; ++row) {
            const float z = OceanMeshLayout::Coordinate(row, quadsPerAxis_);
            for (int column = 0; column < verticesPerAxis; ++column) {
                const float x = OceanMeshLayout::Coordinate(column, quadsPerAxis_);
                const TerrainSample sample = terrain.Sample(x, z);
                const float shallow = CoastalWaterOptics::LagoonWeight(-sample.height);
                vertices.emplace_back(Vector3(x, 0.0f, z), WaterVertexColor(shallow),
                                      Vector2(sample.height, sample.signedShoreDistance));
            }
        }

        for (int row = 0; row < quadsPerAxis_; ++row) {
            for (int column = 0; column < quadsPerAxis_; ++column) {
                const auto topLeft = static_cast<std::uint16_t>(
                    row * verticesPerAxis + column);
                const auto topRight = static_cast<std::uint16_t>(topLeft + 1U);
                const auto bottomLeft = static_cast<std::uint16_t>(
                    topLeft + verticesPerAxis);
                const auto bottomRight = static_cast<std::uint16_t>(bottomLeft + 1U);
                indices.insert(indices.end(), {topLeft, bottomLeft, topRight,
                                               topRight, bottomLeft, bottomRight});
            }
        }

        ValidateProceduralTriangleMeshOrThrow(vertices.size(), indices, "ocean");
        vertexCount_ = static_cast<int>(vertices.size());
        primitiveCount_ = static_cast<int>(indices.size() / 3U);
        vertices_ = std::make_unique<VertexBuffer>(
            device_, VertexPositionColorTexture::getVertexDeclarationStatic(), vertexCount_,
            BufferUsage::WriteOnly);
        vertices_->SetData(vertices.data(), vertexCount_);
        indices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(indices.size()),
            BufferUsage::WriteOnly);
        indices_->SetData(indices.data(), static_cast<int>(indices.size()));
    }

    void OceanRenderer::DrawGeometry()
    {
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(indices_.get());
        device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0,
                                      primitiveCount_);
    }

    void OceanRenderer::Draw(const Matrix& view, const Matrix& projection,
                             const Vector3& cameraPosition, const float timeSeconds,
                             const float playerX, const float playerZ,
                             const float playerRippleIntensity,
                             const float playerRipplePhase,
                             const float playerSplashIntensity,
                             const float playerSplashPhase)
    {
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::AlphaBlend);
        device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);

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
            customEffect_->SetUniformVec4("uCoastalOptics", CoastalWaterOptics::ExtinctionPerMetre,
                CoastalWaterOptics::LagoonDecayPerMetre, CoastalWaterOptics::ContactFadeDepth,
                CoastalWaterOptics::MaximumBodyOpacity);
            customEffect_->SetUniformVec4("uSurfaceAppearance", CoastalSurfaceAppearance::ReflectionSlopeSoftness,
                CoastalSurfaceAppearance::FoamFullDepth, CoastalSurfaceAppearance::FoamEndDepth,
                CoastalSurfaceAppearance::FoamShoreDecayMetres);
            customEffect_->SetUniformVec2("uPlayerWaterPosition", playerX, playerZ);
            customEffect_->SetUniformFloat(
                "uPlayerRippleIntensity",
                std::isfinite(playerRippleIntensity)
                    ? std::clamp(playerRippleIntensity, 0.0f, 1.0f) : 0.0f);
            customEffect_->SetUniformFloat(
                "uPlayerRipplePhase",
                std::isfinite(playerRipplePhase)
                    ? playerRipplePhase - std::floor(playerRipplePhase) : 0.0f);
            customEffect_->SetUniformFloat(
                "uPlayerSplashIntensity",
                std::isfinite(playerSplashIntensity)
                    ? std::clamp(playerSplashIntensity, 0.0f, 1.0f) : 0.0f);
            customEffect_->SetUniformFloat(
                "uPlayerSplashPhase",
                std::isfinite(playerSplashPhase)
                    ? std::clamp(playerSplashPhase, 0.0f, 1.0f) : 1.0f);
            DrawGeometry();
        } else {
            fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
            for (auto& pass : fallbackEffect_->getCurrentTechniqueProperty()->getPassesProperty()) {
                pass.Apply();
                DrawGeometry();
            }
        }

        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
    }
}
