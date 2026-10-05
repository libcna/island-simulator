#pragma once

#include "IslandSimulator/WorldCollision.hpp"

#include <cstdint>
#include <vector>

namespace island
{
    class IslandTerrain;

    /** @brief One deterministic broadleaf canopy-tree placement. */
    struct CanopyPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief One deterministic fern or broadleaf understory placement. */
    struct UnderstoryPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief One low non-solid bush that bridges grass and medium understory height. */
    struct BushPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief Renderer-independent layout for the island's layered interior jungle. */
    class JungleLayout final
    {
    public:
        static constexpr std::uint32_t DefaultSeed = 0x43b0a7d1U;
        static constexpr int CanopyVariantCount = 4;
        static constexpr int UnderstoryVariantCount = 4;
        static constexpr int BushVariantCount = 3;
        static constexpr float CanopyMinimumScale = 0.78f;
        static constexpr float CanopyMaximumScale = 1.28f;
        static constexpr float UnderstoryMinimumScale = 0.72f;
        static constexpr float UnderstoryMaximumScale = 1.34f;
        static constexpr float BushMinimumScale = 0.68f;
        static constexpr float BushMaximumScale = 1.26f;

        explicit JungleLayout(const IslandTerrain& terrain,
                              std::uint32_t seed = DefaultSeed);

        [[nodiscard]] const std::vector<CanopyPlacement>& Canopy() const noexcept
        {
            return canopy_;
        }

        [[nodiscard]] const std::vector<UnderstoryPlacement>& Understory() const noexcept
        {
            return understory_;
        }

        [[nodiscard]] const std::vector<BushPlacement>& Bushes() const noexcept
        {
            return bushes_;
        }

        [[nodiscard]] const std::vector<CircularObstacle>& Obstacles() const noexcept
        {
            return obstacles_;
        }

    private:
        [[nodiscard]] static std::uint32_t Hash(std::int32_t x, std::int32_t z,
                                                std::uint32_t seed) noexcept;
        [[nodiscard]] static float UnitRandom(std::uint32_t value) noexcept;
        [[nodiscard]] static float ClusterNoise(float x, float z,
                                                std::uint32_t seed,
                                                float cellSize) noexcept;

        std::vector<CanopyPlacement> canopy_;
        std::vector<UnderstoryPlacement> understory_;
        std::vector<BushPlacement> bushes_;
        std::vector<CircularObstacle> obstacles_;
    };
}
