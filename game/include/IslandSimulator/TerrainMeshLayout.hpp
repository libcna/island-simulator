#pragma once

#include <cstddef>

namespace island
{
    /** @brief Renderer-independent description of one regular terrain mesh cell. */
    struct TerrainCellLayout
    {
        int column = 0;
        int row = 0;
        int firstGridColumn = 0;
        int firstGridRow = 0;
        int quadColumns = 0;
        int quadRows = 0;
        float minimumX = 0.0f;
        float maximumX = 0.0f;
        float minimumZ = 0.0f;
        float maximumZ = 0.0f;
    };

    /**
     * @brief Fixed portable mesh partition for the finite island terrain.
     *
     * Adjacent cells duplicate their edge vertices. That small cost lets each cell own compact
     * 16-bit buffers and be culled independently without a shared-buffer draw-range dependency.
     */
    class TerrainMeshLayout final
    {
    public:
        static constexpr int Resolution = 241;
        static constexpr int CellsPerAxis = 8;
        static constexpr int QuadsPerAxis = Resolution - 1;
        static constexpr int QuadsPerCell = QuadsPerAxis / CellsPerAxis;
        static constexpr int VerticesPerCellAxis = QuadsPerCell + 1;
        static constexpr float Extent = 340.0f;
        static constexpr std::size_t CellCount = CellsPerAxis * CellsPerAxis;

        static_assert(QuadsPerAxis % CellsPerAxis == 0);
        static_assert(VerticesPerCellAxis * VerticesPerCellAxis < 65536);

        /** @brief Converts an inclusive full-grid coordinate to its world coordinate. */
        [[nodiscard]] static constexpr float Coordinate(const int gridCoordinate) noexcept
        {
            return -Extent + 2.0f * Extent * static_cast<float>(gridCoordinate)
                / static_cast<float>(QuadsPerAxis);
        }

        /** @brief Returns the cell at the supplied row-major index. */
        [[nodiscard]] static constexpr TerrainCellLayout Cell(const std::size_t index) noexcept
        {
            const int column = static_cast<int>(index % CellsPerAxis);
            const int row = static_cast<int>(index / CellsPerAxis);
            const int firstColumn = column * QuadsPerCell;
            const int firstRow = row * QuadsPerCell;
            return TerrainCellLayout{
                column,
                row,
                firstColumn,
                firstRow,
                QuadsPerCell,
                QuadsPerCell,
                Coordinate(firstColumn),
                Coordinate(firstColumn + QuadsPerCell),
                Coordinate(firstRow),
                Coordinate(firstRow + QuadsPerCell),
            };
        }
    };
}
