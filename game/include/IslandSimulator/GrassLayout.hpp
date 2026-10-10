#pragma once

#include <cstdint>
#include <vector>

namespace island
{
    class CanopyLightField;
    class GroundCoverField;
    class IslandTerrain;

    /** @brief One deterministic grass-cluster anchor shared by tests and rendering. */
    struct GrassPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        float spreadScale = 1.0f;
        float windPhase = 0.0f;
        float colorVariation = 0.0f;
        /** Coherent 0--1 cool-to-warm material identity for the surrounding biome. */
        float biomeTint = 0.5f;
        bool pathScale = false;
    };

    /** @brief Renderer-independent deterministic layout for batched grass clusters. */
    class GrassLayout final
    {
    public:
        static constexpr std::uint32_t DefaultSeed = 0x6b4d2f91U;
        static constexpr float CandidateSpacing = 2.15f;
        static constexpr float MinimumScale = 0.72f;
        static constexpr float MaximumScale = 1.24f;
        static constexpr float MinimumSpreadScale = 0.88f;
        static constexpr float MaximumSpreadScale = 1.12f;
        // Dense cover adds at most one companion cluster per candidate site.
        static constexpr float SatelliteStart = 0.50f;
        static constexpr float SatelliteMinimumOffset = 0.85f, SatelliteMaximumOffset = 1.20f;

        /**
         * @brief Places grass from the shared ground-cover response.
         *
         * Light and cover fields must describe the same terrain; both are only read here.
         */
        GrassLayout(const IslandTerrain& terrain, const CanopyLightField& light,
                    const GroundCoverField& cover, std::uint32_t seed = DefaultSeed);

        [[nodiscard]] const std::vector<GrassPlacement>& Patches() const noexcept
        {
            return patches_;
        }

    private:
        [[nodiscard]] static std::uint32_t Hash(std::int32_t x, std::int32_t z,
                                                std::uint32_t seed) noexcept;
        [[nodiscard]] static float UnitRandom(std::uint32_t value) noexcept;
        std::vector<GrassPlacement> patches_;
    };
}
