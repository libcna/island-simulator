#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/BearMotion.hpp"
#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/FrogPopulation.hpp"
#include "IslandSimulator/CicadaColony.hpp"
#include "IslandSimulator/FlySwarms.hpp"

namespace island
{
    /** @brief Renderer-independent volume values for the first ambient soundscape layers. */
    struct SoundscapeMix
    {
        float surfVolume = 0.0f;
        float breezeVolume = 0.0f;
        float jungleVolume = 0.0f;
        float streamVolume = 0.0f;
    };

    /** @brief Depth-driven gains applied to existing loops; it adds no audio source. */
    struct UnderwaterAudioMix
    {
        float immersion = 0.0f;
        float airGain = 1.0f;
        float surfGain = 1.0f;
        float waterGain = 1.0f;
        float waterPitchOffset = 0.0f;
    };

    /** @brief Maps authoritative eye depth to a bounded, finite submerged sound response. */
    [[nodiscard]] UnderwaterAudioMix UnderwaterAudioResponse(float submergedDepth) noexcept;

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

    /** @brief Generates a seamless thirteen-second close stream-water bed. */
    [[nodiscard]] AmbientLoop GenerateStreamLoop();
    /** @brief Original nineteen-second ember hiss, wood crackles and sparse pops. */
    [[nodiscard]] AmbientLoop GenerateCampfireLoop();

    /** @brief Original seamless seventeen-second underground electrical-generator hum. */
    [[nodiscard]] AmbientLoop GenerateBunkerHumLoop();

    /** @brief Generates one four-second stronger-wave wash for sparse shoreline accents. */
    [[nodiscard]] AmbientLoop GenerateWaveAccent();

    /** @brief Generates a second four-second wave shape for repetition control. */
    [[nodiscard]] AmbientLoop GenerateWaveAccentAlternate();

    /** @brief Generates a short low wooden hull-creak phrase near the grounded wreck. */
    [[nodiscard]] AmbientLoop GenerateShipwreckCreak();

    /** @brief Generates a second structurally distinct wreck-creak phrase. */
    [[nodiscard]] AmbientLoop GenerateShipwreckCreakAlternate();

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

    /** @brief Generates a lower, rough paired squawk for nearby canopy parrots. */
    [[nodiscard]] AmbientLoop GenerateParrotCall();

    /** @brief Generates an alternate three-part parrot contact phrase. */
    [[nodiscard]] AmbientLoop GenerateParrotCallAlternate();

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

    /** @brief One sparse positional wreck-creak request sharing the wave transient voice. */
    struct ShipwreckCreakRequest
    {
        bool trigger = false;
        std::uint8_t variant = 0;
        float volume = 0.0f;
        float pitch = 0.0f;
    };

    /** @brief Deterministic proximity/wave scheduler with no missed-event backlog. */
    class ShipwreckCreakScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] ShipwreckCreakRequest Update(
            float deltaSeconds, float surfVolume, float distanceToShipwreck,
            bool playbackSlotAvailable) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;

        std::uint32_t randomState_ = 0xc8ea41f7U;
        float secondsUntilCreak_ = 0.0f;
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
        Monkey,
        Boar,
        Snake,
        Bear,
        Goat,
        Frog,
        Cicada,
        Fly
    };

    /** @brief Nearest currently eligible world-space source for one wildlife species. */
    struct WildlifeAudioSource
    {
        bool available = false;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        std::uint8_t kind = 0U;
    };

    /** @brief Original short low croak phrase for stream and wetland frogs. */
    [[nodiscard]] AmbientLoop GenerateFrogCall(std::uint8_t variant);

    struct FrogCallRequest
    {
        bool trigger = false;
        std::uint8_t variant = 0U;
        std::size_t animalIndex = 0U;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /** @brief Sparse habitat-local frog voice sharing the existing transient source. */
    class FrogCallScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] FrogCallRequest Update(float deltaSeconds, float listenerX,
            float listenerY, float listenerZ, std::span<const FrogState> frogs,
            bool playbackSlotAvailable) noexcept;
    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::uint32_t randomState_ = 0xf206ca11U;
        float cooldown_ = 0.0f;
        std::size_t nextAnimal_ = 0U;
    };

    /** @brief Original pulsed daytime cicada buzz, not a recorded sample. */
    [[nodiscard]] AmbientLoop GenerateCicadaCall(std::uint8_t variant);

    struct CicadaCallRequest
    {
        bool trigger = false;
        std::uint8_t variant = 0U;
        std::size_t animalIndex = 0U;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /** @brief Sparse trunk-local cicada calls sharing the existing transient source. */
    class CicadaCallScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] CicadaCallRequest Update(float deltaSeconds, float listenerX,
            float listenerY, float listenerZ, std::span<const CicadaState> cicadas,
            bool playbackSlotAvailable) noexcept;
    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::uint32_t randomState_ = 0xc1cada17U;
        float cooldown_ = 0.0f;
        std::size_t nextAnimal_ = 0U;
    };

    /** @brief Short restrained close-range wing buzz for one fly swarm. */
    [[nodiscard]] AmbientLoop GenerateFlyBuzz(std::uint8_t variant);

    struct FlyBuzzRequest
    {
        bool trigger = false;
        std::uint8_t variant = 0U;
        std::size_t animalIndex = 0U;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /** @brief Intermittent swarm-local buzz sharing the bounded wildlife voice. */
    class FlyBuzzScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] FlyBuzzRequest Update(float deltaSeconds, float listenerX,
            float listenerY, float listenerZ, std::span<const FlyState> flies,
            bool playbackSlotAvailable) noexcept;
    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::uint32_t randomState_ = 0xf17b0221U;
        float cooldown_ = 0.0f;
        std::size_t nextAnimal_ = 0U;
    };

    enum class BoarCallKind : std::uint8_t { None, Breath, Snuffle, Grunt };

    /** @brief Original synthesized mono voice, 1.2 seconds; not a recorded animal sample. */
    [[nodiscard]] AmbientLoop GenerateBoarCall(BoarCallKind kind, std::uint8_t variant);
    [[nodiscard]] float BoarCallVolume(BoarCallKind kind, float distance) noexcept;

    struct BoarCallRequest
    {
        BoarCallKind kind = BoarCallKind::None;
        std::size_t animalIndex = 0;
        std::uint8_t variant = 0;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /** @brief Behavior-aware, per-animal cooldowns; consumes at most one shared wildlife slot.
     * Input order is the stable BoarHerd order. Muted/busy callers pass false for the slot.
     * Inaudible/dormant animals are virtualized; no backlog of missed calls is replayed.
     */
    class BoarCallScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] BoarCallRequest Update(float deltaSeconds,
            float listenerX, float listenerY, float listenerZ,
            std::span<const BoarState> boars, bool playbackSlotAvailable) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::array<float, BoarHerd::BoarCount> cooldowns_{};
        std::array<std::uint8_t, BoarHerd::BoarCount> callPhases_{};
        std::uint32_t randomState_ = 0xb0a45eedU;
        float herdCooldown_ = 0.0f;
        std::size_t nextAnimal_ = 0;
    };

    enum class GoatCallKind : std::uint8_t { None, Breath, Bleat, Hoof };

    /** @brief Original synthesized mono goat voice/hoof detail, 1.2 seconds. */
    [[nodiscard]] AmbientLoop GenerateGoatCall(GoatCallKind kind, std::uint8_t variant);
    [[nodiscard]] float GoatCallVolume(GoatCallKind kind, float distance) noexcept;

    struct GoatCallRequest
    {
        GoatCallKind kind = GoatCallKind::None;
        std::size_t animalIndex = 0;
        std::uint8_t variant = 0;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /** @brief Behavior-aware sparse goat bleats, breathing and hoof-scuff detail. */
    class GoatCallScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] GoatCallRequest Update(float deltaSeconds,
            float listenerX, float listenerY, float listenerZ,
            std::span<const GoatState> goats, bool playbackSlotAvailable) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::array<float, GoatHabitat::Count> cooldowns_{};
        std::array<std::uint8_t, GoatHabitat::Count> callPhases_{};
        std::uint32_t randomState_ = 0x60a7b1eaU;
        float herdCooldown_ = 0.0f;
        std::size_t nextAnimal_ = 0;
    };

    /** @brief One original broadband hiss; two variants share a one-second budget. */
    [[nodiscard]] AmbientLoop GenerateSnakeHiss(std::uint8_t variant);
    [[nodiscard]] float SnakeHissVolume(float distance) noexcept;

    struct SnakeHissRequest
    {
        bool trigger = false;
        std::size_t animalIndex = 0;
        std::uint8_t variant = 0;
        float volume = 0.0f;
        float pitch = 0.0f;
        WildlifeAudioSource source;
    };

    /** @brief Close-retreat hisses with per-snake and population-wide cooldowns. */
    class SnakeHissScheduler final
    {
    public:
        void Reset() noexcept;
        [[nodiscard]] SnakeHissRequest Update(float deltaSeconds,
            float listenerX, float listenerY, float listenerZ,
            std::span<const SnakeState> snakes, bool playbackSlotAvailable) noexcept;

    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::array<float, SnakePopulation::Count> cooldowns_{};
        std::uint32_t randomState_ = 0x51a6e55eU;
        float populationCooldown_ = 0.0f;
        std::size_t nextAnimal_ = 0;
    };

    enum class BearCallKind : std::uint8_t {None,Breath,Snuffle,Huff};
    /** Original low, non-aggressive adult bear breath, snuffle or warning huff. */
    [[nodiscard]] AmbientLoop GenerateBearCall(BearCallKind kind,std::uint8_t variant);
    [[nodiscard]] float BearCallVolume(BearCallKind kind,float distance) noexcept;

    struct BearCallRequest {
        BearCallKind kind=BearCallKind::None;
        std::uint8_t variant=0;
        float volume=0,pitch=0;
        WildlifeAudioSource source;
    };

    /** Behavior-aware sparse voice for the island's one peaceful bear. */
    class BearCallScheduler final {
    public:
        void Reset() noexcept;
        [[nodiscard]] BearCallRequest Update(float deltaSeconds,float listenerX,float listenerY,
            float listenerZ,const BearState& bear,bool playbackSlotAvailable) noexcept;
    private:
        [[nodiscard]] float NextUnit() noexcept;
        std::uint32_t randomState_=0xbea401ceU;
        float cooldown_=0;
        std::uint8_t callPhase_=0;
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
                                                     bool swimming,
                                                     float coastalWindExposure=.5f,
                                                     float streamPresence=0.0f,
                                                     float seasonalWetlandPresence=0.0f) noexcept;

        /** @brief Jumps to the correct mix, used before playback begins. */
        void Reset(float signedShoreDistance, bool swimming,
                   float coastalWindExposure=.5f,
                   float streamPresence=0.0f,
                   float seasonalWetlandPresence=0.0f) noexcept;

        /** @brief Advances click-free zone crossfades. */
        [[nodiscard]] SoundscapeMix Update(float signedShoreDistance, bool swimming,
                                           float deltaSeconds,
                                           float coastalWindExposure=.5f,
                                           float streamPresence=0.0f,
                                           float seasonalWetlandPresence=0.0f) noexcept;

        [[nodiscard]] const SoundscapeMix& Mix() const noexcept { return mix_; }

    private:
        SoundscapeMix mix_;
        double variationSeconds_ = 0.0;
    };
}
