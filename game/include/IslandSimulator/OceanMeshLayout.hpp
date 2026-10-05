#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace island
{
    /** @brief Portable regular-grid topology used by the animated ocean surface. */
    struct OceanMeshLayout final
    {
        static constexpr float Extent = 850.0f;
        static constexpr int QuadsPerAxis = 160;
        static constexpr int VerticesPerAxis = QuadsPerAxis + 1;
        static constexpr std::size_t VertexCount =
            static_cast<std::size_t>(VerticesPerAxis) * VerticesPerAxis;
        static constexpr std::size_t TriangleCount =
            static_cast<std::size_t>(QuadsPerAxis) * QuadsPerAxis * 2U;
        static constexpr float GridSpacing = Extent * 2.0f / static_cast<float>(QuadsPerAxis);

        /** @brief Converts a grid coordinate to its world-space horizontal coordinate. */
        [[nodiscard]] static constexpr float Coordinate(const int gridCoordinate) noexcept
        {
            return -Extent + static_cast<float>(gridCoordinate) * GridSpacing;
        }
    };

    static_assert(OceanMeshLayout::VertexCount
                  <= static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U,
                  "The ocean grid must remain addressable by portable 16-bit indices");
}
