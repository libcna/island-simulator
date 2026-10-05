#pragma once

#include "IslandSimulator/WorldCollision.hpp"

#include <cstdint>
#include <vector>

namespace island
{
    class IslandTerrain;
    class JungleLayout;
    class VegetationLayout;

    /** @brief One irregular rock instance with independent axis scale. */
    struct RockPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
        std::uint8_t variant = 0U;
        bool solid = true;
    };

    /** @brief One fallen natural log placed away from authored walking routes. */
    struct FallenLogPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float length = 1.0f;
        float radius = 0.25f;
        std::uint8_t variant = 0U;
    };

    /** @brief Deterministic beach/interior rocks and fallen-wood composition. */
    class NaturalPropLayout final
    {
    public:
        static constexpr std::uint32_t DefaultSeed = 0xa7816c39U;
        static constexpr int RockVariantCount = 4;
        static constexpr int LogVariantCount = 3;
        static constexpr float RockMinimumScale = 0.48f;
        static constexpr float RockMaximumScale = 1.85f;
        static constexpr float LogMinimumLength = 3.8f;
        static constexpr float LogMaximumLength = 7.2f;

        NaturalPropLayout(const IslandTerrain& terrain,
                          const VegetationLayout& palms,
                          const JungleLayout& jungle,
                          std::uint32_t seed = DefaultSeed);

        [[nodiscard]] const std::vector<RockPlacement>& Rocks() const noexcept { return rocks_; }
        [[nodiscard]] const std::vector<FallenLogPlacement>& FallenLogs() const noexcept
        {
            return fallenLogs_;
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
                                                std::uint32_t seed) noexcept;

        std::vector<RockPlacement> rocks_;
        std::vector<FallenLogPlacement> fallenLogs_;
        std::vector<CircularObstacle> obstacles_;
    };
}
