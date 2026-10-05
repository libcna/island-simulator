#include "IslandSimulator/TerrainRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
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
layout(location = 2) in vec2 aTerrainData;

uniform mat4 uViewProjection;

out vec3 vWorld;
out vec3 vBaseColor;
out vec2 vTerrainData;

void main()
{
    vWorld = aPosition;
    vBaseColor = aColor.rgb;
    vTerrainData = aTerrainData;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vWorld;
in vec3 vBaseColor;
in vec2 vTerrainData;

uniform vec3 uCameraPosition;

out vec4 fragColor;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 fraction = fract(p);
    fraction = fraction * fraction * (3.0 - 2.0 * fraction);
    float lower = mix(hash12(cell), hash12(cell + vec2(1.0, 0.0)), fraction.x);
    float upper = mix(hash12(cell + vec2(0.0, 1.0)),
                      hash12(cell + vec2(1.0, 1.0)), fraction.x);
    return mix(lower, upper, fraction.y) - 0.5;
}

void main()
{
    vec2 p = vWorld.xz;
    float distanceToCamera = length(vWorld - uCameraPosition);
    float detailFade = 1.0 - smoothstep(95.0, 285.0, distanceToCamera);

    float broad = 0.0;
    float medium = 0.0;
    float fine = 0.0;
    if (detailFade > 0.0) {
        broad = valueNoise(p * 0.060);
        medium = valueNoise(p * 0.31 + vec2(17.3, -9.7));
        fine = sin(p.x * 1.73 + p.y * 0.41)
            * sin(p.y * 1.37 - p.x * 0.56);
    }

    float signedShoreDistance = vTerrainData.x;
    float slopeRadians = vTerrainData.y;
    float sand = (1.0 - smoothstep(7.0, 19.0, signedShoreDistance))
        * smoothstep(-4.0, 0.8, signedShoreDistance);
    float interior = smoothstep(8.0, 25.0, signedShoreDistance);
    float steep = smoothstep(0.30, 0.70, slopeRadians) * interior;

    vec3 color = vBaseColor;
    float sandVariation = (broad * 0.190 + medium * 0.125 + fine * 0.026)
        * sand * detailFade;
    color *= vec3(1.0 + sandVariation * 0.92,
                  1.0 + sandVariation * 0.72,
                  1.0 + sandVariation * 0.43);
    color += vec3(0.043, 0.026, -0.011) * medium * sand * detailFade;

    float groundVariation = (broad * 0.185 + medium * 0.120 + fine * 0.024)
        * interior * detailFade;
    color *= vec3(1.0 + groundVariation * 0.55,
                  1.0 + groundVariation,
                  1.0 + groundVariation * 0.48);
    float exposedSoil = smoothstep(0.16, 0.40, broad * 0.62 + medium * 0.38)
        * interior * detailFade;
    color = mix(color, color * vec3(0.82, 0.76, 0.61), exposedSoil * 0.18);
    color = mix(color, color * vec3(0.82, 0.78, 0.68), steep * 0.24 * detailFade);

    float wetEdge = (1.0 - smoothstep(0.0, 2.3, abs(signedShoreDistance)))
        * smoothstep(-1.3, 0.2, signedShoreDistance);
    color = mix(color, color * vec3(0.76, 0.81, 0.78), wetEdge * 0.42);

    float fog = smoothstep(260.0, 720.0, distanceToCamera);
    color = mix(color, vec3(0.53, 0.78, 0.88), fog);
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
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
                                  "terrain/terrain.es.vert.glsl", ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "terrain/terrain.es.frag.glsl", ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "terrain/terrain.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "terrain/terrain.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color BlendColor(const Color& from, const Color& to, const float amount)
        {
            const float t = std::clamp(amount, 0.0f, 1.0f);
            const auto channel = [t](const std::uint8_t a, const std::uint8_t b) {
                return static_cast<int>(std::lround(static_cast<float>(a)
                                                    + (static_cast<float>(b)
                                                       - static_cast<float>(a)) * t));
            };
            return Color(channel(from.getRProperty(), to.getRProperty()),
                         channel(from.getGProperty(), to.getGProperty()),
                         channel(from.getBProperty(), to.getBProperty()), 255);
        }

        [[nodiscard]] Color TerrainColor(const TerrainSample& sample)
        {
            const float height = sample.height;
            Color base;
            if (height < -3.0f) {
                base = Color(38, 130, 145, 255);
            } else if (height < 0.0f) {
                base = BlendColor(Color(38, 130, 145, 255), Color(113, 192, 172, 255),
                                  (height + 3.0f) / 3.0f);
            } else if (height < 1.4f) {
                base = BlendColor(Color(186, 174, 118, 255), Color(226, 201, 139, 255),
                                  height / 1.4f);
            } else if (height < 2.2f) {
                base = Color(220, 196, 131, 255);
            } else if (height < 4.5f) {
                base = BlendColor(Color(204, 186, 123, 255), Color(86, 145, 72, 255),
                                  (height - 2.2f) / 2.3f);
            } else if (height < 9.0f) {
                base = Color(79, 139, 70, 255);
            } else {
                base = BlendColor(Color(79, 139, 70, 255), Color(45, 101, 53, 255),
                                  (height - 9.0f) / 14.0f);
            }

            const Vector3 normal(sample.normal.x, sample.normal.y, sample.normal.z);
            const Vector3 sun = Vector3::Normalize(Vector3(
                environment::SunDirectionX,
                environment::SunDirectionY,
                environment::SunDirectionZ));
            const float light = std::clamp(
                0.52f + 0.48f * Vector3::Dot(normal, sun), 0.34f, 1.0f);
            return Color(
                static_cast<std::uint8_t>(static_cast<float>(base.getRProperty()) * light),
                static_cast<std::uint8_t>(static_cast<float>(base.getGProperty()) * light),
                static_cast<std::uint8_t>(static_cast<float>(base.getBProperty()) * light),
                static_cast<std::uint8_t>(255));
        }
    }

    TerrainRenderer::TerrainRenderer(GraphicsDevice& device, const IslandTerrain& terrain)
        : device_(device), fallbackEffect_(std::make_unique<BasicEffect>(device))
    {
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setTextureEnabledProperty(false);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        fallbackEffect_->setFogStartProperty(260.0f);
        fallbackEffect_->setFogEndProperty(720.0f);

        if (device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            try {
                const CNA::Graphics::ShaderPackageEXT package = CreateShaderPackage();
                materialEffect_ = std::make_unique<ShaderEffect>(device_, package);
                if (materialEffect_->IsEffectValid()) {
                    materialReady_ = true;
                    std::cout << "island-simulator: terrain portable sand/soil material shader\n";
                } else {
                    std::cerr << "island-simulator: terrain material fallback: "
                              << materialEffect_->GetCompileErrorEXT() << '\n';
                    materialEffect_.reset();
                }
            } catch (const std::exception& exception) {
                std::cerr << "island-simulator: terrain material fallback: "
                          << exception.what() << '\n';
                materialEffect_.reset();
            }
        } else {
            std::cerr << "island-simulator: terrain material fallback: custom effects unavailable\n";
        }

        CreateMesh(terrain);
    }

    TerrainRenderer::~TerrainRenderer() = default;

    void TerrainRenderer::CreateMesh(const IslandTerrain& terrain)
    {
        cells_.clear();
        cells_.reserve(TerrainMeshLayout::CellCount);
        vertexCount_ = 0;
        primitiveCount_ = 0;

        for (std::size_t cellIndex = 0; cellIndex < TerrainMeshLayout::CellCount; ++cellIndex) {
            const TerrainCellLayout layout = TerrainMeshLayout::Cell(cellIndex);
            std::vector<VertexPositionColorTexture> vertices;
            std::vector<std::uint16_t> indices;
            vertices.reserve(static_cast<std::size_t>(TerrainMeshLayout::VerticesPerCellAxis
                                                       * TerrainMeshLayout::VerticesPerCellAxis));
            indices.reserve(static_cast<std::size_t>(layout.quadColumns * layout.quadRows * 6));
            float minimumHeight = std::numeric_limits<float>::infinity();
            float maximumHeight = -std::numeric_limits<float>::infinity();

            for (int localRow = 0; localRow <= layout.quadRows; ++localRow) {
                const float z = TerrainMeshLayout::Coordinate(layout.firstGridRow + localRow);
                for (int localColumn = 0; localColumn <= layout.quadColumns; ++localColumn) {
                    const float x = TerrainMeshLayout::Coordinate(
                        layout.firstGridColumn + localColumn);
                    const TerrainSample sample = terrain.Sample(x, z);
                    vertices.emplace_back(Vector3(x, sample.height, z), TerrainColor(sample),
                                          Vector2(sample.signedShoreDistance,
                                                  sample.slopeRadians));
                    minimumHeight = std::min(minimumHeight, sample.height);
                    maximumHeight = std::max(maximumHeight, sample.height);
                }
            }

            for (int row = 0; row < layout.quadRows; ++row) {
                for (int column = 0; column < layout.quadColumns; ++column) {
                    const auto topLeft = static_cast<std::uint16_t>(
                        row * TerrainMeshLayout::VerticesPerCellAxis + column);
                    const auto topRight = static_cast<std::uint16_t>(topLeft + 1U);
                    const auto bottomLeft = static_cast<std::uint16_t>(
                        topLeft + TerrainMeshLayout::VerticesPerCellAxis);
                    const auto bottomRight = static_cast<std::uint16_t>(bottomLeft + 1U);
                    indices.insert(indices.end(), {topLeft, bottomLeft, topRight,
                                                   topRight, bottomLeft, bottomRight});
                }
            }

            RenderCell cell;
            cell.vertexCount = static_cast<int>(vertices.size());
            cell.primitiveCount = static_cast<int>(indices.size() / 3U);
            cell.bounds = BoundingBox(
                Vector3(layout.minimumX, minimumHeight - 0.1f, layout.minimumZ),
                Vector3(layout.maximumX, maximumHeight + 0.1f, layout.maximumZ));
            cell.vertices = std::make_unique<VertexBuffer>(
                device_, VertexPositionColorTexture::getVertexDeclarationStatic(),
                cell.vertexCount, BufferUsage::WriteOnly);
            cell.vertices->SetData(vertices.data(), cell.vertexCount);
            cell.indices = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits, static_cast<int>(indices.size()),
                BufferUsage::WriteOnly);
            cell.indices->SetData(indices.data(), static_cast<int>(indices.size()));
            vertexCount_ += cell.vertexCount;
            primitiveCount_ += cell.primitiveCount;
            cells_.push_back(std::move(cell));
        }
    }

    void TerrainRenderer::DrawGeometry(const RenderCell& cell)
    {
        device_.SetVertexBuffer(cell.vertices.get());
        device_.setIndicesProperty(cell.indices.get());
        device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, cell.vertexCount, 0,
                                      cell.primitiveCount);
    }

    void TerrainRenderer::Draw(const Matrix& view, const Matrix& projection,
                               const Vector3& cameraPosition)
    {
        const BoundingFrustum frustum(view * projection);
        if (materialReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            materialEffect_->Apply();
            materialEffect_->SetUniformMat4("uViewProjection", viewProjection);
            materialEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                            cameraPosition.Y, cameraPosition.Z);
        } else {
            fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        visibleCellCount_ = 0;
        for (const RenderCell& cell : cells_) {
            if (!frustum.Intersects(cell.bounds)) {
                continue;
            }
            if (materialReady_) {
                DrawGeometry(cell);
            } else {
                for (auto& pass : fallbackEffect_->getCurrentTechniqueProperty()
                                      ->getPassesProperty()) {
                    pass.Apply();
                    DrawGeometry(cell);
                }
            }
            ++visibleCellCount_;
        }
    }
}
