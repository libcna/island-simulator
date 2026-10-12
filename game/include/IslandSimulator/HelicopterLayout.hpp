#pragma once

#include "IslandSimulator/WorldCollision.hpp"

#include <array>
#include <optional>
#include <span>

namespace island {
class IslandTerrain;
class JungleLayout;
class RenderedTerrainSurface;

struct HelicopterPoint { float x=0.0f,y=0.0f,z=0.0f; };
struct HelicopterSkidContact { float x=0.0f,groundY=0.0f,z=0.0f; };

struct HelicopterPlacement {
    float x=0.0f,z=0.0f,yawRadians=0.0f,groundY=0.0f,floorY=0.0f;
    std::array<HelicopterSkidContact,4> skidContacts{};

    [[nodiscard]] HelicopterPoint World(float across,float height,float along) const noexcept;
    [[nodiscard]] HelicopterPoint Local(float worldX,float worldY,float worldZ) const noexcept;
    [[nodiscard]] float SupportHeight(float worldX,float worldZ,float terrainHeight) const noexcept;
    [[nodiscard]] bool IntersectsCapsule(float worldX,float worldZ,float radius) const noexcept;
    [[nodiscard]] float SafeTravelFraction(float fromX,float fromZ,float toX,float toZ,
                                           float radius) const noexcept;
    [[nodiscard]] bool InPilotFootwell(float worldX,float worldZ) const noexcept;
    [[nodiscard]] bool InRearCabin(float worldX,float worldZ) const noexcept;
};

/** Deterministic inland site and walkable two-door floor plan for the grounded helicopter. */
class HelicopterLayout final {
public:
    static constexpr float MainRotorRadius=5.65f;
    static constexpr float SiteClearance=MainRotorRadius+0.55f;
    static constexpr float BodyHalfWidth=1.08f,BodyFront=2.72f,BodyRear=-2.10f;
    static constexpr float FloorClearance=0.44f,MaximumRelief=0.55f;
    static constexpr float MinimumCentreRadius=28.0f,MaximumCentreRadius=94.0f;
    static constexpr float MinimumShoreDistance=74.0f;

    [[nodiscard]] static std::optional<HelicopterPlacement> Find(
        const IslandTerrain&,const RenderedTerrainSurface&,
        std::span<const CircularObstacle> solidObstacles,
        std::span<const CircularObstacle> lowFoliage);
    [[nodiscard]] static std::optional<HelicopterPlacement> FindInWorld(
        const IslandTerrain&,const JungleLayout&,
        std::span<const CircularObstacle> solidObstacles);
};
}
