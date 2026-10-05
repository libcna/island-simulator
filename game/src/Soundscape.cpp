#include "IslandSimulator/Soundscape.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Microsoft/Xna/Framework/Audio/AudioChannels.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "SharpRuntime/SharpRuntimeHelper.hpp"

using namespace Microsoft::Xna::Framework::Audio;

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
    }

    Soundscape::Soundscape() = default;

    Soundscape::~Soundscape() = default;

    bool Soundscape::Start(const float signedShoreDistance, const bool swimming,
                           const float waterDepth, const float horizontalSpeed) noexcept
    {
        if (startAttempted_) {
            return available_;
        }
        startAttempted_ = true;
        model_.Reset(signedShoreDistance, swimming);
        waterMovementModel_.Reset(waterDepth, swimming, horizontalSpeed);
        try {
            const AmbientLoop surf = GenerateSurfLoop();
            const AmbientLoop breeze = GenerateBreezeLoop();
            const AmbientLoop jungle = GenerateJungleLoop();
            const AmbientLoop wave = GenerateWaveAccent();
            const AmbientLoop waveAlternate = GenerateWaveAccentAlternate();
            const AmbientLoop waterMovement = GenerateWaterMovementLoop();
            const AmbientLoop waterTransition = GenerateWaterTransitionAccent();
            const AmbientLoop waterTransitionAlternate =
                GenerateWaterTransitionAccentAlternate();
            const std::vector<SharpRuntime::bytecs> surfBytes = ToPcmBytes(surf);
            const std::vector<SharpRuntime::bytecs> breezeBytes = ToPcmBytes(breeze);
            const std::vector<SharpRuntime::bytecs> jungleBytes = ToPcmBytes(jungle);
            const std::vector<SharpRuntime::bytecs> waveBytes = ToPcmBytes(wave);
            const std::vector<SharpRuntime::bytecs> waveAlternateBytes =
                ToPcmBytes(waveAlternate);
            const std::vector<SharpRuntime::bytecs> waterMovementBytes = ToPcmBytes(waterMovement);
            const std::vector<SharpRuntime::bytecs> waterTransitionBytes =
                ToPcmBytes(waterTransition);
            const std::vector<SharpRuntime::bytecs> waterTransitionAlternateBytes =
                ToPcmBytes(waterTransitionAlternate);

            surfEffect_ = std::make_unique<SoundEffect>(
                surfBytes, 0, static_cast<SharpRuntime::intcs>(surfBytes.size()), surf.sampleRate,
                AudioChannels::Mono, 0, static_cast<SharpRuntime::intcs>(surf.samples.size()));
            surfEffect_->setNameProperty("Procedural shoreline surf");
            surfInstance_ = std::make_unique<SoundEffectInstance>(surfEffect_->CreateInstance());
            surfInstance_->setIsLoopedProperty(true);

            breezeEffect_ = std::make_unique<SoundEffect>(
                breezeBytes, 0, static_cast<SharpRuntime::intcs>(breezeBytes.size()),
                breeze.sampleRate, AudioChannels::Mono, 0,
                static_cast<SharpRuntime::intcs>(breeze.samples.size()));
            breezeEffect_->setNameProperty("Procedural canopy breeze");
            breezeInstance_ = std::make_unique<SoundEffectInstance>(
                breezeEffect_->CreateInstance());
            breezeInstance_->setIsLoopedProperty(true);

            jungleEffect_ = std::make_unique<SoundEffect>(
                jungleBytes, 0, static_cast<SharpRuntime::intcs>(jungleBytes.size()),
                jungle.sampleRate, AudioChannels::Mono, 0,
                static_cast<SharpRuntime::intcs>(jungle.samples.size()));
            jungleEffect_->setNameProperty("Procedural daytime jungle");
            jungleInstance_ = std::make_unique<SoundEffectInstance>(
                jungleEffect_->CreateInstance());
            jungleInstance_->setIsLoopedProperty(true);

            waveEffect_ = std::make_unique<SoundEffect>(
                waveBytes, wave.sampleRate, AudioChannels::Mono);
            waveEffect_->setNameProperty("Procedural stronger wave");
            waveAlternateEffect_ = std::make_unique<SoundEffect>(
                waveAlternateBytes, waveAlternate.sampleRate, AudioChannels::Mono);
            waveAlternateEffect_->setNameProperty("Procedural stronger wave alternate");

            waterMovementEffect_ = std::make_unique<SoundEffect>(
                waterMovementBytes, 0,
                static_cast<SharpRuntime::intcs>(waterMovementBytes.size()),
                waterMovement.sampleRate, AudioChannels::Mono, 0,
                static_cast<SharpRuntime::intcs>(waterMovement.samples.size()));
            waterMovementEffect_->setNameProperty("Procedural player water movement");
            waterMovementInstance_ = std::make_unique<SoundEffectInstance>(
                waterMovementEffect_->CreateInstance());
            waterMovementInstance_->setIsLoopedProperty(true);

            waterTransitionEffect_ = std::make_unique<SoundEffect>(
                waterTransitionBytes, waterTransition.sampleRate, AudioChannels::Mono);
            waterTransitionEffect_->setNameProperty("Procedural player water transition");
            waterTransitionAlternateEffect_ = std::make_unique<SoundEffect>(
                waterTransitionAlternateBytes, waterTransitionAlternate.sampleRate,
                AudioChannels::Mono);
            waterTransitionAlternateEffect_->setNameProperty(
                "Procedural player water transition alternate");

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
            waveAccentsEnabled_ = true;
            waterTransitionsEnabled_ = true;
            std::cout << "island-simulator: soundscape started (procedural surf and breeze)\n";
            std::cout << "island-simulator: subtle daytime-jungle layer enabled\n";
            std::cout << "island-simulator: player water-movement layer enabled\n";
            std::cout << "island-simulator: water-transition accents enabled\n";
            std::cout << "island-simulator: two water-transition variants enabled\n";
            std::cout << "island-simulator: sparse stronger-wave accents enabled\n";
            std::cout << "island-simulator: two stronger-wave variants enabled\n";
            return true;
        } catch (const std::exception& exception) {
            error_ = exception.what();
            available_ = false;
            breezeInstance_.reset();
            jungleInstance_.reset();
            jungleEffect_.reset();
            waterMovementInstance_.reset();
            waterTransitionAlternateEffect_.reset();
            waterTransitionEffect_.reset();
            waterMovementEffect_.reset();
            waveAlternateEffect_.reset();
            waveEffect_.reset();
            breezeEffect_.reset();
            surfInstance_.reset();
            surfEffect_.reset();
            std::cerr << "island-simulator: audio unavailable; continuing silently: "
                      << error_ << '\n';
            return false;
        }
    }

    void Soundscape::Update(const float signedShoreDistance, const bool swimming,
                            const float waterDepth, const float horizontalSpeed,
                            const float deltaSeconds) noexcept
    {
        (void)model_.Update(signedShoreDistance, swimming, deltaSeconds);
        (void)waterMovementModel_.Update(waterDepth, swimming, horizontalSpeed, deltaSeconds);
        ApplyVolumes();
        const WaterTransitionRequest waterTransition = waterTransitionScheduler_.Update(
            waterDepth, swimming, horizontalSpeed, deltaSeconds);
        SoundEffect* waterTransitionEffect = waterTransition.variant == 1U
            ? waterTransitionAlternateEffect_.get() : waterTransitionEffect_.get();
        if (available_ && !muted_ && waterTransitionsEnabled_
            && waterTransition.type != WaterTransitionType::None && waterTransitionEffect) {
            try {
                if (waterTransitionEffect->Play(waterTransition.volume,
                                                waterTransition.pitch,
                                                waterTransition.pan)) {
                    ++playedWaterTransitionCount_;
                    if (playedWaterTransitionCount_ == 1) {
                        std::cout << "island-simulator: first water-transition accent played\n"
                                  << std::flush;
                    }
                } else if (!waterTransitionFailureReported_) {
                    waterTransitionFailureReported_ = true;
                    std::cerr << "island-simulator: water-transition accent could not acquire "
                                 "a playback instance; continuous layers continue\n";
                }
            } catch (const std::exception& exception) {
                waterTransitionsEnabled_ = false;
                if (!waterTransitionFailureReported_) {
                    waterTransitionFailureReported_ = true;
                    std::cerr << "island-simulator: water-transition accents disabled; "
                                 "continuous layers continue: " << exception.what() << '\n';
                }
            }
        }
        const WaveAccentRequest accent = waveScheduler_.Update(
            deltaSeconds, model_.Mix().surfVolume);
        SoundEffect* waveEffect = accent.variant == 1U
            ? waveAlternateEffect_.get() : waveEffect_.get();
        if (!available_ || muted_ || !waveAccentsEnabled_ || !accent.trigger || !waveEffect) {
            return;
        }
        try {
            if (waveEffect->Play(accent.volume, accent.pitch, accent.pan)) {
                ++playedWaveAccentCount_;
                if (playedWaveAccentCount_ == 1) {
                    std::cout << "island-simulator: first stronger-wave accent played\n"
                              << std::flush;
                }
            } else if (!waveFailureReported_) {
                waveFailureReported_ = true;
                std::cerr << "island-simulator: stronger-wave accent could not acquire "
                             "a playback instance; ambient beds continue\n";
            }
        } catch (const std::exception& exception) {
            waveAccentsEnabled_ = false;
            if (!waveFailureReported_) {
                waveFailureReported_ = true;
                std::cerr << "island-simulator: stronger-wave accents disabled; ambient beds "
                             "continue: " << exception.what() << '\n';
            }
        }
    }

    void Soundscape::ToggleMuted() noexcept
    {
        muted_ = !muted_;
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
        waterTransitionAlternateEffect_.reset();
        waterTransitionEffect_.reset();
        waterMovementEffect_.reset();
        waveAlternateEffect_.reset();
        waveEffect_.reset();
        breezeEffect_.reset();
        surfInstance_.reset();
        surfEffect_.reset();
        std::cerr << "island-simulator: audio stopped after a runtime failure; "
                     "continuing silently: " << error_ << '\n';
    }
}
