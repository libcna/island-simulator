#pragma once

#include <memory>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class ShaderEffect;
    class Texture2D;
}

namespace island
{
    /** @brief One owned, fully mipmapped litter texture with a diagnosed procedural fallback. */
    class ForestFloorMaterial final
    {
    public:
        explicit ForestFloorMaterial(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~ForestFloorMaterial();
        ForestFloorMaterial(const ForestFloorMaterial&) = delete;
        ForestFloorMaterial& operator=(const ForestFloorMaterial&) = delete;

        // Call after CanopyLighting::Bind: the disabled path uses its complete slot-0 texture.
        void Bind(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
        static const char* Shader;

    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
        bool anisotropicFiltering_ = false;
    };
}
