#include "IslandSimulator/JungleRenderer.hpp"

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
        constexpr float CanopyNearDistance = 155.0f;
        constexpr float CanopyMaximumDistance = 475.0f;
        constexpr float UnderstoryNearDistance = 58.0f;
        constexpr float UnderstoryMaximumDistance = 128.0f;

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
    vColor = aColor.rgb;
    vWorld = world.xyz;
    gl_Position = uViewProjection * world;
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
    float fog = smoothstep(250.0, 660.0, distanceToCamera);
    vec3 color = mix(vColor, vec3(0.53, 0.78, 0.88), fog);
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
                               const float windPhase, const bool farMesh)
        {
            const int rings = farMesh ? 2 : 4;
            const int slices = farMesh ? 6 : 9;
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
                    const float light = 0.70f + 0.29f * std::max(
                        0.0f, direction.X * 0.52f + direction.Y * 0.75f - direction.Z * 0.20f);
                    geometry.vertices.emplace_back(
                        centre + Vector3(direction.X * radius.X, direction.Y * radius.Y,
                                         direction.Z * radius.Z),
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

        [[nodiscard]] PlantGeometry CreateCanopyGeometry(const int variant, const bool farMesh)
        {
            PlantGeometry geometry;
            geometry.vertices.reserve(farMesh ? 150U : 430U);
            geometry.indices.reserve(farMesh ? 700U : 1900U);
            float crownX = 0.0f;
            float crownY = 0.0f;
            float crownZ = 0.0f;
            AppendCanopyTrunk(geometry, variant, farMesh, crownX, crownY, crownZ);

            const int clusterCount = farMesh ? 4 : 8;
            const Color leafBase = variant % 2 == 0
                ? Color(34, 104 + variant * 5, 48, 255)
                : Color(42, 119 + variant * 4, 52, 255);
            for (int cluster = 0; cluster < clusterCount; ++cluster) {
                const float angle = static_cast<float>(variant) * 0.57f
                    + static_cast<float>(cluster) * 2.39996323f;
                const float radial = cluster == 0 ? 0.0f
                    : 1.4f + 0.34f * static_cast<float>((cluster + variant) % 4);
                const float vertical = static_cast<float>((cluster * 3 + variant) % 5) * 0.38f;
                const Vector3 centre(crownX + std::cos(angle) * radial,
                                     crownY - 0.35f + vertical,
                                     crownZ + std::sin(angle) * radial);
                const float width = 2.65f + 0.22f * static_cast<float>((cluster + variant) % 3);
                const float phase = std::fmod(static_cast<float>(cluster) * 0.139f
                                              + static_cast<float>(variant) * 0.23f, 1.0f);
                AppendLeafCluster(geometry, centre,
                                  Vector3(width, 1.65f + 0.13f * variant, width * 0.91f),
                                  cluster % 3 == 0 ? ShadeColor(leafBase, 1.10f) : leafBase,
                                  phase, farMesh);
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
                  << totalUnderstoryCount_ << " understory plants, "
                  << (instancingSupported_ ? "hardware instancing" : "per-plant fallback")
                  << '\n';
        if (customShaderReady_) {
            std::cout << "island-simulator: jungle portable wind shader with canopy/understory LOD\n";
        } else {
            std::cerr << "island-simulator: jungle stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    JungleRenderer::~JungleRenderer() = default;

    void JungleRenderer::CreateMeshes()
    {
        const auto upload = [this](const PlantGeometry& geometry, PlantMesh& mesh) {
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
                   canopyNearMeshes_[variant]);
            upload(CreateCanopyGeometry(static_cast<int>(variant), true),
                   canopyFarMeshes_[variant]);
        }
        for (std::size_t variant = 0; variant < understoryNearMeshes_.size(); ++variant) {
            upload(CreateUnderstoryGeometry(static_cast<int>(variant), false),
                   understoryNearMeshes_[variant]);
            upload(CreateUnderstoryGeometry(static_cast<int>(variant), true),
                   understoryFarMeshes_[variant]);
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
            cell.canopy.push_back({Matrix::CreateScale(tree.scale)
                                       * Matrix::CreateRotationY(tree.yawRadians)
                                       * Matrix::CreateTranslation(tree.x, tree.y, tree.z),
                                   tree.variant});
            const float radius = 5.5f * tree.scale;
            extendBounds(cell, Vector3(tree.x - radius, tree.y - 0.1f, tree.z - radius),
                         Vector3(tree.x + radius, tree.y + 16.5f * tree.scale, tree.z + radius));
            ++totalCanopyCount_;
        }
        for (const UnderstoryPlacement& plant : layout.Understory()) {
            JungleCell& cell = cells_[CellIndex(plant.x, plant.z)];
            cell.understory.push_back({Matrix::CreateScale(plant.scale)
                                           * Matrix::CreateRotationY(plant.yawRadians)
                                           * Matrix::CreateTranslation(plant.x, plant.y, plant.z),
                                       plant.variant});
            const float radius = 2.2f * plant.scale;
            extendBounds(cell, Vector3(plant.x - radius, plant.y - 0.05f, plant.z - radius),
                         Vector3(plant.x + radius, plant.y + 2.2f * plant.scale, plant.z + radius));
            ++totalUnderstoryCount_;
        }
        for (auto& transforms : visibleCanopyNear_) transforms.reserve(layout.Canopy().size());
        for (auto& transforms : visibleCanopyFar_) transforms.reserve(layout.Canopy().size());
        for (auto& transforms : visibleUnderstoryNear_) transforms.reserve(layout.Understory().size());
        for (auto& transforms : visibleUnderstoryFar_) transforms.reserve(layout.Understory().size());
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
        visibleCanopyCount_ = 0;
        visibleUnderstoryCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        const BoundingFrustum frustum(view * projection);
        for (const JungleCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const Vector3 centre = (cell.bounds.Min + cell.bounds.Max) * 0.5f;
            const float deltaX = centre.X - cameraPosition.X;
            const float deltaZ = centre.Z - cameraPosition.Z;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            bool cellVisible = false;
            if (distanceSquared <= CanopyMaximumDistance * CanopyMaximumDistance) {
                const bool nearLod = distanceSquared <= CanopyNearDistance * CanopyNearDistance;
                for (const DrawInstance& instance : cell.canopy) {
                    (nearLod ? visibleCanopyNear_ : visibleCanopyFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleCanopyCount_;
                    cellVisible = true;
                }
            }
            if (distanceSquared <= UnderstoryMaximumDistance * UnderstoryMaximumDistance) {
                const bool nearLod = distanceSquared <= UnderstoryNearDistance * UnderstoryNearDistance;
                for (const DrawInstance& instance : cell.understory) {
                    (nearLod ? visibleUnderstoryNear_ : visibleUnderstoryFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleUnderstoryCount_;
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
        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
