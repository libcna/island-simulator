#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace island
{
    /** @brief Portable low-cost ribbon topology following the authoritative island shoreline. */
    struct ShorelineMeshLayout final
    {
        static constexpr int SegmentCount = 384;
        static constexpr int AcrossQuadCount = 6;
        static constexpr int AcrossVertexCount = AcrossQuadCount + 1;
        static constexpr float LandwardOffset = -2.5f;
        static constexpr float OffshoreOffset = 5.5f;
        static constexpr std::size_t VertexCount =
            static_cast<std::size_t>(SegmentCount + 1) * AcrossVertexCount;
        static constexpr std::size_t TriangleCount =
            static_cast<std::size_t>(SegmentCount) * AcrossQuadCount * 2U;

        [[nodiscard]] static constexpr float AcrossFraction(const int index) noexcept
        {
            return static_cast<float>(index) / static_cast<float>(AcrossQuadCount);
        }

        [[nodiscard]] static constexpr float RadialOffset(const int index) noexcept
        {
            return LandwardOffset
                + (OffshoreOffset - LandwardOffset) * AcrossFraction(index);
        }
    };

    static_assert(ShorelineMeshLayout::VertexCount
                  <= static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U,
                  "The shoreline ribbon must remain addressable by portable 16-bit indices");
}
