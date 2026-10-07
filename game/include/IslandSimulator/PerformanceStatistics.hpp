#pragma once

#include <cstddef>
#include <span>

namespace island
{
    /** @brief Stable summary of positive wall-clock frame-duration samples. */
    struct FrameTimeSummary
    {
        std::size_t sampleCount = 0U;
        double minimumMilliseconds = 0.0;
        double maximumMilliseconds = 0.0;
        double meanMilliseconds = 0.0;
        double medianMilliseconds = 0.0;
        double percentile95Milliseconds = 0.0;
        double percentile99Milliseconds = 0.0;
        double meanFramesPerSecond = 0.0;
        std::size_t framesOver120HzBudget = 0U;
        std::size_t framesOver60HzBudget = 0U;
        std::size_t framesOver30HzBudget = 0U;
    };

    /**
     * @brief Calculates interpolated percentiles and arithmetic mean for frame durations.
     * @throws std::invalid_argument when the sample set is empty or contains a non-positive or
     *         non-finite duration.
     */
    [[nodiscard]] FrameTimeSummary CalculateFrameTimeSummary(
        std::span<const double> frameMilliseconds);
}
