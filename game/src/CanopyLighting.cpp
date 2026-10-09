#include "IslandSimulator/CanopyLighting.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include <chrono>
#include <iostream>
#include <vector>

namespace island
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    static_assert(CanopyLightField::DefaultResolution == GeneratedTextureBudget::CanopyLightSize);
    CanopyLighting::CanopyLighting(GraphicsDevice& device, const IslandTerrain& terrain,
                                   const JungleLayout& layout) : device_(device)
    {
        const auto start = std::chrono::steady_clock::now();
        const auto casters = MakeCanopyOccluders(layout.Canopy());
        field_ = std::make_unique<CanopyLightField>(terrain, casters);
        const auto& field = *field_;
        std::vector<Color> pixels;
        pixels.reserve(field.Texels().size());
        for (const auto transmission : field.Texels()) {
            pixels.emplace_back(transmission, transmission, transmission, static_cast<std::uint8_t>(255));
        }
        texture_ = std::make_unique<Texture2D>(device, field.Resolution(), field.Resolution());
        texture_->SetData(pixels.data(), static_cast<int>(pixels.size()));
        constexpr const char* diagnostic =
            "island-simulator: shared fixed-sun canopy transmission lights terrain and grass";
        std::cout << diagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        std::cout << "island-simulator: canopy CPU receiver field " << field.Texels().size()
                  << " bytes retained\n";
        std::cout << "island-simulator: canopy light " << casters.size() << " lobes, "
                  << GeneratedTextureBudget::CanopyLightRgba8Bytes << " RGBA8 bytes, bake/upload "
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start).count() << " ms\n";
    }
    CanopyLighting::~CanopyLighting() = default;
    void CanopyLighting::Bind(ShaderEffect& effect)
    {
        device_.getSamplerStatesProperty()[0] = SamplerState::LinearClamp;
        effect.SetTexture(0, *texture_);
        effect.SetUniformInt("uCanopyLight", 0);
        effect.SetUniformVec2("uCanopyMap", CanopyLightField::HalfExtent,
                              1.0f / (2.0f * CanopyLightField::HalfExtent));
        effect.SetUniformVec3("uCanopyAmbient", CanopyAmbientTint.red,
                              CanopyAmbientTint.green, CanopyAmbientTint.blue);
    }
}
