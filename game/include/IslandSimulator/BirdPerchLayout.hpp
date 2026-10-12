#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace island
{
    class IslandTerrain;
    class JungleLayout;
    class NaturalPropLayout;
    class ShipwreckLayout;

    enum class BirdPerchKind : std::uint8_t
    {
        Branch,
        Rock,
        Wreck
    };

    /** @brief One point on real static world geometry where a bird's feet can settle. */
    struct BirdPerchSite
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float headingRadians = 0.0f;
        BirdPerchKind kind = BirdPerchKind::Branch;
    };

    /** @brief Deterministic bounded perch anchors derived from existing world placements. */
    class BirdPerchLayout final
    {
    public:
        static constexpr std::size_t BranchSiteCount = 5U;
        static constexpr std::size_t RockSiteCount = 2U;
        static constexpr std::size_t WreckSiteCount = 1U;
        static constexpr std::size_t SiteCount = BranchSiteCount + RockSiteCount
            + WreckSiteCount;

        BirdPerchLayout(const IslandTerrain& terrain, const JungleLayout& jungle,
                        const NaturalPropLayout& props,
                        const ShipwreckLayout& shipwreck);

        [[nodiscard]] std::span<const BirdPerchSite> Sites() const noexcept
        {
            return sites_;
        }

    private:
        std::vector<BirdPerchSite> sites_;
    };
}
