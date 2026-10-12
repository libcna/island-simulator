#pragma once

#include <memory>
#include <string>

namespace Microsoft::Xna::Framework
{
    struct Matrix;
    struct Vector3;
}

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

    /** @brief Thin animated foam and wash ribbon following the exact radial shoreline. */
    class ShorelineRenderer final
    {
    public:
        ShorelineRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          const IslandTerrain& terrain);
        ~ShorelineRenderer();

        ShorelineRenderer(const ShorelineRenderer&) = delete;
        ShorelineRenderer& operator=(const ShorelineRenderer&) = delete;
        ShorelineRenderer(ShorelineRenderer&&) = delete;
        ShorelineRenderer& operator=(ShorelineRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  float timeSeconds);

        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] int VertexCount() const noexcept { return vertexCount_; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }
        [[nodiscard]] int TidePoolCount() const noexcept { return tidePoolCount_; }

    private:
        void CreateMesh(const IslandTerrain& terrain);

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices_;
        std::string fallbackReason_;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
        int drawCallCount_ = 0;
        int tidePoolCount_ = 0;
        bool customShaderReady_ = false;
    };
}
