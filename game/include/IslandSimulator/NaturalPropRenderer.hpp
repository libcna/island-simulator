#pragma once

#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class DynamicVertexBuffer;
    class GraphicsDevice;
    class IndexBuffer;
    class ShaderEffect;
    class VertexBuffer;
    class VertexDeclaration;
}

namespace island
{
    /** @brief Portable instanced rocks, fallen logs, and beach driftwood with culling and LOD. */
    class NaturalPropRenderer final
    {
    public:
        NaturalPropRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                            const NaturalPropLayout& layout);
        ~NaturalPropRenderer();

        NaturalPropRenderer(const NaturalPropRenderer&) = delete;
        NaturalPropRenderer& operator=(const NaturalPropRenderer&) = delete;
        NaturalPropRenderer(NaturalPropRenderer&&) = delete;
        NaturalPropRenderer& operator=(NaturalPropRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition);

        [[nodiscard]] bool UsesInstancing() const noexcept { return instancingSupported_; }
        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] int TotalRockCount() const noexcept { return totalRockCount_; }
        [[nodiscard]] int TotalLogCount() const noexcept { return totalLogCount_; }
        [[nodiscard]] int TotalDriftwoodCount() const noexcept { return totalDriftwoodCount_; }
        [[nodiscard]] int VisibleRockCount() const noexcept { return visibleRockCount_; }
        [[nodiscard]] int VisibleLogCount() const noexcept { return visibleLogCount_; }
        [[nodiscard]] int VisibleDriftwoodCount() const noexcept { return visibleDriftwoodCount_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        struct PropMesh
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> instances;
            int vertexCount = 0;
            int primitiveCount = 0;
            int instanceCapacity = 0;
        };

        struct DrawInstance
        {
            Microsoft::Xna::Framework::Matrix transform;
            std::uint8_t variant = 0U;
        };

        struct PropCell
        {
            Microsoft::Xna::Framework::BoundingBox bounds;
            std::vector<DrawInstance> rocks;
            std::vector<DrawInstance> logs;
            std::vector<DrawInstance> driftwood;
            bool occupied = false;
        };

        [[nodiscard]] static const Microsoft::Xna::Framework::Graphics::VertexDeclaration&
            InstanceDeclaration();
        void CreateMeshes();
        void CreateCells(const NaturalPropLayout& layout);
        void DrawInstances(PropMesh& mesh,
                           const std::vector<Microsoft::Xna::Framework::Matrix>& transforms);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::array<PropMesh, NaturalPropLayout::RockVariantCount> rockNearMeshes_;
        std::array<PropMesh, NaturalPropLayout::RockVariantCount> rockFarMeshes_;
        std::array<PropMesh, NaturalPropLayout::LogVariantCount> logNearMeshes_;
        std::array<PropMesh, NaturalPropLayout::LogVariantCount> logFarMeshes_;
        std::array<PropMesh, NaturalPropLayout::DriftwoodVariantCount> driftwoodNearMeshes_;
        std::array<PropMesh, NaturalPropLayout::DriftwoodVariantCount> driftwoodFarMeshes_;
        std::array<PropCell, TerrainMeshLayout::CellCount> cells_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::RockVariantCount> visibleRockNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::RockVariantCount> visibleRockFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::LogVariantCount> visibleLogNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::LogVariantCount> visibleLogFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::DriftwoodVariantCount> visibleDriftwoodNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::DriftwoodVariantCount> visibleDriftwoodFar_;
        std::string fallbackReason_;
        bool instancingSupported_ = false;
        bool customShaderReady_ = false;
        int totalRockCount_ = 0;
        int totalLogCount_ = 0;
        int totalDriftwoodCount_ = 0;
        int visibleRockCount_ = 0;
        int visibleLogCount_ = 0;
        int visibleDriftwoodCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
    };
}
