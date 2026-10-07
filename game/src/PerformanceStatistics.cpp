#include "IslandSimulator/PerformanceStatistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace island
{
    namespace
    {
        [[nodiscard]] double Percentile(const std::vector<double>& sortedSamples,
                                        const double percentile) noexcept
        {
            const double position = percentile
                * static_cast<double>(sortedSamples.size() - 1U);
            const auto lowerIndex = static_cast<std::size_t>(std::floor(position));
            const auto upperIndex = static_cast<std::size_t>(std::ceil(position));
            const double fraction = position - static_cast<double>(lowerIndex);
            return std::lerp(sortedSamples[lowerIndex], sortedSamples[upperIndex], fraction);
        }
    }

    FrameTimeSummary CalculateFrameTimeSummary(
        const std::span<const double> frameMilliseconds)
    {
        if (frameMilliseconds.empty()) {
            throw std::invalid_argument("Frame-time statistics require at least one sample");
        }
        for (const double sample : frameMilliseconds) {
            if (!std::isfinite(sample) || sample <= 0.0) {
                throw std::invalid_argument(
                    "Frame-time statistics require finite positive samples");
            }
        }

        std::vector<double> sortedSamples(frameMilliseconds.begin(), frameMilliseconds.end());
        std::ranges::sort(sortedSamples);
        const double mean = std::accumulate(sortedSamples.begin(), sortedSamples.end(), 0.0)
            / static_cast<double>(sortedSamples.size());
        const auto countOverBudget = [&sortedSamples](const double budgetMilliseconds) {
            return static_cast<std::size_t>(std::ranges::count_if(
                sortedSamples,
                [budgetMilliseconds](const double sample) {
                    return sample > budgetMilliseconds;
                }));
        };
        return {
            .sampleCount = sortedSamples.size(),
            .minimumMilliseconds = sortedSamples.front(),
            .maximumMilliseconds = sortedSamples.back(),
            .meanMilliseconds = mean,
            .medianMilliseconds = Percentile(sortedSamples, 0.50),
            .percentile95Milliseconds = Percentile(sortedSamples, 0.95),
            .percentile99Milliseconds = Percentile(sortedSamples, 0.99),
            .meanFramesPerSecond = 1000.0 / mean,
            .framesOver120HzBudget = countOverBudget(1000.0 / 120.0),
            .framesOver60HzBudget = countOverBudget(1000.0 / 60.0),
            .framesOver30HzBudget = countOverBudget(1000.0 / 30.0),
        };
    }
}
