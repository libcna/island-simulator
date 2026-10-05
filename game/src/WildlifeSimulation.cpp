#include "IslandSimulator/WildlifeSimulation.hpp"

#include <algorithm>

namespace island
{
    WildlifeSimulationTier WildlifeActivationPolicy::Classify(
        const float distance, const float fullDistance, const float reducedDistance) noexcept
    {
        const float safeFullDistance = std::max(0.0f, fullDistance);
        const float safeReducedDistance = std::max(safeFullDistance, reducedDistance);
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
