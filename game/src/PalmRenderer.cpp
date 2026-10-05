#include "IslandSimulator/PalmRenderer.hpp"

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
        constexpr float NearLodDistance = 210.0f;
        constexpr float MaximumDistance = 520.0f;

        struct PalmGeometry
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
    float broad = sin(uTime * 0.73 + phase + local.y * 0.11);
    float detail = sin(uTime * 1.19 + phase * 1.67 + local.x * 0.31 - local.z * 0.27);
    float sway = (broad * 0.115 + detail * 0.032) * aWindData.x;
    local.x += sway;
    local.z += sway * 0.37;
    local.y -= abs(sway) * 0.045 * aWindData.x;

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
    float fog = smoothstep(230.0, 620.0, distanceToCamera);
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
                                  "palms/palms.es.vert.glsl", ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "palms/palms.es.frag.glsl", ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "palms/palms.desktop.vert.glsl", ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "palms/palms.desktop.frag.glsl", ShaderSource(false, FragmentBody)),
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

        void AppendRoots(PalmGeometry& geometry, const int variant)
        {
            const Color color(82 + variant * 3, 65 + variant * 2, 38, 255);
            for (int root = 0; root < 5; ++root) {
                const float angle = static_cast<float>(root) * 2.0f
                    * std::numbers::pi_v<float> / 5.0f + static_cast<float>(variant) * 0.29f;
                const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
                const Vector3 side(-direction.Z, 0.0f, direction.X);
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(direction * 0.20f + Vector3(0.0f, 0.16f, 0.0f),
                                               ShadeColor(color, 1.08f), Vector2(0.0f, 0.0f));
                geometry.vertices.emplace_back(direction * 0.70f + side * 0.12f
                                                   + Vector3(0.0f, 0.025f, 0.0f),
                                               color, Vector2(0.0f, 0.0f));
                geometry.vertices.emplace_back(direction * 0.70f - side * 0.12f
                                                   + Vector3(0.0f, 0.025f, 0.0f),
                                               ShadeColor(color, 0.82f), Vector2(0.0f, 0.0f));
                geometry.indices.insert(geometry.indices.end(), {base,
                    static_cast<std::uint16_t>(base + 1U),
                    static_cast<std::uint16_t>(base + 2U)});
            }
        }

        void AppendTrunk(PalmGeometry& geometry, const int variant, const bool farMesh,
                         float& crownX, float& crownY, float& crownZ)
        {
            const int rings = farMesh ? 5 : 9;
            const int sides = farMesh ? 6 : 8;
            const float height = 9.4f + static_cast<float>(variant) * 0.55f;
            const float curveAngle = 0.72f + static_cast<float>(variant) * 1.37f;
            const float lean = 0.55f + static_cast<float>((variant * 5) % 3) * 0.25f;
            const std::uint16_t base = static_cast<std::uint16_t>(geometry.vertices.size());

            for (int ring = 0; ring <= rings; ++ring) {
                const float t = static_cast<float>(ring) / static_cast<float>(rings);
                const float bend = lean * t * t;
                const float centreX = std::cos(curveAngle) * bend
                    + std::sin(t * std::numbers::pi_v<float>) * 0.10f;
                const float centreZ = std::sin(curveAngle) * bend
                    + std::cos(t * std::numbers::pi_v<float> * 0.8f) * 0.08f * t;
                const float radius = 0.36f + (0.17f - 0.36f) * t;
                const float y = height * t;
                for (int sideIndex = 0; sideIndex < sides; ++sideIndex) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(sideIndex) / static_cast<float>(sides);
                    const float light = 0.66f + 0.32f
                        * std::max(0.0f, std::cos(angle) * 0.62f + std::sin(angle) * 0.35f);
                    const float ringVariation = ring % 2 == 0 ? 1.04f : 0.94f;
                    const Color trunkBase(132 + variant * 4, 91 + variant * 2,
                                          48 + variant * 2, 255);
                    geometry.vertices.emplace_back(
                        Vector3(centreX + std::cos(angle) * radius, y,
                                centreZ + std::sin(angle) * radius),
                        ShadeColor(trunkBase, light * ringVariation), Vector2(0.0f, 0.0f));
                }
            }

            for (int ring = 0; ring < rings; ++ring) {
                for (int sideIndex = 0; sideIndex < sides; ++sideIndex) {
                    const int nextSide = (sideIndex + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(base + ring * sides + sideIndex);
                    const auto lowerNext = static_cast<std::uint16_t>(
                        base + ring * sides + nextSide);
                    const auto upper = static_cast<std::uint16_t>(
                        base + (ring + 1) * sides + sideIndex);
                    const auto upperNext = static_cast<std::uint16_t>(
                        base + (ring + 1) * sides + nextSide);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }

            crownX = std::cos(curveAngle) * lean;
            crownY = height;
            crownZ = std::sin(curveAngle) * lean;
        }

        void AppendFronds(PalmGeometry& geometry, const int variant, const bool farMesh,
                          const float crownX, const float crownY, const float crownZ)
        {
            const int segments = farMesh ? 3 : 7;
            const int frondCount = farMesh ? 7 : 10 + variant % 3;
            const float rotation = static_cast<float>(variant) * 0.41f;
            for (int frond = 0; frond < frondCount; ++frond) {
                const float angle = rotation + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(frond) / static_cast<float>(frondCount);
                const Vector3 direction(std::cos(angle), 0.0f, std::sin(angle));
                const Vector3 side(-direction.Z, 0.0f, direction.X);
                const float length = 4.2f + 0.28f * static_cast<float>((frond + variant) % 4);
                const float lift = 0.42f + 0.18f
                    * static_cast<float>((frond + 2 * variant) % 3);
                const float drop = 1.15f + 0.22f
                    * static_cast<float>((frond + variant) % 5);
                const std::uint16_t base = static_cast<std::uint16_t>(geometry.vertices.size());
                const Color leafBase = frond % 3 == 0
                    ? Color(42, 128 + variant * 3, 52, 255)
                    : (frond % 3 == 1 ? Color(51, 147, 58 + variant * 2, 255)
                                      : Color(34, 112, 49, 255));
                const float windPhase = std::fmod(
                    static_cast<float>(frond) / static_cast<float>(frondCount)
                        + static_cast<float>(variant) * 0.117f,
                    1.0f);

                for (int segment = 0; segment <= segments; ++segment) {
                    const float t = static_cast<float>(segment) / static_cast<float>(segments);
                    const float radial = length * t;
                    const float arch = lift * std::sin(t * std::numbers::pi_v<float>)
                        - drop * t * t;
                    const float width = segment == segments
                        ? 0.035f
                        : 0.12f + 0.52f
                            * std::pow(std::sin(t * std::numbers::pi_v<float>), 0.62f);
                    const Vector3 centre(crownX + direction.X * radial,
                                         crownY + arch,
                                         crownZ + direction.Z * radial);
                    const float shade = 0.78f + 0.22f
                        * std::max(0.0f, direction.X * 0.62f + direction.Z * 0.35f);
                    const Color color = ShadeColor(leafBase, shade * (0.92f + 0.08f * t));
                    const Vector2 windData(t * t, windPhase);
                    geometry.vertices.emplace_back(centre - side * width, color, windData);
                    geometry.vertices.emplace_back(centre + side * width, color, windData);
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
        }

        [[nodiscard]] PalmGeometry CreatePalmGeometry(const int variant, const bool farMesh)
        {
            PalmGeometry geometry;
            geometry.vertices.reserve(farMesh ? 160U : 360U);
            geometry.indices.reserve(farMesh ? 480U : 1000U);
            if (!farMesh) {
                AppendRoots(geometry, variant);
            }
            float crownX = 0.0f;
            float crownY = 0.0f;
            float crownZ = 0.0f;
            AppendTrunk(geometry, variant, farMesh, crownX, crownY, crownZ);
            AppendFronds(geometry, variant, farMesh, crownX, crownY, crownZ);
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

    const VertexDeclaration& PalmRenderer::InstanceDeclaration()
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

    PalmRenderer::PalmRenderer(GraphicsDevice& device, const VegetationLayout& layout)
        : device_(device)
    {
        instancingSupported_ = device_.SupportsCapability(CNA::GraphicsCapability::Instancing);
        effect_ = std::make_unique<BasicEffect>(device_);
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        effect_->setFogStartProperty(230.0f);
        effect_->setFogEndProperty(620.0f);

        if (!instancingSupported_) {
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
                    fallbackReason_ = "the palm shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable palm shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMeshes();
        CreateCells(layout);
        std::cout << "island-simulator: palms " << totalPalmCount_ << " in "
                  << TerrainMeshLayout::CellCount << " cells, "
                  << (instancingSupported_ ? "hardware instancing" : "per-palm fallback")
                  << '\n';
        if (customShaderReady_) {
            std::cout << "island-simulator: palm portable wind shader with near/far mesh LOD\n";
        } else {
            std::cerr << "island-simulator: palm stock-effect fallback: " << fallbackReason_
                      << '\n';
        }
    }

    PalmRenderer::~PalmRenderer() = default;

    void PalmRenderer::CreateMeshes()
    {
        const auto upload = [this](const PalmGeometry& geometry, PalmMesh& mesh) {
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

        for (std::size_t variant = 0; variant < nearMeshes_.size(); ++variant) {
            upload(CreatePalmGeometry(static_cast<int>(variant), false), nearMeshes_[variant]);
            upload(CreatePalmGeometry(static_cast<int>(variant), true), farMeshes_[variant]);
        }
    }

    void PalmRenderer::CreateCells(const VegetationLayout& layout)
    {
        for (const PalmPlacement& palm : layout.Palms()) {
            PalmCell& cell = cells_[CellIndex(palm.x, palm.z)];
            PalmDrawInstance instance;
            instance.transform = Matrix::CreateScale(palm.scale)
                * Matrix::CreateRotationY(palm.yawRadians)
                * Matrix::CreateTranslation(palm.x, palm.y, palm.z);
            instance.variant = palm.variant;
            cell.instances.push_back(instance);
            ++totalPalmCount_;

            const Vector3 minimum(palm.x - 5.8f * palm.scale, palm.y - 0.1f,
                                  palm.z - 5.8f * palm.scale);
            const Vector3 maximum(palm.x + 5.8f * palm.scale,
                                  palm.y + 15.0f * palm.scale,
                                  palm.z + 5.8f * palm.scale);
            if (!cell.occupied) {
                cell.bounds = BoundingBox(minimum, maximum);
                cell.occupied = true;
            } else {
                cell.bounds = BoundingBox(Vector3::Min(cell.bounds.Min, minimum),
                                          Vector3::Max(cell.bounds.Max, maximum));
            }
        }

        for (auto& transforms : visibleNearTransforms_) {
            transforms.reserve(static_cast<std::size_t>(totalPalmCount_));
        }
        for (auto& transforms : visibleFarTransforms_) {
            transforms.reserve(static_cast<std::size_t>(totalPalmCount_));
        }
    }

    void PalmRenderer::DrawVariant(PalmMesh& mesh, const std::vector<Matrix>& transforms)
    {
        if (transforms.empty()) {
            return;
        }
        device_.setIndicesProperty(mesh.indices.get());

        if (instancingSupported_) {
            const int count = static_cast<int>(transforms.size());
            if (mesh.instances == nullptr || count > mesh.instanceCapacity) {
                int newCapacity = std::max(16, mesh.instanceCapacity);
                while (newCapacity < count) {
                    newCapacity *= 2;
                }
                mesh.instances = std::make_unique<DynamicVertexBuffer>(
                    device_, InstanceDeclaration(), newCapacity, BufferUsage::WriteOnly);
                mesh.instanceCapacity = newCapacity;
            }
            mesh.instances->SetDataRaw(transforms.data(), count, static_cast<int>(sizeof(Matrix)));
            device_.SetVertexBuffers({VertexBufferBinding(mesh.vertices.get(), 0, 0),
                                      VertexBufferBinding(mesh.instances.get(), 0, 1)});
            if (!customShaderReady_) {
                effect_->setWorldProperty(Matrix::getIdentityProperty());
                effect_->Apply();
            }
            device_.DrawInstancedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                            mesh.vertexCount, 0, mesh.primitiveCount, count);
            ++drawCallCount_;
            return;
        }

        device_.SetVertexBuffer(mesh.vertices.get());
        for (const Matrix& transform : transforms) {
            effect_->setWorldProperty(transform);
            effect_->Apply();
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                          mesh.vertexCount, 0, mesh.primitiveCount);
            ++drawCallCount_;
        }
    }

    void PalmRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition, const float timeSeconds)
    {
        for (auto& transforms : visibleNearTransforms_) {
            transforms.clear();
        }
        for (auto& transforms : visibleFarTransforms_) {
            transforms.clear();
        }
        visiblePalmCount_ = 0;
        visibleNearPalmCount_ = 0;
        visibleFarPalmCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        const BoundingFrustum frustum(view * projection);
        for (const PalmCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const Vector3 centre = (cell.bounds.Min + cell.bounds.Max) * 0.5f;
            const float deltaX = centre.X - cameraPosition.X;
            const float deltaZ = centre.Z - cameraPosition.Z;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            if (distanceSquared > MaximumDistance * MaximumDistance) {
                continue;
            }
            const bool nearLod = distanceSquared <= NearLodDistance * NearLodDistance;
            ++visibleCellCount_;
            for (const PalmDrawInstance& instance : cell.instances) {
                if (nearLod) {
                    visibleNearTransforms_[instance.variant].push_back(instance.transform);
                    ++visibleNearPalmCount_;
                } else {
                    visibleFarTransforms_[instance.variant].push_back(instance.transform);
                    ++visibleFarPalmCount_;
                }
                ++visiblePalmCount_;
            }
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
            effect_->setViewProperty(view);
            effect_->setProjectionProperty(projection);
        }

        for (std::size_t variant = 0; variant < nearMeshes_.size(); ++variant) {
            DrawVariant(nearMeshes_[variant], visibleNearTransforms_[variant]);
            DrawVariant(farMeshes_[variant], visibleFarTransforms_[variant]);
        }
        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
