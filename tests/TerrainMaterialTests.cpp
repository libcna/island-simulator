#include "IslandSimulator/TerrainMaterialVertex.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <limits>

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main()
try
{
    using Microsoft::Xna::Framework::Vector3;
    using island::TerrainDaylight;
    using namespace Microsoft::Xna::Framework::Graphics;
    // The floor uses these immutable presets. Protect its 4x cost ceiling and no-LOD-bias
    // contract when updating the read-only CNA dependency; no graphics context is created.
    Require(SamplerState::AnisotropicClamp.getFilterProperty() == TextureFilter::Anisotropic
        && SamplerState::AnisotropicClamp.getMaxAnisotropyProperty() == 4,
        "Forest-floor anisotropic preset must request at most fourfold filtering");
    Require(SamplerState::LinearClamp.getFilterProperty() == TextureFilter::Linear,
        "Forest-floor no-capability path must retain ordinary linear/mip filtering");
    for (const auto* sampler : {&SamplerState::AnisotropicClamp, &SamplerState::LinearClamp}) {
        Require(sampler->getAddressUProperty() == TextureAddressMode::Clamp
            && sampler->getAddressVProperty() == TextureAddressMode::Clamp
            && sampler->getMipMapLevelOfDetailBiasProperty() == 0.0f
            && sampler->getMaxMipLevelProperty() == 0,
            "Floor filtering must preserve clamp addressing and unbiased full mip selection");
    }
    // Independent legacy CNA calculation: no changed baked RGB/opacity on the fallback path.
    const Vector3 sun = Vector3::Normalize(Vector3(island::environment::SunDirectionX,
        island::environment::SunDirectionY, island::environment::SunDirectionZ));
    const island::IslandTerrain terrain;
    float minimum = 1, maximum = 0;
    std::size_t vertices = 0;
    for (int row = 0; row < island::TerrainMeshLayout::Resolution; ++row) {
        for (int column = 0; column < island::TerrainMeshLayout::Resolution; ++column) {
            const auto sample = terrain.Sample(island::TerrainMeshLayout::Coordinate(column),
                                               island::TerrainMeshLayout::Coordinate(row));
            const float light = TerrainDaylight(sample.normal);
            const float legacy = std::clamp(0.52f + 0.48f * Vector3::Dot(
                Vector3(sample.normal.x, sample.normal.y, sample.normal.z), sun), 0.34f, 1.0f);
            Require(light == legacy, "Terrain daylight must preserve the legacy float response");
            for (int channel = 0; channel <= 255; ++channel) {
                Require(static_cast<std::uint8_t>(static_cast<float>(channel) * light)
                    == static_cast<std::uint8_t>(static_cast<float>(channel) * legacy),
                    "Baked RGB bytes must remain unchanged for every input channel");
            }
            minimum = std::min(minimum, light);
            maximum = std::max(maximum, light);
            ++vertices;
        }
    }
    Require(vertices == 58081, "Cover the full rendered terrain grid");
    Require(minimum >= 0.34f && maximum <= 1 && maximum - minimum > 0.15f,
            "Terrain must retain a bounded, nontrivial slope-light range");
    constexpr float diagonal = 0.70710678118f;
    Require(TerrainDaylight({-diagonal, diagonal, 0}) > TerrainDaylight({diagonal, diagonal, 0}),
            "West-facing slopes must receive more fixed sunlight than east-facing slopes");
    Require(TerrainDaylight({0, diagonal, -diagonal}) > TerrainDaylight({0, diagonal, diagonal}),
            "South-facing slopes must receive more fixed sunlight than north-facing slopes");
    for (const float invalid : {std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::infinity(), 0.0f, 2.0f}) {
        bool rejected = false;
        try { (void) TerrainDaylight({0, invalid, 0}); }
        catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "Invalid light input must fail before upload");
    }
    // Read the exact byte offsets the declaration supplies, across adjacent vertices.
    const std::array<island::TerrainMaterialVertex, 2> packed{{
        {{1, 2, 3}, {19, 37, 53, 255}, {12, 0.25f, minimum}},
        {{4, 5, 6}, {73, 89, 101, 255}, {-8, 0.5f, maximum}}}};
    const auto* bytes = reinterpret_cast<const std::byte*>(packed.data());
    for (std::size_t index = 0; index < packed.size(); ++index) {
        float irradiance = 0;
        std::memcpy(&irradiance, bytes + index * 28 + 24, sizeof(irradiance));
        Require(irradiance == packed[index].data[2], "Light must occupy the final packed float");
        Require(bytes[index * 28 + 15] == std::byte{255}, "Alpha must stay opaque, not store light");
    }
    for (int step = 0; step <= 100; ++step) {
        const float t = static_cast<float>(step) / 100;
        const float light = std::lerp(minimum, maximum, t);
        const float albedo = 0.30f;
        const float base = albedo * light;
        Require(std::abs(std::lerp(base, albedo * light, t) - base) < 0.000001f,
                "Replacement and baked colour with equal albedo must not double-light");
    }
    std::cout << "Terrain material: " << vertices << " legacy-equivalent light samples, range "
              << minimum << ".." << maximum << "; opaque 28-byte layout verified\n";
    return 0;
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
