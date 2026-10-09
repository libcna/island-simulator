#pragma once

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace island
{
    // RGB is already lit for BasicEffect; alpha remains ordinary opaque alpha.
    // The custom material uses data.z to light only its replacement palette colours.
    struct TerrainMaterialVertex
    {
        float position[3];
        std::uint8_t color[4];
        float data[3]; // Shore distance, slope radians, fixed-sun irradiance.
    };
    static_assert(std::is_trivially_copyable_v<TerrainMaterialVertex>);
    static_assert(std::is_standard_layout_v<TerrainMaterialVertex>);
    static_assert(sizeof(TerrainMaterialVertex) == 28);
    static_assert(offsetof(TerrainMaterialVertex, position) == 0);
    static_assert(offsetof(TerrainMaterialVertex, color) == 12);
    static_assert(offsetof(TerrainMaterialVertex, data) == 16);

    /** @brief Existing terrain daylight response, shared by baked RGB and material palettes. */
    [[nodiscard]] inline float TerrainDaylight(const TerrainNormal& normal)
    {
        const float lengthSquared = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
        if (!std::isfinite(lengthSquared) || std::abs(lengthSquared - 1.0f) > 0.001f) {
            throw std::invalid_argument("Terrain daylight requires a finite unit normal");
        }
        constexpr float x = environment::SunDirectionX;
        constexpr float y = environment::SunDirectionY;
        constexpr float z = environment::SunDirectionZ;
        const float reciprocalLength = 1.0f / std::sqrt(x * x + y * y + z * z);
        const float dot = normal.x * (x * reciprocalLength)
            + normal.y * (y * reciprocalLength) + normal.z * (z * reciprocalLength);
        return std::clamp(0.52f + 0.48f * dot, 0.34f, 1.0f);
    }
}
