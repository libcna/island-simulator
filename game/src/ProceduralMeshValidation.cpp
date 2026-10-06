#include "IslandSimulator/ProceduralMeshValidation.hpp"

#include <limits>
#include <stdexcept>
#include <string>

namespace island
{
    namespace
    {
        [[nodiscard]] std::runtime_error MeshError(
            const std::string_view label, const std::string_view message)
        {
            return std::runtime_error(
                "procedural " + std::string(label) + " mesh " + std::string(message));
        }
    }

    void ValidateProceduralTriangleMeshOrThrow(
        const std::size_t vertexCount, const std::span<const std::uint16_t> indices,
        const std::string_view label)
    {
        constexpr std::size_t MaximumVertexCount =
            static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;
        if (vertexCount == 0U) {
            throw MeshError(label, "has no vertices");
        }
        if (vertexCount > MaximumVertexCount) {
            throw MeshError(label, "exceeds the portable 16-bit vertex limit");
        }
        if (indices.empty()) {
            throw MeshError(label, "has no triangle indices");
        }
        if (indices.size() % 3U != 0U) {
            throw MeshError(label, "index count is not a complete triangle list");
        }
        if (indices.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw MeshError(label, "index count exceeds the graphics API limit");
        }
        for (const std::uint16_t index : indices) {
            if (static_cast<std::size_t>(index) >= vertexCount) {
                throw MeshError(label, "contains an out-of-range vertex index");
            }
        }
    }
}
