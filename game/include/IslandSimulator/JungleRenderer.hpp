#pragma once

#include "IslandSimulator/JungleLayout.hpp"
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
    /** @brief Instanced canopy, medium understory, and low bushes with portable LOD. */
    class JungleRenderer final
    {
    public:
        JungleRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       const JungleLayout& layout);
        ~JungleRenderer();

        JungleRenderer(const JungleRenderer&) = delete;
        JungleRenderer& operator=(const JungleRenderer&) = delete;
        JungleRenderer(JungleRenderer&&) = delete;
        JungleRenderer& operator=(JungleRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  float timeSeconds);

        [[nodiscard]] bool UsesInstancing() const noexcept { return instancingSupported_; }
        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] int TotalCanopyCount() const noexcept { return totalCanopyCount_; }
        [[nodiscard]] int TotalUnderstoryCount() const noexcept { return totalUnderstoryCount_; }
        [[nodiscard]] int TotalBushCount() const noexcept { return totalBushCount_; }
        [[nodiscard]] int VisibleCanopyCount() const noexcept { return visibleCanopyCount_; }
        [[nodiscard]] int VisibleUnderstoryCount() const noexcept { return visibleUnderstoryCount_; }
        [[nodiscard]] int VisibleBushCount() const noexcept { return visibleBushCount_; }
        [[nodiscard]] int VisibleCellCount() const noexcept { return visibleCellCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        struct PlantMesh
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

        struct JungleCell
        {
            Microsoft::Xna::Framework::BoundingBox bounds;
            std::vector<DrawInstance> canopy;
            std::vector<DrawInstance> understory;
            std::vector<DrawInstance> bushes;
            bool occupied = false;
        };

        [[nodiscard]] static const Microsoft::Xna::Framework::Graphics::VertexDeclaration&
            InstanceDeclaration();
        void CreateMeshes();
        void CreateCells(const JungleLayout& layout);
        void DrawInstances(PlantMesh& mesh,
                           const std::vector<Microsoft::Xna::Framework::Matrix>& transforms);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::array<PlantMesh, JungleLayout::CanopyVariantCount> canopyNearMeshes_;
        std::array<PlantMesh, JungleLayout::CanopyVariantCount> canopyFarMeshes_;
        std::array<PlantMesh, JungleLayout::UnderstoryVariantCount> understoryNearMeshes_;
        std::array<PlantMesh, JungleLayout::UnderstoryVariantCount> understoryFarMeshes_;
        std::array<PlantMesh, JungleLayout::BushVariantCount> bushNearMeshes_;
        std::array<PlantMesh, JungleLayout::BushVariantCount> bushFarMeshes_;
        std::array<JungleCell, TerrainMeshLayout::CellCount> cells_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::CanopyVariantCount> visibleCanopyNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::CanopyVariantCount> visibleCanopyFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::UnderstoryVariantCount> visibleUnderstoryNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::UnderstoryVariantCount> visibleUnderstoryFar_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::BushVariantCount> visibleBushNear_;
        std::array<std::vector<Microsoft::Xna::Framework::Matrix>,
                   JungleLayout::BushVariantCount> visibleBushFar_;
        std::string fallbackReason_;
        bool instancingSupported_ = false;
        bool customShaderReady_ = false;
        int totalCanopyCount_ = 0;
        int totalUnderstoryCount_ = 0;
        int totalBushCount_ = 0;
        int visibleCanopyCount_ = 0;
        int visibleUnderstoryCount_ = 0;
        int visibleBushCount_ = 0;
        int visibleCellCount_ = 0;
        int drawCallCount_ = 0;
    };
}
