#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;
    enum class OctopusBehavior : std::uint8_t { Den, Crawl, Forage, Swim };

    struct OctopusState
    {
        float x=0.0f,y=0.0f,z=0.0f,headingRadians=0.0f,speed=0.0f;
        float mantlePulse=0.0f,armWave=0.0f,hideAmount=0.0f,scale=1.0f;
        OctopusBehavior behavior=OctopusBehavior::Den;
        WildlifeSimulationTier simulationTier=WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant=0U;
    };
    struct OctopusDen { float x=0.0f,z=0.0f,radius=0.0f; };

    /** @brief Four peaceful octopuses in bounded reef and rocky-shallow dens. */
    class OctopusGroup final
    {
    public:
        static constexpr std::size_t Count=4U;
        static constexpr float MinimumShoreDistance=-38.0f,MaximumShoreDistance=-7.0f;
        static constexpr float FullSimulationDistance=110.0f,ReducedSimulationDistance=280.0f;
        explicit OctopusGroup(const IslandTerrain& terrain);
        void Update(float deltaSeconds,float observerX,float observerY,float observerZ)noexcept;
        [[nodiscard]] std::span<const OctopusState> Octopuses()const noexcept{return states_;}
        [[nodiscard]] std::span<const OctopusDen> Dens()const noexcept{return dens_;}
        [[nodiscard]] int FullSimulationCount()const noexcept{return fullCount_;}
        [[nodiscard]] int ReducedSimulationCount()const noexcept{return reducedCount_;}
        [[nodiscard]] int DormantSimulationCount()const noexcept{return dormantCount_;}
    private:
        struct Agent { OctopusState state;OctopusDen den;float routePhase=0,pulsePhase=0,armPhase=0,behaviorSeconds=0,accumulatedSeconds=0;std::uint32_t transitionCount=0; };
        void Initialize(std::size_t index);
        void UpdateAgent(Agent&,float,float,float,bool)noexcept;
        [[nodiscard]] bool ValidHabitat(float,float,const OctopusDen&)const noexcept;
        [[nodiscard]] float DesiredHeight(const Agent&)const noexcept;
        const IslandTerrain& terrain_;std::vector<Agent> agents_;std::vector<OctopusState> states_;std::vector<OctopusDen> dens_;
        float elapsedSeconds_=0;std::uint64_t frameIndex_=0;int fullCount_=0,reducedCount_=0,dormantCount_=0;
    };
}
