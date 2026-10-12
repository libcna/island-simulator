#include "IslandSimulator/Soundscape.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "IslandSimulator/RuntimeDiagnostics.hpp"
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
        constexpr std::string_view BoundedStagingDiagnostic =
            "island-simulator: audio sources use sequential generation and upload with bounded "
            "staging memory";
        constexpr std::string_view DcCenteringDiagnostic =
            "island-simulator: persistent ambient PCM is DC-centered before quantization";
        constexpr std::string_view AmbientVariationDiagnostic =
            "island-simulator: ambient beds use slow non-amplifying gain drift";
        constexpr std::string_view CoastalWindMixDiagnostic =
            "island-simulator: surf and breeze gains follow bounded coastal wind exposure";
        constexpr std::string_view StreamSoundDiagnostic =
            "island-simulator: positional stream loop follows the nearest shared-route point";
        constexpr std::string_view FootstepEnabledDiagnostic =
            "island-simulator: distance-scheduled sand and soil footsteps enabled";
        constexpr std::string_view FootstepPlaybackDiagnostic =
            "island-simulator: first distance-scheduled land footstep played";
        constexpr std::string_view WaterTransitionPlaybackDiagnostic =
            "island-simulator: first water-transition accent played";

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
                           const float oceanCurrentStrength,
                           const float coastalWindExposure,
                           const float playerX, const float playerZ) noexcept
    {
        if (startAttempted_) {
            return available_;
        }
        startAttempted_ = true;
        streamSound_ = StreamSoundField::Sample(playerX, playerZ);
        model_.Reset(signedShoreDistance, swimming, coastalWindExposure,
                     streamSound_.presence);
        waterMovementModel_.Reset(
            waterDepth, swimming, horizontalSpeed, oceanCurrentStrength);
        try {
            // Generate, upload, and release one source at a time. Retaining every PCM vector
            // plus every byte copy here raised transient startup memory for no
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

            streamEffect_ = CreateLoopedSoundEffect(GenerateStreamLoop());
            streamEffect_->setNameProperty("Procedural flowing stream");
            streamInstance_ = std::make_unique<SoundEffectInstance>(
                streamEffect_->CreateInstance());
            streamInstance_->setIsLoopedProperty(true);

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

            sandFootstepEffect_ = CreateOneShotSoundEffect(GenerateSandFootstep());
            sandFootstepEffect_->setNameProperty("Procedural dry-sand footstep");
            sandFootstepAlternateEffect_ = CreateOneShotSoundEffect(
                GenerateSandFootstepAlternate());
            sandFootstepAlternateEffect_->setNameProperty(
                "Procedural dry-sand footstep alternate");
            soilFootstepEffect_ = CreateOneShotSoundEffect(GenerateSoilFootstep());
            soilFootstepEffect_->setNameProperty("Procedural interior-soil footstep");
            soilFootstepAlternateEffect_ = CreateOneShotSoundEffect(
                GenerateSoilFootstepAlternate());
            soilFootstepAlternateEffect_->setNameProperty(
                "Procedural interior-soil footstep alternate");

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
            for (std::size_t i = 0; i < boarEffects_.size(); ++i) {
                boarEffects_[i] = CreateOneShotSoundEffect(GenerateBoarCall(
                    static_cast<BoarCallKind>(1U + i / 2U), static_cast<std::uint8_t>(i % 2U)));
                boarEffects_[i]->setNameProperty("Original procedural boar voice " + std::to_string(i));
            }
            for (std::size_t i = 0; i < snakeHissEffects_.size(); ++i) {
                snakeHissEffects_[i] = CreateOneShotSoundEffect(
                    GenerateSnakeHiss(static_cast<std::uint8_t>(i)));
                snakeHissEffects_[i]->setNameProperty(
                    "Original procedural snake hiss " + std::to_string(i));
            }

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
            streamPlaybackActive_ = StreamSoundField::ShouldUseStreamPlayback(
                swimming, streamSound_.presence, false);
            if (streamPlaybackActive_) {
                streamInstance_->Play();
            } else {
                waterMovementInstance_->Play();
            }
            if (surfInstance_->getStateProperty() != SoundState::Playing
                || breezeInstance_->getStateProperty() != SoundState::Playing
                || jungleInstance_->getStateProperty() != SoundState::Playing
                || (streamPlaybackActive_
                    ? streamInstance_->getStateProperty() != SoundState::Playing
                    : waterMovementInstance_->getStateProperty() != SoundState::Playing)) {
                throw std::runtime_error("CNA did not transition all looping tracks to Playing");
            }
            waveScheduler_.Reset();
            waterTransitionScheduler_.Reset(waterDepth, swimming);
            footstepScheduler_.Reset();
            wildlifeScheduler_.Reset();
            boarScheduler_.Reset();
            snakeHissScheduler_.Reset();
            waveAccentsEnabled_ = true;
            waterTransitionsEnabled_ = true;
            footstepsEnabled_ = true;
            wildlifeCallsEnabled_ = true;
            waveSecondsRemaining_ = 0.0f;
            waterTransitionSecondsRemaining_ = 0.0f;
            footstepSecondsRemaining_ = 0.0f;
            wildlifeSecondsRemaining_ = 0.0f;
            std::cout << BoundedStagingDiagnostic << '\n';
            std::cout << "island-simulator: soundscape started (procedural surf and breeze)\n";
            std::cout << DcCenteringDiagnostic << '\n';
            std::cout << AmbientVariationDiagnostic << '\n';
            std::cout << CoastalWindMixDiagnostic << '\n';
            std::cout << "island-simulator: subtle daytime-jungle layer enabled\n";
            std::cout << StreamSoundDiagnostic << '\n';
            std::cout << "island-simulator: player water-movement layer enabled\n";
            std::cout << "island-simulator: water-transition accents enabled\n";
            std::cout << "island-simulator: two water-transition variants enabled\n";
            std::cout << FootstepEnabledDiagnostic << '\n';
            std::cout << "island-simulator: sparse stronger-wave accents enabled\n";
            std::cout << "island-simulator: two stronger-wave variants enabled\n";
            std::cout << "island-simulator: positional bird and monkey calls enabled "
                         "(one pooled wildlife voice)\n";
            std::cout << "island-simulator: positional boar breath/snuffle/grunt enabled "
                         "(shared wildlife voice, six original clips)\n";
            std::cout << "island-simulator: positional close-retreat snake hissing enabled "
                         "(shared wildlife voice, two original clips)\n";
#ifdef __ANDROID__
            // SDLActivity does not expose stdout through logcat. Mirror only the bounded set
            // consumed by Android runtime qualification without adding no-op calls elsewhere.
            MirrorRuntimeDiagnosticToPlatformLog(BoundedStagingDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(DcCenteringDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(AmbientVariationDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(CoastalWindMixDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(StreamSoundDiagnostic);
            MirrorRuntimeDiagnosticToPlatformLog(FootstepEnabledDiagnostic);
#endif
            return true;
        } catch (const std::exception& exception) {
            error_ = exception.what();
            available_ = false;
            breezeInstance_.reset();
            jungleInstance_.reset();
            jungleEffect_.reset();
            streamInstance_.reset();
            streamEffect_.reset();
            streamPlaybackActive_ = false;
            waterMovementInstance_.reset();
            waterTransitionInstance_.reset();
            footstepInstance_.reset();
            soilFootstepAlternateEffect_.reset();
            soilFootstepEffect_.reset();
            sandFootstepAlternateEffect_.reset();
            sandFootstepEffect_.reset();
            waterTransitionAlternateEffect_.reset();
            waterTransitionEffect_.reset();
            wildlifeInstance_.reset();
            monkeyCallAlternateEffect_.reset();
            for (auto& effect : boarEffects_) effect.reset();
            for (auto& effect : snakeHissEffects_) effect.reset();
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
            footstepSecondsRemaining_ = 0.0f;
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
        const float deltaSeconds,const float coastalWindExposure,
        const SoundscapeListenerState& listenerState,
        const std::span<const BirdState> birds,
        const std::span<const MonkeyState> monkeys,
        const std::span<const BoarState> boars,
        const std::span<const SnakeState> snakes) noexcept
    {
        const float safeDelta = std::isfinite(deltaSeconds)
            ? std::clamp(deltaSeconds, 0.0f, 0.25f) : 0.0f;
        waveSecondsRemaining_ = std::max(0.0f, waveSecondsRemaining_ - safeDelta);
        waterTransitionSecondsRemaining_ = std::max(
            0.0f, waterTransitionSecondsRemaining_ - safeDelta);
        footstepSecondsRemaining_ = std::max(
            0.0f, footstepSecondsRemaining_ - safeDelta);
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
        if (footstepInstance_) {
            try {
                if (footstepSecondsRemaining_ <= 0.0f
                    || footstepInstance_->getStateProperty() == SoundState::Stopped) {
                    if (footstepInstance_->getStateProperty() != SoundState::Stopped) {
                        footstepInstance_->Stop();
                    }
                    footstepInstance_.reset();
                    footstepSecondsRemaining_ = 0.0f;
                }
            } catch (const std::exception& exception) {
                footstepInstance_.reset();
                footstepSecondsRemaining_ = 0.0f;
                footstepsEnabled_ = false;
                if (!footstepFailureReported_) {
                    footstepFailureReported_ = true;
                    std::cerr << "island-simulator: land footsteps disabled; ambient beds "
                                 "continue: " << exception.what() << '\n';
                }
            }
        }

        streamSound_ = StreamSoundField::Sample(listenerState.x, listenerState.z);
        (void)model_.Update(signedShoreDistance, swimming, safeDelta,
                            coastalWindExposure, streamSound_.presence);
        (void)waterMovementModel_.Update(
            waterDepth, swimming, horizontalSpeed, safeDelta, oceanCurrentStrength);
        const bool wantsStreamPlayback = StreamSoundField::ShouldUseStreamPlayback(
            swimming, streamSound_.presence, streamPlaybackActive_);
        if (available_ && wantsStreamPlayback != streamPlaybackActive_) {
            try {
                if (wantsStreamPlayback) {
                    waterMovementInstance_->Stop();
                    streamInstance_->Play();
                } else {
                    streamInstance_->Stop();
                    waterMovementInstance_->Play();
                }
                streamPlaybackActive_ = wantsStreamPlayback;
            } catch (const std::exception& exception) {
                DisableAfterRuntimeFailure(exception.what());
                return;
            }
        }
        ApplyVolumes();

        AudioListener listener;
        listener.setPositionProperty(Vector3(
            listenerState.x, listenerState.y, listenerState.z));
        listener.setForwardProperty(Vector3(
            listenerState.forwardX, listenerState.forwardY, listenerState.forwardZ));
        listener.setUpProperty(Vector3::Up);
        listener.setVelocityProperty(Vector3(
            listenerState.velocityX, 0.0f, listenerState.velocityZ));

        if (available_ && streamPlaybackActive_ && streamInstance_ && streamSound_.valid) {
            try {
                AudioEmitter emitter;
                emitter.setPositionProperty(Vector3(
                    streamSound_.x, listenerState.y, streamSound_.z));
                emitter.setDopplerScaleProperty(0.0f);
                streamInstance_->Apply3D(listener, emitter);
            } catch (const std::exception& exception) {
                DisableAfterRuntimeFailure(exception.what());
                return;
            }
        }

        if (wildlifeInstance_) {
            try {
                if (wildlifeSecondsRemaining_ <= 0.0f
                    || wildlifeInstance_->getStateProperty() == SoundState::Stopped) {
                    wildlifeInstance_.reset();
                    wildlifeSecondsRemaining_ = 0.0f;
                    activeWildlifeCall_ = {};
                } else if (available_ && !muted_) {
                    if (activeWildlifeCall_.type == WildlifeCallType::Boar) {
                        if (activeBoarCall_.animalIndex >= boars.size()) {
                            wildlifeInstance_->Stop();
                            wildlifeSecondsRemaining_ = 0.0f;
                        } else {
                            const BoarState& boar = boars[activeBoarCall_.animalIndex];
                            const float y = boar.y + 0.55f * boar.scale;
                            const float distance = std::hypot(std::hypot(boar.x - listenerState.x,
                                boar.z - listenerState.z), y - listenerState.y);
                            if (!std::isfinite(distance) || boar.simulationTier == WildlifeSimulationTier::Dormant) {
                                wildlifeInstance_->Stop();
                                wildlifeSecondsRemaining_ = 0.0f;
                            } else {
                                activeWildlifeCall_.source = {true, boar.x, y, boar.z};
                                wildlifeInstance_->setVolumeProperty(boarVoiceGain_
                                    * BoarCallVolume(activeBoarCall_.kind, distance));
                            }
                        }
                    } else if (activeWildlifeCall_.type == WildlifeCallType::Snake) {
                        if (activeSnakeHiss_.animalIndex >= snakes.size()) {
                            wildlifeInstance_->Stop();
                            wildlifeSecondsRemaining_ = 0.0f;
                        } else {
                            const SnakeState& snake = snakes[activeSnakeHiss_.animalIndex];
                            const SnakePoint head = snake.body.front();
                            const float distance = std::hypot(std::hypot(head.x-listenerState.x,
                                head.z-listenerState.z),head.y-listenerState.y);
                            if (!std::isfinite(distance)
                                || snake.tier == WildlifeSimulationTier::Dormant
                                || snake.behavior != SnakeBehavior::Retreat) {
                                wildlifeInstance_->Stop();
                                wildlifeSecondsRemaining_ = 0.0f;
                            } else {
                                activeWildlifeCall_.source = {true,head.x,head.y,head.z};
                                wildlifeInstance_->setVolumeProperty(snakeVoiceGain_
                                    * SnakeHissVolume(distance));
                            }
                        }
                    }
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
                if (waterTransitionInstance_) {
                    waterTransitionInstance_->Stop();
                    waterTransitionInstance_.reset();
                    waterTransitionSecondsRemaining_ = 0.0f;
                }
                int activeLowerPriorityVoices = static_cast<int>(waveInstance_ != nullptr)
                    + static_cast<int>(footstepInstance_ != nullptr)
                    + static_cast<int>(wildlifeInstance_ != nullptr);
                if (activeLowerPriorityVoices >= 2 && wildlifeInstance_) {
                    wildlifeInstance_->Stop();
                    wildlifeInstance_.reset();
                    wildlifeSecondsRemaining_ = 0.0f;
                    activeWildlifeCall_ = {};
                    --activeLowerPriorityVoices;
                }
                if (activeLowerPriorityVoices >= 2 && waveInstance_) {
                    waveInstance_->Stop();
                    waveInstance_.reset();
                    waveSecondsRemaining_ = 0.0f;
                    --activeLowerPriorityVoices;
                }
                if (activeLowerPriorityVoices >= 2 && footstepInstance_) {
                    footstepInstance_->Stop();
                    footstepInstance_.reset();
                    footstepSecondsRemaining_ = 0.0f;
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
                    std::cout << WaterTransitionPlaybackDiagnostic << '\n' << std::flush;
#ifdef __ANDROID__
                    MirrorRuntimeDiagnosticToPlatformLog(WaterTransitionPlaybackDiagnostic);
#endif
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

        const FootstepRequest footstep = footstepScheduler_.Update(
            safeDelta, swimming, signedShoreDistance,
            std::hypot(listenerState.velocityX, listenerState.velocityZ),
            listenerState.x, listenerState.z);
        SoundEffect* footstepEffect = nullptr;
        if (footstep.surface == FootstepSurface::Sand) {
            footstepEffect = footstep.variant == 1U
                ? sandFootstepAlternateEffect_.get() : sandFootstepEffect_.get();
        } else if (footstep.surface == FootstepSurface::Soil) {
            footstepEffect = footstep.variant == 1U
                ? soilFootstepAlternateEffect_.get() : soilFootstepEffect_.get();
        }
        if (available_ && !muted_ && footstepsEnabled_ && footstepEffect) {
            const int activeOtherTransientVoices =
                static_cast<int>(waterTransitionInstance_ != nullptr)
                + static_cast<int>(waveInstance_ != nullptr)
                + static_cast<int>(wildlifeInstance_ != nullptr);
            if (activeOtherTransientVoices < 2) {
                try {
                    if (footstepInstance_) {
                        footstepInstance_->Stop();
                        footstepInstance_.reset();
                    }
                    footstepInstance_ = std::make_unique<SoundEffectInstance>(
                        footstepEffect->CreateInstance());
                    footstepInstance_->setVolumeProperty(footstep.volume);
                    footstepInstance_->setPitchProperty(footstep.pitch);
                    footstepInstance_->setPanProperty(footstep.pan);
                    footstepInstance_->Play();
                    if (footstepInstance_->getStateProperty() != SoundState::Playing) {
                        throw std::runtime_error(
                            "CNA did not start the land-footstep playback instance");
                    }
                    footstepSecondsRemaining_ = 0.34f;
                    ++playedFootstepCount_;
                    if (playedFootstepCount_ == 1) {
                        std::cout << FootstepPlaybackDiagnostic << '\n' << std::flush;
#ifdef __ANDROID__
                        MirrorRuntimeDiagnosticToPlatformLog(FootstepPlaybackDiagnostic);
#endif
                    }
                } catch (const std::exception& exception) {
                    footstepInstance_.reset();
                    footstepSecondsRemaining_ = 0.0f;
                    footstepsEnabled_ = false;
                    if (!footstepFailureReported_) {
                        footstepFailureReported_ = true;
                        std::cerr << "island-simulator: land footsteps disabled; ambient beds "
                                     "continue: " << exception.what() << '\n';
                    }
                }
            }
        }

        const WaveAccentRequest accent = waveScheduler_.Update(
            safeDelta, model_.Mix().surfVolume);
        SoundEffect* waveEffect = accent.variant == 1U
            ? waveAlternateEffect_.get() : waveEffect_.get();
        const int activeTransientVoices = (waterTransitionInstance_ ? 1 : 0)
            + (footstepInstance_ ? 1 : 0)
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
        WildlifeCallRequest wildlifeCall = wildlifeScheduler_.Update(
            safeDelta, swimming, signedShoreDistance, listenerState.x, listenerState.y,
            listenerState.z, nearestBirdSource_, nearestMonkeySource_, wildlifeSlotAvailable);
        // Due birds/monkeys get first refusal. A nearby retreating snake then gets
        // priority over boars; all species share the same single transient voice.
        const SnakeHissRequest snakeHiss = snakeHissScheduler_.Update(safeDelta,
            listenerState.x,listenerState.y,listenerState.z,snakes,
            wildlifeSlotAvailable && wildlifeCall.type == WildlifeCallType::None);
        if (snakeHiss.trigger) {
            wildlifeCall = {WildlifeCallType::Snake,snakeHiss.variant,
                snakeHiss.volume,snakeHiss.pitch,snakeHiss.source};
        }
        const BoarCallRequest boarCall = boarScheduler_.Update(safeDelta,
            listenerState.x, listenerState.y, listenerState.z, boars,
            wildlifeSlotAvailable && wildlifeCall.type == WildlifeCallType::None);
        if (boarCall.kind != BoarCallKind::None) {
            wildlifeCall = {WildlifeCallType::Boar, boarCall.variant,
                boarCall.volume, boarCall.pitch, boarCall.source};
        }
        if (wildlifeCall.type == WildlifeCallType::None) {
            return;
        }

        SoundEffect* wildlifeEffect = nullptr;
        if (wildlifeCall.type == WildlifeCallType::Bird) {
            wildlifeEffect = wildlifeCall.variant == 1U
                ? birdCallAlternateEffect_.get() : birdCallEffect_.get();
        } else if (wildlifeCall.type == WildlifeCallType::Monkey) {
            wildlifeEffect = wildlifeCall.variant == 1U
                ? monkeyCallAlternateEffect_.get() : monkeyCallEffect_.get();
        } else if (wildlifeCall.type == WildlifeCallType::Boar) {
            const std::size_t index = (static_cast<std::size_t>(boarCall.kind) - 1U) * 2U
                + boarCall.variant;
            wildlifeEffect = boarEffects_.at(index).get();
        } else if (wildlifeCall.type == WildlifeCallType::Snake) {
            wildlifeEffect = snakeHissEffects_.at(wildlifeCall.variant).get();
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
            if (wildlifeInstance_->getStateProperty() != SoundState::Playing) {
                throw std::runtime_error("Wildlife voice did not enter Playing state");
            }
            activeWildlifeCall_ = wildlifeCall;
            if (wildlifeCall.type == WildlifeCallType::Boar) {
                activeBoarCall_ = boarCall;
                const float distance = std::hypot(std::hypot(boarCall.source.x - listenerState.x,
                    boarCall.source.z - listenerState.z), boarCall.source.y - listenerState.y);
                boarVoiceGain_ = boarCall.volume / std::max(0.0001f, BoarCallVolume(boarCall.kind, distance));
            } else if (wildlifeCall.type == WildlifeCallType::Snake) {
                activeSnakeHiss_ = snakeHiss;
                const float distance = std::hypot(std::hypot(snakeHiss.source.x-listenerState.x,
                    snakeHiss.source.z-listenerState.z),snakeHiss.source.y-listenerState.y);
                snakeVoiceGain_ = snakeHiss.volume
                    / std::max(.0001f,SnakeHissVolume(distance));
            }
            wildlifeSecondsRemaining_ = wildlifeCall.type == WildlifeCallType::Monkey
                ? 2.35f : wildlifeCall.type == WildlifeCallType::Snake ? 1.05f : 1.50f;
            if (wildlifeCall.type == WildlifeCallType::Bird) {
                ++playedBirdCallCount_;
                if (playedBirdCallCount_ == 1) {
                    std::cout << "island-simulator: first positional bird call played\n"
                              << std::flush;
                }
            } else if (wildlifeCall.type == WildlifeCallType::Monkey) {
                ++playedMonkeyCallCount_;
                if (playedMonkeyCallCount_ == 1) {
                    std::cout << "island-simulator: first positional monkey call played\n"
                              << std::flush;
                }
            } else if (wildlifeCall.type == WildlifeCallType::Boar) {
                ++playedBoarCallCount_;
                if (playedBoarCallCount_ == 1) {
                    std::cout << "island-simulator: first positional boar call played; animal "
                              << boarCall.animalIndex << " kind " << static_cast<int>(boarCall.kind)
                              << '\n' << std::flush;
                }
            } else {
                ++playedSnakeHissCount_;
                if (playedSnakeHissCount_ == 1) {
                    std::cout << "island-simulator: first positional snake hiss played; animal "
                              << snakeHiss.animalIndex << '\n' << std::flush;
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
        SetMuted(!muted_);
    }

    void Soundscape::SetMuted(const bool muted) noexcept
    {
        muted_ = muted;
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
            stopTransient(footstepInstance_);
            stopTransient(wildlifeInstance_);
            waveSecondsRemaining_ = 0.0f;
            waterTransitionSecondsRemaining_ = 0.0f;
            footstepSecondsRemaining_ = 0.0f;
            wildlifeSecondsRemaining_ = 0.0f;
            activeWildlifeCall_ = {};
        }
        ApplyVolumes();
        std::cout << "island-simulator: soundscape " << (muted_ ? "muted" : "unmuted") << '\n';
    }

    void Soundscape::ApplyVolumes() noexcept
    {
        if (!available_ || !surfInstance_ || !breezeInstance_ || !jungleInstance_
            || !streamInstance_ || !waterMovementInstance_) {
            return;
        }
        try {
            const float gain = muted_ ? 0.0f : 1.0f;
            surfInstance_->setVolumeProperty(model_.Mix().surfVolume * gain);
            breezeInstance_->setVolumeProperty(model_.Mix().breezeVolume * gain);
            jungleInstance_->setVolumeProperty(model_.Mix().jungleVolume * gain);
            streamInstance_->setVolumeProperty(model_.Mix().streamVolume * gain);
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
        streamInstance_.reset();
        streamEffect_.reset();
        streamPlaybackActive_ = false;
        waterMovementInstance_.reset();
        waterTransitionInstance_.reset();
        footstepInstance_.reset();
        soilFootstepAlternateEffect_.reset();
        soilFootstepEffect_.reset();
        sandFootstepAlternateEffect_.reset();
        sandFootstepEffect_.reset();
        waterTransitionAlternateEffect_.reset();
        waterTransitionEffect_.reset();
        wildlifeInstance_.reset();
        monkeyCallAlternateEffect_.reset();
        for (auto& effect : boarEffects_) effect.reset();
        for (auto& effect : snakeHissEffects_) effect.reset();
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
        footstepSecondsRemaining_ = 0.0f;
        wildlifeSecondsRemaining_ = 0.0f;
        activeWildlifeCall_ = {};
        std::cerr << "island-simulator: audio stopped after a runtime failure; "
                     "continuing silently: " << error_ << '\n';
    }
}
