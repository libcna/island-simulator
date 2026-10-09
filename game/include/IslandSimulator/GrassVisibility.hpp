#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace island
{
    /** Shared grass batching and horizontal visibility policy; independent of graphics APIs. */
    struct GrassVisibility final
    {
        static constexpr int CellsPerAxis = 16;
        static constexpr std::size_t CellCount = CellsPerAxis * CellsPerAxis;
        static constexpr float LayoutExtent = 300.0f;
        static constexpr float FadeWidth = 16.0f;

        // Layout generation guarantees finite, island-scale coordinates.
        [[nodiscard]] static std::size_t CellIndex(float x, float z) noexcept
        {
            constexpr float width = 2.0f * LayoutExtent / CellsPerAxis;
            const int column = std::clamp(static_cast<int>(std::floor((x + LayoutExtent) / width)),
                                          0, CellsPerAxis - 1);
            const int row = std::clamp(static_cast<int>(std::floor((z + LayoutExtent) / width)),
                                       0, CellsPerAxis - 1);
            return static_cast<std::size_t>(row * CellsPerAxis + column);
        }

        [[nodiscard]] static float FadeStart(float maximumDistance)
        {
            if (!std::isfinite(maximumDistance) || maximumDistance <= 0.0f) {
                throw std::invalid_argument("Grass maximum draw distance must be finite and positive");
            }
            return std::max(0.0f, maximumDistance - FadeWidth);
        }

        // CPU reference for GLSL's 1 - smoothstep(start, end, anchorDistance).
        [[nodiscard]] static float Weight(float distance, float maximumDistance)
        {
            const float start = FadeStart(maximumDistance);
            if (!std::isfinite(distance) || distance < 0.0f) {
                throw std::invalid_argument("Grass anchor distance must be finite and nonnegative");
            }
            const float t = std::clamp((distance - start) / (maximumDistance - start), 0.0f, 1.0f);
            return 1.0f - t * t * (3.0f - 2.0f * t);
        }

        // Preconditions: finite coordinates and ordered bounds, established during cell creation.
        // Distance to the enclosing rectangle is <= distance to every enclosed grass anchor.
        [[nodiscard]] static float NearestDistanceSquared(float x, float z, float minX,
                                                          float minZ, float maxX, float maxZ) noexcept
        {
            const float dx = x - std::clamp(x, minX, maxX);
            const float dz = z - std::clamp(z, minZ, maxZ);
            return dx * dx + dz * dz;
        }
    };
}
