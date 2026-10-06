#include "IslandSimulator/PlayerMotion.hpp"

#include "IslandSimulator/WorldCollisionGrid.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace island
{
    namespace
    {
        constexpr float WaterEntryTerrainHeight = -0.45f;
        constexpr float WaterExitTerrainHeight = -0.16f;
        constexpr float SwimEyeHeight = 0.58f;
        constexpr float MaximumStepSeconds = 0.1f;
        constexpr float MaximumHorizontalSubstep = 0.14f;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }
    }

    float PlayerMotion::MoveTowards(const float current, const float target,
                                    const float maximumDelta) noexcept
    {
        if (current < target) {
            return std::min(current + maximumDelta, target);
        }
        return std::max(current - maximumDelta, target);
    }

    PlayerMotion::GroundContact PlayerMotion::SampleGroundContact(
        const IslandTerrain& terrain, const float x, const float z) noexcept
    {
        constexpr float diagonal = 0.70710678f;
        constexpr std::array<std::array<float, 2>, 9> offsets = {{
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {-1.0f, 0.0f},
            {0.0f, 1.0f},
            {0.0f, -1.0f},
            {diagonal, diagonal},
            {-diagonal, diagonal},
            {diagonal, -diagonal},
            {-diagonal, -diagonal},
        }};

        GroundContact contact;
        contact.height = -std::numeric_limits<float>::infinity();
        for (const auto& offset : offsets) {
            const TerrainSample sample = terrain.Sample(x + offset[0] * CapsuleRadius,
                                                        z + offset[1] * CapsuleRadius);
            contact.height = std::max(contact.height, sample.height);
            contact.maximumSlopeRadians = std::max(contact.maximumSlopeRadians,
                                                    sample.slopeRadians);
        }
        return contact;
    }

    bool PlayerMotion::TryHorizontalStep(const IslandTerrain& terrain,
                                         const std::span<const CircularObstacle> obstacles,
                                         const WorldCollisionGrid* collisionGrid,
                                         const float deltaX, const float deltaZ) noexcept
    {
        const float distance = std::hypot(deltaX, deltaZ);
        if (distance <= 0.000001f) {
            return true;
        }

        const float candidateX = position_.x + deltaX;
        const float candidateZ = position_.z + deltaZ;
        if (collisionGrid) {
            if (collisionGrid->IntersectsCircle(candidateX, candidateZ, CapsuleRadius)) {
                return false;
            }
        } else {
            for (const CircularObstacle& obstacle : obstacles) {
                const float combinedRadius = CapsuleRadius + std::max(0.0f, obstacle.radius);
                const float separationX = candidateX - obstacle.x;
                const float separationZ = candidateZ - obstacle.z;
                if (separationX * separationX + separationZ * separationZ
                    < combinedRadius * combinedRadius) {
                    return false;
                }
            }
        }

        if (mode_ == LocomotionMode::Walking) {
            const GroundContact current = SampleGroundContact(terrain, position_.x, position_.z);
            const GroundContact candidate = SampleGroundContact(terrain, candidateX, candidateZ);
            const float rise = candidate.height - current.height;
            const float pathSlope = std::atan2(std::max(0.0f, rise), distance);
            const bool uphillOnSteepGround = rise > 0.002f
                && candidate.maximumSlopeRadians > MaximumWalkSlopeRadians;
            if (rise > MaximumStepHeight || pathSlope > MaximumWalkSlopeRadians
                || uphillOnSteepGround) {
                return false;
            }
        }

        position_.x = candidateX;
        position_.z = candidateZ;
        return true;
    }

    void PlayerMotion::SetPosition(const IslandTerrain& terrain, const float x, const float z) noexcept
    {
        position_.x = x;
        position_.z = z;
        velocityX_ = 0.0f;
        velocityZ_ = 0.0f;
        mode_ = terrain.Height(x, z) < WaterEntryTerrainHeight
            ? LocomotionMode::Swimming
            : LocomotionMode::Walking;
        ResolveModeAndHeight(terrain, std::numeric_limits<float>::infinity());
    }

    float PlayerMotion::OffshoreDistance(const IslandTerrain& terrain) const noexcept
    {
        return std::max(0.0f, -terrain.SignedShoreDistance(position_.x, position_.z));
    }

    float PlayerMotion::OceanCurrentStrength(const IslandTerrain& terrain) const noexcept
    {
        if (mode_ != LocomotionMode::Swimming) {
            return 0.0f;
        }
        return SmoothStep((OffshoreDistance(terrain) - SoftCurrentStart)
                          / (FullCurrentDistance - SoftCurrentStart));
    }

    void PlayerMotion::ResolveModeAndHeight(const IslandTerrain& terrain,
                                            const float maximumVerticalDelta) noexcept
    {
        const TerrainSample centre = terrain.Sample(position_.x, position_.z);
        if (mode_ == LocomotionMode::Walking && centre.height < WaterEntryTerrainHeight) {
            mode_ = LocomotionMode::Swimming;
        } else if (mode_ == LocomotionMode::Swimming && centre.height > WaterExitTerrainHeight) {
            mode_ = LocomotionMode::Walking;
        }

        if (mode_ == LocomotionMode::Walking) {
            const GroundContact contact = SampleGroundContact(terrain, position_.x, position_.z);
            groundHeight_ = contact.height;
            groundSlopeRadians_ = contact.maximumSlopeRadians;
        } else {
            groundHeight_ = centre.height;
            groundSlopeRadians_ = centre.slopeRadians;
        }

        const float target = mode_ == LocomotionMode::Walking
            ? groundHeight_ + EyeHeight
            : SwimEyeHeight;
        if (target > position_.y) {
            // Never smooth the camera through rising terrain. Downhill motion is eased to avoid
            // a saw-tooth camera path over dense height samples.
            position_.y = target;
        } else {
            position_.y = MoveTowards(position_.y, target, maximumVerticalDelta);
        }
    }

    void PlayerMotion::Update(const IslandTerrain& terrain, float inputX, float inputZ,
                              const float deltaSeconds,
                              const std::span<const CircularObstacle> obstacles) noexcept
    {
        UpdateInternal(terrain, inputX, inputZ, deltaSeconds, obstacles, nullptr);
    }

    void PlayerMotion::Update(const IslandTerrain& terrain, const float inputX,
                              const float inputZ, const float deltaSeconds,
                              const WorldCollisionGrid& obstacles) noexcept
    {
        UpdateInternal(terrain, inputX, inputZ, deltaSeconds, {}, &obstacles);
    }

    void PlayerMotion::UpdateInternal(
        const IslandTerrain& terrain, float inputX, float inputZ,
        const float deltaSeconds, const std::span<const CircularObstacle> obstacles,
        const WorldCollisionGrid* collisionGrid) noexcept
    {
        const float dt = std::clamp(deltaSeconds, 0.0f, MaximumStepSeconds);
        const float inputLength = std::hypot(inputX, inputZ);
        if (inputLength > 1.0f) {
            inputX /= inputLength;
            inputZ /= inputLength;
        }

        const float speed = mode_ == LocomotionMode::Walking ? WalkSpeed : SwimSpeed;
        float targetVelocityX = inputX * speed;
        float targetVelocityZ = inputZ * speed;

        const float radius = std::hypot(position_.x, position_.z);
        const float outwardX = radius > 0.001f ? position_.x / radius : 0.0f;
        const float outwardZ = radius > 0.001f ? position_.z / radius : 0.0f;
        const float offshore = OffshoreDistance(terrain);
        if (mode_ == LocomotionMode::Swimming && offshore > SoftCurrentStart) {
            const float strength = OceanCurrentStrength(terrain);
            const float outwardSpeed = targetVelocityX * outwardX + targetVelocityZ * outwardZ;
            if (outwardSpeed > 0.0f) {
                targetVelocityX -= outwardX * outwardSpeed * strength;
                targetVelocityZ -= outwardZ * outwardSpeed * strength;
            }
            const float inwardCurrent = 0.25f + 1.85f * strength;
            targetVelocityX -= outwardX * inwardCurrent;
            targetVelocityZ -= outwardZ * inwardCurrent;
        }

        const bool moving = std::hypot(targetVelocityX, targetVelocityZ) > 0.001f;
        const float acceleration = mode_ == LocomotionMode::Walking
            ? (moving ? 13.0f : 9.0f)
            : (moving ? 4.5f : 2.8f);
        velocityX_ = MoveTowards(velocityX_, targetVelocityX, acceleration * dt);
        velocityZ_ = MoveTowards(velocityZ_, targetVelocityZ, acceleration * dt);

        const float totalDeltaX = velocityX_ * dt;
        const float totalDeltaZ = velocityZ_ * dt;
        const float movementDistance = std::hypot(totalDeltaX, totalDeltaZ);
        const int substeps = std::max(1, static_cast<int>(
            std::ceil(movementDistance / MaximumHorizontalSubstep)));
        const float stepX = totalDeltaX / static_cast<float>(substeps);
        const float stepZ = totalDeltaZ / static_cast<float>(substeps);
        for (int step = 0; step < substeps; ++step) {
            if (TryHorizontalStep(terrain, obstacles, collisionGrid, stepX, stepZ)) {
                continue;
            }

            // Sliding along one axis keeps glancing contact from feeling like a complete stop.
            if (!TryHorizontalStep(terrain, obstacles, collisionGrid, stepX, 0.0f)) {
                velocityX_ = 0.0f;
            }
            if (!TryHorizontalStep(terrain, obstacles, collisionGrid, 0.0f, stepZ)) {
                velocityZ_ = 0.0f;
            }
        }

        const float newOffshore = OffshoreDistance(terrain);
        if (newOffshore > SafetyDistance) {
            const float angle = std::atan2(position_.z, position_.x);
            const float safeRadius = terrain.ShoreRadius(angle) + SafetyDistance;
            position_.x = std::cos(angle) * safeRadius;
            position_.z = std::sin(angle) * safeRadius;
            const float outwardVelocity = velocityX_ * std::cos(angle)
                + velocityZ_ * std::sin(angle);
            if (outwardVelocity > 0.0f) {
                velocityX_ -= std::cos(angle) * outwardVelocity;
                velocityZ_ -= std::sin(angle) * outwardVelocity;
            }
        }

        ResolveModeAndHeight(terrain, 1.8f * dt);
    }
}
