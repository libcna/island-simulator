#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class DynamicVertexBuffer;
    class GraphicsDevice;
    class IndexBuffer;
    class VertexBuffer;
    class VertexDeclaration;
}

namespace island
{
    /** @brief Procedural instanced palm rendering with portable non-instanced fallback. */
    class PalmRenderer final
    {
    public:
        PalmRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     const VegetationLayout& layout);
        ~PalmRenderer();

        PalmRenderer(const PalmRenderer&) = delete;
        PalmRenderer& operator=(const PalmRenderer&) = delete;
        PalmRenderer(PalmRenderer&&) = delete;
        PalmRenderer& operator=(PalmRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition);

        [[nodiscard]] bool UsesInstancing() const noexcept { return instancingSupported_; }
        [[nodiscard]] int TotalPalmCount() const noexcept { return totalPalmCount_; }
        [[nodiscard]] int VisiblePalmCount() const noexcept { return visiblePalmCount_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        struct PalmMesh
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> instances;
            int vertexCount = 0;
            int primitiveCount = 0;
            int instanceCapacity = 0;
        };

        struct PalmDrawInstance
        {
            Microsoft::Xna::Framework::Matrix transform;
            std::uint8_t variant = 0U;
        };

        struct PalmCell
        {
            Microsoft::Xna::Framework::BoundingBox bounds;
            std::vector<PalmDrawInstance> instances;
            bool occupied = false;
        };

        [[nodiscard]] static const Microsoft::Xna::Framework::Graphics::VertexDeclaration&
            InstanceDeclaration();
        void CreateMeshes();
        void CreateCells(const VegetationLayout& layout);
        void DrawVariant(std::size_t variant,
                         const std::vector<Microsoft::Xna::Framework::Matrix>& transforms);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::array<PalmMesh, VegetationLayout::PalmVariantCount> meshes_;
        std::array<PalmCell, TerrainMeshLayout::CellCount> cells_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   VegetationLayout::PalmVariantCount> visibleTransforms_;
        bool instancingSupported_ = false;
        int totalPalmCount_ = 0;
        int visiblePalmCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
    };
}
