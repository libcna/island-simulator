#pragma once

#include "IslandSimulator/WorldCollision.hpp"
#include <array>
#include <optional>
#include <span>

namespace island {
class IslandTerrain;
class RenderedTerrainSurface;
class JungleLayout;

struct HutFooting { float x, groundY, z; };
struct HutPlacement {
    float x, z, yawRadians, floorY;
    std::array<HutFooting,6> footings;
};

// Placement only. No invisible runtime collision is installed before the hut is rendered.
class HutLayout final {
public:
    static constexpr float Width=4.2f, Depth=3.4f, RoofOverhang=0.55f;
    static constexpr float SiteClearance=5.0f;
    static constexpr float FloorClearance=0.45f, MaximumRelief=0.45f;
    // Conservative decorative-building proxy, including the outside post corners.
    static constexpr float CollisionRadius=2.83f;
    static constexpr int CandidateCount=96*6;
    [[nodiscard]] static std::optional<HutPlacement> Find(
        const IslandTerrain& terrain, const RenderedTerrainSurface& surface,
        std::span<const CircularObstacle> occupied);
    // Shared live/report placement includes non-solid foliage exclusion discs.
    [[nodiscard]] static std::optional<HutPlacement> FindInWorld(
        const IslandTerrain& terrain, const JungleLayout& jungle,
        std::span<const CircularObstacle> solidObstacles);
};
}
