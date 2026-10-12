#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace island
{
    class IslandTerrain;

    struct TidePoolDefinition
    {
        float angleRadians;
        float inlandDistance;
        float radius;
        float depth;
    };

    /** @brief Five authored shallow rock basins sharing terrain and shoreline geometry. */
    class TidePoolLayout final
    {
    public:
        inline static constexpr std::array Definitions{
            TidePoolDefinition{.27f,26.0f,4.8f,.62f},
            TidePoolDefinition{.40f,34.0f,4.0f,.50f},
            TidePoolDefinition{2.43f,25.0f,4.4f,.57f},
            TidePoolDefinition{-2.42f,24.0f,4.7f,.66f},
            TidePoolDefinition{-2.53f,33.0f,3.8f,.48f},
        };
        static constexpr int RimSegments = 20;

        [[nodiscard]] static std::pair<float,float> Centre(
            const IslandTerrain& terrain, std::size_t index) noexcept;
        [[nodiscard]] static float Depression(
            const IslandTerrain& terrain, float x, float z) noexcept;
        [[nodiscard]] static float WaterRadius(std::size_t index) noexcept;
        [[nodiscard]] static float WaterLevel(
            const IslandTerrain& terrain, std::size_t index) noexcept;
    };
}
