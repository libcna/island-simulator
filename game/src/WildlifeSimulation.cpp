#include "IslandSimulator/WildlifeSimulation.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    WildlifeSimulationTier WildlifeActivationPolicy::Classify(
        const float distance, const float fullDistance, const float reducedDistance) noexcept
    {
        if (!std::isfinite(distance)) {
            return WildlifeSimulationTier::Dormant;
        }
        const float safeFullDistance = std::isfinite(fullDistance)
            ? std::max(0.0f, fullDistance) : 0.0f;
        const float safeReducedDistance = std::isfinite(reducedDistance)
            ? std::max(safeFullDistance, reducedDistance) : safeFullDistance;
        if (distance <= safeFullDistance) {
            return WildlifeSimulationTier::Full;
        }
        if (distance <= safeReducedDistance) {
            return WildlifeSimulationTier::Reduced;
        }
        return WildlifeSimulationTier::Dormant;
    }

    std::uint32_t WildlifeActivationPolicy::UpdateDivisor(
        const WildlifeSimulationTier tier) noexcept
    {
        switch (tier) {
        case WildlifeSimulationTier::Full:
            return 1U;
        case WildlifeSimulationTier::Reduced:
            return 4U;
        case WildlifeSimulationTier::Dormant:
            return 30U;
        }
        return 30U;
    }
}
