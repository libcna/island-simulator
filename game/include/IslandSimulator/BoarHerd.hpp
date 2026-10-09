#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "IslandSimulator/WildlifeSimulation.hpp"
#include "IslandSimulator/WorldCollision.hpp"

namespace island
{
    class IslandTerrain;

    enum class BoarBehavior : std::uint8_t { Rest, Forage, Wander, Retreat };

    /** @brief Ground-relative pose; yaw zero faces positive X, matching other wildlife. */
    struct BoarState
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float homeX = 0.0f, homeZ = 0.0f;
        float headingRadians = 0.0f;
        float speed = 0.0f;
        float gaitPhase = 0.0f;
        float rootingPose = 0.0f;
        float scale = 1.0f;
        BoarBehavior behavior = BoarBehavior::Rest;
        WildlifeSimulationTier simulationTier = WildlifeSimulationTier::Dormant;
        std::uint8_t colorVariant = 0;
    };

    /** @brief Small deterministic, non-hostile inland population. No renderer dependencies. */
    class BoarHerd final
    {
    public:
        static constexpr std::size_t BoarCount = 6;
        static constexpr float HomeRadius = 27.0f;
        // Conservatively includes the adult snout and terrain-tilted body, not just its centre.
        static constexpr float CollisionRadius = 1.30f;
        static constexpr float MinimumShoreDistance = 38.0f;
        static constexpr float MaximumSlope = 0.48f;
        static constexpr float FullSimulationDistance = 95.0f;
        static constexpr float ReducedSimulationDistance = 220.0f;

        explicit BoarHerd(const IslandTerrain& terrain,
                          std::span<const CircularObstacle> obstacles = {});
        /** @brief Rebuilds valid homes transactionally; refuses impossible habitat. */
        void SetObstacles(std::span<const CircularObstacle> obstacles);
        void Update(float deltaSeconds, float observerX, float observerY,
                    float observerZ) noexcept;
        [[nodiscard]] std::span<const BoarState> Boars() const noexcept { return states_; }
        [[nodiscard]] bool IsValidGround(float x, float z) const noexcept;

    private:
        struct Agent
        {
            BoarState state;
            float remainingSeconds = 0.0f;
            float pendingSeconds = 0.0f;
            float targetHeading = 0.0f;
            std::uint32_t decisions = 0;
        };
        void Initialize(std::size_t index);
        void Step(Agent& agent, float dt, float observerX, float observerZ,
                  bool observerValid) noexcept;

        const IslandTerrain& terrain_;
        std::vector<CircularObstacle> obstacles_;
        std::array<Agent, BoarCount> agents_{};
        std::array<BoarState, BoarCount> states_{};
        std::uint64_t frame_ = 0;
    };
}
