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
    /** @brief Sunny tropical sky with slow wind-driven clouds and a static fallback. */
    class SkyRenderer final
    {
    public:
        explicit SkyRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~SkyRenderer();

        SkyRenderer(const SkyRenderer&) = delete;
        SkyRenderer& operator=(const SkyRenderer&) = delete;
        SkyRenderer(SkyRenderer&&) = delete;
        SkyRenderer& operator=(SkyRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  float timeSeconds);

        [[nodiscard]] bool UsesCustomShader() const noexcept { return customShaderReady_; }
        [[nodiscard]] const std::string& FallbackReason() const noexcept { return fallbackReason_; }

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> fallbackEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> customEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices_;
        std::string fallbackReason_;
        int vertexCount_ = 0;
        int primitiveCount_ = 0;
        bool customShaderReady_ = false;
    };
}
