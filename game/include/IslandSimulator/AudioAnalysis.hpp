#pragma once

#include "IslandSimulator/SoundscapeModel.hpp"

#include <cstddef>
#include <cstdint>

namespace island
{
    /** @brief Deterministic signal measurements used by the host audio-quality report. */
    struct AudioSignalStatistics
    {
        std::size_t sampleCount = 0U;
        int sampleRate = 0;
        double durationSeconds = 0.0;
        double mean = 0.0;
        double rootMeanSquare = 0.0;
        double peak = 0.0;
        double seamStep = 0.0;
        double maximumInternalStep = 0.0;
        double zeroCrossingsPerSecond = 0.0;
        std::uint64_t fingerprint = 0U;
        bool finite = false;
    };

    /** @brief Measures normalized PCM16 values without changing or allocating sample data. */
    [[nodiscard]] AudioSignalStatistics AnalyzeAudioSignal(
        const AmbientLoop& signal) noexcept;
}
