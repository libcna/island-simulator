#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace island
{
    // Shared indices smooth adjacent faces; duplicate vertices preserve hard material/cap seams.
    // Winding defines the outward direction. Invalid, unused or degenerate vertices are rejected.
    [[nodiscard]] std::vector<std::array<float, 3>> BuildSurfaceNormals(
        std::span<const std::array<float, 3>> positions,
        std::span<const std::uint16_t> indices);
    // References point from a local interior/axis towards each vertex. Reject inward winding
    // instead of silently flipping a normal that might hide incorrect procedural topology.
    [[nodiscard]] std::vector<std::array<float, 3>> BuildOutwardSurfaceNormals(
        std::span<const std::array<float, 3>> positions,
        std::span<const std::uint16_t> indices,
        std::span<const std::array<float, 3>> outwardReferences);
}
