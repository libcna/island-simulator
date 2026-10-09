#include "IslandSimulator/BoarPose.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace island
{
    BoarBodyProfile MakeBoarBodyProfile(float dip) noexcept
    {
        dip = std::isfinite(dip) ? std::clamp(dip, 0.0f, 0.15f) : 0.0f;
        return {{{-0.72f, 0.64f, 0.025f, 0.05f}, {-0.60f, 0.64f, 0.22f, 0.25f},
                 {-0.40f, 0.66f, 0.315f, 0.32f}, {-0.08f, 0.69f, 0.335f, 0.35f},
                 {0.25f, 0.74f, 0.31f, 0.37f}, {0.47f, 0.65f - dip * 0.15f, 0.245f, 0.29f},
                 {0.62f, 0.60f - dip * 0.45f, 0.21f, 0.245f},
                 {0.78f, 0.51f - dip * 0.8f, 0.175f, 0.17f},
                 {0.91f, 0.43f - dip, 0.128f, 0.11f},
                 {1.04f, 0.42f - dip, 0.112f, 0.093f},
                 {1.06f, 0.42f - dip, 0.005f, 0.005f}}};
    }

    float BoarBackHeight(const BoarBodyProfile& profile, float z, float x) noexcept
    {
        z = std::isfinite(z) ? std::clamp(z, profile.front().z, profile.back().z) : 0.0f;
        x = std::isfinite(x) ? x : 0.0f;
        const auto height = [x](const BoarBodyRing& ring) {
            const float side = std::clamp(x / ring.radiusX, -1.0f, 1.0f);
            return ring.centreY + ring.radiusY * std::sqrt(std::max(0.0f, 1.0f - side * side));
        };
        for (std::size_t i = 1; i < profile.size(); ++i) {
            if (z <= profile[i].z) {
                const auto& a = profile[i - 1];
                const auto& b = profile[i];
                return std::lerp(height(a), height(b), (z - a.z) / (b.z - a.z));
            }
        }
        return height(profile.back());
    }

    BoarLegPose EvaluateBoarLeg(float phase, float movement, float bob,
                                const bool front, const bool left, float groundHeight) noexcept
    {
        constexpr float Pi = std::numbers::pi_v<float>;
        movement = std::isfinite(movement) ? std::clamp(movement, 0.0f, 1.0f) : 0.0f;
        bob = std::isfinite(bob) ? std::clamp(bob, 0.0f, 0.018f) : 0.0f;
        phase = std::isfinite(phase) ? std::fmod(phase, 2.0f * Pi) : 0.0f;
        // Offset all four feet during slow walking; approach diagonal pairs at a faster pace.
        const float rearOffset = std::lerp(0.75f, 0.5f, movement);
        float cycle = phase / (2.0f * Pi) + (left ? 0.5f : 0.0f) + (front ? 0.0f : rearOffset);
        cycle -= std::floor(cycle);
        const bool stance = cycle < BoarLegPose::StanceFraction;
        float travel, lift = 0.0f;
        if (stance) {
            travel = 1.0f - 2.0f * cycle / BoarLegPose::StanceFraction;
        } else {
            const float t = (cycle - BoarLegPose::StanceFraction) / (1.0f - BoarLegPose::StanceFraction);
            travel = -std::cos(Pi * t);
            const float arch = std::sin(Pi * t);
            lift = arch * arch * 0.105f * movement;
        }
        const float x = left ? -0.21f : 0.21f;
        const float z = front ? 0.35f : -0.46f;
        const BoarJoint root{x, (front ? 0.56f : 0.59f) + bob, z};
        const bool invalidGround = !std::isfinite(groundHeight);
        if (invalidGround) groundHeight = 0.0f;
        BoarJoint hoof{x, groundHeight + BoarLegPose::HoofHeight + lift,
                      z + travel * 0.245f * movement};
        // Preserve horizontal stride and bone lengths. A near-straight limit avoids a singular
        // knee; high targets cannot fold the hoof into the hip. Callers must expose this limit.
        constexpr float maximumReach = BoarLegPose::UpperLength + BoarLegPose::LowerLength - 0.0001f;
        const float minimumY = root.y - std::sqrt(maximumReach * maximumReach
                                                 - (hoof.z - root.z) * (hoof.z - root.z));
        const float requestedY = hoof.y;
        hoof.y = std::clamp(hoof.y, minimumY, root.y - 0.10f);
        const float dy = hoof.y - root.y, dz = hoof.z - root.z;
        const float distance = std::sqrt(dy * dy + dz * dz);
        const float upper = BoarLegPose::UpperLength, lower = BoarLegPose::LowerLength;
        const float along = (upper * upper - lower * lower + distance * distance) / (2.0f * distance);
        const float bend = std::sqrt(std::max(0.0f, upper * upper - along * along));
        const float direction = front ? 1.0f : -1.0f;
        const BoarJoint knee{x, root.y + dy * along / distance - direction * dz * bend / distance,
                             root.z + dz * along / distance + direction * dy * bend / distance};
        return {root, knee, hoof, stance || movement == 0.0f,
                invalidGround || hoof.y != requestedY};
    }
}
