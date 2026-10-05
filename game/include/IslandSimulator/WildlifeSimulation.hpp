#pragma once

#include <cstdint>

namespace island
{
    /** @brief Distance-scaled simulation quality shared by lightweight wildlife. */
    enum class WildlifeSimulationTier : std::uint8_t
    {
        Full,
        Reduced,
        Dormant
    };

    /** @brief Stateless activation policy used by wildlife agents. */
    class WildlifeActivationPolicy final
    {
    public:
        [[nodiscard]] static WildlifeSimulationTier Classify(
            float distance, float fullDistance, float reducedDistance) noexcept;

        [[nodiscard]] static std::uint32_t UpdateDivisor(
            WildlifeSimulationTier tier) noexcept;
    };
}
