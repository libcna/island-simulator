#pragma once

#include "IslandSimulator/WorldCollision.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace island
{
    class IslandTerrain;

    /** @brief The authored large wooden ship grounded in one beach shallows. */
    struct ShipwreckPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float heelRadians = 0.0f;
        float pitchRadians = 0.0f;
        float length = 0.0f;
        float beam = 0.0f;
        float hullHeight = 0.0f;
    };

    enum class ShipwreckDebrisKind : std::uint8_t
    {
        Plank,
        HullFragment,
        Spar
    };

    /** @brief One non-solid substantial fragment scattered from the wreck toward shore. */
    struct ShipwreckDebrisPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float length = 1.0f;
        float width = 0.25f;
        ShipwreckDebrisKind kind = ShipwreckDebrisKind::Plank;
    };

    /** @brief Deterministic 20 m shipwreck landmark and beach-directed debris composition. */
    class ShipwreckLayout final
    {
    public:
        static constexpr float AuthoredCoastAngle = -0.72f;
        static constexpr float AuthoredOffshoreDistance = 7.2f;
        static constexpr float ShipLength = 20.0f;
        static constexpr float ShipBeam = 5.2f;
        static constexpr float ShipHullHeight = 4.4f;
        static constexpr std::size_t DebrisCount = 12U;
        static constexpr std::size_t CollisionProxyCount = 6U;

        explicit ShipwreckLayout(const IslandTerrain& terrain) noexcept;

        [[nodiscard]] const ShipwreckPlacement& Ship() const noexcept { return ship_; }
        [[nodiscard]] const std::array<ShipwreckDebrisPlacement, DebrisCount>& Debris()
            const noexcept
        {
            return debris_;
        }
        [[nodiscard]] const std::array<CircularObstacle, CollisionProxyCount>& Obstacles()
            const noexcept
        {
            return obstacles_;
        }

    private:
        ShipwreckPlacement ship_;
        std::array<ShipwreckDebrisPlacement, DebrisCount> debris_{};
        std::array<CircularObstacle, CollisionProxyCount> obstacles_{};
    };
}
