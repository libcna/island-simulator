#pragma once

#include <cstdint>
#include <string>

namespace island
{
    /** @brief Stable procedural-layout identity shared by runtime diagnostics and reports. */
    struct WorldSeedSet
    {
        std::uint32_t palms = 0U;
        std::uint32_t grass = 0U;
        std::uint32_t jungle = 0U;
        std::uint32_t naturalProps = 0U;

        [[nodiscard]] friend constexpr bool operator==(
            const WorldSeedSet&, const WorldSeedSet&) noexcept = default;
    };

    /** @brief Returns the authoritative seeds used by the four procedural layout systems. */
    [[nodiscard]] WorldSeedSet AuthoritativeWorldSeeds() noexcept;

    /** @brief Formats one seed as exactly eight uppercase hexadecimal digits for the bitmap UI. */
    [[nodiscard]] std::string WorldSeedOverlayHex(std::uint32_t seed);

    /** @brief Produces a stable, report-friendly text block for copying from the debug overlay. */
    [[nodiscard]] std::string WorldSeedClipboardText(const WorldSeedSet& seeds);
}
