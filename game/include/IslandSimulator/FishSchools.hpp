#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"

namespace island
{
    class IslandTerrain;

    /** @brief Renderer-independent pose for one shallow-water fish. */
    struct FishState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float bodyWave = 0.0f;
        float scale = 1.0f;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t schoolIndex = 0U;
        std::uint8_t colorVariant = 0U;
    };

    /** @brief Deterministic near-shore fish schools with bounded water constraints. */
    class FishSchools final
    {
    public:
        static constexpr std::size_t SchoolCount = 8U;
        static constexpr std::size_t FishPerSchool = 6U;
        static constexpr std::size_t FishCount = SchoolCount * FishPerSchool;
        static constexpr float FullSimulationDistance = 80.0f;
        static constexpr float ReducedSimulationDistance = 225.0f;

        explicit FishSchools(const IslandTerrain& terrain);

        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;

        [[nodiscard]] std::span<const FishState> Fish() const noexcept;
        [[nodiscard]] int FullSimulationCount() const noexcept { return fullSimulationCount_; }
        [[nodiscard]] int ReducedSimulationCount() const noexcept {
            return reducedSimulationCount_;
        }
        [[nodiscard]] int DormantSimulationCount() const noexcept {
            return dormantSimulationCount_;
        }

    private:
        struct Agent
        {
            FishState state;
            float alongOffset = 0.0f;
            float radialOffset = 0.0f;
            float wanderPhase = 0.0f;
            float wanderFrequency = 0.0f;
            float depthPhase = 0.0f;
            float bodyPhase = 0.0f;
            float bodySpeed = 0.0f;
        };

        struct School
        {
            float routeAngle = 0.0f;
            float offshoreDistance = 0.0f;
            float routeSpeed = 0.0f;
            float centreX = 0.0f;
            float centreY = 0.0f;
            float centreZ = 0.0f;
            float accumulatedSeconds = 0.0f;
            int direction = 1;
            WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        };

        void InitializeSchool(std::size_t schoolIndex);
        void InitializeAgent(std::size_t fishIndex);
        void UpdateSchool(std::size_t schoolIndex, float deltaSeconds) noexcept;
        void ResolveSchoolCentre(School& school) const noexcept;

        const IslandTerrain& terrain_;
        std::vector<School> schools_;
        std::vector<Agent> agents_;
        std::vector<FishState> states_;
        float elapsedSeconds_ = 0.0f;
        std::uint64_t frameIndex_ = 0U;
        int fullSimulationCount_ = 0;
        int reducedSimulationCount_ = 0;
        int dormantSimulationCount_ = 0;
    };
}
