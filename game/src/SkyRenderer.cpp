#include "IslandSimulator/SkyRenderer.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/SkyCloudDrift.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"

#include <algorithm>
#include <cmath>
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
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
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
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int LatitudeSegments = 18;
        constexpr int LongitudeSegments = 36;
        constexpr float SkyRadius = 900.0f;

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProjection;
uniform vec3 uCameraPosition;

out vec3 vDirection;

void main()
{
    vDirection = normalize(aPosition);
    vec3 world = uCameraPosition + aPosition * 900.0;
    gl_Position = uViewProjection * vec4(world, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vDirection;

uniform vec3 uSunDirection;
uniform float uCloudDriftRadians;

out vec4 fragColor;

float skyHash(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float skyNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 t = fract(p);
    t = t * t * (3.0 - 2.0 * t);
    return mix(mix(skyHash(cell), skyHash(cell + vec2(1.0, 0.0)), t.x),
               mix(skyHash(cell + vec2(0.0, 1.0)), skyHash(cell + vec2(1.0, 1.0)), t.x), t.y);
}

// One fixed distant fair-weather cumulus mass: azimuth, elevation, angular half-width and
// half-height, eroded by two octaves of value noise mostly from above (flat base, billowing top),
// sunlit white on top and grey-blue below. Called once per mass with literal arguments: a
// constant array indexed by a loop counter crashed the Android emulator's host renderer
// (gfxstream + SwiftShader) on 2026-10-10 while Mesa and ANGLE accepted it.
vec3 cumulusMass(vec3 color, vec4 cloud, float seed, float azimuth, float skyElevation,
                 float sunAzimuth, vec3 hazeColor)
{
    float dAzimuth = azimuth - cloud.x;
    dAzimuth -= 6.28318530718 * floor(dAzimuth / 6.28318530718 + 0.5);
    vec2 local = vec2(dAzimuth / cloud.z, (skyElevation - cloud.y) / cloud.w);
    float radial = dot(local, local);
    if (radial > 2.6) return color;
    vec2 noiseCoordinate = vec2(dAzimuth * 9.0, skyElevation * 38.0) + vec2(seed * 7.3, seed * 3.1);
    float detail = skyNoise(noiseCoordinate) * 0.62 + skyNoise(noiseCoordinate * 2.3 + 11.0) * 0.38;
    float topErosion = (detail - 0.5) * (0.9 + 0.8 * max(local.y, 0.0));
    float shape = 1.0 - radial + topErosion * 0.9 - max(-local.y - 0.55, 0.0) * 3.0;
    float coverage = smoothstep(0.0, 0.32, shape);
    if (coverage <= 0.0) return color;
    float topLight = smoothstep(-1.0, 0.9, local.y + (detail - 0.5) * 0.8);
    float dSun = azimuth - sunAzimuth;
    dSun -= 6.28318530718 * floor(dSun / 6.28318530718 + 0.5);
    float sunSide = 0.5 + 0.5 * cos(dSun) * 0.6;
    vec3 cloudColor = mix(vec3(0.60, 0.66, 0.74), vec3(0.98, 0.97, 0.94), topLight * (0.55 + 0.45 * sunSide));
    // Far masses sit inside the haze: soften toward the horizon colour.
    cloudColor = mix(cloudColor, hazeColor, 0.30 * (1.0 - smoothstep(0.03, 0.20, skyElevation)));
    return mix(color, cloudColor, coverage * 0.92);
}

void main()
{
    vec3 direction = normalize(vDirection);
    vec3 sun = normalize(uSunDirection);
    float elevation = clamp(direction.y * 0.92 + 0.08, 0.0, 1.0);
    float verticalBlend = pow(elevation, 0.58);
    // The horizon haze is the same colour every renderer fogs toward, so distant terrain,
    // water and islets dissolve into the sky instead of standing in front of it.
    vec3 hazeColor = vec3(0.53, 0.78, 0.88);
    vec3 horizon = vec3(0.70, 0.86, 0.92);
    vec3 zenith = vec3(0.12, 0.42, 0.74);
    vec3 color = mix(horizon, zenith, verticalBlend);
    float haze = exp(-max(direction.y, 0.0) * 9.0);
    color = mix(color, hazeColor, haze * 0.85);
    color = mix(color, vec3(0.80, 0.86, 0.88), smoothstep(0.03, -0.02, direction.y) * 0.5);

    float sunDot = max(dot(direction, sun), 0.0);
    float glow = pow(sunDot, 5.0);
    float halo = pow(sunDot, 38.0);
    float innerHalo = pow(sunDot, 240.0);
    float disc = smoothstep(0.99970, 0.99991, sunDot);
    color += vec3(0.30, 0.22, 0.10) * glow * 0.30;
    color += vec3(1.0, 0.80, 0.48) * halo * 0.18;
    color += vec3(1.0, 0.91, 0.67) * innerHalo * 0.31;
    color = mix(color, vec3(1.0, 0.95, 0.76), disc * 0.96);

    float cloudAltitude = smoothstep(0.20, 0.38, elevation)
        * (1.0 - smoothstep(0.78, 0.94, elevation));
    float driftCos = cos(uCloudDriftRadians);
    float driftSin = sin(uCloudDriftRadians);
    vec3 cloudDirection = direction;
    cloudDirection.xz = mat2(driftCos, -driftSin, driftSin, driftCos) * direction.xz;
    float warp = sin(dot(cloudDirection, vec3(7.0, 2.0, -5.0)) * 1.7) * 0.54;
    float longBand = sin(cloudDirection.y * 33.0 + cloudDirection.x * 4.2
                         - cloudDirection.z * 3.1 + warp);
    float fineBand = sin(cloudDirection.y * 59.0 - cloudDirection.x * 7.0
                         + cloudDirection.z * 5.0 + warp * 1.7);
    float breakup = sin(dot(cloudDirection.xz, vec2(13.0, 9.0)) + 0.35);
    float wisps = smoothstep(0.72, 1.38,
                             longBand * 0.86 + fineBand * 0.28 + breakup * 0.36)
        * cloudAltitude;
    float awayFromSun = 1.0 - smoothstep(0.72, 0.96, sunDot);
    color = mix(color, vec3(0.90, 0.95, 0.96), wisps * awayFromSun * 0.14);

    // Cumulus masses in azimuth/elevation space; nine low over the sea, two a little higher.
    float azimuth = atan(direction.z, direction.x) - uCloudDriftRadians;
    float skyElevation = asin(clamp(direction.y, -1.0, 1.0));
    float sunAzimuth = atan(sun.z, sun.x);
    if (direction.y > -0.02 && direction.y < 0.45) {
        color = cumulusMass(color, vec4(0.35, 0.075, 0.36, 0.060), 0.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(1.10, 0.060, 0.22, 0.042), 1.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(1.95, 0.090, 0.42, 0.075), 2.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(2.60, 0.055, 0.19, 0.036), 3.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(3.30, 0.110, 0.30, 0.058), 4.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(3.95, 0.070, 0.46, 0.068), 5.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(4.70, 0.085, 0.24, 0.048), 6.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(5.25, 0.060, 0.34, 0.050), 7.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(5.90, 0.100, 0.26, 0.052), 8.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(0.80, 0.230, 0.17, 0.040), 9.0, azimuth, skyElevation, sunAzimuth, hazeColor);
        color = cumulusMass(color, vec4(4.20, 0.260, 0.20, 0.046), 10.0, azimuth, skyElevation, sunAzimuth, hazeColor);
    }

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
                                  CNA::ShaderStageEXT::Vertex, "main", "sky/sky.es.vert.glsl",
                                  ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main", "sky/sky.es.frag.glsl",
                                  ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main", "sky/sky.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main", "sky/sky.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color FallbackSkyColor(const float y)
        {
            const float elevation = std::clamp(y * 0.92f + 0.08f, 0.0f, 1.0f);
            const float blend = std::pow(elevation, 0.58f);
            const auto mixChannel = [blend](const float horizon, const float zenith) {
                return static_cast<int>(std::lround(
                    (horizon + (zenith - horizon) * blend) * 255.0f));
            };
            return Color(mixChannel(0.70f, 0.12f), mixChannel(0.86f, 0.42f),
                         mixChannel(0.92f, 0.74f), 255);
        }
    }

    SkyRenderer::SkyRenderer(GraphicsDevice& device)
        : device_(device)
    {
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(false);

        if (!device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            fallbackReason_ = "the active renderer does not support custom effects";
        } else {
            try {
                customEffect_ = std::make_unique<ShaderEffect>(device_, CreateShaderPackage());
                if (customEffect_->IsEffectValid()) {
                    customShaderReady_ = true;
                } else {
                    fallbackReason_ = "the sky shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable sky shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        std::vector<VertexPositionColor> vertices;
        std::vector<std::uint16_t> indices;
        vertices.reserve(static_cast<std::size_t>((LatitudeSegments + 1)
                                                  * LongitudeSegments));
        indices.reserve(static_cast<std::size_t>(LatitudeSegments
                                                 * LongitudeSegments * 6));
        for (int latitude = 0; latitude <= LatitudeSegments; ++latitude) {
            const float latitudeAngle = -std::numbers::pi_v<float> * 0.5f
                + std::numbers::pi_v<float> * static_cast<float>(latitude)
                    / static_cast<float>(LatitudeSegments);
            const float horizontal = std::cos(latitudeAngle);
            const float y = std::sin(latitudeAngle);
            for (int longitude = 0; longitude < LongitudeSegments; ++longitude) {
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(longitude) / static_cast<float>(LongitudeSegments);
                vertices.emplace_back(Vector3(horizontal * std::cos(angle), y,
                                              horizontal * std::sin(angle)),
                                      FallbackSkyColor(y));
            }
        }
        for (int latitude = 0; latitude < LatitudeSegments; ++latitude) {
            for (int longitude = 0; longitude < LongitudeSegments; ++longitude) {
                const int next = (longitude + 1) % LongitudeSegments;
                const auto lower = static_cast<std::uint16_t>(
                    latitude * LongitudeSegments + longitude);
                const auto lowerNext = static_cast<std::uint16_t>(
                    latitude * LongitudeSegments + next);
                const auto upper = static_cast<std::uint16_t>(
                    (latitude + 1) * LongitudeSegments + longitude);
                const auto upperNext = static_cast<std::uint16_t>(
                    (latitude + 1) * LongitudeSegments + next);
                indices.insert(indices.end(), {lower, upper, lowerNext,
                                               lowerNext, upper, upperNext});
            }
        }

        ValidateProceduralTriangleMeshOrThrow(vertices.size(), indices, "sky dome");
        vertexCount_ = static_cast<int>(vertices.size());
        primitiveCount_ = static_cast<int>(indices.size() / 3U);
        vertices_ = std::make_unique<VertexBuffer>(
            device_, VertexPositionColor::getVertexDeclarationStatic(), vertexCount_,
            BufferUsage::WriteOnly);
        vertices_->SetData(vertices.data(), vertexCount_);
        indices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(indices.size()),
            BufferUsage::WriteOnly);
        indices_->SetData(indices.data(), static_cast<int>(indices.size()));

        if (customShaderReady_) {
            std::cout << "island-simulator: fixed sunny sky with layered horizontal high wisps, "
                         "portable shader\n";
            constexpr const char* volumeDiagnostic =
                "island-simulator: sky adds shared-haze horizon, warm sun glow and eleven fixed distant cumulus masses";
            std::cout << volumeDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(volumeDiagnostic);
            constexpr const char* driftDiagnostic =
                "island-simulator: fair-weather cumulus and wisps drift slowly west with prevailing wind";
            std::cout << driftDiagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(driftDiagnostic);
        } else {
            std::cerr << "island-simulator: gradient sky stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    SkyRenderer::~SkyRenderer() = default;

    void SkyRenderer::Draw(const Matrix& view, const Matrix& projection,
                           const Vector3& cameraPosition,const float timeSeconds)
    {
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::None);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(indices_.get());

        if (customShaderReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            customEffect_->Apply();
            customEffect_->SetUniformMat4("uViewProjection", viewProjection);
            customEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                          cameraPosition.Y, cameraPosition.Z);
            const Vector3 sun = Vector3::Normalize(Vector3(
                environment::SunDirectionX,
                environment::SunDirectionY,
                environment::SunDirectionZ));
            customEffect_->SetUniformVec3("uSunDirection", sun.X, sun.Y, sun.Z);
            customEffect_->SetUniformFloat("uCloudDriftRadians",sky_cloud::DriftAngle(timeSeconds));
        } else {
            fallbackEffect_->setWorldProperty(Matrix::CreateScale(SkyRadius)
                                               * Matrix::CreateTranslation(cameraPosition));
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
            fallbackEffect_->Apply();
        }

        device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0,
                                      primitiveCount_);
        device_.SetVertexBuffer(nullptr);
        device_.setIndicesProperty(nullptr);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
    }
}
