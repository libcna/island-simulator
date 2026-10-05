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

    /** @brief Generates a seamless six-second water-motion bed for wading and swimming. */
    [[nodiscard]] AmbientLoop GenerateWaterMovementLoop();

    /** @brief Generates one short non-looping water entry/exit accent. */
    [[nodiscard]] AmbientLoop GenerateWaterTransitionAccent();

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

    /** @brief Smoothed gain and pitch for the player's continuous movement through water. */
    struct WaterMovementMix
    {
        float volume = 0.0f;
        float pitch = 0.0f;
    };

    /** @brief Maps authoritative water depth and player speed to a non-chattering loop mix. */
    class WaterMovementModel final
    {
    public:
        [[nodiscard]] static WaterMovementMix TargetMix(float waterDepth, bool swimming,
                                                        float horizontalSpeed) noexcept;
        void Reset(float waterDepth, bool swimming, float horizontalSpeed) noexcept;
        [[nodiscard]] WaterMovementMix Update(float waterDepth, bool swimming,
                                              float horizontalSpeed,
                                              float deltaSeconds) noexcept;
        [[nodiscard]] const WaterMovementMix& Mix() const noexcept { return mix_; }

    private:
        WaterMovementMix mix_;
    };

    enum class WaterTransitionType : std::uint8_t
    {
        None,
        Enter,
        Exit
    };

    /** @brief One optional water-boundary accent with bounded playback variation. */
    struct WaterTransitionRequest
    {
        WaterTransitionType type = WaterTransitionType::None;
        float volume = 0.0f;
        float pitch = 0.0f;
        float pan = 0.0f;
    };

    /** @brief Hysteretic, cooldown-protected detector for player water entry and exit. */
    class WaterTransitionScheduler final
    {
    public:
        void Reset(float waterDepth, bool swimming) noexcept;
        [[nodiscard]] WaterTransitionRequest Update(float waterDepth, bool swimming,
                                                    float horizontalSpeed,
                                                    float deltaSeconds) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;

        std::uint32_t randomState_ = 0xc0145a7eU;
        float cooldownSeconds_ = 0.0f;
        bool inWater_ = false;
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
