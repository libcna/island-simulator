#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include <array>
#include <cstdint>
#include <optional>

namespace island {
struct GoatHome {
    float x,y,z;
    bool operator==(const GoatHome&) const = default;
};

// Placement foundation only: no runtime population or invisible goat collisions.
class GoatHabitat final {
public:
    static constexpr std::size_t Count=6;
    static constexpr float BodyClearance=1.1f, TreeClearance=9.0f;
    static constexpr float MinimumSeparation=8.0f, HerdRadius=40.0f;
    static constexpr int CandidateCount=55*55;
    static constexpr std::uint32_t Seed=0x6f617431U;
    [[nodiscard]] static bool Accepts(const IslandTerrain& terrain,
        const RenderedTerrainSurface& surface,const JungleLayout& jungle,
        const WorldCollisionGrid& obstacles,float x,float z);
    [[nodiscard]] static std::optional<std::array<GoatHome,Count>> Find(
        const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
        const JungleLayout& jungle,const WorldCollisionGrid& obstacles);
};
}
