#include "IslandSimulator/PlayerMotion.hpp"

#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/StreamBedPatch.hpp"
#include "IslandSimulator/CaveGroundPatch.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

namespace island
{
    namespace
    {
        constexpr float WaterEntryTerrainHeight = -0.45f;
        constexpr float WaterExitTerrainHeight = -0.16f;
        constexpr float MaximumStepSeconds = 0.1f;
        constexpr float MaximumHorizontalSubstep = 0.14f;
        constexpr float DepenetrationSkin = 0.001f;
        constexpr int MaximumDepenetrationPasses = 8;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        void MoveVectorTowards(float& currentX, float& currentZ,
                               const float targetX, const float targetZ,
                               const float maximumDelta) noexcept
        {
            const float deltaX = targetX - currentX;
            const float deltaZ = targetZ - currentZ;
            const float distance = std::hypot(deltaX, deltaZ);
            if (distance <= maximumDelta || distance <= 0.000001f) {
                currentX = targetX;
                currentZ = targetZ;
                return;
            }

            const float scale = maximumDelta / distance;
            currentX += deltaX * scale;
            currentZ += deltaZ * scale;
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
        const IslandTerrain& terrain, const float x, const float z) const noexcept
    {
        GroundContact contact;
        contact.height = -std::numeric_limits<float>::infinity();
        for (const PlayerGroundOffset& offset : GroundContactOffsets) {
            const float px=x+offset.x*CapsuleRadius,pz=z+offset.z*CapsuleRadius;
            auto sample=terrain.Sample(px,pz);
            if(groundPatch_&&groundPatch_->Contains(px,pz)) {
                sample.height=groundPatch_->SampleHeight(px,pz);
                const auto height=[&](float sx,float sz){return groundPatch_->Contains(sx,sz)
                    ?groundPatch_->SampleHeight(sx,sz):terrain.Height(sx,sz);};
                sample.slopeRadians=std::atan(std::hypot(height(px+.1f,pz)-height(px-.1f,pz),
                    height(px,pz+.1f)-height(px,pz-.1f))/.2f);
            }
            if(caveGround_&&caveGround_->Contains(px,pz)) {
                sample.height=caveGround_->SampleHeight(px,pz);
                const auto height=[&](float sx,float sz){return caveGround_->Contains(sx,sz)
                    ?caveGround_->SampleHeight(sx,sz):terrain.Height(sx,sz);};
                sample.slopeRadians=std::atan(std::hypot(height(px+.1f,pz)-height(px-.1f,pz),
                    height(px,pz+.1f)-height(px,pz-.1f))/.2f);
            }
            contact.height = std::max(contact.height, sample.height);
            contact.maximumSlopeRadians = std::max(contact.maximumSlopeRadians,
                                                    sample.slopeRadians);
        }
        return contact;
    }

    int PlayerMotion::ResolveObstaclePenetration(
        const std::span<const CircularObstacle> obstacles) noexcept
    {
        int correctionCount = 0;
        for (int pass = 0; pass < MaximumDepenetrationPasses; ++pass) {
            bool corrected = false;
            for (const CircularObstacle& obstacle : obstacles) {
                if (!std::isfinite(obstacle.x) || !std::isfinite(obstacle.z)
                    || !std::isfinite(obstacle.radius)) {
                    continue;
                }
                const float combinedRadius = CapsuleRadius + std::max(0.0f, obstacle.radius);
                float separationX = position_.x - obstacle.x;
                float separationZ = position_.z - obstacle.z;
                const float distanceSquared = separationX * separationX
                    + separationZ * separationZ;
                if (distanceSquared >= combinedRadius * combinedRadius) {
                    continue;
                }

                float distance = std::sqrt(std::max(0.0f, distanceSquared));
                if (distance > 0.000001f) {
                    separationX /= distance;
                    separationZ /= distance;
                } else {
                    // An exact centre overlap has no separating direction. Prefer away from the
                    // island centre so teleport recovery remains deterministic and unsurprising.
                    const float obstacleRadius = std::hypot(obstacle.x, obstacle.z);
                    if (obstacleRadius > 0.000001f) {
                        separationX = obstacle.x / obstacleRadius;
                        separationZ = obstacle.z / obstacleRadius;
                    } else {
                        separationX = 1.0f;
                        separationZ = 0.0f;
                    }
                    distance = 0.0f;
                }

                const float pushDistance = combinedRadius - distance + DepenetrationSkin;
                position_.x += separationX * pushDistance;
                position_.z += separationZ * pushDistance;
                const float inwardVelocity = velocityX_ * separationX
                    + velocityZ_ * separationZ;
                if (inwardVelocity < 0.0f) {
                    velocityX_ -= separationX * inwardVelocity;
                    velocityZ_ -= separationZ * inwardVelocity;
                }
                ++correctionCount;
                corrected = true;
            }
            if (!corrected) {
                break;
            }
        }
        return correctionCount;
    }

    PlayerMotion::HorizontalStepResult PlayerMotion::TryHorizontalStep(
        const IslandTerrain& terrain,
        const std::span<const CircularObstacle> obstacles,
        const WorldCollisionGrid* collisionGrid,
        const float deltaX, const float deltaZ) noexcept
    {
        const float distance = std::hypot(deltaX, deltaZ);
        if (distance <= 0.000001f) {
            return HorizontalStepResult::Accepted;
        }

        const float candidateX = position_.x + deltaX;
        const float candidateZ = position_.z + deltaZ;
        if(swimSurface_ && mode_==LocomotionMode::Swimming
            && swimDepth_.EyeHeight()<SwimmingEyeHeight-.001f) {
            const float minX=std::min(position_.x,candidateX)-CapsuleRadius;
            const float minZ=std::min(position_.z,candidateZ)-CapsuleRadius;
            const float maxX=std::max(position_.x,candidateX)+CapsuleRadius;
            const float maxZ=std::max(position_.z,candidateZ)+CapsuleRadius;
            if(minX<-TerrainMeshLayout::Extent||minZ<-TerrainMeshLayout::Extent
                ||maxX>TerrainMeshLayout::Extent||maxZ>TerrainMeshLayout::Extent)
                return HorizontalStepResult::TerrainBlocked;
            // All intersected triangle vertices bound the complete swept capsule
            // rectangle, not merely its endpoint. Conservative near cell edges.
            if(swimSurface_->HeightRange(minX,minZ,maxX,maxZ)[1]+SwimDepth::BottomClearance
                >swimDepth_.EyeHeight()+.00001f) return HorizontalStepResult::TerrainBlocked;
        }
        if (collisionGrid) {
            if (collisionGrid->IntersectsCircle(candidateX, candidateZ, CapsuleRadius)) {
                return HorizontalStepResult::ObstacleBlocked;
            }
        } else {
            for (const CircularObstacle& obstacle : obstacles) {
                const float combinedRadius = CapsuleRadius + std::max(0.0f, obstacle.radius);
                const float separationX = candidateX - obstacle.x;
                const float separationZ = candidateZ - obstacle.z;
                if (separationX * separationX + separationZ * separationZ
                    < combinedRadius * combinedRadius) {
                    return HorizontalStepResult::ObstacleBlocked;
                }
            }
        }

        if (mode_ == LocomotionMode::Walking) {
            const GroundContact current = SampleGroundContact(terrain, position_.x, position_.z);
            const GroundContact candidate = SampleGroundContact(terrain, candidateX, candidateZ);
            if(cave_&&cave_->SafeTravelFraction(
                {position_.x,std::max(current.height+.001f,position_.y-EyeHeight),position_.z},
                {candidateX,std::max(candidate.height+.001f,position_.y-EyeHeight),candidateZ},
                CapsuleRadius,CapsuleHeight)<1)
                return HorizontalStepResult::ObstacleBlocked;
            const float rise = candidate.height - current.height;
            const float pathSlope = std::atan2(std::max(0.0f, rise), distance);
            const bool uphillOnSteepGround = rise > 0.002f
                && candidate.maximumSlopeRadians > MaximumWalkSlopeRadians;
            if (rise > MaximumStepHeight || pathSlope > MaximumWalkSlopeRadians
                || uphillOnSteepGround) {
                return HorizontalStepResult::TerrainBlocked;
            }
        }

        position_.x = candidateX;
        position_.z = candidateZ;
        return HorizontalStepResult::Accepted;
    }

    void PlayerMotion::SetPosition(const IslandTerrain& terrain, const float x, const float z) noexcept
    {
        swimDepth_.Reset(); swimAscend_=swimDescend_=false;
        position_.x = std::isfinite(x)
            ? x : (std::isfinite(position_.x) ? position_.x : 0.0f);
        position_.z = std::isfinite(z)
            ? z : (std::isfinite(position_.z) ? position_.z : 0.0f);
        velocityX_ = 0.0f;
        velocityZ_ = 0.0f;
        movementTrace_ = PlayerMovementTrace{};
        depenetrationPending_ = true;
        mode_ = terrain.Height(position_.x, position_.z) < WaterEntryTerrainHeight
            ? LocomotionMode::Swimming
            : LocomotionMode::Walking;
        ResolveModeAndHeight(terrain, std::numeric_limits<float>::infinity());
        if(cave_&&cave_->IntersectsCapsule({position_.x,position_.y-EyeHeight,position_.z},
            CapsuleRadius,CapsuleHeight)) {
            const auto safe=cave_->World(0,0,-3);
            position_.x=safe.x;position_.z=safe.z;
            ResolveModeAndHeight(terrain,std::numeric_limits<float>::infinity());
        }
#ifndef NDEBUG
        assert(std::isfinite(position_.x) && std::isfinite(position_.y)
               && std::isfinite(position_.z));
        assert(std::isfinite(groundHeight_) && std::isfinite(groundSlopeRadians_));
#endif
    }

    void PlayerMotion::SetSwimInput(const RenderedTerrainSurface* surface,
                                    bool ascend,bool descend) noexcept
    {
        swimSurface_=surface;
        swimAscend_=ascend; swimDescend_=descend;
        if(!surface) swimDepth_.Reset();
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

    OceanCurrentDrift PlayerMotion::OceanCurrentVelocity(
        const IslandTerrain& terrain) const noexcept
    {
        const float strength = OceanCurrentStrength(terrain);
        const float radius = std::hypot(position_.x, position_.z);
        if (strength <= 0.0f || radius <= 0.001f) {
            return {};
        }

        const float outwardX = position_.x / radius;
        const float outwardZ = position_.z / radius;
        const float angle = std::atan2(position_.z, position_.x);
        const float inwardSpeed = strength * (0.25f + 1.85f * strength);
        const float circulationVariation = 0.72f + 0.28f * std::sin(angle * 3.0f + 0.65f);
        const float tangentialSpeed = strength * (0.18f + 0.42f * strength)
            * circulationVariation;
        return {
            -outwardX * inwardSpeed - outwardZ * tangentialSpeed,
            -outwardZ * inwardSpeed + outwardX * tangentialSpeed,
        };
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
            swimDepth_.Reset();
            const GroundContact contact = SampleGroundContact(terrain, position_.x, position_.z);
            groundHeight_ = contact.height;
            groundSlopeRadians_ = contact.maximumSlopeRadians;
        } else {
            groundHeight_ = centre.height;
            groundSlopeRadians_ = centre.slopeRadians;
        }

        const float target = mode_ == LocomotionMode::Walking
            ? groundHeight_ + EyeHeight
            : swimSurface_ ? swimDepth_.EyeHeight() : SwimmingEyeHeight;
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
        const float dt = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, MaximumStepSeconds) : 0.0f;
        inputX = std::isfinite(inputX) ? inputX : 0.0f;
        inputZ = std::isfinite(inputZ) ? inputZ : 0.0f;
        const float inputLength = std::hypot(inputX, inputZ);
        if (inputLength > 1.0f) {
            inputX /= inputLength;
            inputZ /= inputLength;
        }

        PlayerMovementTrace trace;
        trace.startX = position_.x;
        trace.startZ = position_.z;
        if (depenetrationPending_ && (collisionGrid != nullptr || !obstacles.empty())) {
            trace.obstacleDepenetrations = ResolveObstaclePenetration(
                collisionGrid != nullptr ? collisionGrid->Obstacles() : obstacles);
            depenetrationPending_ = false;
        }

        const float speed = mode_ == LocomotionMode::Walking
            ? (running_ ? RunSpeed : WalkSpeed)
            : SwimSpeed;
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
            const OceanCurrentDrift current = OceanCurrentVelocity(terrain);
            targetVelocityX += current.x;
            targetVelocityZ += current.z;
        }

        const bool moving = std::hypot(targetVelocityX, targetVelocityZ) > 0.001f;
        const float acceleration = mode_ == LocomotionMode::Walking
            ? (moving ? (running_ ? 16.0f : 13.0f) : 9.0f)
            : (moving ? 4.5f : 2.8f);
        MoveVectorTowards(velocityX_, velocityZ_, targetVelocityX, targetVelocityZ,
                          acceleration * dt);

        const float totalDeltaX = velocityX_ * dt;
        const float totalDeltaZ = velocityZ_ * dt;
        const float movementDistance = std::hypot(totalDeltaX, totalDeltaZ);
        trace.attemptedX = position_.x + totalDeltaX;
        trace.attemptedZ = position_.z + totalDeltaZ;
        trace.valid = movementDistance > 0.000001f
            || trace.obstacleDepenetrations > 0;
        const auto recordRejection = [&trace](const HorizontalStepResult result) {
            if (result == HorizontalStepResult::ObstacleBlocked) {
                ++trace.obstacleRejections;
            } else if (result == HorizontalStepResult::TerrainBlocked) {
                ++trace.terrainRejections;
            }
        };
        const int substeps = std::max(1, static_cast<int>(
            std::ceil(movementDistance / MaximumHorizontalSubstep)));
        const float stepX = totalDeltaX / static_cast<float>(substeps);
        const float stepZ = totalDeltaZ / static_cast<float>(substeps);
        for (int step = 0; step < substeps; ++step) {
            const HorizontalStepResult direct = TryHorizontalStep(
                terrain, obstacles, collisionGrid, stepX, stepZ);
            if (direct == HorizontalStepResult::Accepted) {
                continue;
            }
            recordRejection(direct);
            ++trace.slideSubsteps;

            // Sliding along one axis keeps glancing contact from feeling like a complete stop.
            const HorizontalStepResult xSlide = TryHorizontalStep(
                terrain, obstacles, collisionGrid, stepX, 0.0f);
            if (xSlide != HorizontalStepResult::Accepted) {
                recordRejection(xSlide);
                velocityX_ = 0.0f;
            }
            const HorizontalStepResult zSlide = TryHorizontalStep(
                terrain, obstacles, collisionGrid, 0.0f, stepZ);
            if (zSlide != HorizontalStepResult::Accepted) {
                recordRejection(zSlide);
                velocityZ_ = 0.0f;
            }
        }

        const float newOffshore = OffshoreDistance(terrain);
        if (newOffshore > SafetyDistance) {
            trace.safetyClamped = true;
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

        trace.resolvedX = position_.x;
        trace.resolvedZ = position_.z;
        if (trace.valid || trace.safetyClamped) {
            movementTrace_ = trace;
        }

        if(swimSurface_ && mode_==LocomotionMode::Swimming) {
            constexpr float bound=TerrainMeshLayout::Extent-CapsuleRadius;
            if(std::abs(position_.x)<=bound && std::abs(position_.z)<=bound) {
                const float support=swimSurface_->HeightRange(position_.x-CapsuleRadius,
                    position_.z-CapsuleRadius,position_.x+CapsuleRadius,position_.z+CapsuleRadius)[1];
                swimDepth_.Update(support,swimAscend_,swimDescend_,dt);
            } else swimDepth_.Reset();
        }
        ResolveModeAndHeight(terrain, 1.8f * dt);
#ifndef NDEBUG
        assert(std::isfinite(position_.x) && std::isfinite(position_.y)
               && std::isfinite(position_.z));
        assert(std::isfinite(velocityX_) && std::isfinite(velocityZ_));
        assert(std::isfinite(groundHeight_) && std::isfinite(groundSlopeRadians_));
        assert(groundSlopeRadians_ >= 0.0f
               && groundSlopeRadians_ <= std::numbers::pi_v<float> * 0.5f + 0.001f);
#endif
    }
}
