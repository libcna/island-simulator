#include "IslandSimulator/BenchmarkTraversalRoute.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    namespace
    {
        [[nodiscard]] constexpr float CatmullRom(const float p0, const float p1,
                                                 const float p2, const float p3,
                                                 const float amount) noexcept
        {
            const float amountSquared = amount * amount;
            const float amountCubed = amountSquared * amount;
            return 0.5f * ((2.0f * p1)
                + (-p0 + p2) * amount
                + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * amountSquared
                + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * amountCubed);
        }

        [[nodiscard]] constexpr float CatmullRomDerivative(
            const float p0, const float p1, const float p2, const float p3,
            const float amount) noexcept
        {
            const float amountSquared = amount * amount;
            return 0.5f * ((-p0 + p2)
                + 2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * amount
                + 3.0f * (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * amountSquared);
        }
    }

    BenchmarkRouteSample BenchmarkTraversalRoute::Sample(
        const float normalizedProgress) const noexcept
    {
        const float progress = std::isfinite(normalizedProgress)
            ? std::clamp(normalizedProgress, 0.0f, 1.0f) : 0.0f;
        constexpr std::size_t segmentCount = RouteWaypoints.size() - 1U;
        const float scaledProgress = progress * static_cast<float>(segmentCount);
        const std::size_t segmentIndex = std::min(
            static_cast<std::size_t>(scaledProgress), segmentCount - 1U);
        const float amount = progress >= 1.0f
            ? 1.0f : scaledProgress - static_cast<float>(segmentIndex);

        const BenchmarkRouteWaypoint& p0 = RouteWaypoints[
            segmentIndex == 0U ? 0U : segmentIndex - 1U];
        const BenchmarkRouteWaypoint& p1 = RouteWaypoints[segmentIndex];
        const BenchmarkRouteWaypoint& p2 = RouteWaypoints[segmentIndex + 1U];
        const BenchmarkRouteWaypoint& p3 = RouteWaypoints[
            std::min(segmentIndex + 2U, RouteWaypoints.size() - 1U)];

        BenchmarkRouteSample sample;
        sample.x = CatmullRom(p0.x, p1.x, p2.x, p3.x, amount);
        sample.z = CatmullRom(p0.z, p1.z, p2.z, p3.z, amount);
        sample.forwardX = CatmullRomDerivative(p0.x, p1.x, p2.x, p3.x, amount);
        sample.forwardZ = CatmullRomDerivative(p0.z, p1.z, p2.z, p3.z, amount);
        const float tangentLength = std::hypot(sample.forwardX, sample.forwardZ);
        if (tangentLength > 0.0001f) {
            sample.forwardX /= tangentLength;
            sample.forwardZ /= tangentLength;
        } else {
            const float fallbackX = p2.x - p1.x;
            const float fallbackZ = p2.z - p1.z;
            const float fallbackLength = std::hypot(fallbackX, fallbackZ);
            if (fallbackLength > 0.0001f) {
                sample.forwardX = fallbackX / fallbackLength;
                sample.forwardZ = fallbackZ / fallbackLength;
            }
        }
        sample.segmentIndex = segmentIndex;
        return sample;
    }

    float BenchmarkTraversalRoute::ApproximateLength(const std::size_t subdivisions) const noexcept
    {
        if (subdivisions == 0U) return 0.0f;

        BenchmarkRouteSample previous = Sample(0.0f);
        float length = 0.0f;
        for (std::size_t index = 1U; index <= subdivisions; ++index) {
            const BenchmarkRouteSample current = Sample(
                static_cast<float>(index) / static_cast<float>(subdivisions));
            length += std::hypot(current.x - previous.x, current.z - previous.z);
            previous = current;
        }
        return length;
    }
}
