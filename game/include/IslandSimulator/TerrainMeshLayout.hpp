#pragma once

#include <array>
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
     * One portable 16-bit grid shares edge vertices. Cell-ordered index ranges retain independent
     * culling bounds and allow adjacent visible cells to merge into fewer draw submissions.
     */
    class TerrainMeshLayout final
    {
    public:
        static constexpr int Resolution = 241;
        static constexpr int CellsPerAxis = 8;
        static constexpr int QuadsPerAxis = Resolution - 1;
        static constexpr int QuadsPerCell = QuadsPerAxis / CellsPerAxis;
        static constexpr float Extent = 340.0f;
        static constexpr std::size_t CellCount = CellsPerAxis * CellsPerAxis;

        struct CellRun
        {
            std::size_t firstCell = 0U;
            std::size_t cellCount = 0U;
        };

        struct CellRuns
        {
            std::array<CellRun, CellCount> values{};
            std::size_t count = 0U;
        };

        static_assert(QuadsPerAxis % CellsPerAxis == 0);
        static_assert(Resolution * Resolution < 65536);

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

        /** @brief Coalesces row-major visible cells into contiguous index-buffer runs. */
        [[nodiscard]] static constexpr CellRuns CoalesceVisibleCells(
            const std::array<bool, CellCount>& visible) noexcept
        {
            CellRuns result;
            std::size_t cell = 0U;
            while (cell < CellCount) {
                while (cell < CellCount && !visible[cell]) ++cell;
                if (cell == CellCount) break;

                const std::size_t first = cell;
                while (cell < CellCount && visible[cell]) ++cell;
                result.values[result.count++] = CellRun{first, cell - first};
            }
            return result;
        }
    };
}
