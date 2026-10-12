#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include <array>
#include <span>
#include <vector>

namespace island {
struct CavePoint { float x=0, y=0, z=0; };
enum class CaveSurface { Interior, Floor, Exterior, Entrance, Back };
struct CaveTriangle {
    std::array<CavePoint,3> points;
    CavePoint normal;
    CaveSurface surface;
};

// One finite, project-owned rock shell, not a heightfield painted to resemble a cave.
// The same triangles serve rendering and collision. World integration must replace
// the covered terrain with GroundTarget before publishing this landmark to the player.
class CaveLayout final {
public:
    static constexpr float Length=20.0f;
    static constexpr float OuterLength=22.0f;
    static constexpr int DepthSegments=20;
    static constexpr int ArchSegments=16;
    explicit CaveLayout(const IslandTerrain& terrain);
    [[nodiscard]] CavePoint Entrance() const noexcept { return entrance_; }
    [[nodiscard]] CavePoint World(float across,float height,float depth) const noexcept;
    [[nodiscard]] CavePoint Local(CavePoint point) const noexcept;
    [[nodiscard]] std::span<const CaveTriangle> Triangles() const noexcept { return triangles_; }
    [[nodiscard]] const std::array<CavePoint,2>& Bounds() const noexcept { return bounds_; }
    [[nodiscard]] static float Floor(float depth) noexcept;
    // Local smooth terrain excavation/approach target. It must be meshed at sufficient
    // resolution and sampled from those exact triangles for walking support.
    [[nodiscard]] float GroundTarget(float x,float z,float originalHeight) const noexcept;
    [[nodiscard]] bool ReservesHabitat(float x,float z,float margin=0) const noexcept;
    // Capsule base is the feet; height includes both spherical caps. Invalid input fails closed.
    [[nodiscard]] bool IntersectsCapsule(CavePoint feet,float radius,float height) const noexcept;
    // Largest safe prefix of a straight move. Includes initial solid occupancy and limits
    // steps to 10 cm, so a player cannot tunnel through the finite-thickness roof/walls.
    [[nodiscard]] float SafeTravelFraction(CavePoint from,CavePoint to,
                                         float radius,float height) const noexcept;
private:
    [[nodiscard]] bool InsideRock(CavePoint point) const noexcept;
    CavePoint entrance_;
    float cosine_=1,sine_=0;
    std::array<CavePoint,2> bounds_{};
    std::vector<CaveTriangle> triangles_;
};
}
