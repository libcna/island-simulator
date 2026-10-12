#pragma once

#include <memory>
#include <span>
#include <string>

#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/SoundscapeModel.hpp"
#include "IslandSimulator/StreamSoundField.hpp"

namespace Microsoft::Xna::Framework::Audio
{
    class SoundEffect;
    class SoundEffectInstance;
}

namespace island
{
    /** @brief Renderer-independent camera state consumed by positional audio. */
    struct SoundscapeListenerState
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float forwardX = 0.0f;
        float forwardY = 0.0f;
        float forwardZ = -1.0f;
        float velocityX = 0.0f;
        float velocityZ = 0.0f;
    };

    /** @brief Read-only runtime state consumed by opt-in audio diagnostics. */
    struct SoundscapeDebugState
    {
        WildlifeAudioSource nearestBird;
        WildlifeAudioSource nearestMonkey;
        WildlifeCallRequest activeWildlifeCall;
        int playingSourceCount = 0;
        int sourceBudget = 6;
        bool campfirePlaying = false;
        float campfireVolume=0;
    };

    /** @brief Owns the procedural ambient layers and their CNA playback instances. */
    class Soundscape final
    {
    public:
        static constexpr int PersistentSourceCount = 4;
        static constexpr int MaximumPlayingSourceCount = 6;

        Soundscape();
        ~Soundscape();

        Soundscape(const Soundscape&) = delete;
        Soundscape& operator=(const Soundscape&) = delete;
        Soundscape(Soundscape&&) = delete;
        Soundscape& operator=(Soundscape&&) = delete;

        /** @brief Builds PCM and begins the looping layers, or records a graceful device failure. */
        bool Start(float signedShoreDistance, bool swimming, float waterDepth,
                   float horizontalSpeed, float oceanCurrentStrength,
                   float coastalWindExposure=.5f,
                   float playerX=0.0f, float playerZ=0.0f) noexcept;

        /** @brief Advances location-driven crossfades for active playback. */
        void Update(float signedShoreDistance, bool swimming, float waterDepth,
                    float horizontalSpeed, float oceanCurrentStrength, float deltaSeconds,
                    float coastalWindExposure,
                    const SoundscapeListenerState& listener,
                    std::span<const BirdState> birds,
                    std::span<const MonkeyState> monkeys,
                    std::span<const BoarState> boars,
                    std::span<const SnakeState> snakes,
                    const BearState* bear=nullptr) noexcept;

        /** @brief Refreshes diagnostic emitter candidates even when audio playback is disabled. */
        void RefreshDebugSources(float listenerX, float listenerY, float listenerZ,
                                 std::span<const BirdState> birds,
                                 std::span<const MonkeyState> monkeys) noexcept;

        /** @brief Toggles user-requested silence without destroying the active loops. */
        void ToggleMuted() noexcept;

        /** @brief Applies a persisted mute preference without relying on a synthetic toggle. */
        void SetMuted(bool muted) noexcept;

        [[nodiscard]] bool IsAvailable() const noexcept { return available_; }
        [[nodiscard]] bool IsMuted() const noexcept { return muted_; }
        [[nodiscard]] bool WasStartAttempted() const noexcept { return startAttempted_; }
        [[nodiscard]] const SoundscapeMix& Mix() const noexcept { return model_.Mix(); }
        [[nodiscard]] const WaterMovementMix& WaterMix() const noexcept
        {
            return waterMovementModel_.Mix();
        }
        [[nodiscard]] const std::string& Error() const noexcept { return error_; }
        [[nodiscard]] int PlayedWaveAccentCount() const noexcept
        {
            return playedWaveAccentCount_;
        }
        [[nodiscard]] int PlayedWaterTransitionCount() const noexcept
        {
            return playedWaterTransitionCount_;
        }
        [[nodiscard]] int PlayedFootstepCount() const noexcept
        {
            return playedFootstepCount_;
        }
        [[nodiscard]] int PlayedBirdCallCount() const noexcept
        {
            return playedBirdCallCount_;
        }
        [[nodiscard]] int PlayedMonkeyCallCount() const noexcept
        {
            return playedMonkeyCallCount_;
        }
        [[nodiscard]] int PlayedBoarCallCount() const noexcept { return playedBoarCallCount_; }
        [[nodiscard]] int PlayedSnakeHissCount() const noexcept { return playedSnakeHissCount_; }
        [[nodiscard]] int PlayedBearCallCount() const noexcept{return playedBearCallCount_;}
        [[nodiscard]] bool IsWildlifeCallActive() const noexcept
        {
            return wildlifeSecondsRemaining_ > 0.0f;
        }
        [[nodiscard]] SoundscapeDebugState DebugState() const noexcept;

    private:
        void ApplyVolumes() noexcept;
        void DisableAfterRuntimeFailure(const std::string& message) noexcept;

        SoundscapeModel model_;
        WaterMovementModel waterMovementModel_;
        WaveAccentScheduler waveScheduler_;
        WaterTransitionScheduler waterTransitionScheduler_;
        FootstepScheduler footstepScheduler_;
        WildlifeCallScheduler wildlifeScheduler_;
        BoarCallScheduler boarScheduler_;
        SnakeHissScheduler snakeHissScheduler_;
        BearCallScheduler bearCallScheduler_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>, 6> boarEffects_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>, 2> snakeHissEffects_;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>,6> bearEffects_;
        BoarCallRequest activeBoarCall_;
        SnakeHissRequest activeSnakeHiss_;
        BearCallRequest activeBearCall_;
        float boarVoiceGain_ = 1.0f;
        float snakeVoiceGain_ = 1.0f;
        float bearVoiceGain_=1.0f;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> surfEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> surfInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> breezeEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> breezeInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> jungleEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> jungleInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> streamEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> streamInstance_;
        StreamSoundSample streamSound_;
        bool streamPlaybackActive_ = false;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> campfireEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> campfireInstance_;
        bool campfirePlaybackActive_ = false;
        float campfirePresence_=0,campfireVolume_=0;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> waveEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> waveAlternateEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> waveInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> waterMovementEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance>
            waterMovementInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            waterTransitionEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            waterTransitionAlternateEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance>
            waterTransitionInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> sandFootstepEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            sandFootstepAlternateEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> soilFootstepEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            soilFootstepAlternateEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance>
            footstepInstance_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> birdCallEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            birdCallAlternateEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> monkeyCallEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>
            monkeyCallAlternateEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance>
            wildlifeInstance_;
        WildlifeAudioSource nearestBirdSource_;
        WildlifeAudioSource nearestMonkeySource_;
        WildlifeCallRequest activeWildlifeCall_;
        std::string error_;
        bool available_ = false;
        bool muted_ = false;
        bool startAttempted_ = false;
        bool waveAccentsEnabled_ = false;
        bool waveFailureReported_ = false;
        bool waterTransitionsEnabled_ = false;
        bool waterTransitionFailureReported_ = false;
        bool footstepsEnabled_ = false;
        bool footstepFailureReported_ = false;
        bool wildlifeCallsEnabled_ = false;
        bool wildlifeFailureReported_ = false;
        float waveSecondsRemaining_ = 0.0f;
        float waterTransitionSecondsRemaining_ = 0.0f;
        float footstepSecondsRemaining_ = 0.0f;
        float wildlifeSecondsRemaining_ = 0.0f;
        int playedWaveAccentCount_ = 0;
        int playedWaterTransitionCount_ = 0;
        int playedFootstepCount_ = 0;
        int playedBirdCallCount_ = 0;
        int playedMonkeyCallCount_ = 0;
        int playedBoarCallCount_ = 0;
        int playedSnakeHissCount_ = 0;
        int playedBearCallCount_=0;
    };
}
