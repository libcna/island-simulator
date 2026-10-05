#pragma once

#include <memory>
#include <vector>

#include "IslandSimulator/IslandTerrain.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class GraphicsDevice;
    class IndexBuffer;
    class ShaderEffect;
    class VertexBuffer;
}

namespace island
{
    /** @brief Frustum-culled island terrain with portable procedural material variation. */
    class TerrainRenderer final
    {
    public:
        TerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        const IslandTerrain& terrain);
        ~TerrainRenderer();

        TerrainRenderer(const TerrainRenderer&) = delete;
        TerrainRenderer& operator=(const TerrainRenderer&) = delete;
        TerrainRenderer(TerrainRenderer&&) = delete;
        TerrainRenderer& operator=(TerrainRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition);

        [[nodiscard]] bool UsesMaterialShader() const noexcept { return materialReady_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int TotalCellCount() const noexcept
        {
            return static_cast<int>(cells_.size());
        }
        [[nodiscard]] int VertexCount() const noexcept { return vertexCount_; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_; }

    private:
        struct RenderCell
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices;
            Microsoft::Xna::Framework::BoundingBox bounds;
            int vertexCount = 0;
            int primitiveCount = 0;
        };

        void CreateMesh(const IslandTerrain& terrain);
        void DrawGeometry(const RenderCell& cell);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> materialEffect_;
        std::vector<RenderCell> cells_;
        bool materialReady_ = false;
        int visibleCellCount_ = 0;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
    };
}
