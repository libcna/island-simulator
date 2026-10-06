#include "IslandSimulator/GrassRenderer.hpp"

#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

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
        constexpr float NearDistance = 82.0f;
        constexpr float FarDistance = 135.0f;
        constexpr int NearBladeCount = 28;
        constexpr int FarBladeCount = 5;
        constexpr int BladeLevels = 4;
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
        };

        static_assert(sizeof(GrassInstanceData) == 40U);

        struct ClusterGeometry
        {
            std::vector<VertexPositionColorTexture> vertices;
            std::vector<std::uint16_t> indices;
        };

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aWindData;
layout(location = 3) in vec4 aPlacement;
layout(location = 4) in vec4 aVariation;
layout(location = 5) in vec2 aSlope;

uniform mat4 uViewProjection;
uniform float uTime;
uniform vec2 uPlayerPosition;
uniform float uPlayerInteractionStrength;

out vec3 vColor;
out vec3 vWorld;

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
    vec3 world = aPlacement.xyz
        + tangentX * (localX * aVariation.y)
        + surfaceNormal * (aPosition.y * aVariation.x)
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
    float sway = (broad * 0.085 + detail * 0.026) * heightWeight * aVariation.x;
    world.x += sway;
    world.z += sway * 0.46;
    world.y -= abs(sway) * 0.055 * heightWeight;

    float colorShift = (aVariation.w - 0.5) * 0.14;
    vColor = aColor.rgb * vec3(1.0 + colorShift * 0.55,
                               1.0 + colorShift,
                               1.0 + colorShift * 0.42);
    vWorld = world;
    gl_Position = uViewProjection * vec4(world, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;

uniform vec3 uCameraPosition;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float groundBlend = smoothstep(82.0, 125.0, distanceToCamera);
    vec3 color = mix(vColor, vec3(0.255, 0.465, 0.255), groundBlend);
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

        [[nodiscard]] Color BladeColor(const int blade, const float heightFraction)
        {
            const float bladeShift = static_cast<float>((blade * 17) % 9) / 8.0f - 0.5f;
            const float light = 0.80f + heightFraction * 0.20f;
            const int red = static_cast<int>(std::lround((40.0f + bladeShift * 9.0f) * light));
            const int green = static_cast<int>(std::lround((124.0f + bladeShift * 19.0f) * light));
            const int blue = static_cast<int>(std::lround((38.0f + bladeShift * 8.0f) * light));
            return Color(static_cast<std::uint8_t>(std::clamp(red, 0, 255)),
                         static_cast<std::uint8_t>(std::clamp(green, 0, 255)),
                         static_cast<std::uint8_t>(std::clamp(blue, 0, 255)),
                         static_cast<std::uint8_t>(255));
        }

        void AppendBlade(ClusterGeometry& geometry, const int blade, const int bladeCount,
                         const bool farMesh)
        {
            const float normalizedRadius = std::sqrt(
                (static_cast<float>(blade) + 0.45f) / static_cast<float>(bladeCount));
            const float angle = GoldenAngle * static_cast<float>(blade) + (farMesh ? 0.51f : 0.0f);
            const float baseX = std::cos(angle) * normalizedRadius * (farMesh ? 0.82f : 1.02f);
            const float baseZ = std::sin(angle) * normalizedRadius * (farMesh ? 0.82f : 1.02f);
            const float facing = angle * 1.71f + static_cast<float>(blade % 4) * 0.43f;
            const Vector3 direction(std::cos(facing), 0.0f, std::sin(facing));
            const Vector3 side(-direction.Z, 0.0f, direction.X);
            const float individual = 0.87f
                + 0.19f * static_cast<float>((blade * 5 + bladeCount) % 7) / 6.0f;
            const float height = (farMesh ? 0.60f : 0.40f
                + 0.045f * static_cast<float>(blade % 6)) * individual;
            const float width = farMesh ? 0.11f
                : 0.023f + 0.004f * static_cast<float>(blade % 5);
            const float lean = farMesh ? 0.12f
                : 0.07f + 0.018f * static_cast<float>(blade % 6);
            const float phase = std::fmod(static_cast<float>(blade) * 0.173f, 1.0f);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());

            for (int level = 0; level < BladeLevels; ++level) {
                const float t = static_cast<float>(level) / static_cast<float>(BladeLevels - 1);
                const float taper = level == BladeLevels - 1
                    ? 0.018f
                    : 0.58f + 0.42f * std::sin(t * std::numbers::pi_v<float>);
                const Vector3 centre(baseX, 0.0f, baseZ);
                const Vector3 curved = centre + Vector3(0.0f, 1.0f, 0.0f) * (height * t)
                    + direction * (lean * t * t);
                const Vector3 halfWidth = side * (width * taper);
                const Color color = BladeColor(blade, t);
                const Vector2 windData(t, phase);
                geometry.vertices.emplace_back(curved - halfWidth, color, windData);
                geometry.vertices.emplace_back(curved + halfWidth, color, windData);
            }

            for (int level = 0; level < BladeLevels - 1; ++level) {
                const auto left = static_cast<std::uint16_t>(base + level * 2);
                const auto right = static_cast<std::uint16_t>(left + 1U);
                const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                geometry.indices.insert(geometry.indices.end(), {left, nextLeft, right,
                                                                 right, nextLeft, nextRight});
            }
        }

        [[nodiscard]] ClusterGeometry CreateClusterGeometry(const int bladeCount,
                                                            const bool farMesh)
        {
            ClusterGeometry geometry;
            geometry.vertices.reserve(static_cast<std::size_t>(bladeCount * BladeLevels * 2));
            geometry.indices.reserve(static_cast<std::size_t>(bladeCount
                                                               * (BladeLevels - 1) * 6));
            for (int blade = 0; blade < bladeCount; ++blade) {
                AppendBlade(geometry, blade, bladeCount, farMesh);
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
                          VertexElementUsage::TextureCoordinate, 3)};
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
                         "instanced vertex shader\n";
        } else {
            std::cerr << "island-simulator: grass " << totalPatchCount_
                      << " patches, stock-effect fallback: " << fallbackReason_ << '\n';
        }
    }

    GrassRenderer::~GrassRenderer() = default;

    void GrassRenderer::CreateMeshes()
    {
        const auto upload = [this](const ClusterGeometry& geometry, GrassMesh& mesh) {
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
        upload(CreateClusterGeometry(NearBladeCount, false), nearMesh_);
        upload(CreateClusterGeometry(FarBladeCount, true), farMesh_);
    }

    void GrassRenderer::CreateCells(const IslandTerrain& terrain, const GrassLayout& layout)
    {
        std::array<std::vector<GrassInstanceData>, CellCount> instances;
        for (const GrassPlacement& patch : layout.Patches()) {
            const TerrainSample sample = terrain.Sample(patch.x, patch.z);
            const std::size_t index = CellIndex(patch.x, patch.z);
            instances[index].push_back(GrassInstanceData{
                patch.x, patch.y, patch.z, patch.yawRadians,
                patch.scale, patch.spreadScale, patch.windPhase, patch.colorVariation,
                sample.normal.x, sample.normal.z});

            GrassCell& cell = cells_[index];
            cell.fallbackTransforms.push_back(
                Matrix::CreateScale(patch.spreadScale, patch.scale, patch.spreadScale)
                * Matrix::CreateRotationY(patch.yawRadians)
                * Matrix::CreateTranslation(patch.x, patch.y, patch.z));
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
            if (instances[index].empty()) {
                continue;
            }
            cells_[index].instances = std::make_unique<VertexBuffer>(
                device_, InstanceDeclaration(), static_cast<int>(instances[index].size()),
                BufferUsage::WriteOnly);
            cells_[index].instances->SetDataRaw(instances[index].data(),
                                                static_cast<int>(instances[index].size()),
                                                static_cast<int>(sizeof(GrassInstanceData)));
        }
    }

    void GrassRenderer::DrawInstancedMesh(const GrassMesh& mesh, const GrassCell& cell)
    {
        device_.SetVertexBuffers({VertexBufferBinding(mesh.vertices.get(), 0, 0),
                                  VertexBufferBinding(cell.instances.get(), 0, 1)});
        device_.setIndicesProperty(mesh.indices.get());
        device_.DrawInstancedPrimitives(PrimitiveType::TriangleList, 0, 0, mesh.vertexCount,
                                        0, mesh.primitiveCount, cell.patchCount);
        ++drawCallCount_;
    }

    void GrassRenderer::DrawFallbackMesh(const GrassMesh& mesh, const GrassCell& cell)
    {
        device_.SetVertexBuffer(mesh.vertices.get());
        device_.setIndicesProperty(mesh.indices.get());
        for (const Matrix& transform : cell.fallbackTransforms) {
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
            const GrassMesh& mesh = distanceSquared <= NearDistance * NearDistance
                ? nearMesh_ : farMesh_;
            if (customShaderReady_) {
                DrawInstancedMesh(mesh, cell);
            } else {
                DrawFallbackMesh(mesh, cell);
            }
        }

        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
