#include "IslandSimulator/SoundscapeModel.hpp"

#include <algorithm>
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

    SoundscapeMix SoundscapeModel::TargetMix(const float signedShoreDistance,
                                             const bool swimming) noexcept
    {
        const float inlandDistance = std::max(0.0f, signedShoreDistance);
        const float surfPresence = 1.0f - SmoothStep(18.0f, 125.0f, inlandDistance);
        const float canopyPresence = SmoothStep(12.0f, 105.0f, inlandDistance);
        SoundscapeMix result;
        result.surfVolume = 0.055f + surfPresence * 0.34f;
        result.breezeVolume = 0.075f + canopyPresence * 0.12f;
        if (swimming) {
            result.surfVolume = std::max(result.surfVolume, 0.35f);
            result.breezeVolume *= 0.72f;
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
        mix_.surfVolume += (target.surfVolume - mix_.surfVolume) * blend;
        mix_.breezeVolume += (target.breezeVolume - mix_.breezeVolume) * blend;
        return mix_;
    }
}
