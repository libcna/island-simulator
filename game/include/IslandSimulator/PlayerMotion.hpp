#pragma once

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/WorldCollision.hpp"

#include <array>
#include <span>

namespace island
{
    class WorldCollisionGrid;

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

    /** @brief Unit footprint offset used by the authoritative capsule ground query. */
    struct PlayerGroundOffset
    {
        float x = 0.0f;
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
        static constexpr float GroundSampleDiagonal = 0.70710678f;
        static constexpr std::array<PlayerGroundOffset, 9> GroundContactOffsets = {{
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {-1.0f, 0.0f},
            {0.0f, 1.0f},
            {0.0f, -1.0f},
            {GroundSampleDiagonal, GroundSampleDiagonal},
            {-GroundSampleDiagonal, GroundSampleDiagonal},
            {GroundSampleDiagonal, -GroundSampleDiagonal},
            {-GroundSampleDiagonal, -GroundSampleDiagonal},
        }};

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

        /** @brief Advances movement using the static-world spatial collision index. */
        void Update(const IslandTerrain& terrain, float inputX, float inputZ,
                    float deltaSeconds, const WorldCollisionGrid& obstacles) noexcept;

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

        /** @brief Returns the normalized strength of the active outer-ocean current. */
        [[nodiscard]] float OceanCurrentStrength(const IslandTerrain& terrain) const noexcept;

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
        [[nodiscard]] bool TryHorizontalStep(const IslandTerrain& terrain,
                                             std::span<const CircularObstacle> obstacles,
                                             const WorldCollisionGrid* collisionGrid,
                                             float deltaX, float deltaZ) noexcept;
        void UpdateInternal(const IslandTerrain& terrain, float inputX, float inputZ,
                            float deltaSeconds, std::span<const CircularObstacle> obstacles,
                            const WorldCollisionGrid* collisionGrid) noexcept;
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
