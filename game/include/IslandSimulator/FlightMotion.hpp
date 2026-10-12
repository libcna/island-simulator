#pragma once

#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <memory>
#include <optional>

namespace island
{
    // Player flight, independent of the unrestricted developer inspection camera.
    class FlightMotion final
    {
    public:
        static constexpr float Radius = IslandTerrain::NominalDiameter * .5f + 80.0f;
        static constexpr float MaximumHeight = 100.0f;
        static constexpr float Clearance = PlayerMotion::EyeHeight;
        static constexpr float Speed = 12.0f;
        void SetCave(const CaveLayout* cave,const CaveGroundPatch* ground) noexcept
        {cave_=cave;caveGround_=ground;}
        void Activate(const IslandTerrain& terrain, PlayerPosition position);
        void Deactivate() noexcept { active_ = false; }
        void Update(float worldX, float vertical, float worldZ, bool fast, float seconds);
        [[nodiscard]] bool IsActive() const noexcept { return active_; }
        [[nodiscard]] const PlayerPosition& Position() const noexcept { return position_; }
        [[nodiscard]] float HeightAboveGround() const
        { return surface_ ? position_.y-GroundHeight(position_.x,position_.z) : 0.0f; }
        // No valid ground means remain in flight, never manufacture an unsafe landing.
        [[nodiscard]] std::optional<PlayerPosition> FindLanding(
            const IslandTerrain& terrain, const WorldCollisionGrid& obstacles) const;
    private:
        [[nodiscard]] float GroundHeight(float x,float z) const;
        const CaveLayout* cave_ = nullptr;
        const CaveGroundPatch* caveGround_ = nullptr;
        std::unique_ptr<RenderedTerrainSurface> surface_;
        PlayerPosition position_;
        bool active_ = false;
    };
}
