#include "IslandSimulator/ForestFloorMaterial.hpp"
#include "IslandSimulator/ForestFloorPixels.hpp"
#include "IslandSimulator/ForestFloorSampling.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace island
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    namespace
    {
        constexpr const char* SourcePath = "Content/terrain/forest-floor.png";
    }

    ForestFloorMaterial::ForestFloorMaterial(GraphicsDevice& device) : device_(device)
    {
        const auto start = std::chrono::steady_clock::now();
        try {
            const auto levels = LoadOpaqueImageMips(device, SourcePath, forest_floor::RuntimeTextureSize);
            auto texture = std::make_unique<Texture2D>(device, forest_floor::RuntimeTextureSize,
                forest_floor::RuntimeTextureSize, true, SurfaceFormat::Color);
            for (std::size_t level = 0; level < levels.size(); ++level) {
                const auto& pixels = levels[level].pixels;
                texture->SetData(static_cast<int>(level), nullptr,
                    reinterpret_cast<const std::uint8_t*>(pixels.data()), 0,
                    static_cast<int>(pixels.size()*sizeof(forest_floor::Pixel)));
            }
            anisotropicFiltering_ = device_.SupportsCapability(CNA::GraphicsCapability::AnisotropicFiltering);
            texture_ = std::move(texture); // Publish only after upload and sampler capability selection.
            const std::string message = "island-simulator: forest-floor material loaded "
                + std::string(SourcePath) + "; 512x512, 10 mips, 1398100 RGBA8 bytes; interior windows";
            std::cout << message << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(message);
            const std::string filtering = anisotropicFiltering_
                ? "island-simulator: forest-floor filter anisotropic, requested maximum 4x, device-clamped"
                : "island-simulator: forest-floor filter trilinear, anisotropy unavailable";
            std::cout << filtering << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(filtering);
            std::cout << "island-simulator: forest-floor decode/reduce/upload "
                << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now()-start).count()
                << " ms\n";
        } catch (const std::exception& error) {
            const std::string message = "island-simulator: forest-floor material fallback: "
                + std::string(SourcePath) + ": " + error.what() + "; retaining procedural soil";
            std::cerr << message << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(message);
        }
    }

    ForestFloorMaterial::~ForestFloorMaterial() = default;

    void ForestFloorMaterial::Bind(ShaderEffect& effect)
    {
        if (texture_) {
            // Keep the actual world gradients and full mip chain. Negative LOD bias would
            // sharpen by undersampling; bounded anisotropy preserves grazing-angle detail.
            // CNA clamps its 4x preset to the current context's supported maximum.
            device_.getSamplerStatesProperty()[1] = anisotropicFiltering_
                ? SamplerState::AnisotropicClamp : SamplerState::LinearClamp;
            effect.SetTexture(1, *texture_);
        }
        effect.SetUniformInt("uForestFloor", texture_ ? 1 : 0);
        effect.SetUniformFloat("uForestFloorEnabled", texture_ ? 1.0f : 0.0f);
    }

    // Keep arithmetic and scale aligned with the tested CPU source-window reference.
    static_assert(forest_floor::CellMetres == 0.32f && forest_floor::UvPerMetre == 0.80f);
    const char* ForestFloorMaterial::Shader = R"(
uniform sampler2D uForestFloor;
uniform float uForestFloorEnabled;
uint floorAnchorHash(ivec2 p)
{
    uint h = uint(p.x)*0x9e3779b9u + uint(p.y)*0x85ebca6bu + 0x27d4eb2fu;
    h = (h ^ (h >> 16u))*0x7feb352du;
    h = (h ^ (h >> 15u))*0x846ca68bu;
    return h ^ (h >> 16u);
}
vec2 floorRotate(vec2 p, uint turns)
{
    if (turns == 1u) return vec2(-p.y, p.x);
    if (turns == 2u) return -p;
    if (turns == 3u) return vec2(p.y, -p.x);
    return p;
}
vec3 floorWindow(vec2 p, ivec2 anchor, vec2 dx, vec2 dy)
{
    uint h = floorAnchorHash(anchor);
    uint turns = (h >> 20u) & 3u;
    vec2 centre = vec2(0.34) + vec2(float(h & 1023u), float((h >> 10u) & 1023u))*(0.32/1023.0);
    vec2 uv = centre + floorRotate((p-vec2(anchor)*0.32)*0.80, turns);
    // Rotate analytic world gradients, never differentiate discontinuous hashed UVs.
    return textureGrad(uForestFloor, uv, floorRotate(dx*0.80, turns),
                       floorRotate(dy*0.80, turns)).rgb;
}
vec3 forestFloorColor(vec2 p, vec2 dx, vec2 dy)
{
    vec2 grid = p/0.32;
    ivec2 anchor = ivec2(floor(grid));
    vec2 t = fract(grid);
    vec2 s = t*t*t*(t*(t*6.0-15.0)+10.0);
    vec2 a = s*s*s, b = (1.0-s)*(1.0-s)*(1.0-s);
    vec2 w = a/(a+b);
    return mix(mix(floorWindow(p, anchor, dx, dy),
                   floorWindow(p, anchor+ivec2(1,0), dx, dy), w.x),
               mix(floorWindow(p, anchor+ivec2(0,1), dx, dy),
                   floorWindow(p, anchor+ivec2(1,1), dx, dy), w.x), w.y);
}
)";
}
