#include "IslandSimulator/ShipwreckLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace island
{
    namespace
    {
        struct DebrisAuthoring
        {
            float tangentOffset;
            float inwardOffset;
            float yawOffset;
            float length;
            float width;
            ShipwreckDebrisKind kind;
        };

        constexpr std::array<DebrisAuthoring, ShipwreckLayout::DebrisCount> DebrisAuthoringData = {{
            {-7.6f, 4.0f, -0.18f, 3.8f, 0.42f, ShipwreckDebrisKind::HullFragment},
            {6.9f, 3.1f, 0.22f, 4.6f, 0.48f, ShipwreckDebrisKind::HullFragment},
            {-4.9f, 6.8f, 0.38f, 4.9f, 0.31f, ShipwreckDebrisKind::Plank},
            {2.8f, 7.5f, -0.31f, 3.7f, 0.28f, ShipwreckDebrisKind::Plank},
            {8.4f, 8.9f, 0.17f, 5.6f, 0.33f, ShipwreckDebrisKind::Spar},
            {-9.2f, 9.6f, -0.52f, 2.8f, 0.25f, ShipwreckDebrisKind::Plank},
            {-1.1f, 10.8f, 0.47f, 3.3f, 0.27f, ShipwreckDebrisKind::Plank},
            {5.4f, 12.0f, -0.26f, 2.5f, 0.24f, ShipwreckDebrisKind::Plank},
            {-6.2f, 13.5f, 0.12f, 4.2f, 0.30f, ShipwreckDebrisKind::Plank},
            {0.9f, 15.0f, -0.44f, 3.0f, 0.24f, ShipwreckDebrisKind::Plank},
            {7.2f, 16.6f, 0.33f, 2.3f, 0.22f, ShipwreckDebrisKind::Plank},
            {-3.7f, 18.1f, -0.10f, 2.0f, 0.21f, ShipwreckDebrisKind::Plank},
        }};
    }

    ShipwreckLayout::ShipwreckLayout(const IslandTerrain& terrain) noexcept
    {
        const float outwardX = std::cos(AuthoredCoastAngle);
        const float outwardZ = std::sin(AuthoredCoastAngle);
        const float shoreRadius = terrain.ShoreRadius(AuthoredCoastAngle);
        const float radius = shoreRadius + AuthoredOffshoreDistance;

        ship_.x = outwardX * radius;
        ship_.y = 0.02f;
        ship_.z = outwardZ * radius;
        ship_.yawRadians = AuthoredCoastAngle + std::numbers::pi_v<float> * 0.5f + 0.16f;
        ship_.heelRadians = -0.14f;
        ship_.pitchRadians = 0.035f;
        ship_.length = ShipLength;
        ship_.beam = ShipBeam;
        ship_.hullHeight = ShipHullHeight;

        const float tangentX = -outwardZ;
        const float tangentZ = outwardX;
        for (std::size_t index = 0; index < debris_.size(); ++index) {
            const DebrisAuthoring& authored = DebrisAuthoringData[index];
            ShipwreckDebrisPlacement& debris = debris_[index];
            debris.x = ship_.x + tangentX * authored.tangentOffset
                - outwardX * authored.inwardOffset;
            debris.z = ship_.z + tangentZ * authored.tangentOffset
                - outwardZ * authored.inwardOffset;
            const float support = terrain.Height(debris.x, debris.z);
            debris.y = support < 0.0f ? std::max(-0.10f, support + 0.09f)
                                       : support + 0.055f;
            debris.yawRadians = ship_.yawRadians + authored.yawOffset;
            debris.length = authored.length;
            debris.width = authored.width;
            debris.kind = authored.kind;
        }

        constexpr std::array<float, CollisionProxyCount> AlongHull = {
            -8.0f, -5.1f, -2.4f, 2.2f, 5.2f, 8.0f};
        const float forwardX = std::sin(ship_.yawRadians);
        const float forwardZ = std::cos(ship_.yawRadians);
        for (std::size_t index = 0; index < obstacles_.size(); ++index) {
            obstacles_[index] = CircularObstacle{
                ship_.x + forwardX * AlongHull[index],
                ship_.z + forwardZ * AlongHull[index],
                index == 0U || index + 1U == obstacles_.size() ? 1.35f : 1.72f};
        }
    }
}
