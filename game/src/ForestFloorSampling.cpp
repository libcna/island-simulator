#include "IslandSimulator/ForestFloorSampling.hpp"

#include <cmath>
#include <stdexcept>

namespace island::forest_floor
{
    std::uint32_t AnchorHash(const std::int32_t x, const std::int32_t z) noexcept
    {
        // Explicit unsigned wrap matches GLSL 330 / ES 300 integer arithmetic. No sine hash.
        // Add, rather than XOR, the products: XOR creates systematic collisions between
        // some (x,z) and (-x,-z) pairs before avalanche mixing.
        std::uint32_t value = static_cast<std::uint32_t>(x) * 0x9e3779b9U
                            + static_cast<std::uint32_t>(z) * 0x85ebca6bU + 0x27d4eb2fU;
        value = (value ^ (value >> 16U)) * 0x7feb352dU;
        value = (value ^ (value >> 15U)) * 0x846ca68bU;
        return value ^ (value >> 16U);
    }

    std::array<float, 2> Rotate(const float x, const float z, const std::uint32_t turns) noexcept
    {
        switch (turns & 3U) {
            case 1: return {-z, x};
            case 2: return {-x, -z};
            case 3: return {z, -x};
            default: return {x, z};
        }
    }

    Windows SampleWindows(const float worldX, const float worldZ)
    {
        if (!std::isfinite(worldX) || !std::isfinite(worldZ)
            || std::abs(worldX) > 1000.0f || std::abs(worldZ) > 1000.0f) {
            throw std::invalid_argument("Forest-floor sampling requires finite island coordinates");
        }
        const float gridX = worldX / CellMetres, gridZ = worldZ / CellMetres;
        const auto ix = static_cast<std::int32_t>(std::floor(gridX));
        const auto iz = static_cast<std::int32_t>(std::floor(gridZ));
        const auto blend = [](const float t) {
            const float smooth = t*t*t*(t*(t*6.0f-15.0f)+10.0f);
            const float a = smooth*smooth*smooth;
            const float b = (1.0f-smooth)*(1.0f-smooth)*(1.0f-smooth);
            return a / (a+b); // Sharpen the overlap without a discontinuous winning patch.
        };
        const float tx = blend(gridX - static_cast<float>(ix));
        const float tz = blend(gridZ - static_cast<float>(iz));
        Windows result{};
        for (int z = 0; z < 2; ++z) for (int x = 0; x < 2; ++x) {
            const auto ax = ix+x, az = iz+z;
            const auto hash = AnchorHash(ax, az);
            const auto rotation = (hash >> 20U) & 3U;
            const auto offset = Rotate((worldX-static_cast<float>(ax)*CellMetres)*UvPerMetre,
                                       (worldZ-static_cast<float>(az)*CellMetres)*UvPerMetre, rotation);
            result[static_cast<std::size_t>(z*2+x)] = {
                0.34f + static_cast<float>(hash & 1023U)*(0.32f/1023.0f) + offset[0],
                0.34f + static_cast<float>((hash >> 10U) & 1023U)*(0.32f/1023.0f) + offset[1],
                (x ? tx : 1.0f-tx)*(z ? tz : 1.0f-tz), ax, az, rotation};
        }
        return result;
    }
}
