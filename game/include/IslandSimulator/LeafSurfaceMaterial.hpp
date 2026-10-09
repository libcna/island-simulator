#pragma once
#include "IslandSimulator/LeafSurfaceModel.hpp"
#include <memory>
#include <string>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class ShaderEffect;
    class Texture2D;
}
namespace island
{
    class LeafSurfaceMaterial final
    {
    public:
        explicit LeafSurfaceMaterial(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~LeafSurfaceMaterial();
        LeafSurfaceMaterial(const LeafSurfaceMaterial&)=delete;
        LeafSurfaceMaterial& operator=(const LeafSurfaceMaterial&)=delete;
        // Slot 2; call after shared canopy (0) and terrain height (1) bindings.
        void Bind(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
        static std::string Shader();
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
        leaf_surface::Rgb mean_{1,1,1};
    };
}
