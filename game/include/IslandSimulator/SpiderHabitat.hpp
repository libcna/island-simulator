#pragma once
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include <array>
#include <optional>

namespace island {
struct SpiderGroundHome {
    float x,y,z;
    std::size_t canopyIndex;
    bool operator==(const SpiderGroundHome&) const = default;
};
// Ground-spider placement foundation. Web anchors and live animals are separate work.
class SpiderHabitat final {
public:
    static constexpr std::size_t GroundCount=1000;
    static constexpr float BodyRadius=.19f,PatrolRadius=.70f,MinimumSeparation=1.45f;
    static constexpr std::uint32_t Seed=0x73706964U;
    [[nodiscard]] static bool AcceptsPatch(const IslandTerrain& terrain,
        const RenderedTerrainSurface& surface,const JungleLayout& jungle,
        const WorldCollisionGrid& obstacles,float x,float z,std::size_t canopyIndex);
    [[nodiscard]] static std::optional<std::array<SpiderGroundHome,GroundCount>> FindGround(
        const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
        const JungleLayout& jungle,const WorldCollisionGrid& obstacles);
};
}
