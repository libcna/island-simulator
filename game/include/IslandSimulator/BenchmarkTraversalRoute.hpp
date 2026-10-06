#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace island
{
    /** @brief One horizontal control point on the deterministic performance camera rail. */
    struct BenchmarkRouteWaypoint
    {
        float x = 0.0f;
        float z = 0.0f;
    };

    /** @brief Horizontal position and unit look direction sampled from the camera rail. */
    struct BenchmarkRouteSample
    {
        float x = 0.0f;
        float z = 0.0f;
        float forwardX = 0.0f;
        float forwardZ = 1.0f;
        std::size_t segmentIndex = 0U;
    };

    /**
     * @brief Deterministic ground-level camera rail for sustained renderer profiling.
     *
     * The rail crosses shallow water, beach, the primary trail, both clearings, dense jungle,
     * and the opposite coast. It is intentionally renderer-independent so its continuity and
     * coverage can be regression-tested without a graphics device.
     */
    class BenchmarkTraversalRoute final
    {
    public:
        static constexpr std::array<BenchmarkRouteWaypoint, 11> RouteWaypoints = {{
            {0.0f, -242.0f},
            {0.0f, -218.0f},
            {-17.0f, -172.0f},
            {-24.0f, -94.0f},
            {43.0f, -18.0f},
            {16.0f, 45.0f},
            {-82.0f, 62.0f},
            {7.0f, 106.0f},
            {-11.0f, 154.0f},
            {-18.0f, 205.0f},
            {0.0f, 240.0f},
        }};

        /** @brief Samples the open Catmull-Rom rail at clamped normalized progress. */
        [[nodiscard]] BenchmarkRouteSample Sample(float normalizedProgress) const noexcept;

        /** @brief Approximates horizontal rail length with deterministic uniform subdivision. */
        [[nodiscard]] float ApproximateLength(std::size_t subdivisions = 512U) const noexcept;

        /** @brief Exposes authored control points for diagnostics and tests. */
        [[nodiscard]] constexpr std::span<const BenchmarkRouteWaypoint> Waypoints() const noexcept
        {
            return RouteWaypoints;
        }
    };
}
