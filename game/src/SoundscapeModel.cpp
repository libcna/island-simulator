#include "IslandSimulator/SoundscapeModel.hpp"
#include "IslandSimulator/CoastalWindResponse.hpp"

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

        [[nodiscard]] AmbientLoop GenerateFootstepClip(const std::uint32_t seed,
                                                       const bool sand,
                                                       const bool alternate)
        {
            constexpr std::size_t frameCount = static_cast<std::size_t>(
                AmbientSampleRate * 3 / 10);
            constexpr float durationSeconds = 0.30f;
            const std::vector<float> noise = Noise(seed, frameCount);
            const std::vector<float> grit = CircularSmooth(noise, sand ? 2U : 5U);
            const std::vector<float> crunch = CircularSmooth(noise, sand ? 11U : 19U);
            const std::vector<float> body = CircularSmooth(noise, sand ? 58U : 104U);
            std::vector<float> samples(frameCount);
            for (std::size_t index = 0; index < samples.size(); ++index) {
                const float unit = static_cast<float>(index)
                    / static_cast<float>(samples.size() - 1U);
                const float seconds = static_cast<float>(index)
                    / static_cast<float>(AmbientSampleRate);
                const float attack = SmoothStep(0.0f, sand ? 0.045f : 0.035f, unit);
                const float release = 1.0f - SmoothStep(sand ? 0.17f : 0.13f, 1.0f, unit);
                const float startFrequency = sand ? 94.0f : 76.0f;
                const float endFrequency = sand ? 54.0f : 42.0f;
                const float sweep = endFrequency - startFrequency;
                const float thumpPhase = 2.0f * std::numbers::pi_v<float>
                    * (startFrequency * seconds
                       + 0.5f * sweep * seconds * seconds / durationSeconds);
                const float thump = std::sin(thumpPhase) * std::exp(-unit * 8.5f);
                const float firstCrunch = std::exp(
                    -std::pow((unit - (alternate ? 0.19f : 0.13f)) / 0.065f, 2.0f));
                const float secondCrunch = std::exp(
                    -std::pow((unit - (alternate ? 0.43f : 0.36f)) / 0.095f, 2.0f));
                const float looseTexture = firstCrunch + secondCrunch * 0.58f;
                const float texture = sand
                    ? (noise[index] - grit[index]) * 0.34f
                        + grit[index] * (0.48f + looseTexture * 0.22f)
                        + crunch[index] * 0.42f + body[index] * 0.24f
                        + thump * 0.17f
                    : (noise[index] - grit[index]) * (0.20f + looseTexture * 0.25f)
                        + grit[index] * 0.34f + crunch[index] * 0.52f
                        + body[index] * 0.61f + thump * 0.34f;
                samples[index] = attack * release * texture;
            }
            const float smoothing = sand ? 0.22f : 0.34f;
            for (std::size_t index = 1U; index < samples.size(); ++index) {
                samples[index] = samples[index] * (1.0f - smoothing)
                    + samples[index - 1U] * smoothing;
            }
            samples.front() = 0.0f;
            samples.back() = 0.0f;
            return Quantize(samples, sand ? 0.58f : 0.62f);
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

    AmbientLoop GenerateStreamLoop()
    {
        constexpr int loopSeconds = 13;
        constexpr std::size_t frameCount = static_cast<std::size_t>(
            AmbientSampleRate * loopSeconds);
        const std::vector<float> noise = Noise(0x57ea6d21U, frameCount);
        const std::vector<float> droplets = CircularSmooth(noise, 2U);
        const std::vector<float> trickle = CircularSmooth(noise, 17U);
        const std::vector<float> bed = CircularSmooth(noise, 240U);
        const std::vector<float> current = CircularSmooth(noise, 1750U);
        std::vector<float> samples(frameCount);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const float phase = static_cast<float>(index)
                / static_cast<float>(samples.size());
            const float flow = 0.58f
                + 0.15f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (3.0f * phase + 0.11f))
                + 0.08f * std::sin(2.0f * std::numbers::pi_v<float>
                                   * (8.0f * phase + 0.43f));
            const float pebble = std::sin(2.0f * std::numbers::pi_v<float>
                                          * (61.0f * phase + 0.2f));
            samples[index] = ((droplets[index] - trickle[index] * 0.22f) * 0.48f
                              + trickle[index] * 0.62f + bed[index] * 0.46f) * flow
                + current[index] * 0.52f + pebble * trickle[index] * 0.035f;
        }
        return Quantize(samples, 0.62f, true);
    }

    AmbientLoop GenerateCampfireLoop()
    {
        constexpr std::size_t count=AmbientSampleRate*19U;
        const auto noise=Noise(0xcaf17019U,count);
        const auto soft=CircularSmooth(noise,3U),body=CircularSmooth(noise,130U);
        std::vector<float> samples(count);
        for(std::size_t i=0;i<count;++i) {
            const float phase=static_cast<float>(i)/count;
            samples[i]=soft[i]*.022f+body[i]*(.26f+.06f*std::sin(phase*6*std::numbers::pi_v<float>));
        }
        DeterministicNoise random(0x123ca719U);
        for(int event=0;event<173;++event) {
            const auto start=static_cast<std::size_t>((random.Next()+1)*.4999f*count);
            const float amplitude=.035f+.14f*(random.Next()+1)*.5f;
            const float decay=42.0f+65.0f*(random.Next()+1)*.5f;
            const float frequency=650.0f+1300.0f*(random.Next()+1)*.5f;
            for(int offset=0;offset<AmbientSampleRate/5;++offset) {
                const auto index=(start+static_cast<std::size_t>(offset))%count;
                const float seconds=static_cast<float>(offset)/AmbientSampleRate;
                const float attack=std::min(1.0f,seconds*1900);
                samples[index]+=amplitude*attack*std::exp(-seconds*decay)
                    *(soft[index]*2.3f+noise[index]*.14f+.13f*std::sin(2*std::numbers::pi_v<float>*frequency*seconds));
            }
        }
        return Quantize(samples,.68f,true);
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

    namespace
    {
        [[nodiscard]] AmbientLoop GenerateShipwreckCreakClip(const bool alternate)
        {
            constexpr std::size_t frameCount = static_cast<std::size_t>(
                AmbientSampleRate * 9 / 5);
            const std::vector<float> texture = CircularSmooth(
                Noise(alternate ? 0x71de92b3U : 0xc8ea517dU, frameCount),
                alternate ? 23U : 17U);
            std::vector<float> samples(frameCount, 0.0f);
            for (std::size_t index = 0; index < samples.size(); ++index) {
                const float unit = static_cast<float>(index)
                    / static_cast<float>(samples.size() - 1U);
                const float seconds = static_cast<float>(index)
                    / static_cast<float>(AmbientSampleRate);
                const float attack = SmoothStep(0.0f, alternate ? 0.10f : 0.075f, unit);
                const float release = 1.0f - SmoothStep(
                    alternate ? 0.49f : 0.57f, 1.0f, unit);
                const float startFrequency = alternate ? 73.0f : 96.0f;
                const float endFrequency = alternate ? 108.0f : 58.0f;
                const float sweep = endFrequency - startFrequency;
                const float phase = 2.0f * std::numbers::pi_v<float>
                    * (startFrequency * seconds
                       + 0.5f * sweep * seconds * seconds / 1.8f);
                const float strain = std::sin(phase)
                    + 0.31f * std::sin(phase * 2.013f + 0.37f)
                    + 0.12f * std::sin(phase * 3.071f + 0.81f);
                const float secondary = alternate
                    ? SmoothStep(0.31f, 0.43f, unit)
                        * (1.0f - SmoothStep(0.62f, 0.86f, unit))
                    : SmoothStep(0.18f, 0.29f, unit)
                        * (1.0f - SmoothStep(0.46f, 0.72f, unit));
                samples[index] = attack * release
                    * (strain * (0.64f + secondary * 0.28f)
                       + texture[index] * (0.18f + secondary * 0.16f));
            }
            samples.front() = 0.0f;
            samples.back() = 0.0f;
            return Quantize(samples, alternate ? 0.61f : 0.64f);
        }
    }

    AmbientLoop GenerateShipwreckCreak()
    {
        return GenerateShipwreckCreakClip(false);
    }

    AmbientLoop GenerateShipwreckCreakAlternate()
    {
        return GenerateShipwreckCreakClip(true);
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

    AmbientLoop GenerateSandFootstep()
    {
        return GenerateFootstepClip(0x5a4d6c13U, true, false);
    }

    AmbientLoop GenerateSandFootstepAlternate()
    {
        return GenerateFootstepClip(0x1fd28b79U, true, true);
    }

    AmbientLoop GenerateSoilFootstep()
    {
        return GenerateFootstepClip(0x70a15eedU, false, false);
    }

    AmbientLoop GenerateSoilFootstepAlternate()
    {
        return GenerateFootstepClip(0xc4419a37U, false, true);
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

    void ShipwreckCreakScheduler::Reset() noexcept
    {
        randomState_ = 0xc8ea41f7U;
        secondsUntilCreak_ = 10.0f + NextUnit() * 7.0f;
    }

    float ShipwreckCreakScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU)
            / static_cast<float>(0x01000000U);
    }

    ShipwreckCreakRequest ShipwreckCreakScheduler::Update(
        const float deltaSeconds, const float surfVolume,
        const float distanceToShipwreck, const bool playbackSlotAvailable) noexcept
    {
        secondsUntilCreak_ -= SafeDeltaSeconds(deltaSeconds);
        if (!std::isfinite(surfVolume) || !std::isfinite(distanceToShipwreck)
            || distanceToShipwreck < 0.0f) {
            return {};
        }
        const bool audible = surfVolume >= 0.12f && distanceToShipwreck <= 34.0f;
        if (!audible) {
            if (secondsUntilCreak_ <= 0.0f) {
                secondsUntilCreak_ = 4.0f + NextUnit() * 6.0f;
            }
            return {};
        }
        if (secondsUntilCreak_ > 0.0f || !playbackSlotAvailable) {
            return {};
        }

        const float proximity = 1.0f - SmoothStep(5.0f, 34.0f, distanceToShipwreck);
        ShipwreckCreakRequest request;
        request.trigger = true;
        request.variant = NextUnit() < 0.5f ? 0U : 1U;
        request.volume = std::clamp(
            0.035f + proximity * (0.055f + surfVolume * 0.15f), 0.035f, 0.16f);
        request.pitch = -0.075f + NextUnit() * 0.12f;
        secondsUntilCreak_ = 17.0f + NextUnit() * 24.0f;
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

    void FootstepScheduler::Reset() noexcept
    {
        randomState_ = 0xf00757e9U;
        previousX_ = 0.0f;
        previousZ_ = 0.0f;
        distanceSinceStep_ = 0.0f;
        hasPreviousPosition_ = false;
        wasMoving_ = false;
        leftFoot_ = false;
    }

    float FootstepScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU)
            / static_cast<float>(0x01000000U);
    }

    FootstepRequest FootstepScheduler::Update(
        const float deltaSeconds, const bool swimming, const float signedShoreDistance,
        const float horizontalSpeed, const float listenerX, const float listenerZ) noexcept
    {
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f
            || !std::isfinite(signedShoreDistance) || !std::isfinite(horizontalSpeed)
            || !std::isfinite(listenerX) || !std::isfinite(listenerZ)) {
            return {};
        }
        const float safeDelta = SafeDeltaSeconds(deltaSeconds);
        const float safeSpeed = std::max(0.0f, horizontalSpeed);
        if (!hasPreviousPosition_) {
            previousX_ = listenerX;
            previousZ_ = listenerZ;
            hasPreviousPosition_ = true;
            return {};
        }

        const float travelled = std::hypot(listenerX - previousX_, listenerZ - previousZ_);
        previousX_ = listenerX;
        previousZ_ = listenerZ;
        const float maximumContinuousTravel = std::max(1.25f, safeSpeed * safeDelta * 2.25f);
        if (!std::isfinite(travelled) || travelled > maximumContinuousTravel) {
            distanceSinceStep_ = 0.0f;
            wasMoving_ = false;
            return {};
        }
        if (swimming) {
            distanceSinceStep_ = 0.0f;
            wasMoving_ = false;
            return {};
        }
        if (safeSpeed < 0.45f || travelled < 0.00001f) {
            if (safeSpeed < 0.45f) {
                wasMoving_ = false;
            }
            return {};
        }

        const float runBlend = std::clamp(
            (safeSpeed - PlayerMotion::WalkSpeed)
                / (PlayerMotion::RunSpeed - PlayerMotion::WalkSpeed),
            0.0f, 1.0f);
        const float strideDistance = 2.05f + runBlend * 0.70f;
        if (!wasMoving_) {
            distanceSinceStep_ = strideDistance * 0.52f;
            wasMoving_ = true;
        }
        distanceSinceStep_ += travelled;
        if (distanceSinceStep_ < strideDistance) {
            return {};
        }
        distanceSinceStep_ = std::fmod(distanceSinceStep_, strideDistance);

        FootstepRequest request;
        request.surface = signedShoreDistance <= 18.0f
            ? FootstepSurface::Sand : FootstepSurface::Soil;
        request.variant = NextUnit() < 0.5f ? 0U : 1U;
        request.volume = 0.12f + runBlend * 0.065f + NextUnit() * 0.018f;
        request.pitch = request.surface == FootstepSurface::Sand
            ? -0.015f + NextUnit() * 0.09f + runBlend * 0.025f
            : -0.060f + NextUnit() * 0.08f + runBlend * 0.030f;
        leftFoot_ = !leftFoot_;
        request.pan = (leftFoot_ ? -0.055f : 0.055f)
            + (NextUnit() - 0.5f) * 0.030f;
        return request;
    }

    AmbientLoop GenerateBoarCall(const BoarCallKind kind, const std::uint8_t variant)
    {
        constexpr std::size_t frames = AmbientSampleRate * 6U / 5U;
        std::vector<float> samples(frames, 0.0f);
        if (kind == BoarCallKind::None) return Quantize(samples, 0.0f);
        const bool alternate = (variant & 1U) != 0U;
        DeterministicNoise noise(0x51a7b0a4U + static_cast<std::uint32_t>(kind) * 7919U
            + static_cast<std::uint32_t>(alternate) * 137U);
        float lowNoise = 0.0f, slowerNoise = 0.0f;
        double phase = 0.0;
        for (std::size_t i = 0; i < frames; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(AmbientSampleRate);
            lowNoise += 0.18f * (noise.Next() - lowNoise);
            slowerNoise += 0.035f * (lowNoise - slowerNoise);
            const float start = alternate ? 0.13f : 0.08f;
            const float duration = kind == BoarCallKind::Breath ? 0.92f : 0.38f;
            const auto pulse = [t](const float begin, const float length) {
                const float u = (t - begin) / length;
                if (u <= 0.0f || u >= 1.0f) return 0.0f;
                const float sine = std::sin(std::numbers::pi_v<float> * u);
                return sine * sine;
            };
            float envelope = pulse(start, duration);
            if (kind != BoarCallKind::Breath)
                envelope += (alternate ? 0.64f : 0.82f) * pulse(alternate ? 0.69f : 0.58f, 0.32f);
            const float frequency = (alternate ? 91.0f : 108.0f)
                - 19.0f * t + 5.0f * std::sin(t * 31.0f) + 7.0f * slowerNoise;
            phase += 2.0 * std::numbers::pi * static_cast<double>(frequency) / AmbientSampleRate;
            const float voice = static_cast<float>(std::sin(phase) + 0.55 * std::sin(2.0 * phase)
                + 0.32 * std::sin(4.0 * phase) + 0.17 * std::sin(7.0 * phase));
            const float texture = kind == BoarCallKind::Grunt
                ? voice * (0.70f + slowerNoise) + lowNoise * 0.8f
                : kind == BoarCallKind::Snuffle
                    ? 3.5f * (lowNoise - slowerNoise) + voice * 0.12f
                    : 2.8f * lowNoise + voice * 0.025f;
            samples[i] = envelope * texture;
        }
        return Quantize(samples, kind == BoarCallKind::Grunt ? 0.62f : 0.48f);
    }

    float BoarCallVolume(const BoarCallKind kind, const float distance) noexcept
    {
        if (!std::isfinite(distance) || distance < 0.0f) return 0.0f;
        const float radius = kind == BoarCallKind::Breath ? 9.0f
            : kind == BoarCallKind::Snuffle ? 23.0f : kind == BoarCallKind::Grunt ? 42.0f : 0.0f;
        if (distance >= radius) return 0.0f;
        const float base = kind == BoarCallKind::Breath ? 0.14f
            : kind == BoarCallKind::Snuffle ? 0.29f : 0.40f;
        const float fade = 1.0f - distance / radius;
        return base * fade * fade;
    }

    float BoarCallScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU) / 16777216.0f;
    }

    void BoarCallScheduler::Reset() noexcept
    {
        randomState_ = 0xb0a45eedU;
        herdCooldown_ = 2.0f;
        nextAnimal_ = 0;
        callPhases_.fill(0);
        for (float& cooldown : cooldowns_) {
            cooldown = 3.0f + 8.0f * NextUnit();
        }
    }

    BoarCallRequest BoarCallScheduler::Update(const float deltaSeconds,
        const float listenerX, const float listenerY, const float listenerZ,
        const std::span<const BoarState> boars, const bool playbackSlotAvailable) noexcept
    {
        const float dt = SafeDeltaSeconds(deltaSeconds);
        // Paused/invalid time must never emit an overdue call.
        if (dt <= 0.0f) return {};
        herdCooldown_ = std::max(0.0f, herdCooldown_ - dt);
        for (float& cooldown : cooldowns_) cooldown = std::max(0.0f, cooldown - dt);
        if (!playbackSlotAvailable || herdCooldown_ > 0.0f
            || !std::isfinite(listenerX) || !std::isfinite(listenerY)
            || !std::isfinite(listenerZ)) return {};

        const std::size_t count = std::min(boars.size(), cooldowns_.size());
        for (std::size_t offset = 0; offset < count; ++offset) {
            const std::size_t index = (nextAnimal_ + offset) % count;
            const BoarState& boar = boars[index];
            if (cooldowns_[index] > 0.0f || boar.simulationTier == WildlifeSimulationTier::Dormant
                || !std::isfinite(boar.x) || !std::isfinite(boar.y) || !std::isfinite(boar.z)
                || !std::isfinite(boar.scale) || boar.scale <= 0.0f) continue;
            BoarCallKind kind = BoarCallKind::None;
            float interval = 0.0f;
            switch (boar.behavior) {
                case BoarBehavior::Rest:
                    kind = BoarCallKind::Breath; interval = 16.0f; break;
                case BoarBehavior::Forage:
                    kind = BoarCallKind::Snuffle; interval = 7.0f; break;
                case BoarBehavior::Wander:
                    kind = BoarCallKind::Grunt; interval = 12.0f; break;
                case BoarBehavior::Retreat:
                    kind = BoarCallKind::Grunt; interval = 6.0f; break;
                default: continue;
            }
            const float sourceY = boar.y + 0.55f * boar.scale;
            const float distance = std::hypot(std::hypot(boar.x - listenerX,
                boar.z - listenerZ), sourceY - listenerY);
            // Quiet animals still vocalize: start with a contact grunt, then
            // alternate two close-range behavior details. If those details would
            // be virtually inaudible, retain a sparse grunt at the same cooldown.
            if(kind!=BoarCallKind::Grunt&&(callPhases_[index]==0
                ||BoarCallVolume(kind,distance)<.03f))kind=BoarCallKind::Grunt;
            const float volume = BoarCallVolume(kind, distance);
            if (volume < 0.005f) continue;
            BoarCallRequest request;
            request.kind = kind;
            request.animalIndex = index;
            request.variant = static_cast<std::uint8_t>(NextUnit() >= 0.5f);
            request.volume = volume * (0.86f + 0.14f * NextUnit());
            request.pitch = -0.07f + 0.14f * NextUnit();
            request.source = {true, boar.x, sourceY, boar.z};
            cooldowns_[index] = interval + 9.0f * NextUnit();
            callPhases_[index]=static_cast<std::uint8_t>((callPhases_[index]+1U)%3U);
            herdCooldown_ = 2.5f + 2.0f * NextUnit();
            nextAnimal_ = (index + 1U) % count;
            return request;
        }
        return {};
    }

    AmbientLoop GenerateGoatCall(const GoatCallKind kind, const std::uint8_t variant)
    {
        constexpr std::size_t frames = AmbientSampleRate * 6U / 5U;
        std::vector<float> samples(frames, 0.0f);
        if (kind == GoatCallKind::None) return Quantize(samples, 0.0f);
        const bool alternate = (variant & 1U) != 0U;
        DeterministicNoise noise(0x60a7b1eaU + static_cast<std::uint32_t>(kind) * 6151U
            + static_cast<std::uint32_t>(alternate) * 977U);
        float lowNoise = 0.0f, slowNoise = 0.0f;
        double phase = 0.0;
        const auto pulse = [](const float t, const float begin, const float duration) {
            const float u = (t - begin) / duration;
            if (u <= 0.0f || u >= 1.0f) return 0.0f;
            return SmoothStep(0.0f, 0.09f, u) * (1.0f - SmoothStep(0.63f, 1.0f, u));
        };
        for (std::size_t i = 0; i < frames; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(AmbientSampleRate);
            const float rawNoise = noise.Next();
            lowNoise += .13f * (rawNoise - lowNoise);
            slowNoise += .025f * (lowNoise - slowNoise);
            float sample = 0.0f;
            if (kind == GoatCallKind::Bleat) {
                const float envelope = alternate
                    ? pulse(t, .07f, .36f) + .88f * pulse(t, .52f, .48f)
                    : pulse(t, .08f, .88f);
                const float base = (alternate ? 238.0f : 218.0f)
                    + 18.0f * std::sin(t * (alternate ? 34.0f : 29.0f))
                    - 22.0f * t + 5.0f * slowNoise;
                phase += 2.0 * std::numbers::pi * static_cast<double>(base)
                    / static_cast<double>(AmbientSampleRate);
                const float voice = static_cast<float>(std::sin(phase)
                    + .64 * std::sin(2.0 * phase) + .31 * std::sin(3.0 * phase)
                    + .17 * std::sin(5.0 * phase));
                sample = envelope * (voice * (.72f + .12f * slowNoise)
                    + (lowNoise - slowNoise) * .34f);
            } else if (kind == GoatCallKind::Breath) {
                const float envelope = pulse(t, alternate ? .15f : .09f,
                    alternate ? .70f : .84f);
                sample = envelope * ((lowNoise - slowNoise) * 2.8f
                    + rawNoise * .16f);
            } else {
                const auto knock = [t](const float begin, const float strength) {
                    const float u = t - begin;
                    if (u <= 0.0f || u >= .19f) return 0.0f;
                    return strength * std::sin(std::numbers::pi_v<float> * u / .19f)
                        * std::exp(-u * 24.0f);
                };
                const float impact = knock(alternate ? .12f : .08f, 1.0f)
                    + knock(alternate ? .56f : .47f, alternate ? .72f : .82f);
                const float tone = std::sin(t * 2.0f * std::numbers::pi_v<float>
                    * (alternate ? 164.0f : 143.0f));
                sample = impact * (tone * .72f + (lowNoise - slowNoise) * 1.8f);
            }
            samples[i] = sample;
        }
        const float peak = kind == GoatCallKind::Bleat ? .60f
            : kind == GoatCallKind::Breath ? .48f : .54f;
        return Quantize(samples, peak);
    }

    float GoatCallVolume(const GoatCallKind kind, const float distance) noexcept
    {
        if (!std::isfinite(distance) || distance < 0.0f) return 0.0f;
        const float radius = kind == GoatCallKind::Breath ? 10.0f
            : kind == GoatCallKind::Hoof ? 20.0f
            : kind == GoatCallKind::Bleat ? 55.0f : 0.0f;
        if (distance >= radius) return 0.0f;
        const float base = kind == GoatCallKind::Breath ? .15f
            : kind == GoatCallKind::Hoof ? .21f : .40f;
        const float fade = 1.0f - distance / radius;
        return base * fade * fade;
    }

    float GoatCallScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU) / 16777216.0f;
    }

    void GoatCallScheduler::Reset() noexcept
    {
        randomState_ = 0x60a7b1eaU;
        herdCooldown_ = 1.5f;
        nextAnimal_ = 0U;
        callPhases_.fill(0U);
        for (float& cooldown : cooldowns_) cooldown = 2.0f + 6.0f * NextUnit();
    }

    GoatCallRequest GoatCallScheduler::Update(const float deltaSeconds,
        const float listenerX, const float listenerY, const float listenerZ,
        const std::span<const GoatState> goats, const bool playbackSlotAvailable) noexcept
    {
        const float dt = SafeDeltaSeconds(deltaSeconds);
        if (dt <= 0.0f) return {};
        herdCooldown_ = std::max(0.0f, herdCooldown_ - dt);
        for (float& cooldown : cooldowns_) cooldown = std::max(0.0f, cooldown - dt);
        if (!playbackSlotAvailable || herdCooldown_ > 0.0f
            || !std::isfinite(listenerX) || !std::isfinite(listenerY)
            || !std::isfinite(listenerZ)) return {};

        const std::size_t count = std::min(goats.size(), cooldowns_.size());
        for (std::size_t offset = 0; offset < count; ++offset) {
            const std::size_t index = (nextAnimal_ + offset) % count;
            const GoatState& goat = goats[index];
            if (cooldowns_[index] > 0.0f || !std::isfinite(goat.x)
                || !std::isfinite(goat.y) || !std::isfinite(goat.z)
                || !std::isfinite(goat.scale) || goat.scale <= 0.0f) continue;
            GoatCallKind kind = GoatCallKind::None;
            float interval = 0.0f;
            switch (goat.behavior) {
            case GoatBehavior::Graze:
                kind = GoatCallKind::Breath; interval = 13.0f; break;
            case GoatBehavior::Walk:
                kind = GoatCallKind::Hoof; interval = 7.0f; break;
            case GoatBehavior::Rest:
                kind = GoatCallKind::Breath; interval = 17.0f; break;
            case GoatBehavior::Retreat:
                kind = GoatCallKind::Bleat; interval = 6.0f; break;
            }
            const float sourceY = goat.y + .78f * goat.scale;
            const float distance = std::hypot(std::hypot(goat.x - listenerX,
                goat.z - listenerZ), sourceY - listenerY);
            // Every goat introduces itself with a restrained bleat; later requests retain
            // close breathing/hoof detail when that detail would actually be audible.
            if (callPhases_[index] == 0U || GoatCallVolume(kind, distance) < .025f)
                kind = GoatCallKind::Bleat;
            const float volume = GoatCallVolume(kind, distance);
            if (volume < .006f) continue;
            GoatCallRequest request;
            request.kind = kind;
            request.animalIndex = index;
            request.variant = static_cast<std::uint8_t>(NextUnit() >= .5f);
            request.volume = volume * (.86f + .14f * NextUnit());
            request.pitch = -.07f + .15f * NextUnit();
            request.source = {true, goat.x, sourceY, goat.z};
            cooldowns_[index] = interval + 8.0f * NextUnit();
            callPhases_[index] = static_cast<std::uint8_t>((callPhases_[index] + 1U) % 3U);
            herdCooldown_ = 2.8f + 2.2f * NextUnit();
            nextAnimal_ = (index + 1U) % count;
            return request;
        }
        return {};
    }

    AmbientLoop GenerateSnakeHiss(const std::uint8_t variant)
    {
        constexpr std::size_t frames = AmbientSampleRate;
        std::vector<float> samples(frames, 0.0f);
        const bool alternate = (variant & 1U) != 0U;
        DeterministicNoise noise(0x51a6e55eU + static_cast<std::uint32_t>(alternate) * 7919U);
        float low = 0.0f, slower = 0.0f;
        const auto pulse = [](const float t, const float begin, const float length) {
            const float u = (t - begin) / length;
            if (u <= 0.0f || u >= 1.0f) return 0.0f;
            const float attack = SmoothStep(0.0f, 0.09f, u);
            const float release = 1.0f - SmoothStep(0.62f, 1.0f, u);
            return attack * release;
        };
        for (std::size_t i = 0; i < frames; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(AmbientSampleRate);
            const float raw = noise.Next();
            low += 0.11f * (raw - low);
            slower += 0.018f * (low - slower);
            const float bright = raw - low;
            const float mid = low - slower;
            const float envelope = alternate
                ? pulse(t, .06f, .35f) + .84f * pulse(t, .49f, .43f)
                : pulse(t, .07f, .82f);
            const float breathFlutter = .90f + .10f * std::sin(
                2.0f * std::numbers::pi_v<float> * (alternate ? 17.0f : 13.0f) * t);
            samples[i] = envelope * breathFlutter * (bright * .82f + mid * 1.35f);
        }
        return Quantize(samples, .56f);
    }

    float SnakeHissVolume(const float distance) noexcept
    {
        if (!std::isfinite(distance) || distance < 0.0f || distance >= 16.0f) return 0.0f;
        const float fade = 1.0f - distance / 16.0f;
        return .31f * fade * fade;
    }

    float SnakeHissScheduler::NextUnit() noexcept
    {
        randomState_ ^= randomState_ << 13U;
        randomState_ ^= randomState_ >> 17U;
        randomState_ ^= randomState_ << 5U;
        return static_cast<float>(randomState_ & 0x00ffffffU) / 16777216.0f;
    }

    void SnakeHissScheduler::Reset() noexcept
    {
        randomState_ = 0x51a6e55eU;
        populationCooldown_ = .75f;
        nextAnimal_ = 0;
        for (float& cooldown : cooldowns_) cooldown = 1.0f + 3.5f * NextUnit();
    }

    SnakeHissRequest SnakeHissScheduler::Update(const float deltaSeconds,
        const float listenerX, const float listenerY, const float listenerZ,
        const std::span<const SnakeState> snakes, const bool playbackSlotAvailable) noexcept
    {
        const float dt = SafeDeltaSeconds(deltaSeconds);
        if (dt <= 0.0f) return {};
        populationCooldown_ = std::max(0.0f, populationCooldown_ - dt);
        for (float& cooldown : cooldowns_) cooldown = std::max(0.0f, cooldown - dt);
        if (!playbackSlotAvailable || populationCooldown_ > 0.0f
            || !std::isfinite(listenerX) || !std::isfinite(listenerY)
            || !std::isfinite(listenerZ)) return {};

        const std::size_t count = std::min(snakes.size(), cooldowns_.size());
        for (std::size_t offset = 0; offset < count; ++offset) {
            const std::size_t index = (nextAnimal_ + offset) % count;
            const SnakeState& snake = snakes[index];
            const SnakePoint head = snake.body.front();
            if (cooldowns_[index] > 0.0f || snake.tier == WildlifeSimulationTier::Dormant
                || snake.behavior != SnakeBehavior::Retreat
                || !std::isfinite(head.x) || !std::isfinite(head.y) || !std::isfinite(head.z))
                continue;
            const float distance = std::hypot(std::hypot(head.x-listenerX,head.z-listenerZ),
                                               head.y-listenerY);
            const float volume = SnakeHissVolume(distance);
            if (volume < .008f) continue;
            SnakeHissRequest request;
            request.trigger = true;
            request.animalIndex = index;
            request.variant = static_cast<std::uint8_t>(NextUnit() >= .5f);
            request.volume = volume * (.86f + .14f * NextUnit());
            request.pitch = -.08f + .16f * NextUnit();
            request.source = {true,head.x,head.y,head.z};
            cooldowns_[index] = 7.0f + 8.0f * NextUnit();
            populationCooldown_ = 3.0f + 2.0f * NextUnit();
            nextAnimal_ = (index + 1U) % count;
            return request;
        }
        return {};
    }

    AmbientLoop GenerateBearCall(const BearCallKind kind,const std::uint8_t variant)
    {
        constexpr std::size_t frames=AmbientSampleRate*8U/5U;
        std::vector<float> samples(frames,0.0f);
        if(kind==BearCallKind::None)return Quantize(samples,0);
        const bool alternate=(variant&1U)!=0U;
        DeterministicNoise noise(0xbea401ceU+static_cast<std::uint32_t>(kind)*6151U
            +static_cast<std::uint32_t>(alternate)*977U);
        float low=0,slower=0;double phase=0;
        const auto pulse=[](float t,float begin,float duration) {
            const float u=(t-begin)/duration;
            if(u<=0||u>=1)return 0.0f;
            return SmoothStep(0,.12f,u)*(1-SmoothStep(.55f,1,u));
        };
        for(std::size_t i=0;i<frames;++i) {
            const float t=static_cast<float>(i)/AmbientSampleRate;
            const float raw=noise.Next();low+=.10f*(raw-low);slower+=.022f*(low-slower);
            float envelope=0;
            if(kind==BearCallKind::Breath)envelope=pulse(t,.12f,1.16f);
            else if(kind==BearCallKind::Snuffle)envelope=pulse(t,.10f,.35f)
                +.82f*pulse(t,alternate?.63f:.56f,.30f)+.48f*pulse(t,1.03f,.22f);
            else envelope=pulse(t,.09f,.54f)+(alternate?.58f:.36f)*pulse(t,.72f,.42f);
            const float frequency=(kind==BearCallKind::Huff?58.0f:kind==BearCallKind::Snuffle?92.0f:47.0f)
                +(alternate?5.0f:-3.0f)+4.0f*slower;
            phase+=2*std::numbers::pi*frequency/AmbientSampleRate;
            const float chest=static_cast<float>(std::sin(phase)+.48*std::sin(2*phase)+.21*std::sin(3*phase));
            const float breath=2.2f*low+1.25f*(raw-low);
            const float voice=kind==BearCallKind::Breath?breath*.64f+chest*.035f
                :kind==BearCallKind::Snuffle?breath*.93f+chest*.10f
                :breath*.72f+chest*.62f;
            samples[i]=envelope*voice;
        }
        return Quantize(samples,kind==BearCallKind::Huff?.62f:kind==BearCallKind::Snuffle?.48f:.46f);
    }

    float BearCallVolume(const BearCallKind kind,const float distance) noexcept
    {
        if(!std::isfinite(distance)||distance<0)return 0;
        const float radius=kind==BearCallKind::Breath?12.0f:kind==BearCallKind::Snuffle?26.0f:
            kind==BearCallKind::Huff?55.0f:0.0f;
        if(distance>=radius)return 0;
        const float base=kind==BearCallKind::Breath?.13f:kind==BearCallKind::Snuffle?.28f:.46f;
        const float fade=1-distance/radius;return base*fade*fade;
    }

    float BearCallScheduler::NextUnit() noexcept
    {
        randomState_^=randomState_<<13U;randomState_^=randomState_>>17U;randomState_^=randomState_<<5U;
        return static_cast<float>(randomState_&0x00ffffffU)/16777216.0f;
    }
    void BearCallScheduler::Reset() noexcept
    {
        randomState_=0xbea401ceU;cooldown_=1.5f+NextUnit()*1.5f;callPhase_=0;
    }
    BearCallRequest BearCallScheduler::Update(float deltaSeconds,float listenerX,float listenerY,
        float listenerZ,const BearState& bear,bool playbackSlotAvailable) noexcept
    {
        const float dt=SafeDeltaSeconds(deltaSeconds);
        if(dt<=0)return {};
        cooldown_=std::max(0.0f,cooldown_-dt);
        if(!playbackSlotAvailable||cooldown_>0||!std::isfinite(listenerX)||!std::isfinite(listenerY)
            ||!std::isfinite(listenerZ)||!std::isfinite(bear.x)||!std::isfinite(bear.y)||!std::isfinite(bear.z))return {};
        BearCallKind kind=BearCallKind::Breath;float interval=11;
        if(bear.behavior==BearBehavior::Withdraw){kind=BearCallKind::Huff;interval=6;}
        else if(bear.behavior==BearBehavior::Forage){kind=BearCallKind::Snuffle;interval=5;}
        else if(bear.behavior==BearBehavior::Trot||bear.behavior==BearBehavior::Walk){interval=8;}
        if(callPhase_==0&&kind!=BearCallKind::Huff)kind=BearCallKind::Snuffle;
        const float sourceY=bear.y+1.15f;
        const float distance=std::hypot(std::hypot(bear.x-listenerX,bear.z-listenerZ),sourceY-listenerY);
        const float volume=BearCallVolume(kind,distance);
        if(volume<.006f)return {};
        BearCallRequest request;request.kind=kind;request.variant=static_cast<std::uint8_t>(NextUnit()>=.5f);
        request.volume=volume*(.88f+.12f*NextUnit());request.pitch=-.08f+.11f*NextUnit();
        request.source={true,bear.x,sourceY,bear.z};cooldown_=interval+NextUnit()*7;
        callPhase_=static_cast<std::uint8_t>((callPhase_+1U)%3U);return request;
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

    UnderwaterAudioMix UnderwaterAudioResponse(const float submergedDepth) noexcept
    {
        const float safeDepth=std::isfinite(submergedDepth)
            ?std::max(0.0f,submergedDepth):0.0f;
        const float immersion=SmoothStep(.08f,1.2f,safeDepth);
        return {immersion,std::lerp(1.0f,.18f,immersion),
            std::lerp(1.0f,.72f,immersion),std::lerp(1.0f,1.22f,immersion),
            std::lerp(0.0f,-.12f,immersion)};
    }

    SoundscapeMix SoundscapeModel::TargetMix(const float signedShoreDistance,
                                             const bool swimming,
                                             const float coastalWindExposure,
                                             const float streamPresence) noexcept
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
        const float safeStreamPresence = std::isfinite(streamPresence)
            ? std::clamp(streamPresence, 0.0f, 1.0f) : 0.0f;
        result.streamVolume = safeStreamPresence * 0.23f;
        result.jungleVolume += safeStreamPresence * 0.024f;
        result.breezeVolume *= 1.0f - safeStreamPresence * 0.12f;
        const float windScale=CoastalWindResponse::AmbientScale(coastalWindExposure);
        result.surfVolume*=windScale;
        result.breezeVolume*=windScale;
        if (swimming) {
            result.surfVolume = std::max(result.surfVolume, 0.35f);
            result.breezeVolume *= 0.72f;
            result.jungleVolume *= 0.08f;
            result.streamVolume *= 0.45f;
        }
        return result;
    }

    void SoundscapeModel::Reset(const float signedShoreDistance, const bool swimming,
                                const float coastalWindExposure,
                                const float streamPresence) noexcept
    {
        mix_ = TargetMix(signedShoreDistance, swimming, coastalWindExposure,
                         streamPresence);
        variationSeconds_ = 0.0;
    }

    SoundscapeMix SoundscapeModel::Update(const float signedShoreDistance, const bool swimming,
                                          const float deltaSeconds,
                                          const float coastalWindExposure,
                                          const float streamPresence) noexcept
    {
        SoundscapeMix target = TargetMix(signedShoreDistance, swimming,
                                         coastalWindExposure, streamPresence);
        const float safeDelta = SafeDeltaSeconds(deltaSeconds);
        variationSeconds_ += static_cast<double>(safeDelta);
        // Long, mismatched periods keep stationary ambience from becoming a fixed wall of sound.
        // Gains only dip below the authored zone mix, preserving the existing volume ceilings.
        target.surfVolume *= SlowBedGain(variationSeconds_, 0.035f, 0.033, 0.071);
        target.breezeVolume *= SlowBedGain(variationSeconds_, 0.075f, 0.024, 0.055);
        target.jungleVolume *= SlowBedGain(variationSeconds_, 0.055f, 0.029, 0.063);
        target.streamVolume *= SlowBedGain(variationSeconds_, 0.045f, 0.026, 0.067);
        const float blend = 1.0f - std::exp(-safeDelta / 1.8f);
        const float jungleBlend = 1.0f - std::exp(-safeDelta / 2.6f);
        mix_.surfVolume += (target.surfVolume - mix_.surfVolume) * blend;
        mix_.breezeVolume += (target.breezeVolume - mix_.breezeVolume) * blend;
        mix_.jungleVolume += (target.jungleVolume - mix_.jungleVolume) * jungleBlend;
        mix_.streamVolume += (target.streamVolume - mix_.streamVolume) * blend;
        return mix_;
    }
}
