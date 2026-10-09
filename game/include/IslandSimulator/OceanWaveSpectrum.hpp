#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace island
{
    /** @brief One analytic wave: wave vector in radians/metre, amplitude in metres. */
    struct OceanWave final
    {
        float waveX;
        float waveZ;
        float amplitude;
        float angularSpeed;
        float phase;
    };

    // Only the first three broad waves displace geometry. Finer components affect shading, not
    // the low-resolution mesh, swimming physics or the calm sea's maximum physical wave height.
    inline constexpr std::size_t OceanDisplacementWaveCount = 3;
    inline constexpr std::size_t OceanBroadNormalWaveCount = 5;
    inline constexpr std::array<OceanWave, 9> OceanWaves{{
        {0.061f, 0.031f, 0.105f, 0.72f, 0.0f},
        {-0.093f, 0.074f, 0.060f, 0.96f, 1.7f},
        {0.148f, -0.116f, 0.028f, 1.31f, 3.2f},
        {-0.1332f, -0.34521f, 0.014f, 1.83f, 0.8f},
        {0.36494f, 0.60918f, 0.008f, 2.27f, 4.1f},
        {2.3f, 0.91f, 0.014f, 1.76f, 0.7f},
        {-3.1f, 2.7f, 0.010f, 2.22f, 1.2f},
        {2.498f, 6.863f, 0.005f, 3.4f, 4.3f},
        {-4.081f, 8.751f, 0.003f, 4.2f, 2.7f},
    }};

    // Slow coordinate distortion bends the short wave trains into irregular wind patches.
    // Its bounded Jacobian stays positive; it never folds the surface or changes displacement.
    inline constexpr OceanWave OceanWarpX{0.19f, 0.13f, 0.7f, 0.07f, 0.4f};
    inline constexpr OceanWave OceanWarpZ{-0.11f, 0.23f, 0.6f, 0.09f, 2.1f};

    // Each normal-only short wave travels through a different slow wind packet. Envelopes
    // reuse the warp phases and range from 0.30 to 1.00; no surface displacement is added.
    inline constexpr float OceanPacketMean = 0.65f;
    inline constexpr float OceanPacketVariation = 0.35f;
    inline constexpr std::array<std::array<float, 2>, 4> OceanPacketPhases{{
        {0.3f, 1.1f}, {1.8f, 2.9f}, {3.7f, 0.4f}, {5.2f, 4.6f},
    }};
    static_assert(OceanPacketPhases.size() == OceanWaves.size() - OceanBroadNormalWaveCount);

    struct OceanPixelFootprint final
    {
        float dxX = 0.0f;
        float dxZ = 0.0f;
        float dyX = 0.0f;
        float dyZ = 0.0f;
    };

    struct OceanWaveSample final
    {
        float height = 0.0f;
        float broadSlopeX = 0.0f;
        float broadSlopeZ = 0.0f;
        float detailSlopeX = 0.0f;
        float detailSlopeZ = 0.0f;
    };

    /** @brief Smoothly remove waves approaching the pixel Nyquist limit (radians/pixel). */
    [[nodiscard]] float OceanWaveFilter(float phaseFootprint) noexcept;

    /** @brief CPU reference for the shared spectrum; invalid/overflowing inputs return calm water. */
    [[nodiscard]] OceanWaveSample SampleOceanWaves(float x, float z, float timeSeconds,
                                                  OceanPixelFootprint footprint = {}) noexcept;

    /** @brief Emit renderer-independent GLSL functions from the same coefficients as CPU tests. */
    [[nodiscard]] std::string OceanDisplacementGlsl();
    [[nodiscard]] std::string OceanSlopesGlsl();
}
