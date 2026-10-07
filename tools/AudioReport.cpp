#include "IslandSimulator/AudioAnalysis.hpp"
#include "IslandSimulator/SoundscapeModel.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    struct AudioClip final
    {
        AudioClip(const char* clipName, const char* clipFileName, const bool isLooped,
                  const std::size_t sampleCount, island::AmbientLoop clipSignal)
            : name(clipName),
              fileName(clipFileName),
              looped(isLooped),
              expectedSampleCount(sampleCount),
              signal(std::move(clipSignal))
        {
        }

        const char* name;
        const char* fileName;
        bool looped;
        std::size_t expectedSampleCount;
        island::AmbientLoop signal;
        island::AudioSignalStatistics statistics;
        bool valid = false;
    };

    void WriteUint16(std::ostream& output, const std::uint16_t value)
    {
        const std::array bytes = {
            static_cast<char>(value & 0xffU),
            static_cast<char>((value >> 8U) & 0xffU)};
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    void WriteUint32(std::ostream& output, const std::uint32_t value)
    {
        const std::array bytes = {
            static_cast<char>(value & 0xffU),
            static_cast<char>((value >> 8U) & 0xffU),
            static_cast<char>((value >> 16U) & 0xffU),
            static_cast<char>((value >> 24U) & 0xffU)};
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    void WriteWaveFile(const std::filesystem::path& path,
                       const island::AmbientLoop& signal)
    {
        const std::size_t byteCount = signal.samples.size() * sizeof(std::int16_t);
        if (signal.sampleRate <= 0
            || byteCount > std::numeric_limits<std::uint32_t>::max() - 36U) {
            throw std::runtime_error("audio clip cannot be represented as a PCM WAV file");
        }

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("cannot create WAV preview: " + path.string());
        }
        output.write("RIFF", 4);
        WriteUint32(output, static_cast<std::uint32_t>(36U + byteCount));
        output.write("WAVEfmt ", 8);
        WriteUint32(output, 16U);
        WriteUint16(output, 1U);
        WriteUint16(output, 1U);
        WriteUint32(output, static_cast<std::uint32_t>(signal.sampleRate));
        WriteUint32(output, static_cast<std::uint32_t>(signal.sampleRate * 2));
        WriteUint16(output, 2U);
        WriteUint16(output, 16U);
        output.write("data", 4);
        WriteUint32(output, static_cast<std::uint32_t>(byteCount));
        for (const std::int16_t sample : signal.samples) {
            WriteUint16(output, static_cast<std::uint16_t>(sample));
        }
        if (!output) {
            throw std::runtime_error("failed while writing WAV preview: " + path.string());
        }
    }

    [[nodiscard]] bool ValidateClip(const AudioClip& clip) noexcept
    {
        const island::AudioSignalStatistics& statistics = clip.statistics;
        if (!statistics.finite || statistics.sampleRate != 22050
            || statistics.sampleCount != clip.expectedSampleCount
            || statistics.peak < 0.45 || statistics.peak > 0.95
            || statistics.rootMeanSquare < 0.015
            || statistics.rootMeanSquare > 0.75
            || std::abs(statistics.mean) > 0.02
            || statistics.maximumInternalStep > 1.0
            || statistics.zeroCrossingsPerSecond <= 1.0
            || statistics.zeroCrossingsPerSecond > 22050.0) {
            return false;
        }
        if (clip.looped) {
            return std::abs(statistics.mean) <= 0.0001
                && statistics.seamStep < 0.25
                && statistics.seamStep
                    <= statistics.maximumInternalStep * 1.25 + 1.0 / 32768.0;
        }
        return clip.signal.samples.front() == 0 && clip.signal.samples.back() == 0;
    }

    [[nodiscard]] std::string FingerprintString(const std::uint64_t fingerprint)
    {
        std::ostringstream output;
        output << std::hex << std::setfill('0') << std::setw(16) << fingerprint;
        return output.str();
    }

    void WriteReport(std::ostream& output, const std::vector<AudioClip>& clips,
                     const std::size_t totalPcmBytes, const bool valid)
    {
        output.imbue(std::locale::classic());
        output << std::fixed << std::setprecision(8)
               << "{\n"
               << "  \"schemaVersion\": 1,\n"
               << "  \"valid\": " << (valid ? "true" : "false") << ",\n"
               << "  \"clipCount\": " << clips.size() << ",\n"
               << "  \"totalPcmBytes\": " << totalPcmBytes << ",\n"
               << "  \"clips\": [\n";
        for (std::size_t index = 0; index < clips.size(); ++index) {
            const AudioClip& clip = clips[index];
            const island::AudioSignalStatistics& statistics = clip.statistics;
            output << "    {\n"
                   << "      \"name\": \"" << clip.name << "\",\n"
                   << "      \"fileName\": \"" << clip.fileName << "\",\n"
                   << "      \"looped\": " << (clip.looped ? "true" : "false") << ",\n"
                   << "      \"valid\": " << (clip.valid ? "true" : "false") << ",\n"
                   << "      \"sampleRate\": " << statistics.sampleRate << ",\n"
                   << "      \"sampleCount\": " << statistics.sampleCount << ",\n"
                   << "      \"durationSeconds\": " << statistics.durationSeconds << ",\n"
                   << "      \"mean\": " << statistics.mean << ",\n"
                   << "      \"rootMeanSquare\": " << statistics.rootMeanSquare << ",\n"
                   << "      \"peak\": " << statistics.peak << ",\n"
                   << "      \"seamStep\": " << statistics.seamStep << ",\n"
                   << "      \"maximumInternalStep\": "
                   << statistics.maximumInternalStep << ",\n"
                   << "      \"zeroCrossingsPerSecond\": "
                   << statistics.zeroCrossingsPerSecond << ",\n"
                   << "      \"fingerprint\": \""
                   << FingerprintString(statistics.fingerprint) << "\"\n"
                   << "    }" << (index + 1U == clips.size() ? "\n" : ",\n");
        }
        output << "  ]\n}\n";
    }

    [[nodiscard]] std::vector<AudioClip> GenerateClips()
    {
        std::vector<AudioClip> clips;
        clips.reserve(12U);
        clips.push_back({"shoreline surf", "surf.wav", true, 220500U,
                         island::GenerateSurfLoop()});
        clips.push_back({"canopy breeze", "breeze.wav", true, 242550U,
                         island::GenerateBreezeLoop()});
        clips.push_back({"daytime jungle", "jungle.wav", true, 639450U,
                         island::GenerateJungleLoop()});
        clips.push_back({"player water movement", "water-movement.wav", true, 154350U,
                         island::GenerateWaterMovementLoop()});
        clips.push_back({"stronger wave A", "stronger-wave-a.wav", false, 88200U,
                         island::GenerateWaveAccent()});
        clips.push_back({"stronger wave B", "stronger-wave-b.wav", false, 88200U,
                         island::GenerateWaveAccentAlternate()});
        clips.push_back({"water transition A", "water-transition-a.wav", false, 33075U,
                         island::GenerateWaterTransitionAccent()});
        clips.push_back({"water transition B", "water-transition-b.wav", false, 33075U,
                         island::GenerateWaterTransitionAccentAlternate()});
        clips.push_back({"bird call A", "bird-call-a.wav", false, 26460U,
                         island::GenerateBirdCall()});
        clips.push_back({"bird call B", "bird-call-b.wav", false, 29767U,
                         island::GenerateBirdCallAlternate()});
        clips.push_back({"monkey call A", "monkey-call-a.wav", false, 39690U,
                         island::GenerateMonkeyCall()});
        clips.push_back({"monkey call B", "monkey-call-b.wav", false, 46305U,
                         island::GenerateMonkeyCallAlternate()});
        return clips;
    }
}

int main(const int argc, char* argv[])
{
    std::string outputPath;
    std::string waveDirectory;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--output" && index + 1 < argc && outputPath.empty()) {
            outputPath = argv[++index];
        } else if (argument == "--wav-directory" && index + 1 < argc
                   && waveDirectory.empty()) {
            waveDirectory = argv[++index];
        } else {
            std::cerr << "usage: island-audio-report [--output report.json] "
                         "[--wav-directory directory]\n";
            return 2;
        }
    }

    try {
        std::vector<AudioClip> clips = GenerateClips();
        std::size_t totalPcmBytes = 0U;
        bool valid = true;
        std::vector<std::uint64_t> fingerprints;
        fingerprints.reserve(clips.size());
        for (AudioClip& clip : clips) {
            clip.statistics = island::AnalyzeAudioSignal(clip.signal);
            clip.valid = ValidateClip(clip);
            valid = valid && clip.valid;
            totalPcmBytes += clip.signal.samples.size() * sizeof(std::int16_t);
            for (const std::uint64_t fingerprint : fingerprints) {
                valid = valid && fingerprint != clip.statistics.fingerprint;
            }
            fingerprints.push_back(clip.statistics.fingerprint);
        }

        if (!waveDirectory.empty()) {
            const std::filesystem::path directory(waveDirectory);
            std::filesystem::create_directories(directory);
            for (const AudioClip& clip : clips) {
                WriteWaveFile(directory / clip.fileName, clip.signal);
            }
        }

        if (outputPath.empty()) {
            WriteReport(std::cout, clips, totalPcmBytes, valid);
        } else {
            std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error("cannot write audio report to " + outputPath);
            }
            WriteReport(output, clips, totalPcmBytes, valid);
            if (!output) {
                throw std::runtime_error("failed while writing audio report to " + outputPath);
            }
        }

        std::ostream& status = outputPath.empty() ? std::cerr : std::cout;
        status << "island-simulator: audio report " << (valid ? "valid" : "invalid")
               << ", " << clips.size() << " clips, " << totalPcmBytes
               << " PCM bytes\n";
        return valid ? 0 : 1;
    } catch (const std::exception& exception) {
        std::cerr << "island-simulator: audio report failed: " << exception.what() << '\n';
        return 1;
    }
}
