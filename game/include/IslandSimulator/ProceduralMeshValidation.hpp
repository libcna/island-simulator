#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace island
{
    /**
     * @brief Rejects malformed game-generated triangle geometry before a GPU buffer upload.
     * @throws std::runtime_error When counts or any 16-bit index are inconsistent.
     */
    void ValidateProceduralTriangleMeshOrThrow(
        std::size_t vertexCount, std::span<const std::uint16_t> indices,
        std::string_view label);
}
