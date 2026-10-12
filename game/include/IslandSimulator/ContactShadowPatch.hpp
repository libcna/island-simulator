#pragma once

#include <array>
#include <vector>

namespace island
{
    class IslandTerrain;
    class RenderedTerrainSurface;
    struct RockPlacement;
    struct FallenLogPlacement;
    struct DriftwoodPlacement;
    struct ShipwreckPlacement;
    struct ShipwreckDebrisPlacement;
    struct HutPlacement;

    // Soft grounding approximation, not a directional shadow-map silhouette.
    struct ContactFootprint
    {
        float x, z;
        float directionX, directionZ;
        float alongRadius, acrossRadius;
        float opacity;
    };

    struct ContactPatchVertex
    {
        float x, y, z, opacity;
    };

    inline constexpr float ContactPatchSurfaceOffset = 0.025f;
    enum class ContactPatchDetail { Compact, Coastal, SmallAnimal };
    inline constexpr float SmallContactSurfaceOffset = 0.0003f;
    [[nodiscard]] ContactFootprint LandRockContactFootprint(const RockPlacement& rock) noexcept;
    [[nodiscard]] ContactFootprint LogContactFootprint(const FallenLogPlacement& log) noexcept;
    [[nodiscard]] ContactFootprint DriftwoodContactFootprint(const DriftwoodPlacement& wood) noexcept;
    [[nodiscard]] ContactFootprint RockContactFootprint(const RockPlacement& rock) noexcept;
    [[nodiscard]] std::array<ContactFootprint, 2> HullContactFootprints(
        const ShipwreckPlacement& ship) noexcept;
    [[nodiscard]] ContactFootprint DebrisContactFootprint(
        const ShipwreckDebrisPlacement& debris) noexcept;
    [[nodiscard]] std::array<ContactFootprint,7> HutContactFootprints(const HutPlacement& hut) noexcept;

    /** Build a soft ellipse, split at actual terrain grid/diagonal boundaries.
     * All output triangles lie on one rendered terrain triangle, plus a small vertical bias.
     * Footprints require finite, unit directions, radii in [0.05, 8] m and opacity in [0, 1].
     * SmallAnimal uses [0.001,0.2] m radii and a 0.3mm surface bias instead.
     * An optional matching rendered-surface cache avoids procedural corner resampling.
     */
    [[nodiscard]] std::vector<ContactPatchVertex> BuildContactPatch(
        const IslandTerrain& terrain, const ContactFootprint& footprint,
        ContactPatchDetail detail = ContactPatchDetail::Coastal,
        const RenderedTerrainSurface* cachedSurface = nullptr);
}
