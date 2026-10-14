#pragma once

#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/CanopyLodInspection.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <iosfwd>
#include <string>
#include <vector>
#include <span>

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
    struct MonkeyState;
    class CanopyLighting;
    class LeafSurfaceMaterial;
    class RenderedTerrainHeightMap;
    /** @brief Instanced canopy, understory, bushes, and forest-floor plants with portable LOD. */
    class JungleRenderer final
    {
    public:
        JungleRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       const IslandTerrain& terrain, const JungleLayout& layout,
                       CanopyLighting& canopyLighting, RenderedTerrainHeightMap& terrainHeightMap,
                       float maximumDistanceScale = 1.0f);
        ~JungleRenderer();

        JungleRenderer(const JungleRenderer&) = delete;
        JungleRenderer& operator=(const JungleRenderer&) = delete;
        JungleRenderer(JungleRenderer&&) = delete;
        JungleRenderer& operator=(JungleRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  float timeSeconds, std::span<const MonkeyState> monkeys = {});

        [[nodiscard]] bool UsesInstancing() const noexcept { return instancingSupported_; }
        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] int TotalCanopyCount() const noexcept { return totalCanopyCount_; }
        [[nodiscard]] int TotalStandingSnagCount() const noexcept
        {
            return totalStandingSnagCount_;
        }
        [[nodiscard]] int TotalUnderstoryCount() const noexcept { return totalUnderstoryCount_; }
        [[nodiscard]] int TotalBushCount() const noexcept { return totalBushCount_; }
        [[nodiscard]] int TotalGroundPlantCount() const noexcept { return totalGroundPlantCount_; }
        [[nodiscard]] int VisibleCanopyCount() const noexcept { return visibleCanopyCount_; }
        [[nodiscard]] int VisibleStandingSnagCount() const noexcept
        {
            return visibleStandingSnagCount_;
        }
        [[nodiscard]] int CanopyFrustumRejectedCount() const noexcept { return canopyFrustumRejectedCount_; }
        [[nodiscard]] int VisibleCanopyNearCount() const noexcept
        {
            return visibleCanopyNearCount_;
        }
        [[nodiscard]] int VisibleCanopyFarCount() const noexcept
        {
            return visibleCanopyFarCount_;
        }
        [[nodiscard]] int VisibleUnderstoryCount() const noexcept { return visibleUnderstoryCount_; }
        [[nodiscard]] int VisibleUnderstoryNearCount() const noexcept
        {
            return visibleUnderstoryNearCount_;
        }
        [[nodiscard]] int VisibleUnderstoryFarCount() const noexcept
        {
            return visibleUnderstoryFarCount_;
        }
        [[nodiscard]] int VisibleBushCount() const noexcept { return visibleBushCount_; }
        [[nodiscard]] int VisibleBushNearCount() const noexcept { return visibleBushNearCount_; }
        [[nodiscard]] int VisibleBushFarCount() const noexcept { return visibleBushFarCount_; }
        [[nodiscard]] int VisibleGroundPlantCount() const noexcept
        {
            return visibleGroundPlantCount_;
        }
        [[nodiscard]] int VisibleGroundPlantNearCount() const noexcept
        {
            return visibleGroundPlantNearCount_;
        }
        [[nodiscard]] int VisibleGroundPlantFarCount() const noexcept
        {
            return visibleGroundPlantFarCount_;
        }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        // Opt-in QA: the previous draw's actual submitted identities/LODs; no GPU queries.
        void WriteVegetationLodDiagnostics(std::ostream& output) const;
        CanopyInspectionCamera SetLodInspection(const CanopyLodInspection& inspection);

    private:
        struct PlantMesh
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices;
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> instances;
            int vertexCount = 0;
            int primitiveCount = 0;
            int instanceCapacity = 0;
            float horizontalRadius = 0.0f;
            float minimumY = 0.0f;
            float maximumY = 0.0f;
        };

        struct DrawInstance
        {
            Microsoft::Xna::Framework::Matrix transform;
            std::uint8_t variant = 0U;
            float x = 0.0f;
            float z = 0.0f;
            float nearLodDistanceSquared = 0.0f;
        };

        struct CanopyInstance
        {
            DrawInstance draw;
            Microsoft::Xna::Framework::BoundingBox bounds;
        };

        struct JungleCell
        {
            Microsoft::Xna::Framework::BoundingBox bounds;
            std::vector<CanopyInstance> canopy;
            std::vector<DrawInstance> standingSnags;
            std::vector<DrawInstance> understory;
            std::vector<DrawInstance> bushes;
            std::vector<DrawInstance> groundPlants;
            bool occupied = false;
        };

        [[nodiscard]] static const Microsoft::Xna::Framework::Graphics::VertexDeclaration&
            InstanceDeclaration();
        void CreateMeshes();
        void CreateCells(const IslandTerrain& terrain, const JungleLayout& layout);
        void DrawInstances(PlantMesh& mesh,
                           const std::vector<Microsoft::Xna::Framework::Matrix>& transforms);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        CanopyLighting& canopyLighting_;
        RenderedTerrainHeightMap& terrainHeightMap_;
        std::optional<CanopyLodInspection> lodInspection_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::unique_ptr<LeafSurfaceMaterial> leafMaterial_;
        std::array<PlantMesh, JungleLayout::CanopyVariantCount> canopyNearMeshes_;
        std::array<PlantMesh, JungleLayout::CanopyVariantCount> canopyFarMeshes_;
        std::array<PlantMesh, JungleLayout::StandingSnagVariantCount> standingSnagNearMeshes_;
        std::array<PlantMesh, JungleLayout::StandingSnagVariantCount> standingSnagFarMeshes_;
        std::array<PlantMesh, JungleLayout::UnderstoryVariantCount> understoryNearMeshes_;
        std::array<PlantMesh, JungleLayout::UnderstoryVariantCount> understoryFarMeshes_;
        std::array<PlantMesh, JungleLayout::BushVariantCount> bushNearMeshes_;
        std::array<PlantMesh, JungleLayout::BushVariantCount> bushFarMeshes_;
        std::array<PlantMesh, JungleLayout::GroundPlantVariantCount> groundPlantNearMeshes_;
        std::array<PlantMesh, JungleLayout::GroundPlantVariantCount> groundPlantFarMeshes_;
        std::array<JungleCell, TerrainMeshLayout::CellCount> cells_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::CanopyVariantCount> visibleCanopyNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::CanopyVariantCount> visibleCanopyFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::StandingSnagVariantCount> visibleStandingSnagNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::StandingSnagVariantCount> visibleStandingSnagFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::UnderstoryVariantCount> visibleUnderstoryNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::UnderstoryVariantCount> visibleUnderstoryFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::BushVariantCount> visibleBushNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::BushVariantCount> visibleBushFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::GroundPlantVariantCount> visibleGroundPlantNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::GroundPlantVariantCount> visibleGroundPlantFar_;
        std::string fallbackReason_;
        bool instancingSupported_ = false;
        bool customShaderReady_ = false;
        float maximumDistanceScale_ = 1.0f;
        int totalCanopyCount_ = 0;
        int totalStandingSnagCount_ = 0;
        int totalUnderstoryCount_ = 0;
        int totalBushCount_ = 0;
        int totalGroundPlantCount_ = 0;
        int visibleCanopyCount_ = 0;
        int visibleStandingSnagCount_ = 0;
        int canopyFrustumRejectedCount_ = 0;
        int visibleCanopyNearCount_ = 0;
        int visibleCanopyFarCount_ = 0;
        int visibleUnderstoryCount_ = 0;
        int visibleUnderstoryNearCount_ = 0;
        int visibleUnderstoryFarCount_ = 0;
        int visibleBushCount_ = 0;
        int visibleBushNearCount_ = 0;
        int visibleBushFarCount_ = 0;
        int visibleGroundPlantCount_ = 0;
        int visibleGroundPlantNearCount_ = 0;
        int visibleGroundPlantFarCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
    };
}
