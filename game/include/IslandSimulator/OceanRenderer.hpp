#pragma once

#include <memory>
#include <string>

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
    class IslandTerrain;

    /** @brief Cross-renderer tropical ocean surface with a stock-effect fallback. */
    class OceanRenderer final
    {
    public:
        OceanRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      const IslandTerrain& terrain);
        ~OceanRenderer();

        OceanRenderer(const OceanRenderer&) = delete;
        OceanRenderer& operator=(const OceanRenderer&) = delete;
        OceanRenderer(OceanRenderer&&) = delete;
        OceanRenderer& operator=(OceanRenderer&&) = delete;

        /** @brief Draws one animated ocean pass after the opaque island terrain. */
        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  float timeSeconds, float playerX, float playerZ,
                  float playerRippleIntensity, float playerRipplePhase,
                  float playerSplashIntensity, float playerSplashPhase);

        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] const std::string& FallbackReason() const noexcept { return fallbackReason_; }
        [[nodiscard]] int VertexCount() const noexcept { return vertexCount_; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_; }

    private:
        void CreateMesh(const IslandTerrain& terrain);
        void DrawGeometry();

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices_;
        std::string fallbackReason_;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
        bool customShaderReady_ = false;
    };
}
