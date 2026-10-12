#pragma once
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include <span>
#include <vector>
namespace island {
struct ResidentPoint {float x=0,y=0,z=0;bool operator==(const ResidentPoint&) const=default;};
// A closed, deterministic tour through existing accessible parts of the island.
// Construction refuses a disconnected tour rather than teleporting or clearing trees.
class ResidentRoute final {
public:
    static constexpr float Clearance=.50f,GridSpacing=3.0f,Extent=258.0f;
    ResidentRoute(const IslandTerrain& terrain,const RenderedTerrainSurface& ground,const WorldCollisionGrid& solids);
    [[nodiscard]] std::span<const ResidentPoint> Points() const noexcept {return points_;}
    [[nodiscard]] std::span<const std::size_t> PausePoints() const noexcept {return pauses_;}
    [[nodiscard]] float Length() const noexcept {return length_;}
    [[nodiscard]] static bool SafePoint(const IslandTerrain& terrain,const RenderedTerrainSurface& ground,
        const WorldCollisionGrid& solids,float x,float z);
    [[nodiscard]] static bool SafeSegment(const IslandTerrain& terrain,const RenderedTerrainSurface& ground,
        const WorldCollisionGrid& solids,ResidentPoint a,ResidentPoint b);
private:
    std::vector<ResidentPoint> points_;
    std::vector<std::size_t> pauses_;
    float length_=0;
};
}
