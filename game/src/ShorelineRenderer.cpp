#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ShorelineRenderer.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/OceanWaveSpectrum.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/ShorelineMeshLayout.hpp"
#include "IslandSimulator/TidePoolLayout.hpp"

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
        constexpr const char* PorousFoamDiagnostic =
            "island-simulator: shoreline advancing wash uses footprint-filtered porous foam";
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
out float vPool;

void main()
{
    vec3 world = aPosition;
    vPool = aRibbonData.y < 0.0 ? 1.0 : 0.0;
    if (vPool > 0.5) {
        world.y += 0.006 * sin(uTime * 0.38 + dot(world.xz, vec2(0.31, -0.23)));
    } else {
        world.y += oceanHeight(world.xz, uTime) + 0.022;
    }
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
in float vPool;

vec2 cellPoint(vec2 cell)
{
    // Bounded float hash, identical feature set in GLSL 330 and GLSL ES 300.
    vec3 p = fract(vec3(cell.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yzx + 33.33);
    return fract((p.xx + p.yz) * p.zy);
}

float porousFoam(vec2 world, float footprint)
{
    const float frequency = 4.6;
    vec2 p = world * frequency;
    vec2 cell = floor(p);
    vec2 local = fract(p);
    float first = 8.0;
    float second = 8.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 offset = vec2(float(x), float(y));
            vec2 delta = offset + cellPoint(cell + offset) - local;
            float distanceSquared = dot(delta, delta);
            second = min(second, max(first, distanceSquared));
            first = min(first, distanceSquared);
        }
    }
    // Thin joined rims around transparent cells, not opaque white discs. Once cells fall below
    // the pixel footprint, approach their mean coverage rather than sparkle at grazing angles.
    float filterWidth = clamp(footprint * frequency * 0.7, 0.015, 0.24);
    float rim = 1.0 - smoothstep(0.035, 0.11 + filterWidth, second - first);
    float resolved = 1.0 - smoothstep(0.10, 0.36, footprint);
    return mix(0.38, rim, resolved);
}

void main()
{
    const float tau = 6.28318530718;
    float footprint = max(length(dFdx(vWorld.xz)), length(dFdy(vWorld.xz)));
    float distanceToCamera = length(vWorld - uCameraPosition);
    float distanceFade = 1.0 - smoothstep(95.0, 165.0, distanceToCamera);
    if (vPool > 0.5) {
        float edge = 1.0-smoothstep(0.72,1.0,vAcross);
        float ripple = 0.5+0.5*sin(dot(vWorld.xz,vec2(2.1,-1.7))+uTime*.24);
        float glint = pow(max(0.0,0.5+0.5*sin(dot(vWorld.xz,vec2(.73,.41))-uTime*.18)),5.0);
        float alpha = edge*distanceFade*(0.70+0.10*ripple);
        vec3 pool = mix(vec3(.09,.31,.29),vec3(.30,.61,.54),.28*ripple+.22*glint);
        fragColor=vec4(pool*alpha,alpha);
        return;
    }
    float acrossFootprint = max(fwidth(vAcross), 0.002);
    float phaseOffset = sin(vPhase * tau * 7.0 + 1.2) * 0.11
        + sin(vPhase * tau * 19.0 + 2.4) * 0.025;
    float cycle = fract(uTime * 0.115 + phaseOffset);
    float advance = smoothstep(0.0, 0.68, cycle);
    float envelope = smoothstep(0.0, 0.12, cycle)
        * (1.0 - smoothstep(0.64, 0.96, cycle));
    float edgeVariation = sin(dot(vWorld.xz, vec2(0.92, -0.57))
        + sin(dot(vWorld.xz, vec2(0.21, 0.36))) * 1.3);
    // Across increases offshore: the crest advances toward land, leaving wash seaward of it.
    // The envelope vanishes before cycle wrapping, so a new wave does not pop across the beach.
    float front = mix(0.91, 0.27, advance) + edgeVariation * 0.022;
    float width = 0.025 + 0.012 * (0.5 + 0.5 * edgeVariation);
    float crest = 1.0 - smoothstep(width, width + acrossFootprint + 0.022,
                                    abs(vAcross - front));
    float brokenEdge = 0.5 + 0.5 * sin(dot(vWorld.xz, vec2(0.72, -0.51))
        + sin(dot(vWorld.xz, vec2(0.17, 0.23))) * 1.7 + uTime * 0.16);
    float brokenMask = smoothstep(0.12, 0.64, brokenEdge);
    vec2 shoreNormal = vec2(cos(vPhase * tau), sin(vPhase * tau));
    float cells = porousFoam(vWorld.xz + shoreNormal * advance * 1.7, footprint);
    float trailing = smoothstep(front - 0.01, front + 0.055, vAcross)
        * (1.0 - smoothstep(front + 0.10, front + 0.33, vAcross));
    float wash = trailing * mix(0.04, 0.24, cells) * brokenMask;
    crest *= mix(0.12, 0.94, cells) * brokenMask;
    float edgeMask = smoothstep(0.015, 0.16, vAcross)
        * (1.0 - smoothstep(0.80, 0.995, vAcross));
    distanceFade = 1.0 - smoothstep(320.0, 680.0, distanceToCamera);
    float alpha = clamp((crest * 0.76 + wash) * envelope * edgeMask * distanceFade, 0.0, 0.82);
    alpha *= mix(0.72, 1.0, clamp(vBaseMask * 2.0, 0.0, 1.0));

    vec3 foam = mix(vec3(0.78, 0.91, 0.88), vec3(0.96, 0.98, 0.91), crest);
    fragColor = vec4(foam * alpha, alpha);
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
                                  "shoreline/shoreline.es.vert.glsl",
                                  ShaderSource(true, OceanDisplacementGlsl() + VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "shoreline/shoreline.es.frag.glsl",
                                  ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "shoreline/shoreline.desktop.vert.glsl",
                                  ShaderSource(false, OceanDisplacementGlsl() + VertexBody)),
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
        fallbackEffect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
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
                      << ShorelineMeshLayout::VertexCount << " vertices, "
                      << ShorelineMeshLayout::TriangleCount << " triangles\n"
                      << "island-simulator: shoreline broken crest mask modulates wash "
                         "continuity\n"
                      << PorousFoamDiagnostic << '\n';
            std::cout << "island-simulator: rocky shore tide pools " << tidePoolCount_
                      << " static basins in the shared shoreline draw\n";
            MirrorRuntimeDiagnosticToPlatformLog(PorousFoamDiagnostic);
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
        vertices.reserve(ShorelineMeshLayout::VertexCount
            + TidePoolLayout::Definitions.size()*(TidePoolLayout::RimSegments+1U));
        indices.reserve((ShorelineMeshLayout::TriangleCount
            + TidePoolLayout::Definitions.size()*TidePoolLayout::RimSegments)*3U);

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

        for (std::size_t poolIndex=0;poolIndex<TidePoolLayout::Definitions.size();++poolIndex) {
            const auto [centreX,centreZ]=TidePoolLayout::Centre(terrain,poolIndex);
            const float level=TidePoolLayout::WaterLevel(terrain,poolIndex);
            const float radius=TidePoolLayout::WaterRadius(poolIndex);
            const auto centre=static_cast<std::uint16_t>(vertices.size());
            const float marker=-.1f-static_cast<float>(poolIndex)*.1f;
            vertices.emplace_back(Vector3(centreX,level,centreZ),
                Color::FromNonPremultiplied(51,130,119,158),Vector2(0.0f,marker));
            for (int segment=0;segment<TidePoolLayout::RimSegments;++segment) {
                const float angle=2.0f*std::numbers::pi_v<float>
                    *static_cast<float>(segment)/TidePoolLayout::RimSegments;
                vertices.emplace_back(Vector3(centreX+std::cos(angle)*radius,level,
                    centreZ+std::sin(angle)*radius),
                    Color::FromNonPremultiplied(45,116,108,18),Vector2(1.0f,marker));
            }
            for (int segment=0;segment<TidePoolLayout::RimSegments;++segment) {
                const auto current=static_cast<std::uint16_t>(centre+1U+segment);
                const auto next=static_cast<std::uint16_t>(centre+1U
                    +(segment+1)%TidePoolLayout::RimSegments);
                indices.insert(indices.end(),{centre,next,current});
            }
            ++tidePoolCount_;
        }

        ValidateProceduralTriangleMeshOrThrow(vertices.size(), indices, "shoreline");
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
