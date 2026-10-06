#include "IslandSimulator/SkyRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"

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

out vec4 fragColor;

void main()
{
    vec3 direction = normalize(vDirection);
    float elevation = clamp(direction.y * 0.92 + 0.08, 0.0, 1.0);
    float verticalBlend = pow(elevation, 0.58);
    vec3 horizon = vec3(0.64, 0.84, 0.89);
    vec3 zenith = vec3(0.14, 0.48, 0.75);
    vec3 color = mix(horizon, zenith, verticalBlend);

    float sunDot = max(dot(direction, normalize(uSunDirection)), 0.0);
    float halo = pow(sunDot, 38.0);
    float innerHalo = pow(sunDot, 240.0);
    float disc = smoothstep(0.99970, 0.99991, sunDot);
    color += vec3(1.0, 0.80, 0.48) * halo * 0.18;
    color += vec3(1.0, 0.91, 0.67) * innerHalo * 0.31;
    color = mix(color, vec3(1.0, 0.95, 0.76), disc * 0.96);

    float cloudAltitude = smoothstep(0.20, 0.38, elevation)
        * (1.0 - smoothstep(0.78, 0.94, elevation));
    float warp = sin(dot(direction, vec3(7.0, 2.0, -5.0)) * 1.7) * 0.54;
    float longBand = sin(direction.y * 33.0 + direction.x * 4.2
                         - direction.z * 3.1 + warp);
    float fineBand = sin(direction.y * 59.0 - direction.x * 7.0
                         + direction.z * 5.0 + warp * 1.7);
    float breakup = sin(dot(direction.xz, vec2(13.0, 9.0)) + 0.35);
    float wisps = smoothstep(0.72, 1.38,
                             longBand * 0.86 + fineBand * 0.28 + breakup * 0.36)
        * cloudAltitude;
    float awayFromSun = 1.0 - smoothstep(0.72, 0.96, sunDot);
    color = mix(color, vec3(0.90, 0.95, 0.96), wisps * awayFromSun * 0.14);

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
            return Color(mixChannel(0.64f, 0.14f), mixChannel(0.84f, 0.48f),
                         mixChannel(0.89f, 0.75f), 255);
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
        } else {
            std::cerr << "island-simulator: gradient sky stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    SkyRenderer::~SkyRenderer() = default;

    void SkyRenderer::Draw(const Matrix& view, const Matrix& projection,
                           const Vector3& cameraPosition)
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
