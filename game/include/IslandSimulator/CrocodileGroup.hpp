#pragma once

#include "IslandSimulator/WildlifeSimulation.hpp"
#include "IslandSimulator/WorldCollision.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace island {

class IslandTerrain;

enum class CrocodileBehavior : std::uint8_t {Bask,Crawl,Swim,Retreat};

struct CrocodileState {
    float x=0,y=0,z=0;
    float headingRadians=0;
    float speed=0;
    float gaitPhase=0;
    float bodyWave=0;
    float jawOpen=0;
    float submerged=0;
    float scale=1;
    CrocodileBehavior behavior=CrocodileBehavior::Bask;
    WildlifeSimulationTier simulationTier=WildlifeSimulationTier::Dormant;
    std::uint8_t colorVariant=0;
};

struct CrocodileHome {
    float landX=0,landZ=0;
    float waterX=0,waterZ=0;
    float radius=0;
};

/** Three peaceful saltwater crocodiles local to the stream estuary. */
class CrocodileGroup final {
public:
    static constexpr std::size_t CrocodileCount=3;
    static constexpr float FullSimulationDistance=150;
    static constexpr float ReducedSimulationDistance=360;

    explicit CrocodileGroup(const IslandTerrain& terrain);
    void SetObstacles(std::span<const CircularObstacle> obstacles);
    void Update(float deltaSeconds,float observerX,float observerY,float observerZ) noexcept;

    [[nodiscard]] std::span<const CrocodileState> Crocodiles()const noexcept{return states_;}
    [[nodiscard]] std::span<const CrocodileHome> Homes()const noexcept{return homes_;}
    [[nodiscard]] int FullSimulationCount()const noexcept{return fullCount_;}
    [[nodiscard]] int ReducedSimulationCount()const noexcept{return reducedCount_;}
    [[nodiscard]] int DormantSimulationCount()const noexcept{return dormantCount_;}
    [[nodiscard]] bool IsValidHabitat(float x,float z,const CrocodileHome& home)const noexcept;

private:
    struct Agent {
        CrocodileState state;
        CrocodileHome home;
        float behaviorSeconds=0;
        float accumulatedSeconds=0;
        float phaseOffset=0;
        std::uint32_t transitions=0;
    };
    [[nodiscard]] std::pair<float,float> FindAnchor(float lateral,float targetShore,
                                                    bool requireDry)const;
    void InitializeAgents();
    void UpdateAgent(Agent&,float deltaSeconds,float observerX,float observerZ,
                     bool observerValid) noexcept;

    const IslandTerrain& terrain_;
    std::vector<Agent> agents_;
    std::vector<CrocodileState> states_;
    std::vector<CrocodileHome> homes_;
    std::vector<CircularObstacle> obstacles_;
    float elapsedSeconds_=0;
    std::uint64_t frameIndex_=0;
    int fullCount_=0,reducedCount_=0,dormantCount_=0;
};

}
