#pragma once

#include "IslandSimulator/WorldCollision.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace island
{
    class IslandTerrain;

    enum class PalmCondition : std::uint8_t
    {
        Living,
        Fallen,
        Broken
    };

    /** @brief One deterministic palm placement shared by rendering and collision. */
    struct PalmPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        float shoreDistance = 0.0f; // Signed analytic shore distance at placement, metres.
        float damageYawRadians = 0.0f;
        std::uint8_t variant = 0U;
        PalmCondition condition = PalmCondition::Living;
    };

    /** @brief One authored vegetation clearing shared by placement, QA, and terrain material. */
    struct ClearingDefinition
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    /** @brief Renderer-independent, reproducible vegetation placement for the island. */
    class VegetationLayout final
    {
    public:
        static constexpr std::uint32_t DefaultSeed = 0x7a11d5e3U;
        static constexpr int PalmVariantCount = 4;
        static constexpr float PalmMinimumScale = 0.82f;
        static constexpr float PalmMaximumScale = 1.24f;
        // Young palms recruit around adults inside groves.
        static constexpr float PalmJuvenileMinimumScale = 0.42f;
        static constexpr float PalmJuvenileMaximumScale = 0.64f;
        static constexpr float PalmJuvenileChance = 0.32f;
        static constexpr float PalmGroveThreshold = 0.45f;
        static constexpr std::size_t FallenPalmTargetCount = 6U;
        static constexpr std::size_t BrokenPalmTargetCount = 8U;
        static constexpr float PrimaryPathBaseX = -5.0f;
        static constexpr float PrimaryPathAmplitude = 21.0f;
        static constexpr float PrimaryPathZOffset = 34.0f;
        static constexpr float PrimaryPathFrequency = 0.018f;
        static constexpr float PrimaryPathMinimumZ = -225.0f;
        static constexpr float PrimaryPathMaximumZ = 190.0f;
        static constexpr float PrimaryPathReservedHalfWidth = 1.4f;
        static constexpr float PlayerSpawnX = 4.0f;
        static constexpr float PlayerSpawnZ = -225.0f;
        static constexpr float PlayerSpawnReserveX = 0.0f;
        static constexpr float PlayerSpawnReserveZ = -225.0f;
        static constexpr float PlayerSpawnReserveRadius = 17.0f;
        static constexpr ClearingDefinition EastClearing {43.0f, -18.0f, 32.0f};
        static constexpr ClearingDefinition WestClearing {-82.0f, 62.0f, 24.0f};

        explicit VegetationLayout(const IslandTerrain& terrain,
                                  std::uint32_t seed = DefaultSeed);

        [[nodiscard]] const std::vector<PalmPlacement>& Palms() const noexcept { return palms_; }
        [[nodiscard]] const std::vector<CircularObstacle>& Obstacles() const noexcept
        {
            return obstacles_;
        }

        /** @brief Reports whether a point is reserved for a path, clearing, or player spawn. */
        [[nodiscard]] static bool IsReservedRoute(float x, float z) noexcept;

        /** @brief Conservative overlap of a horizontal plant footprint with reserved space. */
        [[nodiscard]] static bool OverlapsReservedRoute(float x, float z, float radius) noexcept;

        /** @brief Reports whether a point is inside either authored vegetation clearing. */
        [[nodiscard]] static bool IsInsideAuthoredClearing(float x, float z) noexcept;

        /** @brief Returns the authored centre line of the primary south-to-north path. */
        [[nodiscard]] static float PrimaryPathCenterX(float z) noexcept;

    private:
        [[nodiscard]] static std::uint32_t Hash(std::int32_t x, std::int32_t z,
                                                std::uint32_t seed) noexcept;
        [[nodiscard]] static float UnitRandom(std::uint32_t value) noexcept;
        [[nodiscard]] static float GroveNoise(float x, float z, std::uint32_t seed,
                                              float cellSize) noexcept;
        [[nodiscard]] static float ClusterNoise(float x, float z,
                                                std::uint32_t seed) noexcept;

        std::vector<PalmPlacement> palms_;
        std::vector<CircularObstacle> obstacles_;
    };
}
