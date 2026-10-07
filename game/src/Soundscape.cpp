#include "IslandSimulator/Soundscape.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Microsoft/Xna/Framework/Audio/AudioChannels.hpp"
#include "Microsoft/Xna/Framework/Audio/AudioEmitter.hpp"
#include "Microsoft/Xna/Framework/Audio/AudioListener.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "SharpRuntime/SharpRuntimeHelper.hpp"

using namespace Microsoft::Xna::Framework::Audio;
using Microsoft::Xna::Framework::Vector3;

namespace island
{
    namespace
    {
        [[nodiscard]] std::vector<SharpRuntime::bytecs> ToPcmBytes(const AmbientLoop& loop)
        {
            std::vector<SharpRuntime::bytecs> bytes;
            bytes.resize(loop.samples.size() * 2U);
            for (std::size_t index = 0; index < loop.samples.size(); ++index) {
                const auto sample = static_cast<std::uint16_t>(loop.samples[index]);
                bytes[index * 2U] = static_cast<SharpRuntime::bytecs>(sample & 0xffU);
                bytes[index * 2U + 1U] = static_cast<SharpRuntime::bytecs>(sample >> 8U);
            }
            return bytes;
        }

        [[nodiscard]] std::unique_ptr<SoundEffect> CreateLoopedSoundEffect(
            const AmbientLoop& loop)
        {
            const std::vector<SharpRuntime::bytecs> bytes = ToPcmBytes(loop);
            return std::make_unique<SoundEffect>(
                bytes, 0, static_cast<SharpRuntime::intcs>(bytes.size()), loop.sampleRate,
                AudioChannels::Mono, 0,
                static_cast<SharpRuntime::intcs>(loop.samples.size()));
        }

        [[nodiscard]] std::unique_ptr<SoundEffect> CreateOneShotSoundEffect(
            const AmbientLoop& loop)
        {
            const std::vector<SharpRuntime::bytecs> bytes = ToPcmBytes(loop);
            return std::make_unique<SoundEffect>(
                bytes, loop.sampleRate, AudioChannels::Mono);
        }

        [[nodiscard]] WildlifeAudioSource NearestBirdSource(
            const std::span<const BirdState> birds, const float listenerX,
            const float listenerY, const float listenerZ) noexcept
        {
            WildlifeAudioSource result;
            float nearestDistanceSquared = std::numeric_limits<float>::max();
            for (const BirdState& bird : birds) {
                if (bird.simulationTier == WildlifeSimulationTier::Dormant) {
                    continue;
                }
                const float dx = bird.x - listenerX;
                const float dy = bird.y - listenerY;
                const float dz = bird.z - listenerZ;
                const float distanceSquared = dx * dx + dy * dy + dz * dz;
                if (distanceSquared < nearestDistanceSquared) {
                    nearestDistanceSquared = distanceSquared;
                    result = {true, bird.x, bird.y, bird.z};
                }
            }
            return result;
        }

        [[nodiscard]] WildlifeAudioSource NearestMonkeySource(
            const std::span<const MonkeyState> monkeys, const float listenerX,
            const float listenerY, const float listenerZ) noexcept
        {
            WildlifeAudioSource result;
            float nearestDistanceSquared = std::numeric_limits<float>::max();
            for (const MonkeyState& monkey : monkeys) {
                if (monkey.simulationTier == WildlifeSimulationTier::Dormant) {
                    continue;
                }
                const float dx = monkey.x - listenerX;
                const float dy = monkey.y - listenerY;
                const float dz = monkey.z - listenerZ;
                const float distanceSquared = dx * dx + dy * dy + dz * dz;
                if (distanceSquared < nearestDistanceSquared) {
                    nearestDistanceSquared = distanceSquared;
                    result = {true, monkey.x, monkey.y + 0.72f * monkey.scale, monkey.z};
                }
            }
            return result;
        }
    }

    Soundscape::Soundscape() = default;

    Soundscape::~Soundscape() = default;

    void Soundscape::RefreshDebugSources(
        const float listenerX, const float listenerY, const float listenerZ,
        const std::span<const BirdState> birds,
        const std::span<const MonkeyState> monkeys) noexcept
    {
        nearestBirdSource_ = NearestBirdSource(
            birds, listenerX, listenerY, listenerZ);
        nearestMonkeySource_ = NearestMonkeySource(
            monkeys, listenerX, listenerY, listenerZ);
    }

    bool Soundscape::Start(const float signedShoreDistance, const bool swimming,
                           const float waterDepth, const float horizontalSpeed,
                           const float oceanCurrentStrength) noexcept
    {
        if (startAttempted_) {
            return available_;
        }
        startAttempted_ = true;
        model_.Reset(signedShoreDistance, swimming);
        waterMovementModel_.Reset(
            waterDepth, swimming, horizontalSpeed, oceanCurrentStrength);
        try {
            // Generate, upload, and release one source at a time. Retaining all twelve PCM
            // vectors plus all twelve byte copies here raised transient startup memory for no
            // playback benefit, especially on Web and Android.
            surfEffect_ = CreateLoopedSoundEffect(GenerateSurfLoop());
            surfEffect_->setNameProperty("Procedural shoreline surf");
            surfInstance_ = std::make_unique<SoundEffectInstance>(surfEffect_->CreateInstance());
            surfInstance_->setIsLoopedProperty(true);

            breezeEffect_ = CreateLoopedSoundEffect(GenerateBreezeLoop());
            breezeEffect_->setNameProperty("Procedural canopy breeze");
            breezeInstance_ = std::make_unique<SoundEffectInstance>(
                breezeEffect_->CreateInstance());
            breezeInstance_->setIsLoopedProperty(true);

            jungleEffect_ = CreateLoopedSoundEffect(GenerateJungleLoop());
            jungleEffect_->setNameProperty("Procedural daytime jungle");
            jungleInstance_ = std::make_unique<SoundEffectInstance>(
                jungleEffect_->CreateInstance());
            jungleInstance_->setIsLoopedProperty(true);

            waveEffect_ = CreateOneShotSoundEffect(GenerateWaveAccent());
            waveEffect_->setNameProperty("Procedural stronger wave");
            waveAlternateEffect_ = CreateOneShotSoundEffect(GenerateWaveAccentAlternate());
            waveAlternateEffect_->setNameProperty("Procedural stronger wave alternate");

            waterMovementEffect_ = CreateLoopedSoundEffect(GenerateWaterMovementLoop());
            waterMovementEffect_->setNameProperty("Procedural player water movement");
            waterMovementInstance_ = std::make_unique<SoundEffectInstance>(
                waterMovementEffect_->CreateInstance());
            waterMovementInstance_->setIsLoopedProperty(true);

            waterTransitionEffect_ = CreateOneShotSoundEffect(
                GenerateWaterTransitionAccent());
            waterTransitionEffect_->setNameProperty("Procedural player water transition");
            waterTransitionAlternateEffect_ = CreateOneShotSoundEffect(
                GenerateWaterTransitionAccentAlternate());
            waterTransitionAlternateEffect_->setNameProperty(
                "Procedural player water transition alternate");

            birdCallEffect_ = CreateOneShotSoundEffect(GenerateBirdCall());
            birdCallEffect_->setNameProperty("Procedural tropical bird call");
            birdCallAlternateEffect_ = CreateOneShotSoundEffect(GenerateBirdCallAlternate());
            birdCallAlternateEffect_->setNameProperty(
                "Procedural tropical bird call alternate");
            monkeyCallEffect_ = CreateOneShotSoundEffect(GenerateMonkeyCall());
            monkeyCallEffect_->setNameProperty("Procedural monkey contact call");
            monkeyCallAlternateEffect_ = CreateOneShotSoundEffect(
                GenerateMonkeyCallAlternate());
            monkeyCallAlternateEffect_->setNameProperty(
                "Procedural monkey contact call alternate");

            // CNA's 3D approximation uses this as the full-volume radius before inverse falloff.
            // Island world units are meters, so 24 m keeps calls local without disappearing.
            SoundEffect::setDistanceScaleProperty(24.0f);

            available_ = true;
            ApplyVolumes();
            if (!available_) {
                throw std::runtime_error(error_.empty() ? "Failed to configure ambient volumes"
                                                        : error_);
            }
            surfInstance_->Play();
            breezeInstance_->Play();
            jungleInstance_->Play();
            waterMovementInstance_->Play();
            if (surfInstance_->getStateProperty() != SoundState::Playing
                || breezeInstance_->getStateProperty() != SoundState::Playing
                || jungleInstance_->getStateProperty() != SoundState::Playing
                || waterMovementInstance_->getStateProperty() != SoundState::Playing) {
                throw std::runtime_error("CNA did not transition all looping tracks to Playing");
            }
            waveScheduler_.Reset();
            waterTransitionScheduler_.Reset(waterDepth, swimming);
            wildlifeScheduler_.Reset();
            waveAccentsEnabled_ = true;
            waterTransitionsEnabled_ = true;
            wildlifeCallsEnabled_ = true;
            waveSecondsRemaining_ = 0.0f;
            waterTransitionSecondsRemaining_ = 0.0f;
            wildlifeSecondsRemaining_ = 0.0f;
            std::cout << "island-simulator: audio sources use sequential generation and upload "
                         "with bounded staging memory\n";
            std::cout << "island-simulator: soundscape started (procedural surf and breeze)\n";
            std::cout << "island-simulator: subtle daytime-jungle layer enabled\n";
            std::cout << "island-simulator: player water-movement layer enabled\n";
            std::cout << "island-simulator: water-transition accents enabled\n";
            std::cout << "island-simulator: two water-transition variants enabled\n";
            std::cout << "island-simulator: sparse stronger-wave accents enabled\n";
            std::cout << "island-simulator: two stronger-wave variants enabled\n";
            std::cout << "island-simulator: positional bird and monkey calls enabled "
                         "(one pooled wildlife voice)\n";
            return true;
        } catch (const std::exception& exception) {
            error_ = exception.what();
            available_ = false;
            breezeInstance_.reset();
            jungleInstance_.reset();
            jungleEffect_.reset();
            waterMovementInstance_.reset();
            waterTransitionInstance_.reset();
            waterTransitionAlternateEffect_.reset();
            waterTransitionEffect_.reset();
            wildlifeInstance_.reset();
            monkeyCallAlternateEffect_.reset();
            monkeyCallEffect_.reset();
            birdCallAlternateEffect_.reset();
            birdCallEffect_.reset();
            waterMovementEffect_.reset();
            waveInstance_.reset();
            waveAlternateEffect_.reset();
            waveEffect_.reset();
            breezeEffect_.reset();
            surfInstance_.reset();
            surfEffect_.reset();
            waveSecondsRemaining_ = 0.0f;
            waterTransitionSecondsRemaining_ = 0.0f;
            wildlifeSecondsRemaining_ = 0.0f;
            activeWildlifeCall_ = {};
            std::cerr << "island-simulator: audio unavailable; continuing silently: "
                      << error_ << '\n';
            return false;
        }
    }

    void Soundscape::Update(
        const float signedShoreDistance, const bool swimming, const float waterDepth,
        const float horizontalSpeed, const float oceanCurrentStrength,
        const float deltaSeconds,
        const SoundscapeListenerState& listenerState,
        const std::span<const BirdState> birds,
        const std::span<const MonkeyState> monkeys) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.25f) : 0.0f;
        waveSecondsRemaining_ = std::max(0.0f, waveSecondsRemaining_ - safeDelta);
        waterTransitionSecondsRemaining_ = std::max(
            0.0f, waterTransitionSecondsRemaining_ - safeDelta);
        wildlifeSecondsRemaining_ = std::max(
            0.0f, wildlifeSecondsRemaining_ - safeDelta);

        if (waveInstance_) {
            try {
                if (waveSecondsRemaining_ <= 0.0f
                    || waveInstance_->getStateProperty() == SoundState::Stopped) {
                    if (waveInstance_->getStateProperty() != SoundState::Stopped) {
                        waveInstance_->Stop();
                    }
                    waveInstance_.reset();
                    waveSecondsRemaining_ = 0.0f;
                }
            } catch (const std::exception& exception) {
                waveInstance_.reset();
                waveSecondsRemaining_ = 0.0f;
                waveAccentsEnabled_ = false;
                if (!waveFailureReported_) {
                    waveFailureReported_ = true;
                    std::cerr << "island-simulator: stronger-wave accents disabled; ambient beds "
                                 "continue: " << exception.what() << '\n';
                }
            }
        }
        if (waterTransitionInstance_) {
            try {
                if (waterTransitionSecondsRemaining_ <= 0.0f
                    || waterTransitionInstance_->getStateProperty() == SoundState::Stopped) {
                    if (waterTransitionInstance_->getStateProperty() != SoundState::Stopped) {
                        waterTransitionInstance_->Stop();
                    }
                    waterTransitionInstance_.reset();
                    waterTransitionSecondsRemaining_ = 0.0f;
                }
            } catch (const std::exception& exception) {
                waterTransitionInstance_.reset();
                waterTransitionSecondsRemaining_ = 0.0f;
                waterTransitionsEnabled_ = false;
                if (!waterTransitionFailureReported_) {
                    waterTransitionFailureReported_ = true;
                    std::cerr << "island-simulator: water-transition accents disabled; "
                                 "continuous layers continue: " << exception.what() << '\n';
                }
            }
        }

        (void)model_.Update(signedShoreDistance, swimming, safeDelta);
        (void)waterMovementModel_.Update(
            waterDepth, swimming, horizontalSpeed, safeDelta, oceanCurrentStrength);
        ApplyVolumes();

        AudioListener listener;
        listener.setPositionProperty(Vector3(
            listenerState.x, listenerState.y, listenerState.z));
        listener.setForwardProperty(Vector3(
            listenerState.forwardX, listenerState.forwardY, listenerState.forwardZ));
        listener.setUpProperty(Vector3::Up);
        listener.setVelocityProperty(Vector3(
            listenerState.velocityX, 0.0f, listenerState.velocityZ));

        if (wildlifeInstance_) {
            try {
                if (wildlifeSecondsRemaining_ <= 0.0f
                    || wildlifeInstance_->getStateProperty() == SoundState::Stopped) {
                    wildlifeInstance_.reset();
                    wildlifeSecondsRemaining_ = 0.0f;
                    activeWildlifeCall_ = {};
                } else if (available_ && !muted_) {
                    AudioEmitter emitter;
                    emitter.setPositionProperty(Vector3(
                        activeWildlifeCall_.source.x, activeWildlifeCall_.source.y,
                        activeWildlifeCall_.source.z));
                    emitter.setDopplerScaleProperty(0.0f);
                    wildlifeInstance_->Apply3D(listener, emitter);
                }
            } catch (const std::exception& exception) {
                wildlifeInstance_.reset();
                wildlifeSecondsRemaining_ = 0.0f;
                activeWildlifeCall_ = {};
                wildlifeCallsEnabled_ = false;
                if (!wildlifeFailureReported_) {
                    wildlifeFailureReported_ = true;
                    std::cerr << "island-simulator: positional wildlife calls disabled; "
                                 "ambient beds continue: " << exception.what() << '\n';
                }
            }
        }

        const WaterTransitionRequest waterTransition = waterTransitionScheduler_.Update(
            waterDepth, swimming, horizontalSpeed, safeDelta);
        SoundEffect* waterTransitionEffect = waterTransition.variant == 1U
            ? waterTransitionAlternateEffect_.get() : waterTransitionEffect_.get();
        if (available_ && !muted_ && waterTransitionsEnabled_
            && waterTransition.type != WaterTransitionType::None && waterTransitionEffect) {
            try {
                if (wildlifeInstance_ && waveInstance_) {
                    wildlifeInstance_->Stop();
                    wildlifeInstance_.reset();
                    wildlifeSecondsRemaining_ = 0.0f;
                    activeWildlifeCall_ = {};
                }
                waterTransitionInstance_ = std::make_unique<SoundEffectInstance>(
                    waterTransitionEffect->CreateInstance());
                waterTransitionInstance_->setVolumeProperty(waterTransition.volume);
                waterTransitionInstance_->setPitchProperty(waterTransition.pitch);
                waterTransitionInstance_->setPanProperty(waterTransition.pan);
                waterTransitionInstance_->Play();
                if (waterTransitionInstance_->getStateProperty() != SoundState::Playing) {
                    throw std::runtime_error(
                        "CNA did not start the water-transition playback instance");
                }
                waterTransitionSecondsRemaining_ = 1.65f;
                ++playedWaterTransitionCount_;
                if (playedWaterTransitionCount_ == 1) {
                    std::cout << "island-simulator: first water-transition accent played\n"
                              << std::flush;
                }
            } catch (const std::exception& exception) {
                waterTransitionInstance_.reset();
                waterTransitionSecondsRemaining_ = 0.0f;
                waterTransitionsEnabled_ = false;
                if (!waterTransitionFailureReported_) {
                    waterTransitionFailureReported_ = true;
                    std::cerr << "island-simulator: water-transition accents disabled; "
                                 "continuous layers continue: " << exception.what() << '\n';
                }
            }
        }

        const WaveAccentRequest accent = waveScheduler_.Update(
            safeDelta, model_.Mix().surfVolume);
        SoundEffect* waveEffect = accent.variant == 1U
            ? waveAlternateEffect_.get() : waveEffect_.get();
        const int activeTransientVoices = (waterTransitionInstance_ ? 1 : 0)
            + (wildlifeInstance_ ? 1 : 0);
        if (available_ && !muted_ && waveAccentsEnabled_ && accent.trigger && waveEffect
            && activeTransientVoices < 2) {
            try {
                waveInstance_ = std::make_unique<SoundEffectInstance>(
                    waveEffect->CreateInstance());
                waveInstance_->setVolumeProperty(accent.volume);
                waveInstance_->setPitchProperty(accent.pitch);
                waveInstance_->setPanProperty(accent.pan);
                waveInstance_->Play();
                if (waveInstance_->getStateProperty() != SoundState::Playing) {
                    throw std::runtime_error(
                        "CNA did not start the stronger-wave playback instance");
                }
                waveSecondsRemaining_ = 4.25f;
                ++playedWaveAccentCount_;
                if (playedWaveAccentCount_ == 1) {
                    std::cout << "island-simulator: first stronger-wave accent played\n"
                              << std::flush;
                }
            } catch (const std::exception& exception) {
                waveInstance_.reset();
                waveSecondsRemaining_ = 0.0f;
                waveAccentsEnabled_ = false;
                if (!waveFailureReported_) {
                    waveFailureReported_ = true;
                    std::cerr << "island-simulator: stronger-wave accents disabled; ambient beds "
                                 "continue: " << exception.what() << '\n';
                }
            }
        }

        RefreshDebugSources(listenerState.x, listenerState.y, listenerState.z,
                            birds, monkeys);
        const bool wildlifeSlotAvailable = available_ && !muted_ && wildlifeCallsEnabled_
            && !wildlifeInstance_ && !waveInstance_ && !waterTransitionInstance_;
        const WildlifeCallRequest wildlifeCall = wildlifeScheduler_.Update(
            safeDelta, swimming, signedShoreDistance, listenerState.x, listenerState.y,
            listenerState.z, nearestBirdSource_, nearestMonkeySource_, wildlifeSlotAvailable);
        if (wildlifeCall.type == WildlifeCallType::None) {
            return;
        }

        SoundEffect* wildlifeEffect = nullptr;
        if (wildlifeCall.type == WildlifeCallType::Bird) {
            wildlifeEffect = wildlifeCall.variant == 1U
                ? birdCallAlternateEffect_.get() : birdCallEffect_.get();
        } else {
            wildlifeEffect = wildlifeCall.variant == 1U
                ? monkeyCallAlternateEffect_.get() : monkeyCallEffect_.get();
        }
        if (!wildlifeEffect) {
            return;
        }

        try {
            wildlifeInstance_ = std::make_unique<SoundEffectInstance>(
                wildlifeEffect->CreateInstance());
            wildlifeInstance_->setVolumeProperty(wildlifeCall.volume);
            wildlifeInstance_->setPitchProperty(wildlifeCall.pitch);
            AudioEmitter emitter;
            emitter.setPositionProperty(Vector3(
                wildlifeCall.source.x, wildlifeCall.source.y, wildlifeCall.source.z));
            emitter.setDopplerScaleProperty(0.0f);
            wildlifeInstance_->Apply3D(listener, emitter);
            wildlifeInstance_->Play();
            activeWildlifeCall_ = wildlifeCall;
            wildlifeSecondsRemaining_ = wildlifeCall.type == WildlifeCallType::Monkey
                ? 2.35f : 1.50f;
            if (wildlifeCall.type == WildlifeCallType::Bird) {
                ++playedBirdCallCount_;
                if (playedBirdCallCount_ == 1) {
                    std::cout << "island-simulator: first positional bird call played\n"
                              << std::flush;
                }
            } else {
                ++playedMonkeyCallCount_;
                if (playedMonkeyCallCount_ == 1) {
                    std::cout << "island-simulator: first positional monkey call played\n"
                              << std::flush;
                }
            }
        } catch (const std::exception& exception) {
            wildlifeInstance_.reset();
            wildlifeSecondsRemaining_ = 0.0f;
            activeWildlifeCall_ = {};
            wildlifeCallsEnabled_ = false;
            if (!wildlifeFailureReported_) {
                wildlifeFailureReported_ = true;
                std::cerr << "island-simulator: positional wildlife calls disabled; "
                             "ambient beds continue: " << exception.what() << '\n';
            }
        }
    }

    void Soundscape::ToggleMuted() noexcept
    {
        muted_ = !muted_;
        if (muted_) {
            const auto stopTransient = [](auto& instance) noexcept {
                if (instance) {
                    try {
                        instance->Stop();
                    } catch (const std::exception&) {
                        // Muting must remain reliable even if a device vanished between frames.
                    }
                    instance.reset();
                }
            };
            stopTransient(waveInstance_);
            stopTransient(waterTransitionInstance_);
            stopTransient(wildlifeInstance_);
            waveSecondsRemaining_ = 0.0f;
            waterTransitionSecondsRemaining_ = 0.0f;
            wildlifeSecondsRemaining_ = 0.0f;
            activeWildlifeCall_ = {};
        }
        ApplyVolumes();
        std::cout << "island-simulator: soundscape " << (muted_ ? "muted" : "unmuted") << '\n';
    }

    void Soundscape::ApplyVolumes() noexcept
    {
        if (!available_ || !surfInstance_ || !breezeInstance_ || !jungleInstance_
            || !waterMovementInstance_) {
            return;
        }
        try {
            const float gain = muted_ ? 0.0f : 1.0f;
            surfInstance_->setVolumeProperty(model_.Mix().surfVolume * gain);
            breezeInstance_->setVolumeProperty(model_.Mix().breezeVolume * gain);
            jungleInstance_->setVolumeProperty(model_.Mix().jungleVolume * gain);
            waterMovementInstance_->setVolumeProperty(
                waterMovementModel_.Mix().volume * gain);
            waterMovementInstance_->setPitchProperty(waterMovementModel_.Mix().pitch);
        } catch (const std::exception& exception) {
            DisableAfterRuntimeFailure(exception.what());
        }
    }

    void Soundscape::DisableAfterRuntimeFailure(const std::string& message) noexcept
    {
        error_ = message;
        available_ = false;
        breezeInstance_.reset();
        jungleInstance_.reset();
        jungleEffect_.reset();
        waterMovementInstance_.reset();
        waterTransitionInstance_.reset();
        waterTransitionAlternateEffect_.reset();
        waterTransitionEffect_.reset();
        wildlifeInstance_.reset();
        monkeyCallAlternateEffect_.reset();
        monkeyCallEffect_.reset();
        birdCallAlternateEffect_.reset();
        birdCallEffect_.reset();
        waterMovementEffect_.reset();
        waveInstance_.reset();
        waveAlternateEffect_.reset();
        waveEffect_.reset();
        breezeEffect_.reset();
        surfInstance_.reset();
        surfEffect_.reset();
        waveSecondsRemaining_ = 0.0f;
        waterTransitionSecondsRemaining_ = 0.0f;
        wildlifeSecondsRemaining_ = 0.0f;
        activeWildlifeCall_ = {};
        std::cerr << "island-simulator: audio stopped after a runtime failure; "
                     "continuing silently: " << error_ << '\n';
    }
}
