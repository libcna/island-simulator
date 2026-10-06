#include "IslandSimulator/ShorelineRenderer.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/ShorelineMeshLayout.hpp"

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
        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aRibbonData;

uniform mat4 uViewProjection;
uniform float uTime;

out vec3 vWorld;
out float vAcross;
out float vPhase;
out float vBaseMask;

float oceanHeight(vec2 p, float time)
{
    float height = 0.0;
    height += sin(dot(p, vec2(0.061, 0.031)) + time * 0.72) * 0.105;
    height += sin(dot(p, vec2(-0.093, 0.074)) + time * 0.96 + 1.7) * 0.060;
    height += sin(dot(p, vec2(0.148, -0.116)) + time * 1.31 + 3.2) * 0.028;
    return height;
}

void main()
{
    vec3 world = aPosition;
    world.y += oceanHeight(world.xz, uTime) + 0.022;
    vWorld = world;
    vAcross = aRibbonData.x;
    vPhase = aRibbonData.y;
    vBaseMask = aColor.a;
    gl_Position = uViewProjection * vec4(world, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vWorld;
in float vAcross;
in float vPhase;
in float vBaseMask;

uniform vec3 uCameraPosition;
uniform float uTime;

out vec4 fragColor;

void main()
{
    const float tau = 6.28318530718;
    float longWave = sin(vPhase * tau * 7.0 - uTime * 0.72
                         + sin(vPhase * tau * 3.0 + 1.2) * 0.62);
    float shortWave = sin(vPhase * tau * 19.0 + uTime * 0.31 + 2.4);
    float front = 0.46 + longWave * 0.085 + shortWave * 0.024;

    float crestDistance = abs(vAcross - front);
    float crest = 1.0 - smoothstep(0.030, 0.112, crestDistance);
    float brokenEdge = 0.68 + 0.32
        * sin(dot(vWorld.xz, vec2(0.72, -0.51)) + uTime * 0.43
              + sin(dot(vWorld.xz, vec2(0.17, 0.23))) * 1.7);
    crest *= smoothstep(0.38, 0.78, brokenEdge);

    float behindFront = 1.0 - smoothstep(front - 0.14, front + 0.01, vAcross);
    float wash = behindFront * (0.10 + 0.07
        * (0.5 + 0.5 * sin(dot(vWorld.xz, vec2(-0.31, 0.28)) - uTime * 0.27)));
    float edgeMask = smoothstep(0.015, 0.16, vAcross)
        * (1.0 - smoothstep(0.80, 0.995, vAcross));
    float distanceToCamera = length(vWorld - uCameraPosition);
    float distanceFade = 1.0 - smoothstep(320.0, 680.0, distanceToCamera);
    float alpha = clamp((crest * 0.78 + wash) * edgeMask * distanceFade, 0.0, 0.88);
    alpha *= mix(0.72, 1.0, clamp(vBaseMask * 2.0, 0.0, 1.0));

    vec3 foam = mix(vec3(0.78, 0.91, 0.88), vec3(0.96, 0.98, 0.91), crest);
    fragColor = vec4(foam * alpha, alpha);
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
                                  "shoreline/shoreline.es.vert.glsl",
                                  ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "shoreline/shoreline.es.frag.glsl",
                                  ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "shoreline/shoreline.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "shoreline/shoreline.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color FallbackColor(const float across)
        {
            const float normalized = std::max(0.0f, 1.0f - std::abs(across - 0.46f) / 0.43f);
            const int alpha = static_cast<int>(std::lround(normalized * normalized * 150.0f));
            return Color::FromNonPremultiplied(218, 241, 232, alpha);
        }
    }

    ShorelineRenderer::ShorelineRenderer(GraphicsDevice& device, const IslandTerrain& terrain)
        : device_(device), fallbackEffect_(std::make_unique<BasicEffect>(device))
    {
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setTextureEnabledProperty(false);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        fallbackEffect_->setFogStartProperty(320.0f);
        fallbackEffect_->setFogEndProperty(680.0f);

        if (!device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            fallbackReason_ = "the active renderer does not support custom effects";
        } else {
            try {
                customEffect_ = std::make_unique<ShaderEffect>(device_, CreateShaderPackage());
                if (customEffect_->IsEffectValid()) {
                    customShaderReady_ = true;
                } else {
                    fallbackReason_ = "the shoreline shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable shoreline shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMesh(terrain);
        if (customShaderReady_) {
            std::cout << "island-simulator: shoreline portable animated foam ribbon, "
                      << vertexCount_ << " vertices, " << primitiveCount_ << " triangles\n";
        } else {
            std::cerr << "island-simulator: shoreline stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    ShorelineRenderer::~ShorelineRenderer() = default;

    void ShorelineRenderer::CreateMesh(const IslandTerrain& terrain)
    {
        std::vector<VertexPositionColorTexture> vertices;
        std::vector<std::uint16_t> indices;
        vertices.reserve(ShorelineMeshLayout::VertexCount);
        indices.reserve(ShorelineMeshLayout::TriangleCount * 3U);

        for (int segment = 0; segment <= ShorelineMeshLayout::SegmentCount; ++segment) {
            const float phase = static_cast<float>(segment)
                / static_cast<float>(ShorelineMeshLayout::SegmentCount);
            const float angle = phase * 2.0f * std::numbers::pi_v<float>;
            const float shoreRadius = terrain.ShoreRadius(angle);
            for (int acrossIndex = 0;
                 acrossIndex < ShorelineMeshLayout::AcrossVertexCount; ++acrossIndex) {
                const float across = ShorelineMeshLayout::AcrossFraction(acrossIndex);
                const float radius = shoreRadius
                    + ShorelineMeshLayout::RadialOffset(acrossIndex);
                vertices.emplace_back(
                    Vector3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius),
                    FallbackColor(across), Vector2(across, phase));
            }
        }

        for (int segment = 0; segment < ShorelineMeshLayout::SegmentCount; ++segment) {
            for (int across = 0; across < ShorelineMeshLayout::AcrossQuadCount; ++across) {
                const auto nearLeft = static_cast<std::uint16_t>(
                    segment * ShorelineMeshLayout::AcrossVertexCount + across);
                const auto nearRight = static_cast<std::uint16_t>(nearLeft + 1U);
                const auto farLeft = static_cast<std::uint16_t>(
                    nearLeft + ShorelineMeshLayout::AcrossVertexCount);
                const auto farRight = static_cast<std::uint16_t>(farLeft + 1U);
                indices.insert(indices.end(), {nearLeft, farLeft, nearRight,
                                               nearRight, farLeft, farRight});
            }
        }

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

    void ShorelineRenderer::Draw(const Matrix& view, const Matrix& projection,
                                 const Vector3& cameraPosition, const float timeSeconds)
    {
        drawCallCount_ = 0;
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::AlphaBlend);
        device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(indices_.get());

        if (customShaderReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            customEffect_->Apply();
            customEffect_->SetUniformMat4("uViewProjection", viewProjection);
            customEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                          cameraPosition.Y, cameraPosition.Z);
            customEffect_->SetUniformFloat("uTime", timeSeconds);
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0,
                                          primitiveCount_);
            ++drawCallCount_;
        } else {
            fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
            for (auto& pass : fallbackEffect_->getCurrentTechniqueProperty()->getPassesProperty()) {
                pass.Apply();
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0,
                                              primitiveCount_);
                ++drawCallCount_;
            }
        }

        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
    }
}
