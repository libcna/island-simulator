#pragma once

#include <cstdint>

namespace island
{
    /** @brief Coarse physical surface class shared by placement, collision, and presentation. */
    enum class TerrainSurface : std::uint8_t
    {
        DeepSeabed,
        ShallowSeabed,
        WetSand,
        DrySand,
        CoastalGround,
        InteriorGround,
        SteepGround
    };

    /** @brief Renderer-independent unit surface normal. */
    struct TerrainNormal
    {
        float x = 0.0f;
        float y = 1.0f;
        float z = 0.0f;
    };

    /** @brief Complete authoritative terrain query at one horizontal world position. */
    struct TerrainSample
    {
        float height = 0.0f;
        float signedShoreDistance = 0.0f;
        TerrainNormal normal;
        float slopeRadians = 0.0f;
        TerrainSurface surface = TerrainSurface::WetSand;

        /** @brief Reports whether mean water level covers the sampled terrain. */
        [[nodiscard]] bool IsSubmerged() const noexcept { return height < 0.0f; }
    };

    /**
     * @brief Deterministic CPU representation shared by island rendering and world queries.
     *
     * The first campaign slice intentionally keeps this representation analytic. Later authored
     * masks and landmark adjustments can layer on top without allowing collision, placement, and
     * rendering to disagree about the shoreline.
     */
    class IslandTerrain final
    {
    public:
        /** @brief Nominal island diameter in metres. */
        static constexpr float NominalDiameter = 500.0f;

        /**
         * @brief Returns the radial shoreline distance for a polar angle.
         * @param angleRadians Angle around the island in radians.
         * @return Shore radius in metres.
         */
        [[nodiscard]] float ShoreRadius(float angleRadians) const noexcept;

        /**
         * @brief Returns a stable island-relative signed shoreline distance approximation.
         * @param x World x coordinate in metres.
         * @param z World z coordinate in metres.
         * @return Positive on land, zero at the shore, and negative offshore.
         */
        [[nodiscard]] float SignedShoreDistance(float x, float z) const noexcept;

        /**
         * @brief Samples terrain or seabed height.
         * @param x World x coordinate in metres.
         * @param z World z coordinate in metres.
         * @return Height in metres relative to mean water level zero.
         */
        [[nodiscard]] float Height(float x, float z) const noexcept;

        /**
         * @brief Samples height, shoreline distance, normal, slope, and coarse surface class.
         *
         * This is the preferred query when a system needs more than height alone. Keeping these
         * derived values together prevents collision, placement, and rendering from using subtly
         * different finite-difference conventions.
         */
        [[nodiscard]] TerrainSample Sample(float x, float z) const noexcept;

        /**
         * @brief Reports whether the coordinate is on dry land.
         * @param x World x coordinate in metres.
         * @param z World z coordinate in metres.
         * @return True for positive terrain height.
         */
        [[nodiscard]] bool IsLand(float x, float z) const noexcept;

        /** @brief Finite-difference spacing used by authoritative surface queries, in metres. */
        static constexpr float SurfaceSampleDistance = 0.75f;

    private:
        [[nodiscard]] static std::uint32_t Hash(std::int32_t x, std::int32_t z,
                                                std::uint32_t seed) noexcept;
        [[nodiscard]] static float ValueNoise(float x, float z, std::uint32_t seed) noexcept;
        [[nodiscard]] static float FractalNoise(float x, float z, std::uint32_t seed) noexcept;
    };
}
