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
    static constexpr float VerandaDepth=0.60f;
    static constexpr float SiteClearance=5.0f;
    static constexpr float FloorClearance=0.45f, MaximumRelief=0.45f;
    // Conservative decorative-building proxy, including the outside post corners.
    static constexpr float CollisionRadius=3.25f;
    static constexpr float MinimumInlandDistance=38.0f;
    static constexpr float MinimumStreamDistance=11.0f,MaximumStreamDistance=120.0f;
    // Authored clearing selected from the deterministic solid-obstacle survey.  Low,
    // non-colliding foliage reserves this footprint while trees and rocks remain natural.
    static constexpr float PreferredClearingX=-137.765f,PreferredClearingZ=-81.2322f;
    static constexpr float DecorativeClearingRadius=SiteClearance;
    static constexpr int CandidateCount=144*12;
    [[nodiscard]] static bool ReservesDecorativeClearing(float x,float z,float radius=0) noexcept;
    [[nodiscard]] static std::optional<HutPlacement> Find(
        const IslandTerrain& terrain, const RenderedTerrainSurface& surface,
        std::span<const CircularObstacle> occupied);
    // Shared live/report placement includes non-solid foliage exclusion discs.
    [[nodiscard]] static std::optional<HutPlacement> FindInWorld(
        const IslandTerrain& terrain, const JungleLayout& jungle,
        std::span<const CircularObstacle> solidObstacles);
};
}
