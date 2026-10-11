#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island::TerrainSurfaceFrame
{
    // Scale before squaring: the cross product of pixel derivatives can be tiny
    // even though the terrain surface is well defined. Do not clamp its length.
    [[nodiscard]] inline TerrainNormal Normalize(TerrainNormal face)
    {
        if (!std::isfinite(face.x) || !std::isfinite(face.y) || !std::isfinite(face.z))
            throw std::invalid_argument("Terrain surface frame requires finite derivatives");
        const float scale = std::max({std::abs(face.x), std::abs(face.y), std::abs(face.z)});
        if (scale == 0.0f) return {0.0f, 1.0f, 0.0f};
        face.x /= scale; face.y /= scale; face.z /= scale;
        const float inverseLength = 1.0f / std::sqrt(face.x * face.x + face.y * face.y + face.z * face.z);
        const float orientation = face.y < 0.0f ? -inverseLength : inverseLength;
        return {face.x * orientation, face.y * orientation, face.z * orientation};
    }

    // Same scale-first arithmetic as the CPU reference. Shader world derivatives
    // are finite; a degenerate footprint gets a stable upward fallback.
    inline constexpr const char* Shader = R"(
vec3 terrainSurfaceNormal(vec3 faceVector) {
    float faceScale=max(abs(faceVector.x),max(abs(faceVector.y),abs(faceVector.z)));
    if (faceScale==0.0) return vec3(0.0,1.0,0.0);
    vec3 scaledFace=faceVector/faceScale;
    vec3 unitFace=scaledFace*inversesqrt(dot(scaledFace,scaledFace));
    return unitFace.y<0.0 ? -unitFace : unitFace;
}
)";
}
