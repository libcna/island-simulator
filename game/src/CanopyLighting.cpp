#include "IslandSimulator/CanopyLighting.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace island
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    static_assert(CanopyLightField::DefaultResolution == GeneratedTextureBudget::CanopyLightSize);
    CanopyLighting::CanopyLighting(GraphicsDevice& device, const CanopyLightField& field,
                                   const GroundCoverField& cover) : device_(device), field_(field)
    {
        const auto start = std::chrono::steady_clock::now();
        if (cover.Resolution() != field.Resolution())
            throw std::invalid_argument("Ground cover and canopy light must share one texel grid");
        std::vector<Color> pixels;
        pixels.reserve(field.Texels().size());
        const auto patches = cover.PatchTexels();
        for (std::size_t i = 0; i < field.Texels().size(); ++i) {
            const auto transmission = field.Texels()[i];
            pixels.emplace_back(transmission, patches[i], transmission, static_cast<std::uint8_t>(255));
        }
        texture_ = std::make_unique<Texture2D>(device, field.Resolution(), field.Resolution());
        texture_->SetData(pixels.data(), static_cast<int>(pixels.size()));
        constexpr const char* diagnostic =
            "island-simulator: shared fixed-sun canopy transmission lights terrain and grass";
        std::cout << diagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        std::cout << "island-simulator: canopy CPU receiver field " << field.Texels().size()
                  << " bytes retained\n";
        constexpr const char* coverDiagnostic =
            "island-simulator: ground-cover patches share the canopy-light texture green channel with grass placement";
        std::cout << coverDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(coverDiagnostic);
        std::cout << "island-simulator: canopy light and ground cover "
                  << GeneratedTextureBudget::CanopyLightRgba8Bytes << " RGBA8 bytes, upload "
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
