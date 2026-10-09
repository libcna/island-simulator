#include "IslandSimulator/LeafSurfaceMaterial.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>

namespace island
{
    using namespace Microsoft::Xna::Framework::Graphics;
    LeafSurfaceMaterial::LeafSurfaceMaterial(GraphicsDevice& device):device_(device)
    {
        constexpr const char* path="Content/vegetation/leaf-lamina.png";
        try {
            const auto levels=LoadOpaqueImageMips(device,path,leaf_surface::TextureSize);
            leaf_surface::Rgb mean{};
            for (std::size_t c=0;c<mean.size();++c) mean[c]=levels.back().pixels[0][c]/255.0f;
            leaf_surface::ValidateMean(mean);
            auto texture=std::make_unique<Texture2D>(device,leaf_surface::TextureSize,
                leaf_surface::TextureSize,true,SurfaceFormat::Color);
            for (std::size_t i=0;i<levels.size();++i) {
                const auto& pixels=levels[i].pixels;
                texture->SetData(static_cast<int>(i),nullptr,
                    reinterpret_cast<const std::uint8_t*>(pixels.data()),0,static_cast<int>(pixels.size()*4));
            }
            mean_=mean;
            texture_=std::move(texture);
            const char* diagnostic="island-simulator: leaf material loaded Content/vegetation/leaf-lamina.png; 256x256, 9 mips, 349524 RGBA8 bytes";
            std::cout << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        } catch (const std::exception& error) {
            const std::string diagnostic=std::string("island-simulator: leaf material fallback: ")
                +path+": "+error.what()+"; retaining procedural veins";
            std::cerr << diagnostic << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        }
    }
    LeafSurfaceMaterial::~LeafSurfaceMaterial()=default;
    void LeafSurfaceMaterial::Bind(ShaderEffect& effect)
    {
        if (texture_) {
            device_.getSamplerStatesProperty()[2]=SamplerState::LinearClamp;
            effect.SetTexture(2,*texture_);
        }
        // The diagnosed fallback uses the complete canopy texture as a legal inactive sampler.
        effect.SetUniformInt("uLeafSurface",texture_ ? 2:0);
        effect.SetUniformFloat("uLeafSurfaceEnabled",texture_ ? 1.0f:0.0f);
        effect.SetUniformVec3("uLeafSurfaceMean",mean_[0],mean_[1],mean_[2]);
    }
    std::string LeafSurfaceMaterial::Shader()
    {
        std::ostringstream source;
        source.imbue(std::locale::classic());
        source << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10)
               << "const float leafSurfaceStrength = " << leaf_surface::Strength << ";\n"
               << "const float leafSurfaceMinimum = " << leaf_surface::MinimumResponse << ";\n"
               << "const float leafSurfaceMaximum = " << leaf_surface::MaximumResponse << ";\n";
        source << R"(
uniform sampler2D uLeafSurface;
uniform float uLeafSurfaceEnabled;
uniform vec3 uLeafSurfaceMean;
vec2 leafSurfaceUv(vec3 leaf)
{
    float side = leaf.z < 0.5 ? -1.0 : 1.0;
    return vec2(0.5 + 0.5 * leaf.y * side, 1.0 - leaf.x);
}
vec3 leafSurfaceModulation(vec2 uv, vec2 dx, vec2 dy)
{
    vec3 sampleColor = textureGrad(uLeafSurface, uv, dx, dy).rgb;
    vec3 response = clamp(sampleColor / uLeafSurfaceMean, vec3(leafSurfaceMinimum), vec3(leafSurfaceMaximum));
    return mix(vec3(1.0), response, leafSurfaceStrength);
}
)";
        return source.str();
    }
}
