#include "IslandSimulator/PalmRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>

#include "CNA/GraphicsCapability.hpp"
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
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBufferBinding.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElement.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        struct PalmGeometry
        {
            std::vector<VertexPositionColor> vertices;
            std::vector<std::uint16_t> indices;
        };

        [[nodiscard]] Color ShadeColor(const Color& color, const float factor)
        {
            const auto channel = [factor](const std::uint8_t value) {
                return static_cast<std::uint8_t>(std::lround(std::clamp(
                    static_cast<float>(value) * factor, 0.0f, 255.0f)));
            };
            return Color(channel(color.getRProperty()), channel(color.getGProperty()),
                         channel(color.getBProperty()), color.getAProperty());
        }

        void AppendTrunk(PalmGeometry& geometry, const int variant, float& crownX,
                         float& crownY, float& crownZ)
        {
            constexpr int rings = 9;
            constexpr int sides = 8;
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
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float light = 0.66f + 0.32f
                        * std::max(0.0f, std::cos(angle) * 0.62f + std::sin(angle) * 0.35f);
                    const float ringVariation = ring % 2 == 0 ? 1.04f : 0.94f;
                    const Color trunkBase(132 + variant * 4, 91 + variant * 2,
                                          48 + variant * 2, 255);
                    geometry.vertices.emplace_back(
                        Vector3(centreX + std::cos(angle) * radius, y,
                                centreZ + std::sin(angle) * radius),
                        ShadeColor(trunkBase, light * ringVariation));
                }
            }

            for (int ring = 0; ring < rings; ++ring) {
                for (int side = 0; side < sides; ++side) {
                    const int nextSide = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(base + ring * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(
                        base + ring * sides + nextSide);
                    const auto upper = static_cast<std::uint16_t>(
                        base + (ring + 1) * sides + side);
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

        void AppendFronds(PalmGeometry& geometry, const int variant, const float crownX,
                          const float crownY, const float crownZ)
        {
            constexpr int segments = 7;
            const int frondCount = 10 + variant % 3;
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
                    geometry.vertices.emplace_back(centre - side * width, color);
                    geometry.vertices.emplace_back(centre + side * width, color);
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

        [[nodiscard]] PalmGeometry CreatePalmGeometry(const int variant)
        {
            PalmGeometry geometry;
            geometry.vertices.reserve(320U);
            geometry.indices.reserve(960U);
            float crownX = 0.0f;
            float crownY = 0.0f;
            float crownZ = 0.0f;
            AppendTrunk(geometry, variant, crownX, crownY, crownZ);
            AppendFronds(geometry, variant, crownX, crownY, crownZ);
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
        instancingSupported_ = device_.SupportsCapability(CNA::GraphicsCapability::Instancing)
            && device_.SupportsCapability(CNA::GraphicsCapability::MultiStreamVertexInput);
        effect_ = std::make_unique<BasicEffect>(device_);
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        effect_->setFogStartProperty(230.0f);
        effect_->setFogEndProperty(620.0f);
        CreateMeshes();
        CreateCells(layout);
        std::cout << "island-simulator: palms " << totalPalmCount_ << " in "
                  << TerrainMeshLayout::CellCount << " cells, "
                  << (instancingSupported_ ? "hardware instancing" : "per-palm fallback")
                  << '\n';
    }

    PalmRenderer::~PalmRenderer() = default;

    void PalmRenderer::CreateMeshes()
    {
        for (std::size_t variant = 0; variant < meshes_.size(); ++variant) {
            const PalmGeometry geometry = CreatePalmGeometry(static_cast<int>(variant));
            PalmMesh& mesh = meshes_[variant];
            mesh.vertexCount = static_cast<int>(geometry.vertices.size());
            mesh.primitiveCount = static_cast<int>(geometry.indices.size() / 3U);
            mesh.vertices = std::make_unique<VertexBuffer>(
                device_, VertexPositionColor::getVertexDeclarationStatic(), mesh.vertexCount,
                BufferUsage::WriteOnly);
            mesh.vertices->SetData(geometry.vertices.data(), mesh.vertexCount);
            mesh.indices = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits, static_cast<int>(geometry.indices.size()),
                BufferUsage::WriteOnly);
            mesh.indices->SetData(geometry.indices.data(),
                                  static_cast<int>(geometry.indices.size()));
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

        for (auto& transforms : visibleTransforms_) {
            transforms.reserve(static_cast<std::size_t>(totalPalmCount_));
        }
    }

    void PalmRenderer::DrawVariant(const std::size_t variant,
                                   const std::vector<Matrix>& transforms)
    {
        if (transforms.empty()) {
            return;
        }
        PalmMesh& mesh = meshes_[variant];
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
            effect_->setWorldProperty(Matrix::getIdentityProperty());
            effect_->Apply();
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
                            const Vector3& cameraPosition)
    {
        for (auto& transforms : visibleTransforms_) {
            transforms.clear();
        }
        visiblePalmCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        const BoundingFrustum frustum(view * projection);
        constexpr float maximumDistance = 520.0f;
        for (const PalmCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const Vector3 centre = (cell.bounds.Min + cell.bounds.Max) * 0.5f;
            const float deltaX = centre.X - cameraPosition.X;
            const float deltaZ = centre.Z - cameraPosition.Z;
            if (deltaX * deltaX + deltaZ * deltaZ > maximumDistance * maximumDistance) {
                continue;
            }
            ++visibleCellCount_;
            for (const PalmDrawInstance& instance : cell.instances) {
                visibleTransforms_[instance.variant].push_back(instance.transform);
                ++visiblePalmCount_;
            }
        }

        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        for (std::size_t variant = 0; variant < meshes_.size(); ++variant) {
            DrawVariant(variant, visibleTransforms_[variant]);
        }
        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
