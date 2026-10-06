#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

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
    class GrassLayout;
    class IslandTerrain;

    /** @brief Cell-batched mesh grass with portable vertex wind and a static fallback. */
    class GrassRenderer final
    {
    public:
        static constexpr int CellsPerAxis = 16;
        static constexpr std::size_t CellCount = CellsPerAxis * CellsPerAxis;

        GrassRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      const IslandTerrain& terrain, const GrassLayout& layout);
        ~GrassRenderer();

        GrassRenderer(const GrassRenderer&) = delete;
        GrassRenderer& operator=(const GrassRenderer&) = delete;
        GrassRenderer(GrassRenderer&&) = delete;
        GrassRenderer& operator=(GrassRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  const Microsoft::Xna::Framework::Vector3& playerPosition,
                  float playerInteractionStrength, float timeSeconds);

        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] int TotalPatchCount() const noexcept { return totalPatchCount_; }
        [[nodiscard]] int VisiblePatchCount() const noexcept { return visiblePatchCount_; }
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
            int patchCount = 0;
            bool occupied = false;
        };

        [[nodiscard]] static const Microsoft::Xna::Framework::Graphics::VertexDeclaration&
            InstanceDeclaration();
        void CreateMeshes();
        void CreateCells(const IslandTerrain& terrain, const GrassLayout& layout);
        void DrawInstancedMesh(const GrassMesh& mesh, const GrassCell& cell);
        void DrawFallbackMesh(const GrassMesh& mesh, const GrassCell& cell);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        GrassMesh nearMesh_;
        GrassMesh farMesh_;
        std::array<GrassCell, CellCount> cells_;
        std::string fallbackReason_;
        int totalPatchCount_ = 0;
        int visiblePatchCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
        bool customShaderReady_ = false;
    };
}
