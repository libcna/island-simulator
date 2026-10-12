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
    class CanopyLighting;
    class ForestFloorMaterial;
    class StreamBedPatch;
    class CaveLayout;
    class CaveGroundPatch;
    /** @brief Frustum-culled island terrain with portable procedural material variation. */
    class TerrainRenderer final
    {
    public:
        TerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        const IslandTerrain& terrain, CanopyLighting& canopyLighting);
        ~TerrainRenderer();

        TerrainRenderer(const TerrainRenderer&) = delete;
        TerrainRenderer& operator=(const TerrainRenderer&) = delete;
        TerrainRenderer(TerrainRenderer&&) = delete;
        TerrainRenderer& operator=(TerrainRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,float seconds);

        [[nodiscard]] bool UsesMaterialShader() const noexcept { return materialReady_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int TotalCellCount() const noexcept
        {
            return static_cast<int>(cells_.size());
        }
        [[nodiscard]] int VertexCount() const noexcept { return vertexCount_+streamVertexCount_+caveVertexCount_; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_+streamPrimitiveCount_+cavePrimitiveCount_; }
        [[nodiscard]] const StreamBedPatch& StreamBed() const noexcept {return *streamBed_;}
        [[nodiscard]] const CaveLayout& Cave() const noexcept {return *cave_;}
        [[nodiscard]] const CaveGroundPatch& CaveGround() const noexcept {return *caveGround_;}
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        struct RenderCell
        {
            Microsoft::Xna::Framework::BoundingBox bounds;
            int firstIndex = 0;
            int primitiveCount = 0;
        };

        void CreateMesh(const IslandTerrain& terrain);
        void DrawGeometry(int firstIndex, int primitiveCount);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        CanopyLighting& canopyLighting_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> materialEffect_;
        std::unique_ptr<ForestFloorMaterial> forestFloor_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices_;
        std::unique_ptr<StreamBedPatch> streamBed_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> streamVertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> streamIndices_;
        Microsoft::Xna::Framework::BoundingBox streamBounds_;
        int streamVertexCount_=0,streamPrimitiveCount_=0;
        std::unique_ptr<CaveLayout> cave_;
        std::unique_ptr<CaveGroundPatch> caveGround_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> caveVertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> caveIndices_;
        Microsoft::Xna::Framework::BoundingBox caveBounds_;
        int caveVertexCount_=0,cavePrimitiveCount_=0;
        std::vector<RenderCell> cells_;
        bool materialReady_ = false;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
    };
}
