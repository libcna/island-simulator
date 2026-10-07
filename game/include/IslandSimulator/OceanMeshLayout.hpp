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

        /** @brief Returns the vertex count for a runtime-selected regular grid. */
        [[nodiscard]] static constexpr std::size_t VertexCountFor(
            const int quadsPerAxis) noexcept
        {
            return quadsPerAxis > 0
                ? (static_cast<std::size_t>(quadsPerAxis) + 1U)
                    * (static_cast<std::size_t>(quadsPerAxis) + 1U)
                : 0U;
        }

        /** @brief Returns the triangle count for a runtime-selected regular grid. */
        [[nodiscard]] static constexpr std::size_t TriangleCountFor(
            const int quadsPerAxis) noexcept
        {
            return quadsPerAxis > 0
                ? static_cast<std::size_t>(quadsPerAxis)
                    * static_cast<std::size_t>(quadsPerAxis) * 2U
                : 0U;
        }

        /** @brief Reports whether the grid fits the portable unsigned 16-bit index path. */
        [[nodiscard]] static constexpr bool IsPortable(const int quadsPerAxis) noexcept
        {
            return quadsPerAxis > 0
                && quadsPerAxis <= static_cast<int>(std::numeric_limits<std::uint8_t>::max())
                && VertexCountFor(quadsPerAxis)
                    <= static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;
        }

        /** @brief Converts a runtime grid coordinate to its world-space coordinate. */
        [[nodiscard]] static constexpr float Coordinate(
            const int gridCoordinate, const int quadsPerAxis) noexcept
        {
            return -Extent + static_cast<float>(gridCoordinate)
                * (Extent * 2.0f / static_cast<float>(quadsPerAxis));
        }
    };

    static_assert(OceanMeshLayout::VertexCount
                  <= static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U,
                  "The ocean grid must remain addressable by portable 16-bit indices");
}
