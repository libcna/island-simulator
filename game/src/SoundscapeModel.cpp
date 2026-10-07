#include "IslandSimulator/SoundscapeModel.hpp"

#include "IslandSimulator/PlayerMotion.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
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

        [[nodiscard]] float SafeDeltaSeconds(const float deltaSeconds) noexcept
        {
            return std::isfinite(deltaSeconds)
                ? std::clamp(deltaSeconds, 0.0f, 0.25f) : 0.0f;
        }

        [[nodiscard]] float NonNegativeFinite(const float value) noexcept
        {
            return std::isfinite(value) ? std::max(0.0f, value) : 0.0f;
        }

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
                                           const float targetPeak,
                                           const bool removeMean = false)
        {
            double sum = 0.0;
            if (removeMean) {
                for (const float sample : source) {
                    sum += static_cast<double>(sample);
                }
            }
            const float mean = removeMean && !source.empty()
                ? static_cast<float>(sum / static_cast<double>(source.size())) : 0.0f;
            float peak = 0.0f;
            for (const float sample : source) {
                peak = std::max(peak, std::abs(sample - mean));
            }
            const float scale = peak > 0.000001f ? targetPeak / peak : 0.0f;
            AmbientLoop loop;
            loop.sampleRate = AmbientSampleRate;
            loop.samples.reserve(source.size());
            for (const float sample : source) {
                const float normalized = std::clamp(
                    (sample - mean) * scale, -0.98f, 0.98f);
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

        [[nodiscard]] float SlowBedGain(const double seconds, const float depth,
                                        const double slowAngularFrequency,
                                        const double fastAngularFrequency) noexcept
        {
            const double slow = std::sin(seconds * slowAngularFrequency);
            const double fast = std::sin(seconds * fastAngularFrequency);
            const double movement = slow * slow * 0.65 + fast * fast * 0.35;
            return 1.0f - depth * static_cast<float>(movement);
        }

        void AddBirdChirp(std::vector<float>& samples, const float startSeconds,
                          const float durationSeconds, const float startFrequency,
                          const float endFrequency, const float amplitude,
                          const float tremoloHz)
        {
            const std::size_t begin = static_cast<std::size_t>(
                startSeconds * static_cast<float>(AmbientSampleRate));
            const std::size_t length = static_cast<std::size_t>(
                durationSeconds * static_cast<float>(AmbientSampleRate));
            for (std::size_t offset = 0; offset < length && begin + offset < samples.size();
                 ++offset) {
                const float seconds = static_cast<float>(offset)
                    / static_cast<float>(AmbientSampleRate);
                const float unit = static_cast<float>(offset)
                    / static_cast<float>(std::max<std::size_t>(1U, length - 1U));
                const float envelope = std::sin(std::numbers::pi_v<float> * unit);
                const float sweep = endFrequency - startFrequency;
                const float phase = 2.0f * std::numbers::pi_v<float>
                    * (startFrequency * seconds
                       + 0.5f * sweep * seconds * seconds / durationSeconds);
                const float tremolo = 0.72f + 0.28f * std::sin(
                    2.0f * std::numbers::pi_v<float> * tremoloHz * seconds);
                samples[begin + offset] += amplitude * envelope * envelope * tremolo
                    * (std::sin(phase) + 0.22f * std::sin(phase * 2.01f));
            }
        }

        void AddMonkeyHoot(std::vector<float>& samples, const std::vector<float>& breath,
                           const float startSeconds, const float durationSeconds,
                           const float startFrequency, const float endFrequency,
                           const float amplitude)
        {
            const std::size_t begin = static_cast<std::size_t>(
                startSeconds * static_cast<float>(AmbientSampleRate));
            const std::size_t length = static_cast<std::size_t>(
                durationSeconds * static_cast<float>(AmbientSampleRate));
            for (std::size_t offset = 0; offset < length && begin + offset < samples.size();
                 ++offset) {
                const float seconds = static_cast<float>(offset)
                    / static_cast<float>(AmbientSampleRate);
                const float unit = static_cast<float>(offset)
                    / static_cast<float>(std::max<std::size_t>(1U, length - 1U));
                const float attack = SmoothStep(0.0f, 0.12f, unit);
                const float release = 1.0f - SmoothStep(0.46f, 1.0f, unit);
                const float envelope = attack * release;
                const float sweep = endFrequency - startFrequency;
                const float phase = 2.0f * std::numbers::pi_v<float>
                    * (startFrequency * seconds
                       + 0.5f * sweep * seconds * seconds / durationSeconds);
                const float voiced = std::sin(phase)
                    + 0.34f * std::sin(phase * 2.0f + 0.28f)
                    + 0.14f * std::sin(phase * 3.0f + 0.61f);
                const float texture = breath[(begin + offset) % breath.size()] * 0.24f;
                samples[begin + offset] += amplitude * envelope * (voiced + texture);
            }
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
        return Quantize(samples, 0.82f, true);
    }

    AmbientLoop GenerateBreezeLoop()
    {
        constexpr int loopSeconds = 11;
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * loopSeconds);
        const std::vector<float> noise = Noise(0xb4ee2e19U, frameCount);
        const std::vector<float> leaves = CircularSmooth(noise, 11U);
        const std::vector<float> canopy = CircularSmooth(noise, 85U);
        const std::vector<float> gust = CircularSmooth(noise, 1450U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index) / static_cast<float>(samples.size());
            const float movement = 0.55f
                + 0.17f * std::sin(2.0f * std::numbers::pi_v<float> * (2.0f * phase + 0.31f))
                + 0.08f * std::sin(2.0f * std::numbers::pi_v<float> * (5.0f * phase + 0.71f));
            samples[index] = (leaves[index] * 0.64f + canopy[index] * 0.48f) * movement
                + gust[index] * 0.58f;
        }
        return Quantize(samples, 0.64f, true);
    }

    AmbientLoop GenerateJungleLoop()
    {
        constexpr int loopSeconds = 29;
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
            Chirp{3.04f, 0.46f, 3380.0f, 520.0f, 0.061f, 19.0f},
            Chirp{5.16f, 0.28f, 4870.0f, -330.0f, 0.049f, 31.0f},
            Chirp{6.82f, 0.38f, 3920.0f, 290.0f, 0.057f, 21.0f},
            Chirp{7.30f, 0.22f, 4520.0f, 180.0f, 0.046f, 29.0f},
            Chirp{9.74f, 0.51f, 3260.0f, 470.0f, 0.064f, 18.0f},
            Chirp{12.06f, 0.30f, 4160.0f, -210.0f, 0.051f, 25.0f},
            Chirp{14.44f, 0.34f, 3650.0f, 360.0f, 0.055f, 22.0f},
            Chirp{15.02f, 0.18f, 4940.0f, -430.0f, 0.043f, 33.0f},
            Chirp{17.83f, 0.43f, 3470.0f, 610.0f, 0.060f, 20.0f},
            Chirp{20.24f, 0.25f, 4380.0f, -190.0f, 0.047f, 28.0f},
            Chirp{22.11f, 0.47f, 3180.0f, 540.0f, 0.063f, 17.0f},
            Chirp{24.67f, 0.29f, 4720.0f, -380.0f, 0.050f, 30.0f},
            Chirp{26.48f, 0.36f, 3860.0f, 330.0f, 0.056f, 21.0f},
            Chirp{27.35f, 0.52f, 3290.0f, 450.0f, 0.062f, 18.0f},
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
        return Quantize(samples, 0.56f, true);
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

    AmbientLoop GenerateWaveAccentAlternate()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(AmbientSampleRate * 4);
        const std::vector<float> noise = Noise(0x7e4af291U, frameCount);
        const std::vector<float> foam = CircularSmooth(noise, 8U);
        const std::vector<float> wash = CircularSmooth(noise, 260U);
        const std::vector<float> body = CircularSmooth(noise, 1460U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index)
                / static_cast<float>(samples.size() - 1U);
            const float attack = SmoothStep(0.0f, 0.19f, phase);
            const float release = 1.0f - SmoothStep(0.49f, 1.0f, phase);
            const float crest = 0.63f
                + 0.22f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (1.5f * phase + 0.34f));
            samples[index] = attack * release
                * ((foam[index] - wash[index] * 0.18f) * crest
                   + wash[index] * 0.72f + body[index] * 0.51f);
        }
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.84f);
    }

    AmbientLoop GenerateWaterMovementLoop()
    {
        constexpr int loopSeconds = 7;
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * loopSeconds);
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
        return Quantize(samples, 0.68f, true);
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

    AmbientLoop GenerateWaterTransitionAccentAlternate()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(AmbientSampleRate * 3 / 2);
        const std::vector<float> noise = Noise(0x2f0a7c5dU, frameCount);
        const std::vector<float> droplets = CircularSmooth(noise, 4U);
        const std::vector<float> splash = CircularSmooth(noise, 37U);
        const std::vector<float> body = CircularSmooth(noise, 330U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index)
                / static_cast<float>(samples.size() - 1U);
            const float attack = SmoothStep(0.0f, 0.055f, phase);
            const float release = 1.0f - SmoothStep(0.11f, 1.0f, phase);
            const float initialSplash = std::exp(-phase * 4.2f);
            samples[index] = attack * release
                * (droplets[index] * (0.28f + initialSplash * 0.34f)
                   + splash[index] * 0.82f + body[index] * 0.48f);
        }
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.76f);
    }

    AmbientLoop GenerateBirdCall()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * 6 / 5);
        std::vector<float> samples(frameCount, 0.0f);
        AddBirdChirp(samples, 0.08f, 0.20f, 2380.0f, 3580.0f, 0.72f, 17.0f);
        AddBirdChirp(samples, 0.34f, 0.18f, 2960.0f, 4080.0f, 0.62f, 19.0f);
        AddBirdChirp(samples, 0.61f, 0.29f, 3490.0f, 2460.0f, 0.68f, 15.0f);
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.68f);
    }

    AmbientLoop GenerateBirdCallAlternate()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * 27 / 20);
        std::vector<float> samples(frameCount, 0.0f);
        AddBirdChirp(samples, 0.10f, 0.31f, 4320.0f, 3230.0f, 0.56f, 22.0f);
        AddBirdChirp(samples, 0.49f, 0.16f, 2780.0f, 3860.0f, 0.67f, 24.0f);
        AddBirdChirp(samples, 0.72f, 0.15f, 3150.0f, 4210.0f, 0.58f, 25.0f);
        AddBirdChirp(samples, 0.96f, 0.22f, 3860.0f, 2910.0f, 0.61f, 18.0f);
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.66f);
    }

    AmbientLoop GenerateMonkeyCall()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * 9 / 5);
        const std::vector<float> breath = CircularSmooth(
            Noise(0x91de4a73U, frameCount), 7U);
        std::vector<float> samples(frameCount, 0.0f);
        AddMonkeyHoot(samples, breath, 0.08f, 0.58f, 510.0f, 372.0f, 0.72f);
        AddMonkeyHoot(samples, breath, 0.84f, 0.66f, 474.0f, 326.0f, 0.64f);
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.72f);
    }

    AmbientLoop GenerateMonkeyCallAlternate()
    {
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * 21 / 10);
        const std::vector<float> breath = CircularSmooth(
            Noise(0x3f2a68c1U, frameCount), 11U);
        std::vector<float> samples(frameCount, 0.0f);
        AddMonkeyHoot(samples, breath, 0.10f, 0.42f, 414.0f, 522.0f, 0.55f);
        AddMonkeyHoot(samples, breath, 0.68f, 0.52f, 536.0f, 398.0f, 0.67f);
        AddMonkeyHoot(samples, breath, 1.34f, 0.49f, 452.0f, 336.0f, 0.57f);
        samples.front() = 0.0f;
        samples.back() = 0.0f;
        return Quantize(samples, 0.70f);
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
        secondsUntilAccent_ -= SafeDeltaSeconds(deltaSeconds);
        if (secondsUntilAccent_ > 0.0f || !std::isfinite(surfVolume)
            || surfVolume < 0.16f) {
            return {};
        }

        WaveAccentRequest request;
        request.trigger = true;
        request.variant = NextUnit() < 0.5f ? 0U : 1U;
        request.volume = std::clamp((surfVolume - 0.05f) * 0.54f, 0.06f, 0.22f);
        request.pitch = -0.06f + NextUnit() * 0.12f;
        request.pan = -0.32f + NextUnit() * 0.64f;
        secondsUntilAccent_ = 14.0f + NextUnit() * 18.0f;
        return request;
    }

    WaterMovementMix WaterMovementModel::TargetMix(const float waterDepth,
                                                    const bool swimming,
                                                    const float horizontalSpeed,
                                                    const float oceanCurrentStrength) noexcept
    {
        const float referenceSpeed = swimming ? PlayerMotion::SwimSpeed
                                              : PlayerMotion::WalkSpeed;
        const float normalizedSpeed = std::clamp(
            NonNegativeFinite(horizontalSpeed) / referenceSpeed, 0.0f, 1.0f);
        const float movement = SmoothStep(0.035f, 0.34f, normalizedSpeed);
        const float depthPresence = swimming
            ? 1.0f
            : SmoothStep(0.035f, 0.38f, NonNegativeFinite(waterDepth));
        const float safeCurrent = swimming && std::isfinite(oceanCurrentStrength)
            ? std::clamp(oceanCurrentStrength, 0.0f, 1.0f) : 0.0f;
        const float currentCue = SmoothStep(0.06f, 0.96f, safeCurrent);

        WaterMovementMix result;
        const float movementVolume = movement * depthPresence * (swimming ? 0.17f : 0.115f);
        const float currentVolume = currentCue * (0.032f + currentCue * 0.048f);
        result.volume = std::max(movementVolume, currentVolume);
        result.pitch = (swimming
            ? -0.14f + normalizedSpeed * 0.08f
            : 0.025f + normalizedSpeed * 0.12f) - currentCue * 0.08f;
        result.currentCue = currentCue;
        return result;
    }

    void WaterMovementModel::Reset(const float waterDepth, const bool swimming,
                                   const float horizontalSpeed,
                                   const float oceanCurrentStrength) noexcept
    {
        mix_ = TargetMix(waterDepth, swimming, horizontalSpeed, oceanCurrentStrength);
    }

    WaterMovementMix WaterMovementModel::Update(const float waterDepth, const bool swimming,
                                                const float horizontalSpeed,
                                                const float deltaSeconds,
                                                const float oceanCurrentStrength) noexcept
    {
        const WaterMovementMix target = TargetMix(
            waterDepth, swimming, horizontalSpeed, oceanCurrentStrength);
        const float safeDelta = SafeDeltaSeconds(deltaSeconds);
        const float currentBlend = 1.0f - std::exp(-safeDelta / 0.65f);
        mix_.currentCue += (target.currentCue - mix_.currentCue) * currentBlend;

        const WaterMovementMix calmTarget = TargetMix(
            waterDepth, swimming, horizontalSpeed, 0.0f);
        const float currentVolume = mix_.currentCue
            * (0.032f + mix_.currentCue * 0.048f);
        const float targetVolume = std::max(calmTarget.volume, currentVolume);
        const float targetPitch = calmTarget.pitch - mix_.currentCue * 0.08f;
        const float volumeTimeConstant = targetVolume > mix_.volume ? 0.14f : 0.34f;
        const float volumeBlend = 1.0f - std::exp(-safeDelta / volumeTimeConstant);
        const float pitchBlend = 1.0f - std::exp(-safeDelta / 0.42f);
        mix_.volume += (targetVolume - mix_.volume) * volumeBlend;
        mix_.pitch += (targetPitch - mix_.pitch) * pitchBlend;
        return mix_;
    }

    void WaterTransitionScheduler::Reset(const float waterDepth, const bool swimming) noexcept
    {
        randomState_ = 0xc0145a7eU;
        cooldownSeconds_ = 0.0f;
        inWater_ = swimming || (std::isfinite(waterDepth) && waterDepth >= 0.075f);
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
            0.0f, cooldownSeconds_ - SafeDeltaSeconds(deltaSeconds));
        if (!std::isfinite(waterDepth) || !std::isfinite(horizontalSpeed)) {
            return {};
        }
        const float safeSpeed = std::max(0.0f, horizontalSpeed);
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
        if (cooldownSeconds_ > 0.0f || safeSpeed < 0.12f) {
            return {};
        }

        const float normalizedSpeed = std::clamp(
            safeSpeed / PlayerMotion::WalkSpeed, 0.0f, 1.0f);
        WaterTransitionRequest request;
        request.type = entering ? WaterTransitionType::Enter : WaterTransitionType::Exit;
        request.variant = NextUnit() < 0.5f ? 0U : 1U;
        request.volume = 0.055f + normalizedSpeed * 0.085f;
        request.pitch = (entering ? -0.055f : 0.025f) + NextUnit() * 0.05f;
        request.pan = -0.18f + NextUnit() * 0.36f;
        cooldownSeconds_ = 1.6f;
        return request;
    }

    void WildlifeCallScheduler::Reset() noexcept
    {
        randomState_ = 0x6c8e9cf5U;
        secondsUntilCall_ = 9.0f + NextUnit() * 7.0f;
    }

    float WildlifeCallScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU)
            / static_cast<float>(0x01000000U);
    }

    WildlifeCallRequest WildlifeCallScheduler::Update(
        const float deltaSeconds, const bool swimming, const float signedShoreDistance,
        const float listenerX, const float listenerY, const float listenerZ,
        const WildlifeAudioSource& bird, const WildlifeAudioSource& monkey,
        const bool playbackSlotAvailable) noexcept
    {
        secondsUntilCall_ -= SafeDeltaSeconds(deltaSeconds);
        if (secondsUntilCall_ > 0.0f || !playbackSlotAvailable) {
            return {};
        }

        const auto distanceSquared = [listenerX, listenerY, listenerZ](
                                         const WildlifeAudioSource& source) noexcept {
            if (!std::isfinite(listenerX) || !std::isfinite(listenerY)
                || !std::isfinite(listenerZ) || !std::isfinite(source.x)
                || !std::isfinite(source.y) || !std::isfinite(source.z)) {
                return std::numeric_limits<float>::infinity();
            }
            const float dx = source.x - listenerX;
            const float dy = source.y - listenerY;
            const float dz = source.z - listenerZ;
            return dx * dx + dy * dy + dz * dz;
        };
        const bool birdEligible = bird.available && distanceSquared(bird) <= 110.0f * 110.0f;
        const bool monkeyEligible = !swimming && std::isfinite(signedShoreDistance)
            && signedShoreDistance >= 24.0f
            && monkey.available && distanceSquared(monkey) <= 78.0f * 78.0f;
        if (!birdEligible && !monkeyEligible) {
            secondsUntilCall_ = 2.0f + NextUnit() * 3.0f;
            return {};
        }

        const bool chooseMonkey = monkeyEligible
            && (!birdEligible || NextUnit() < 0.34f);
        WildlifeCallRequest request;
        request.type = chooseMonkey ? WildlifeCallType::Monkey : WildlifeCallType::Bird;
        request.variant = NextUnit() < 0.5f ? 0U : 1U;
        request.volume = chooseMonkey
            ? 0.48f + NextUnit() * 0.12f
            : (swimming ? 0.28f : 0.36f) + NextUnit() * 0.10f;
        request.pitch = chooseMonkey
            ? -0.08f + NextUnit() * 0.12f
            : -0.04f + NextUnit() * 0.13f;
        request.source = chooseMonkey ? monkey : bird;
        secondsUntilCall_ = chooseMonkey
            ? 27.0f + NextUnit() * 23.0f
            : 13.0f + NextUnit() * 17.0f;
        return request;
    }

    SoundscapeMix SoundscapeModel::TargetMix(const float signedShoreDistance,
                                             const bool swimming) noexcept
    {
        if (!std::isfinite(signedShoreDistance)) {
            return {};
        }
        const float inlandDistance = std::max(0.0f, signedShoreDistance);
        const float surfPresence = 1.0f - SmoothStep(
            SurfFadeStartDistance, SurfFadeEndDistance, inlandDistance);
        const float canopyPresence = SmoothStep(
            BreezeFadeStartDistance, BreezeFadeEndDistance, inlandDistance);
        SoundscapeMix result;
        result.surfVolume = 0.055f + surfPresence * 0.34f;
        result.breezeVolume = 0.075f + canopyPresence * 0.12f;
        result.jungleVolume = SmoothStep(
            JungleFadeStartDistance, JungleFadeEndDistance, inlandDistance) * 0.078f;
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
        variationSeconds_ = 0.0;
    }

    SoundscapeMix SoundscapeModel::Update(const float signedShoreDistance, const bool swimming,
                                          const float deltaSeconds) noexcept
    {
        SoundscapeMix target = TargetMix(signedShoreDistance, swimming);
        const float safeDelta = SafeDeltaSeconds(deltaSeconds);
        variationSeconds_ += static_cast<double>(safeDelta);
        // Long, mismatched periods keep stationary ambience from becoming a fixed wall of sound.
        // Gains only dip below the authored zone mix, preserving the existing volume ceilings.
        target.surfVolume *= SlowBedGain(variationSeconds_, 0.035f, 0.033, 0.071);
        target.breezeVolume *= SlowBedGain(variationSeconds_, 0.075f, 0.024, 0.055);
        target.jungleVolume *= SlowBedGain(variationSeconds_, 0.055f, 0.029, 0.063);
        const float blend = 1.0f - std::exp(-safeDelta / 1.8f);
        const float jungleBlend = 1.0f - std::exp(-safeDelta / 2.6f);
        mix_.surfVolume += (target.surfVolume - mix_.surfVolume) * blend;
        mix_.breezeVolume += (target.breezeVolume - mix_.breezeVolume) * blend;
        mix_.jungleVolume += (target.jungleVolume - mix_.jungleVolume) * jungleBlend;
        return mix_;
    }
}
