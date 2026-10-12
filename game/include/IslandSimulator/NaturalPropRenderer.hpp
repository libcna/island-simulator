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
    /** @brief Portable instanced rocks, wood, and small beach details with distance reduction. */
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
        [[nodiscard]] int TotalReefRockCount() const noexcept { return totalReefRockCount_; }
        [[nodiscard]] int TotalLogCount() const noexcept { return totalLogCount_; }
        [[nodiscard]] int TotalDriftwoodCount() const noexcept { return totalDriftwoodCount_; }
        [[nodiscard]] int TotalBeachDetailCount() const noexcept
        {
            return totalBeachDetailCount_;
        }
        [[nodiscard]] int TotalFallenFrondCount() const noexcept
        {
            return totalFallenFrondCount_;
        }
        [[nodiscard]] int TotalReefHabitatCount() const noexcept
        {
            return totalReefHabitatCount_;
        }
        [[nodiscard]] int VisibleRockCount() const noexcept { return visibleRockCount_; }
        [[nodiscard]] int VisibleReefRockCount() const noexcept { return visibleReefRockCount_; }
        [[nodiscard]] int VisibleLogCount() const noexcept { return visibleLogCount_; }
        [[nodiscard]] int VisibleDriftwoodCount() const noexcept { return visibleDriftwoodCount_; }
        [[nodiscard]] int VisibleDriftwoodNearCount() const noexcept
        {
            return visibleDriftwoodNearCount_;
        }
        [[nodiscard]] int VisibleDriftwoodFarCount() const noexcept
        {
            return visibleDriftwoodFarCount_;
        }
        [[nodiscard]] int VisibleBeachDetailCount() const noexcept
        {
            return visibleBeachDetailCount_;
        }
        [[nodiscard]] int VisibleFallenFrondCount() const noexcept
        {
            return visibleFallenFrondCount_;
        }
        [[nodiscard]] int VisibleReefHabitatCount() const noexcept
        {
            return visibleReefHabitatCount_;
        }
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
            float x = 0.0f;
            float z = 0.0f;
            bool offshore = false;
        };

        struct PropCell
        {
            Microsoft::Xna::Framework::BoundingBox bounds;
            std::vector<DrawInstance> rocks;
            std::vector<DrawInstance> logs;
            std::vector<DrawInstance> driftwood;
            std::vector<DrawInstance> beachDetails;
            std::vector<DrawInstance> fallenFronds;
            std::vector<DrawInstance> reefHabitat;
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
        std::array<PropMesh, NaturalPropLayout::BeachDetailVariantCount> beachDetailMeshes_;
        std::array<PropMesh, NaturalPropLayout::FallenFrondVariantCount> fallenFrondMeshes_;
        std::array<PropMesh, NaturalPropLayout::ReefHabitatVariantCount> reefHabitatNearMeshes_;
        std::array<PropMesh, NaturalPropLayout::ReefHabitatVariantCount> reefHabitatFarMeshes_;
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
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::BeachDetailVariantCount> visibleBeachDetails_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::FallenFrondVariantCount> visibleFallenFronds_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::ReefHabitatVariantCount> visibleReefHabitatNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   NaturalPropLayout::ReefHabitatVariantCount> visibleReefHabitatFar_;
        std::string fallbackReason_;
        bool instancingSupported_ = false;
        bool customShaderReady_ = false;
        int totalRockCount_ = 0;
        int totalReefRockCount_ = 0;
        int totalLogCount_ = 0;
        int totalDriftwoodCount_ = 0;
        int totalBeachDetailCount_ = 0;
        int totalFallenFrondCount_ = 0;
        int totalReefHabitatCount_ = 0;
        int visibleRockCount_ = 0;
        int visibleReefRockCount_ = 0;
        int visibleLogCount_ = 0;
        int visibleDriftwoodCount_ = 0;
        int visibleDriftwoodNearCount_ = 0;
        int visibleDriftwoodFarCount_ = 0;
        int visibleBeachDetailCount_ = 0;
        int visibleFallenFrondCount_ = 0;
        int visibleReefHabitatCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
    };
}
