#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "IslandSimulator/StreamRoute.hpp"

namespace island
{
    /** @brief Nearest audible point on the shared authored stream route. */
    struct StreamSoundSample
    {
        float x = 0.0f;
        float z = 0.0f;
        float distance = 0.0f;
        float presence = 0.0f;
        bool valid = false;
    };

    /** @brief Cheap listener-relative field used by the stream sound and wet audio biome. */
    class StreamSoundField final
    {
    public:
        static constexpr int SegmentCount = 32;
        static constexpr float FullPresenceDistance = 4.0f;
        static constexpr float SilentDistance = 34.0f;

        [[nodiscard]] static StreamSoundSample Sample(float listenerX,
                                                       float listenerZ) noexcept
        {
            if (!std::isfinite(listenerX) || !std::isfinite(listenerZ)) {
                return {};
            }

            float bestDistanceSquared = std::numeric_limits<float>::max();
            float bestX = StreamRoute::SourceX;
            float bestZ = StreamRoute::CentreZ(bestX);
            for (int segment = 0; segment < SegmentCount; ++segment) {
                const float along0 = StreamRoute::NominalLength
                    * static_cast<float>(segment) / static_cast<float>(SegmentCount);
                const float along1 = StreamRoute::NominalLength
                    * static_cast<float>(segment + 1) / static_cast<float>(SegmentCount);
                const float x0 = StreamRoute::SourceX - along0;
                const float x1 = StreamRoute::SourceX - along1;
                const float z0 = StreamRoute::CentreZ(x0);
                const float z1 = StreamRoute::CentreZ(x1);
                const float dx = x1 - x0;
                const float dz = z1 - z0;
                const float lengthSquared = dx * dx + dz * dz;
                const float projection = lengthSquared > 0.000001f
                    ? std::clamp(((listenerX - x0) * dx + (listenerZ - z0) * dz)
                                     / lengthSquared,
                                 0.0f, 1.0f)
                    : 0.0f;
                const float x = x0 + dx * projection;
                const float z = z0 + dz * projection;
                const float distanceX = listenerX - x;
                const float distanceZ = listenerZ - z;
                const float distanceSquared = distanceX * distanceX + distanceZ * distanceZ;
                if (distanceSquared < bestDistanceSquared) {
                    bestDistanceSquared = distanceSquared;
                    bestX = x;
                    bestZ = z;
                }
            }

            const float distance = std::sqrt(bestDistanceSquared);
            const float unit = std::clamp(
                (distance - FullPresenceDistance)
                    / (SilentDistance - FullPresenceDistance),
                0.0f, 1.0f);
            const float smooth = unit * unit * (3.0f - 2.0f * unit);
            return {bestX, bestZ, distance, 1.0f - smooth, true};
        }

        /** @brief Hysteretic arbitration for the stream/player-water shared voice. */
        [[nodiscard]] static bool ShouldUseStreamPlayback(bool swimming,
                                                          float presence,
                                                          bool currentlyActive) noexcept
        {
            if (swimming || !std::isfinite(presence)) {
                return false;
            }
            return presence > (currentlyActive ? 0.005f : 0.015f);
        }
    };
}
