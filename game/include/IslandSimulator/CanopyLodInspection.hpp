#pragma once

#include <cmath>
#include <array>
#include <stdexcept>

namespace island
{
    struct CanopyInspectionCamera
    {
        std::array<float, 3> position, target;
    };

    // Elevated inspection, not player traversal. Preserve the actual X/Z LOD distance from
    // the trunk; aim at measured crown bounds without hiding any neighboring geometry.
    inline CanopyInspectionCamera MakeCanopyInspectionCamera(float x, float z, float distance,
        const std::array<float, 3>& minimum, const std::array<float, 3>& maximum)
    {
        for (float value : {x, z, distance})
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite canopy camera input");
        if (distance <= 0 || distance > 475)
            throw std::invalid_argument("Invalid canopy camera distance");
        for (int i = 0; i < 3; ++i)
            if (!std::isfinite(minimum[i]) || !std::isfinite(maximum[i]) || minimum[i] > maximum[i])
                throw std::invalid_argument("Invalid canopy camera bounds");
        const float radius = std::hypot(x, z);
        const float dx = radius > 0.001f ? x / radius : 0;
        const float dz = radius > 0.001f ? z / radius : 1;
        const float y = minimum[1] * 0.25f + maximum[1] * 0.75f;
        return {{x + dx * distance, y, z + dz * distance},
                {minimum[0] * 0.5f + maximum[0] * 0.5f, y,
                 minimum[2] * 0.5f + maximum[2] * 0.5f}};
    }

    // Explicit capture-only QA. IDs match WriteVegetationLodDiagnostics, not vector ordering.
    struct CanopyLodInspection
    {
        int xMillimetres;
        int zMillimetres;
        bool near;

        [[nodiscard]] bool Matches(float x, float z) const noexcept
        {
            return std::lround(x * 1000.0f) == xMillimetres
                && std::lround(z * 1000.0f) == zMillimetres;
        }
    };
}
