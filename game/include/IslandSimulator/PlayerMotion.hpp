#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/WorldCollision.hpp"

#include <span>

namespace island
{
    /** @brief Physical movement state relevant to animation, audio, and camera presentation. */
    enum class LocomotionMode
    {
        Walking,
        Swimming
    };

    /** @brief Small renderer-independent position type for player simulation tests. */
    struct PlayerPosition
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    /**
     * @brief First traversal slice: accelerated ground movement, swimming, and a soft ocean limit.
     *
     * Input is a world-space horizontal direction. The controller owns speed and acceleration so
     * keyboard and future touch input share identical movement behaviour.
     */
    class PlayerMotion final
    {
    public:
        static constexpr float EyeHeight = 1.72f;
        static constexpr float WalkSpeed = 4.3f;
        static constexpr float SwimSpeed = 2.15f;
        static constexpr float CapsuleRadius = 0.34f;
        static constexpr float MaximumStepHeight = 0.38f;
        static constexpr float MaximumWalkSlopeRadians = 0.66322512f; // 38 degrees
        static constexpr float SoftCurrentStart = 42.0f;
        static constexpr float FullCurrentDistance = 72.0f;
        static constexpr float SafetyDistance = 92.0f;

        /**
         * @brief Places the player and resolves height and locomotion state immediately.
         * @param terrain Authoritative island field.
         * @param x World x coordinate.
         * @param z World z coordinate.
         */
        void SetPosition(const IslandTerrain& terrain, float x, float z) noexcept;

        /**
         * @brief Advances movement by one bounded step.
         * @param terrain Authoritative island field.
         * @param inputX World-space input direction along x.
         * @param inputZ World-space input direction along z.
         * @param deltaSeconds Elapsed seconds; stalls are clamped internally.
         * @param obstacles Optional solid horizontal prop proxies.
         */
        void Update(const IslandTerrain& terrain, float inputX, float inputZ,
                    float deltaSeconds,
                    std::span<const CircularObstacle> obstacles = {}) noexcept;

        /** @brief Returns current eye/camera position. */
        [[nodiscard]] const PlayerPosition& Position() const noexcept { return position_; }

        /** @brief Returns current horizontal velocity along x. */
        [[nodiscard]] float VelocityX() const noexcept { return velocityX_; }

        /** @brief Returns current horizontal velocity along z. */
        [[nodiscard]] float VelocityZ() const noexcept { return velocityZ_; }

        /** @brief Returns whether the player is walking or swimming. */
        [[nodiscard]] LocomotionMode Mode() const noexcept { return mode_; }

        /** @brief Returns the resolved terrain support height under the player footprint. */
        [[nodiscard]] float GroundHeight() const noexcept { return groundHeight_; }

        /** @brief Returns the steepest sampled terrain slope under the player footprint. */
        [[nodiscard]] float GroundSlopeRadians() const noexcept { return groundSlopeRadians_; }

        /** @brief Returns positive distance offshore, or zero on land. */
        [[nodiscard]] float OffshoreDistance(const IslandTerrain& terrain) const noexcept;

    private:
        struct GroundContact
        {
            float height = 0.0f;
            float maximumSlopeRadians = 0.0f;
        };

        [[nodiscard]] static float MoveTowards(float current, float target,
                                               float maximumDelta) noexcept;
        [[nodiscard]] static GroundContact SampleGroundContact(const IslandTerrain& terrain,
                                                               float x, float z) noexcept;
        [[nodiscard]] bool TryWalkStep(const IslandTerrain& terrain,
                                       std::span<const CircularObstacle> obstacles,
                                       float deltaX, float deltaZ) noexcept;
        void ResolveModeAndHeight(const IslandTerrain& terrain,
                                  float maximumVerticalDelta) noexcept;

        PlayerPosition position_;
        float velocityX_ = 0.0f;
        float velocityZ_ = 0.0f;
        float groundHeight_ = 0.0f;
        float groundSlopeRadians_ = 0.0f;
        LocomotionMode mode_ = LocomotionMode::Walking;
    };
}
