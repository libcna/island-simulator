#pragma once

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <cmath>
#include <stdexcept>

namespace island
{
    // The local envelope is the union of measured near/far vertices, already expanded for
    // wind. Placements have positive uniform scale and yaw only; a horizontal cylinder is
    // invariant under yaw, so this AABB also covers every intermediate crown orientation.
    [[nodiscard]] inline Microsoft::Xna::Framework::BoundingBox CanopyInstanceBounds(
        float localRadius, float localMinimumY, float localMaximumY,
        const Microsoft::Xna::Framework::Vector3& anchor, float scale)
    {
        using Microsoft::Xna::Framework::Vector3;
        if (!std::isfinite(localRadius) || !std::isfinite(localMinimumY)
            || !std::isfinite(localMaximumY) || !std::isfinite(scale)
            || !std::isfinite(anchor.X) || !std::isfinite(anchor.Y) || !std::isfinite(anchor.Z)
            || localRadius < 0 || localMinimumY > localMaximumY || scale <= 0) {
            throw std::invalid_argument("Invalid canopy instance envelope");
        }
        // A small world-space cushion absorbs float rounding in the shader's matrix multiply.
        const float radius = localRadius * scale + 0.01f;
        const Vector3 minimum(anchor.X - radius, anchor.Y + localMinimumY * scale - 0.01f,
                              anchor.Z - radius);
        const Vector3 maximum(anchor.X + radius, anchor.Y + localMaximumY * scale + 0.01f,
                              anchor.Z + radius);
        for (float value : {minimum.X, minimum.Y, minimum.Z, maximum.X, maximum.Y, maximum.Z}) {
            if (!std::isfinite(value)) throw std::invalid_argument("Canopy instance bounds overflow");
        }
        return {minimum, maximum};
    }
}
