#pragma once

#include <cstdint>
#include <vector>

namespace island
{
    /** @brief Renderer-independent volume values for the first ambient soundscape layers. */
    struct SoundscapeMix
    {
        float surfVolume = 0.0f;
        float breezeVolume = 0.0f;
    };

    /** @brief Original deterministic mono PCM used by one procedural ambient loop. */
    struct AmbientLoop
    {
        int sampleRate = 0;
        std::vector<std::int16_t> samples;
    };

    /** @brief Generates a seamless ten-second surf bed without external audio assets. */
    [[nodiscard]] AmbientLoop GenerateSurfLoop();

    /** @brief Generates a seamless ten-second light-canopy-breeze bed. */
    [[nodiscard]] AmbientLoop GenerateBreezeLoop();

    /** @brief Generates one four-second stronger-wave wash for sparse shoreline accents. */
    [[nodiscard]] AmbientLoop GenerateWaveAccent();

    /** @brief One optional, bounded stronger-wave playback request. */
    struct WaveAccentRequest
    {
        bool trigger = false;
        float volume = 0.0f;
        float pitch = 0.0f;
        float pan = 0.0f;
    };

    /** @brief Deterministic sparse scheduler that never overlaps its four-second wave accents. */
    class WaveAccentScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] WaveAccentRequest Update(float deltaSeconds,
                                               float surfVolume) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;

        std::uint32_t randomState_ = 0x78d2f9a5U;
        float secondsUntilAccent_ = 0.0f;
    };

    /** @brief Smoothly blends ambient layers from authoritative shoreline/player state. */
    class SoundscapeModel final
    {
    public:
        /** @brief Computes the steady-state mix at one player location. */
        [[nodiscard]] static SoundscapeMix TargetMix(float signedShoreDistance,
                                                     bool swimming) noexcept;

        /** @brief Jumps to the correct mix, used before playback begins. */
        void Reset(float signedShoreDistance, bool swimming) noexcept;

        /** @brief Advances click-free zone crossfades. */
        [[nodiscard]] SoundscapeMix Update(float signedShoreDistance, bool swimming,
                                           float deltaSeconds) noexcept;

        [[nodiscard]] const SoundscapeMix& Mix() const noexcept { return mix_; }

    private:
        SoundscapeMix mix_;
    };
}
