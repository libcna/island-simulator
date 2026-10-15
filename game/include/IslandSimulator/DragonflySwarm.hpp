#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace island {
class IslandTerrain;

enum class DragonflyBehavior : std::uint8_t { Hover, Patrol, Dart };

struct DragonflyState {
    float x=0,y=0,z=0,headingRadians=0,speed=0,wingPhase=0,bankRadians=0,scale=1;
    DragonflyBehavior behavior=DragonflyBehavior::Hover;
    WildlifeSimulationTier simulationTier=WildlifeSimulationTier::Dormant;
    std::uint8_t colorVariant=0;
};

struct DragonflyHome { float x=0,z=0,radius=0; };

/** Nine deterministic stream/wetland dragonflies with hover, patrol and short dart states. */
class DragonflySwarm final {
public:
    static constexpr std::size_t Count=9,HabitatCount=3;
    static constexpr float FullSimulationDistance=55,ReducedSimulationDistance=130;
    explicit DragonflySwarm(const IslandTerrain&);
    void Update(float deltaSeconds,float observerX,float observerY,float observerZ) noexcept;
    [[nodiscard]] std::span<const DragonflyState> Dragonflies()const noexcept{return states_;}
    [[nodiscard]] std::span<const DragonflyHome> Homes()const noexcept{return homes_;}
    [[nodiscard]] int FullSimulationCount()const noexcept{return fullCount_;}
    [[nodiscard]] int ReducedSimulationCount()const noexcept{return reducedCount_;}
    [[nodiscard]] int DormantSimulationCount()const noexcept{return dormantCount_;}
private:
    struct Agent { DragonflyState state;DragonflyHome home;float seconds=0,altitude=0;
        float turnA=0,turnB=0,vertical=0,accumulated=0;std::uint32_t transitions=0; };
    void Initialize(std::size_t);
    void UpdateAgent(Agent&,float,float,float,bool) noexcept;
    [[nodiscard]] bool Valid(float,float,const DragonflyHome&)const noexcept;
    const IslandTerrain& terrain_;
    std::array<Agent,Count> agents_{};
    std::array<DragonflyState,Count> states_{};
    std::array<DragonflyHome,Count> homes_{};
    std::uint64_t frame_=0;int fullCount_=0,reducedCount_=0,dormantCount_=0;
};
}
