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
        float jungleVolume = 0.0f;
    };

    /** @brief Original deterministic mono PCM used by one procedural ambient loop. */
    struct AmbientLoop
    {
        int sampleRate = 0;
        std::vector<std::int16_t> samples;
    };

    /** @brief Generates a seamless ten-second surf bed without external audio assets. */
    [[nodiscard]] AmbientLoop GenerateSurfLoop();

    /** @brief Generates a seamless eleven-second light-canopy-breeze bed. */
    [[nodiscard]] AmbientLoop GenerateBreezeLoop();

    /** @brief Generates a seamless twenty-nine-second subtle daytime-jungle bed. */
    [[nodiscard]] AmbientLoop GenerateJungleLoop();

    /** @brief Generates one four-second stronger-wave wash for sparse shoreline accents. */
    [[nodiscard]] AmbientLoop GenerateWaveAccent();

    /** @brief Generates a second four-second wave shape for repetition control. */
    [[nodiscard]] AmbientLoop GenerateWaveAccentAlternate();

    /** @brief Generates a seamless seven-second water-motion bed for wading and swimming. */
    [[nodiscard]] AmbientLoop GenerateWaterMovementLoop();

    /** @brief Generates one short non-looping water entry/exit accent. */
    [[nodiscard]] AmbientLoop GenerateWaterTransitionAccent();

    /** @brief Generates a second short water-boundary texture. */
    [[nodiscard]] AmbientLoop GenerateWaterTransitionAccentAlternate();

    /** @brief Generates the first short dry-sand player footstep. */
    [[nodiscard]] AmbientLoop GenerateSandFootstep();

    /** @brief Generates a second dry-sand footstep for repetition control. */
    [[nodiscard]] AmbientLoop GenerateSandFootstepAlternate();

    /** @brief Generates the first short interior-soil player footstep. */
    [[nodiscard]] AmbientLoop GenerateSoilFootstep();

    /** @brief Generates a second interior-soil footstep for repetition control. */
    [[nodiscard]] AmbientLoop GenerateSoilFootstepAlternate();

    /** @brief Generates one short, non-looping tropical bird contact call. */
    [[nodiscard]] AmbientLoop GenerateBirdCall();

    /** @brief Generates a second bird phrase with different rhythm and pitch movement. */
    [[nodiscard]] AmbientLoop GenerateBirdCallAlternate();

    /** @brief Generates one restrained monkey contact call for the interior jungle. */
    [[nodiscard]] AmbientLoop GenerateMonkeyCall();

    /** @brief Generates a second monkey contact phrase for repetition control. */
    [[nodiscard]] AmbientLoop GenerateMonkeyCallAlternate();

    /** @brief One optional, bounded stronger-wave playback request. */
    struct WaveAccentRequest
    {
        bool trigger = false;
        std::uint8_t variant = 0;
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
        float currentCue = 0.0f;
    };

    /** @brief Maps water depth, speed, and offshore current to one non-chattering loop mix. */
    class WaterMovementModel final
    {
    public:
        [[nodiscard]] static WaterMovementMix TargetMix(float waterDepth, bool swimming,
                                                        float horizontalSpeed,
                                                        float oceanCurrentStrength = 0.0f) noexcept;
        void Reset(float waterDepth, bool swimming, float horizontalSpeed,
                   float oceanCurrentStrength = 0.0f) noexcept;
        [[nodiscard]] WaterMovementMix Update(float waterDepth, bool swimming,
                                              float horizontalSpeed,
                                              float deltaSeconds,
                                              float oceanCurrentStrength = 0.0f) noexcept;
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
        std::uint8_t variant = 0;
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

    enum class FootstepSurface : std::uint8_t
    {
        None,
        Sand,
        Soil
    };

    /** @brief One distance-scheduled land footstep with bounded playback variation. */
    struct FootstepRequest
    {
        FootstepSurface surface = FootstepSurface::None;
        std::uint8_t variant = 0;
        float volume = 0.0f;
        float pitch = 0.0f;
        float pan = 0.0f;
    };

    /** @brief Emits land footsteps from resolved player travel rather than frame cadence. */
    class FootstepScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] FootstepRequest Update(float deltaSeconds, bool swimming,
                                             float signedShoreDistance,
                                             float horizontalSpeed,
                                             float listenerX, float listenerZ) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;

        std::uint32_t randomState_ = 0xf00757e9U;
        float previousX_ = 0.0f;
        float previousZ_ = 0.0f;
        float distanceSinceStep_ = 0.0f;
        bool hasPreviousPosition_ = false;
        bool wasMoving_ = false;
        bool leftFoot_ = false;
    };

    /** @brief Species associated with one optional positional wildlife call. */
    enum class WildlifeCallType : std::uint8_t
    {
        None,
        Bird,
        Monkey
    };

    /** @brief Nearest currently eligible world-space source for one wildlife species. */
    struct WildlifeAudioSource
    {
        bool available = false;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    /** @brief One bounded, mono positional call request selected from visible wildlife. */
    struct WildlifeCallRequest
    {
        WildlifeCallType type = WildlifeCallType::None;
        std::uint8_t variant = 0;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /**
     * @brief Sparse deterministic scheduler shared by bird and monkey contact calls.
     *
     * A single scheduler prevents both species from chattering at once. Playback may be held while
     * the two transient audio slots are occupied, preserving the six-source application budget.
     */
    class WildlifeCallScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] WildlifeCallRequest Update(float deltaSeconds, bool swimming,
                                                 float signedShoreDistance,
                                                 float listenerX, float listenerY,
                                                 float listenerZ,
                                                 const WildlifeAudioSource& bird,
                                                 const WildlifeAudioSource& monkey,
                                                 bool playbackSlotAvailable) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;

        std::uint32_t randomState_ = 0x6c8e9cf5U;
        float secondsUntilCall_ = 0.0f;
    };

    /** @brief Smoothly blends ambient layers from authoritative shoreline/player state. */
    class SoundscapeModel final
    {
    public:
        static constexpr float BreezeFadeStartDistance = 12.0f;
        static constexpr float SurfFadeStartDistance = 18.0f;
        static constexpr float JungleFadeStartDistance = 30.0f;
        static constexpr float BreezeFadeEndDistance = 105.0f;
        static constexpr float JungleFadeEndDistance = 112.0f;
        static constexpr float SurfFadeEndDistance = 125.0f;

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
        double variationSeconds_ = 0.0;
    };
}
