#include "IslandSimulator/AudioAnalysis.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    AudioSignalStatistics AnalyzeAudioSignal(const AmbientLoop& signal) noexcept
    {
        AudioSignalStatistics result;
        result.sampleCount = signal.samples.size();
        result.sampleRate = signal.sampleRate;
        if (signal.samples.empty() || signal.sampleRate <= 0) {
            return result;
        }

        constexpr double PcmScale = 32768.0;
        double sum = 0.0;
        double squaredSum = 0.0;
        std::size_t zeroCrossings = 0U;
        std::int16_t previous = signal.samples.front();
        result.fingerprint = 14695981039346656037ULL;

        const auto addHashByte = [&result](const std::uint8_t value) noexcept {
            result.fingerprint ^= value;
            result.fingerprint *= 1099511628211ULL;
        };
        for (int shift = 0; shift < 32; shift += 8) {
            addHashByte(static_cast<std::uint8_t>(
                (static_cast<std::uint32_t>(signal.sampleRate) >> shift) & 0xffU));
        }

        for (std::size_t index = 0; index < signal.samples.size(); ++index) {
            const std::int16_t sample = signal.samples[index];
            const auto bits = static_cast<std::uint16_t>(sample);
            addHashByte(static_cast<std::uint8_t>(bits & 0xffU));
            addHashByte(static_cast<std::uint8_t>(bits >> 8U));

            const double normalized = static_cast<double>(sample) / PcmScale;
            sum += normalized;
            squaredSum += normalized * normalized;
            result.peak = std::max(result.peak, std::abs(normalized));
            if (index > 0U) {
                const double step = std::abs(
                    static_cast<double>(sample) - static_cast<double>(previous)) / PcmScale;
                result.maximumInternalStep = std::max(result.maximumInternalStep, step);
                if ((sample < 0 && previous >= 0) || (sample >= 0 && previous < 0)) {
                    ++zeroCrossings;
                }
            }
            previous = sample;
        }

        result.durationSeconds = static_cast<double>(result.sampleCount)
            / static_cast<double>(result.sampleRate);
        result.mean = sum / static_cast<double>(result.sampleCount);
        result.rootMeanSquare = std::sqrt(
            squaredSum / static_cast<double>(result.sampleCount));
        result.seamStep = std::abs(
            static_cast<double>(signal.samples.front())
            - static_cast<double>(signal.samples.back())) / PcmScale;
        result.zeroCrossingsPerSecond = static_cast<double>(zeroCrossings)
            / result.durationSeconds;
        result.finite = std::isfinite(result.durationSeconds)
            && std::isfinite(result.mean) && std::isfinite(result.rootMeanSquare)
            && std::isfinite(result.peak) && std::isfinite(result.seamStep)
            && std::isfinite(result.maximumInternalStep)
            && std::isfinite(result.zeroCrossingsPerSecond);
        return result;
    }
}
