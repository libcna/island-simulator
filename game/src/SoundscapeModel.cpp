#include "IslandSimulator/SoundscapeModel.hpp"

#include "IslandSimulator/PlayerMotion.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace island
{
    namespace
    {
        constexpr int AmbientSampleRate = 22050;
        constexpr int AmbientLoopSeconds = 10;
        constexpr std::size_t AmbientFrameCount = static_cast<std::size_t>(
            AmbientSampleRate * AmbientLoopSeconds);

        class DeterministicNoise final
        {
        public:
            explicit DeterministicNoise(const std::uint32_t seed) : state_(seed) {}

            [[nodiscard]] float Next() noexcept
            {
                state_ ^= state_ << 13U;
                state_ ^= state_ >> 17U;
                state_ ^= state_ << 5U;
                return static_cast<float>(state_ & 0x00ffffffU) / 8388607.5f - 1.0f;
            }

        private:
            std::uint32_t state_;
        };

        [[nodiscard]] std::vector<float> Noise(const std::uint32_t seed,
                                               const std::size_t sampleCount = AmbientFrameCount)
        {
            DeterministicNoise generator(seed);
            std::vector<float> result(sampleCount);
            for (float& sample : result) {
                sample = generator.Next();
            }
            return result;
        }

        [[nodiscard]] std::vector<float> CircularSmooth(const std::vector<float>& input,
                                                        const std::size_t radius)
        {
            std::vector<float> result(input.size());
            const std::size_t window = radius * 2U + 1U;
            double sum = 0.0;
            for (std::size_t offset = input.size() - radius; offset < input.size(); ++offset) {
                sum += input[offset];
            }
            for (std::size_t offset = 0; offset <= radius; ++offset) {
                sum += input[offset];
            }
            for (std::size_t index = 0; index < input.size(); ++index) {
                result[index] = static_cast<float>(sum / static_cast<double>(window));
                const std::size_t leaving = (index + input.size() - radius) % input.size();
                const std::size_t entering = (index + radius + 1U) % input.size();
                sum += static_cast<double>(input[entering])
                    - static_cast<double>(input[leaving]);
            }
            return result;
        }

        [[nodiscard]] AmbientLoop Quantize(const std::vector<float>& source,
                                           const float targetPeak)
        {
            float peak = 0.0f;
            for (const float sample : source) {
                peak = std::max(peak, std::abs(sample));
            }
            const float scale = peak > 0.000001f ? targetPeak / peak : 0.0f;
            AmbientLoop loop;
            loop.sampleRate = AmbientSampleRate;
            loop.samples.reserve(source.size());
            for (const float sample : source) {
                const float normalized = std::clamp(sample * scale, -0.98f, 0.98f);
                loop.samples.push_back(static_cast<std::int16_t>(
                    std::lround(normalized * 32767.0f)));
            }
            return loop;
        }

        [[nodiscard]] float SmoothStep(const float lower, const float upper,
                                       const float value) noexcept
        {
            const float t = std::clamp((value - lower) / (upper - lower), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }
    }

    AmbientLoop GenerateSurfLoop()
    {
        const std::vector<float> noise = Noise(0x51a7f00dU);
        const std::vector<float> foam = CircularSmooth(noise, 4U);
        const std::vector<float> wash = CircularSmooth(noise, 190U);
        const std::vector<float> body = CircularSmooth(noise, 1200U);
        std::vector<float> samples(AmbientFrameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index) / static_cast<float>(samples.size());
            const float swell = 0.66f
                + 0.18f * std::sin(2.0f * std::numbers::pi_v<float> * (3.0f * phase + 0.08f))
                + 0.10f * std::sin(2.0f * std::numbers::pi_v<float> * (7.0f * phase + 0.37f));
            samples[index] = (foam[index] - wash[index] * 0.38f) * swell
                + wash[index] * 0.46f + body[index] * 0.30f;
        }
        return Quantize(samples, 0.82f);
    }

    AmbientLoop GenerateBreezeLoop()
    {
        const std::vector<float> noise = Noise(0xb4ee2e19U);
        const std::vector<float> leaves = CircularSmooth(noise, 11U);
        const std::vector<float> canopy = CircularSmooth(noise, 85U);
        const std::vector<float> gust = CircularSmooth(noise, 1450U);
        std::vector<float> samples(AmbientFrameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index) / static_cast<float>(samples.size());
            const float movement = 0.55f
                + 0.17f * std::sin(2.0f * std::numbers::pi_v<float> * (2.0f * phase + 0.31f))
                + 0.08f * std::sin(2.0f * std::numbers::pi_v<float> * (5.0f * phase + 0.71f));
            samples[index] = (leaves[index] * 0.64f + canopy[index] * 0.48f) * movement
                + gust[index] * 0.58f;
        }
        return Quantize(samples, 0.64f);
    }

    AmbientLoop GenerateJungleLoop()
    {
        constexpr int loopSeconds = 12;
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * loopSeconds);
        const std::vector<float> noise = Noise(0x1a65ec7dU, frameCount);
        const std::vector<float> leafWhisper = CircularSmooth(noise, 8U);
        const std::vector<float> forestFloor = CircularSmooth(noise, 74U);
        const std::vector<float> distantAir = CircularSmooth(noise, 1320U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index) / static_cast<float>(samples.size());
            const float breathing = 0.58f
                + 0.13f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (2.0f * phase + 0.17f))
                + 0.07f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (7.0f * phase + 0.63f));
            samples[index] = (leafWhisper[index] * 0.46f
                              + forestFloor[index] * 0.54f) * breathing
                + distantAir[index] * 0.48f;
        }

        struct Chirp
        {
            float startSeconds;
            float durationSeconds;
            float frequencyHz;
            float sweepHzPerSecond;
            float amplitude;
            float pulseHz;
        };
        constexpr std::array chirps = {
            Chirp{0.72f, 0.32f, 3740.0f, 410.0f, 0.070f, 23.0f},
            Chirp{1.18f, 0.24f, 4210.0f, -260.0f, 0.052f, 27.0f},
            Chirp{2.86f, 0.46f, 3380.0f, 520.0f, 0.061f, 19.0f},
            Chirp{4.43f, 0.28f, 4870.0f, -330.0f, 0.049f, 31.0f},
            Chirp{6.08f, 0.38f, 3920.0f, 290.0f, 0.057f, 21.0f},
            Chirp{7.62f, 0.22f, 4520.0f, 180.0f, 0.046f, 29.0f},
            Chirp{9.14f, 0.51f, 3260.0f, 470.0f, 0.064f, 18.0f},
            Chirp{10.71f, 0.30f, 4160.0f, -210.0f, 0.051f, 25.0f},
        };
        for (const Chirp& chirp : chirps) {
            const std::size_t begin = static_cast<std::size_t>(
                chirp.startSeconds * static_cast<float>(AmbientSampleRate));
            const std::size_t length = static_cast<std::size_t>(
                chirp.durationSeconds * static_cast<float>(AmbientSampleRate));
            for (std::size_t offset = 0; offset < length && begin + offset < samples.size();
                 ++offset) {
                const float seconds = static_cast<float>(offset)
                    / static_cast<float>(AmbientSampleRate);
                const float unit = static_cast<float>(offset)
                    / static_cast<float>(std::max<std::size_t>(1U, length - 1U));
                const float envelope = std::sin(std::numbers::pi_v<float> * unit);
                const float pulse = 0.62f + 0.38f * std::sin(
                    2.0f * std::numbers::pi_v<float> * chirp.pulseHz * seconds);
                const float phase = 2.0f * std::numbers::pi_v<float>
                    * (chirp.frequencyHz * seconds
                       + 0.5f * chirp.sweepHzPerSecond * seconds * seconds);
                samples[begin + offset] += chirp.amplitude * envelope * envelope
                    * pulse * std::sin(phase);
            }
        }
        return Quantize(samples, 0.56f);
    }

    AmbientLoop GenerateWaveAccent()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(AmbientSampleRate * 4);
        const std::vector<float> noise = Noise(0xa11ce57aU, frameCount);
        const std::vector<float> foam = CircularSmooth(noise, 5U);
        const std::vector<float> wash = CircularSmooth(noise, 170U);
        const std::vector<float> body = CircularSmooth(noise, 920U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index)
                / static_cast<float>(samples.size() - 1U);
            const float attack = SmoothStep(0.0f, 0.12f, phase);
            const float release = 1.0f - SmoothStep(0.58f, 1.0f, phase);
            const float crest = 0.74f
                + 0.17f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (2.0f * phase + 0.16f));
            samples[index] = attack * release
                * ((foam[index] - wash[index] * 0.26f) * crest
                   + wash[index] * 0.58f + body[index] * 0.42f);
        }
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.86f);
    }

    AmbientLoop GenerateWaterMovementLoop()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(AmbientSampleRate * 6);
        const std::vector<float> noise = Noise(0x5a17e24bU, frameCount);
        const std::vector<float> droplets = CircularSmooth(noise, 3U);
        const std::vector<float> wash = CircularSmooth(noise, 48U);
        const std::vector<float> body = CircularSmooth(noise, 510U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index) / static_cast<float>(samples.size());
            const float stroke = 0.52f
                + 0.16f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (4.0f * phase + 0.19f))
                + 0.09f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (9.0f * phase + 0.61f));
            samples[index] = (droplets[index] * 0.54f + wash[index] * 0.62f) * stroke
                + body[index] * 0.38f;
        }
        return Quantize(samples, 0.68f);
    }

    AmbientLoop GenerateWaterTransitionAccent()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(AmbientSampleRate * 3 / 2);
        const std::vector<float> noise = Noise(0xe17a5a11U, frameCount);
        const std::vector<float> droplets = CircularSmooth(noise, 2U);
        const std::vector<float> splash = CircularSmooth(noise, 24U);
        const std::vector<float> body = CircularSmooth(noise, 210U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index)
                / static_cast<float>(samples.size() - 1U);
            const float attack = SmoothStep(0.0f, 0.035f, phase);
            const float release = 1.0f - SmoothStep(0.18f, 1.0f, phase);
            const float initialSplash = std::exp(-phase * 5.5f);
            samples[index] = attack * release
                * (droplets[index] * (0.40f + initialSplash * 0.42f)
                   + splash[index] * 0.72f + body[index] * 0.34f);
        }
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.78f);
    }

    void WaveAccentScheduler::Reset() noexcept
    {
        randomState_ = 0x78d2f9a5U;
        secondsUntilAccent_ = 7.0f + NextUnit() * 6.0f;
    }

    float WaveAccentScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU)
            / static_cast<float>(0x01000000U);
    }

    WaveAccentRequest WaveAccentScheduler::Update(const float deltaSeconds,
                                                  const float surfVolume) noexcept
    {
        secondsUntilAccent_ -= std::clamp(deltaSeconds, 0.0f, 0.25f);
        if (secondsUntilAccent_ > 0.0f || surfVolume < 0.16f) {
            return {};
        }

        WaveAccentRequest request;
        request.trigger = true;
        request.volume = std::clamp((surfVolume - 0.05f) * 0.54f, 0.06f, 0.22f);
        request.pitch = -0.06f + NextUnit() * 0.12f;
        request.pan = -0.32f + NextUnit() * 0.64f;
        secondsUntilAccent_ = 14.0f + NextUnit() * 18.0f;
        return request;
    }

    WaterMovementMix WaterMovementModel::TargetMix(const float waterDepth,
                                                    const bool swimming,
                                                    const float horizontalSpeed) noexcept
    {
        const float referenceSpeed = swimming ? PlayerMotion::SwimSpeed
                                              : PlayerMotion::WalkSpeed;
        const float normalizedSpeed = std::clamp(
            std::max(0.0f, horizontalSpeed) / referenceSpeed, 0.0f, 1.0f);
        const float movement = SmoothStep(0.035f, 0.34f, normalizedSpeed);
        const float depthPresence = swimming
            ? 1.0f
            : SmoothStep(0.035f, 0.38f, std::max(0.0f, waterDepth));

        WaterMovementMix result;
        result.volume = movement * depthPresence * (swimming ? 0.17f : 0.115f);
        result.pitch = swimming
            ? -0.14f + normalizedSpeed * 0.08f
            : 0.025f + normalizedSpeed * 0.12f;
        return result;
    }

    void WaterMovementModel::Reset(const float waterDepth, const bool swimming,
                                   const float horizontalSpeed) noexcept
    {
        mix_ = TargetMix(waterDepth, swimming, horizontalSpeed);
    }

    WaterMovementMix WaterMovementModel::Update(const float waterDepth, const bool swimming,
                                                const float horizontalSpeed,
                                                const float deltaSeconds) noexcept
    {
        const WaterMovementMix target = TargetMix(waterDepth, swimming, horizontalSpeed);
        const float safeDelta = std::clamp(deltaSeconds, 0.0f, 0.25f);
        const float volumeTimeConstant = target.volume > mix_.volume ? 0.14f : 0.34f;
        const float volumeBlend = 1.0f - std::exp(-safeDelta / volumeTimeConstant);
        const float pitchBlend = 1.0f - std::exp(-safeDelta / 0.42f);
        mix_.volume += (target.volume - mix_.volume) * volumeBlend;
        mix_.pitch += (target.pitch - mix_.pitch) * pitchBlend;
        return mix_;
    }

    void WaterTransitionScheduler::Reset(const float waterDepth, const bool swimming) noexcept
    {
        randomState_ = 0xc0145a7eU;
        cooldownSeconds_ = 0.0f;
        inWater_ = swimming || waterDepth >= 0.075f;
    }

    float WaterTransitionScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU)
            / static_cast<float>(0x01000000U);
    }

    WaterTransitionRequest WaterTransitionScheduler::Update(const float waterDepth,
                                                            const bool swimming,
                                                            const float horizontalSpeed,
                                                            const float deltaSeconds) noexcept
    {
        cooldownSeconds_ = std::max(
            0.0f, cooldownSeconds_ - std::clamp(deltaSeconds, 0.0f, 0.25f));
        bool nextInWater = inWater_;
        if (swimming || waterDepth >= 0.075f) {
            nextInWater = true;
        } else if (waterDepth <= 0.025f) {
            nextInWater = false;
        }
        if (nextInWater == inWater_) {
            return {};
        }

        const bool entering = nextInWater;
        inWater_ = nextInWater;
        if (cooldownSeconds_ > 0.0f || horizontalSpeed < 0.12f) {
            return {};
        }

        const float normalizedSpeed = std::clamp(
            horizontalSpeed / PlayerMotion::WalkSpeed, 0.0f, 1.0f);
        WaterTransitionRequest request;
        request.type = entering ? WaterTransitionType::Enter : WaterTransitionType::Exit;
        request.volume = 0.055f + normalizedSpeed * 0.085f;
        request.pitch = (entering ? -0.055f : 0.025f) + NextUnit() * 0.05f;
        request.pan = -0.18f + NextUnit() * 0.36f;
        cooldownSeconds_ = 1.6f;
        return request;
    }

    SoundscapeMix SoundscapeModel::TargetMix(const float signedShoreDistance,
                                             const bool swimming) noexcept
    {
        const float inlandDistance = std::max(0.0f, signedShoreDistance);
        const float surfPresence = 1.0f - SmoothStep(18.0f, 125.0f, inlandDistance);
        const float canopyPresence = SmoothStep(12.0f, 105.0f, inlandDistance);
        SoundscapeMix result;
        result.surfVolume = 0.055f + surfPresence * 0.34f;
        result.breezeVolume = 0.075f + canopyPresence * 0.12f;
        result.jungleVolume = SmoothStep(30.0f, 112.0f, inlandDistance) * 0.078f;
        if (swimming) {
            result.surfVolume = std::max(result.surfVolume, 0.35f);
            result.breezeVolume *= 0.72f;
            result.jungleVolume *= 0.08f;
        }
        return result;
    }

    void SoundscapeModel::Reset(const float signedShoreDistance, const bool swimming) noexcept
    {
        mix_ = TargetMix(signedShoreDistance, swimming);
    }

    SoundscapeMix SoundscapeModel::Update(const float signedShoreDistance, const bool swimming,
                                          const float deltaSeconds) noexcept
    {
        const SoundscapeMix target = TargetMix(signedShoreDistance, swimming);
        const float safeDelta = std::clamp(deltaSeconds, 0.0f, 0.25f);
        const float blend = 1.0f - std::exp(-safeDelta / 1.8f);
        const float jungleBlend = 1.0f - std::exp(-safeDelta / 2.6f);
        mix_.surfVolume += (target.surfVolume - mix_.surfVolume) * blend;
        mix_.breezeVolume += (target.breezeVolume - mix_.breezeVolume) * blend;
        mix_.jungleVolume += (target.jungleVolume - mix_.jungleVolume) * jungleBlend;
        return mix_;
    }
}
