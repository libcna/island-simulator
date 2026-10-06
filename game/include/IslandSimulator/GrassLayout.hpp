#pragma once

#include <cstdint>
#include <vector>

namespace island
{
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

        explicit GrassLayout(const IslandTerrain& terrain,
                             std::uint32_t seed = DefaultSeed);

        [[nodiscard]] const std::vector<GrassPlacement>& Patches() const noexcept
        {
            return patches_;
        }

    private:
        [[nodiscard]] static std::uint32_t Hash(std::int32_t x, std::int32_t z,
                                                std::uint32_t seed) noexcept;
        [[nodiscard]] static float UnitRandom(std::uint32_t value) noexcept;
        [[nodiscard]] static float ClusterNoise(float x, float z,
                                                std::uint32_t seed) noexcept;

        std::vector<GrassPlacement> patches_;
    };
}
