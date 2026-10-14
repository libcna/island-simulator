#pragma once

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace island
{
    struct CanopyLightTint { float red, green, blue; };
    inline constexpr CanopyLightTint CanopyAmbientTint{0.46f, 0.53f, 0.62f};
    [[nodiscard]] CanopyLightTint CanopyTransmissionTint(float transmission) noexcept;
    /** @brief Porous upright ellipsoid approximating one foliage lobe, not a solid mesh shadow. */
    struct CanopyOccluder
    {
        float x, y, z;
        float radiusX, radiusY, radiusZ;
        float yaw;
        float density = 0.32f;
    };

    [[nodiscard]] std::vector<CanopyOccluder> MakeCanopyOccluders(
        std::span<const CanopyPlacement> trees);
    /** @brief One porous crown envelope per living palm; fallen/broken stems cast no crown. */
    [[nodiscard]] std::vector<CanopyOccluder> MakePalmCanopyOccluders(
        std::span<const PalmPlacement> palms);
    /** @brief Combined fixed-sun crown inventory used by the game world. */
    [[nodiscard]] std::vector<CanopyOccluder> MakeCanopyOccluders(
        std::span<const CanopyPlacement> trees, std::span<const PalmPlacement> palms);

    /** @brief Length in metres of the forward unit ray inside the porous ellipsoid. */
    [[nodiscard]] float CanopyRayLength(const CanopyOccluder& caster,
                                       float x, float y, float z, TerrainNormal toSun);

    /**
     * @brief CPU-only fixed-daylight transmission over the island ground plane.
     *
     * Texels store direct-light transmission: 255 is unoccluded, 0 is opaque. Ambient light
     * must be added by the consumer. Row increases with world Z; texel centres use half-texel
     * offsets. No GPU resources, changing weather, dynamic shadows, or per-frame updates.
     */
    class CanopyLightField final
    {
    public:
        static constexpr int DefaultResolution = 1024;
        static constexpr float HalfExtent = 300.0f;

        CanopyLightField(const IslandTerrain& terrain, std::span<const CanopyOccluder> casters,
                        int resolution = DefaultResolution,
                        TerrainNormal toSun = {environment::SunDirectionX,
                                               environment::SunDirectionY,
                                               environment::SunDirectionZ});
        [[nodiscard]] int Resolution() const noexcept { return resolution_; }
        [[nodiscard]] std::span<const std::uint8_t> Texels() const noexcept { return texels_; }
        [[nodiscard]] float Sample(float x, float z) const noexcept;
        /** @brief Matches the shader's ground-anchor approximation for low elevated receivers. */
        [[nodiscard]] float SampleReceiver(float x, float y, float z, float groundHeight) const noexcept;
        /** @brief Five-tap spatial average for a stock material shared by a small animal body. */
        [[nodiscard]] float SampleReceiverFootprint(float x, float y, float z, float groundHeight,
                                                   float radius) const noexcept;
        [[nodiscard]] static float TexelCoordinate(int index, int resolution) noexcept;

    private:
        int resolution_;
        TerrainNormal toSun_;
        std::vector<std::uint8_t> texels_;
    };
}
