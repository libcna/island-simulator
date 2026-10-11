#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "IslandSimulator/GrassVisibility.hpp"
#include "IslandSimulator/SnakeGrassContact.hpp"

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
    class VertexDeclaration;
}

namespace island
{
    class CanopyLighting;
    class RenderedTerrainHeightMap;
    class GrassLayout;
    class IslandTerrain;

    /** @brief Cell-batched three-tier mesh grass with portable wind and a static fallback. */
    class GrassRenderer final
    {
    public:
        static constexpr int CellsPerAxis = GrassVisibility::CellsPerAxis;
        static constexpr std::size_t CellCount = GrassVisibility::CellCount;

        GrassRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      const IslandTerrain& terrain, const GrassLayout& layout,
                      CanopyLighting& canopyLighting, RenderedTerrainHeightMap& terrainHeightMap,
                      float maximumDrawDistance = 135.0f);
        ~GrassRenderer();

        GrassRenderer(const GrassRenderer&) = delete;
        GrassRenderer& operator=(const GrassRenderer&) = delete;
        GrassRenderer(GrassRenderer&&) = delete;
        GrassRenderer& operator=(GrassRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  const Microsoft::Xna::Framework::Vector3& playerPosition,
                  float playerInteractionStrength, float timeSeconds,
                  const SnakeGrassContact& snakeContact);

        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] int TotalPatchCount() const noexcept { return totalPatchCount_; }
        [[nodiscard]] int VisiblePatchCount() const noexcept { return visiblePatchCount_; }
        [[nodiscard]] int VisibleNearPatchCount() const noexcept {
            return visibleNearPatchCount_;
        }
        [[nodiscard]] int VisibleMiddlePatchCount() const noexcept {
            return visibleMiddlePatchCount_;
        }
        [[nodiscard]] int VisibleFarPatchCount() const noexcept { return visibleFarPatchCount_; }
        [[nodiscard]] int MixedLodCellCount() const noexcept { return mixedLodCellCount_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        [[nodiscard]] const std::string& FallbackReason() const noexcept { return fallbackReason_; }

    private:
        struct GrassMesh
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices;
            int vertexCount = 0;
            int primitiveCount = 0;
        };

        struct GrassCell
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> instances;
            std::vector<Microsoft::Xna::Framework::Matrix> fallbackTransforms;
            Microsoft::Xna::Framework::BoundingBox bounds;
            Microsoft::Xna::Framework::BoundingBox placementBounds;
            int patchCount = 0;
            bool occupied = false;
        };

        [[nodiscard]] static const Microsoft::Xna::Framework::Graphics::VertexDeclaration&
            InstanceDeclaration();
        void CreateMeshes();
        void CreateCells(const IslandTerrain& terrain, const GrassLayout& layout);
        void DrawInstancedMesh(const GrassMesh& mesh, const GrassCell& cell,
                               int instanceOffset, int instanceCount);
        void DrawFallbackMesh(const GrassMesh& mesh, const GrassCell& cell,
                              int instanceOffset, int instanceCount);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        CanopyLighting& canopyLighting_;
        RenderedTerrainHeightMap& terrainHeightMap_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        GrassMesh nearMesh_;
        GrassMesh middleMesh_;
        GrassMesh farMesh_;
        std::array<GrassCell, CellCount> cells_;
        std::string fallbackReason_;
        int totalPatchCount_ = 0;
        int visiblePatchCount_ = 0;
        int visibleNearPatchCount_ = 0;
        int visibleMiddlePatchCount_ = 0;
        int visibleFarPatchCount_ = 0;
        int mixedLodCellCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
        float maximumDrawDistance_ = 135.0f;
        bool customShaderReady_ = false;
        float meshRadius_ = 0.0f;
        float meshHeight_ = 0.0f;
    };
}
