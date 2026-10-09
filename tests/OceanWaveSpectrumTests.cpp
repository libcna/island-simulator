#include "IslandSimulator/OceanWaveSpectrum.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>

namespace
{
    void Check(const bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    double HeightRange(double x, double z, double time, std::size_t first, std::size_t last)
    {
        double height = 0;
        for (auto index = first; index < last; ++index) {
            const auto& wave = island::OceanWaves[index];
            const auto& a = island::OceanWarpX;
            const auto& b = island::OceanWarpZ;
            const bool detail = index >= island::OceanBroadNormalWaveCount;
            const double px = x + (detail ? a.amplitude * std::sin(x * a.waveX + z * a.waveZ
                                                  + time * a.angularSpeed + a.phase) : 0);
            const double pz = z + (detail ? b.amplitude * std::sin(x * b.waveX + z * b.waveZ
                                                  + time * b.angularSpeed + b.phase) : 0);
            double envelope = 1;
            if (detail) {
                const auto& offset = island::OceanPacketPhases[index-island::OceanBroadNormalWaveCount];
                envelope = island::OceanPacketMean + island::OceanPacketVariation
                    * std::sin(x*a.waveX + z*a.waveZ + time*a.angularSpeed + a.phase + offset[0])
                    * std::sin(x*b.waveX + z*b.waveZ + time*b.angularSpeed + b.phase + offset[1]);
            }
            height += std::sin(px * wave.waveX + pz * wave.waveZ
                              + time * wave.angularSpeed + wave.phase) * wave.amplitude * envelope;
        }
        return height;
    }

    class CommaDecimal final : public std::numpunct<char>
    {
        char do_decimal_point() const override { return ','; }
    };
}

int main()
{
    try {
        double heightBound = 0, slopeBound = 0, heightSpeedBound = 0, slopeSpeedBound = 0;
        const auto& warpX = island::OceanWarpX;
        const auto& warpZ = island::OceanWarpZ;
        const double warpGradientX = std::hypot(warpX.waveX, warpX.waveZ) * warpX.amplitude;
        const double warpGradientZ = std::hypot(warpZ.waveX, warpZ.waveZ) * warpZ.amplitude;
        const double jacobianBound = 1 + std::hypot(warpGradientX, warpGradientZ);
        const double envelopeGradient = island::OceanPacketVariation
            * (std::hypot(warpX.waveX,warpX.waveZ) + std::hypot(warpZ.waveX,warpZ.waveZ));
        const double envelopeSpeed = island::OceanPacketVariation * (warpX.angularSpeed+warpZ.angularSpeed);
        const double envelopeGradientSpeed = envelopeGradient * (warpX.angularSpeed+warpZ.angularSpeed);
        Check(island::OceanPacketMean-island::OceanPacketVariation > 0.29f
                  && island::OceanPacketMean+island::OceanPacketVariation <= 1,
              "Wind packets must vary strength without reversing or amplifying the waves");
        Check((1 - std::abs(warpX.waveX * warpX.amplitude))
                  * (1 - std::abs(warpZ.waveZ * warpZ.amplitude))
                  - std::abs(warpX.waveZ * warpX.amplitude * warpZ.waveX * warpZ.amplitude) > 0.7,
              "Short-wave domain warp must never fold the surface");
        for (std::size_t index = 0; index < island::OceanWaves.size(); ++index) {
            const auto& wave = island::OceanWaves[index];
            const double slope = std::hypot(wave.waveX, wave.waveZ) * wave.amplitude;
            Check(wave.amplitude > 0 && slope > 0 && std::isfinite(slope)
                      && std::isfinite(wave.phase) && wave.angularSpeed > 0,
                  "Wave coefficients must be finite, positive in energy and nonstationary");
            const bool detail = index >= island::OceanBroadNormalWaveCount;
            slopeBound += slope * (detail ? jacobianBound : 1);
            if (detail) slopeBound += wave.amplitude*envelopeGradient;
            const double phaseSpeed = wave.angularSpeed + (detail
                ? std::abs(wave.waveX) * warpX.amplitude * warpX.angularSpeed
                    + std::abs(wave.waveZ) * warpZ.amplitude * warpZ.angularSpeed : 0);
            slopeSpeedBound += slope * phaseSpeed * (detail ? jacobianBound : 1);
            if (detail) slopeSpeedBound += slope * std::hypot(
                warpGradientX * warpX.angularSpeed, warpGradientZ * warpZ.angularSpeed);
            if (detail) slopeSpeedBound += slope*jacobianBound*envelopeSpeed
                + wave.amplitude*(phaseSpeed*envelopeGradient + envelopeGradientSpeed);
            if (index < island::OceanDisplacementWaveCount) {
                heightBound += wave.amplitude;
                heightSpeedBound += wave.amplitude * wave.angularSpeed;
            }
        }
        Check(heightBound < 0.194 && slopeBound < 0.21,
              "Calm ocean must retain its displacement and shading-slope envelopes");
        unsigned samples = 0;
        std::array<double,4> minimumEnvelope{1,1,1,1}, maximumEnvelope{};
        for (int row = -30; row <= 30; ++row) {
            for (int column = -30; column <= 30; ++column) {
                const float x = column * 27.3f, z = row * 27.1f;
                for (float time : {0.0f, 0.1f, 1.3f, 8.7f, 63.0f, 211.0f}) {
                    const auto sample = island::SampleOceanWaves(x, z, time);
                    for (std::size_t packet=0; packet<island::OceanPacketPhases.size(); ++packet) {
                        const auto& offset = island::OceanPacketPhases[packet];
                        const double e = island::OceanPacketMean + island::OceanPacketVariation
                            * std::sin(x*warpX.waveX + z*warpX.waveZ + time*warpX.angularSpeed + warpX.phase + offset[0])
                            * std::sin(x*warpZ.waveX + z*warpZ.waveZ + time*warpZ.angularSpeed + warpZ.phase + offset[1]);
                        Check(e >= 0.299999 && e <= 1.000001,"Packet strength stays inside the authored range");
                        minimumEnvelope[packet]=std::min(minimumEnvelope[packet],e);
                        maximumEnvelope[packet]=std::max(maximumEnvelope[packet],e);
                    }
                    const auto later = island::SampleOceanWaves(x, z, time + 0.001f);
                    const float slopeX = sample.broadSlopeX + sample.detailSlopeX;
                    const float slopeZ = sample.broadSlopeZ + sample.detailSlopeZ;
                    Check(std::abs(sample.height) <= heightBound + 0.00001
                              && std::hypot(slopeX, slopeZ) <= slopeBound + 0.00001,
                          "Sample must obey the analytic envelope");
                    Check(std::abs(sample.height - later.height) < heightSpeedBound * 0.0011 + 0.00001
                              && std::hypot(slopeX - later.broadSlopeX - later.detailSlopeX,
                                            slopeZ - later.broadSlopeZ - later.detailSlopeZ)
                                  < slopeSpeedBound * 0.0011 + 0.0001,
                          "Wave phase must change continuously without wrapping jumps");
                    // Independent double-precision finite differences of each height band.
                    const auto derivative = [x, z, time](std::size_t first, std::size_t last,
                                                        bool alongX) {
                        constexpr double step = 0.002;
                        return (HeightRange(x + (alongX ? step : 0), z + (alongX ? 0 : step),
                                            time, first, last)
                              - HeightRange(x - (alongX ? step : 0), z - (alongX ? 0 : step),
                                            time, first, last)) / (2 * step);
                    };
                    Check(std::abs(sample.broadSlopeX - derivative(0, 5, true)) < 0.00005
                              && std::abs(sample.broadSlopeZ - derivative(0, 5, false)) < 0.00005
                              && std::abs(sample.detailSlopeX - derivative(5, 9, true)) < 0.0003
                              && std::abs(sample.detailSlopeZ - derivative(5, 9, false)) < 0.0003,
                          "Analytic slope must agree with independent height derivatives");
                    const auto filtered = island::SampleOceanWaves(x, z, time, {4, 0, 0, 4});
                    Check(filtered.height == sample.height && filtered.detailSlopeX == 0
                              && filtered.detailSlopeZ == 0,
                          "Screen-space filtering must remove subpixel detail, not displace geometry");
                    ++samples;
                }
            }
        }
        float previous = 1;
        for (std::size_t packet=0; packet<minimumEnvelope.size(); ++packet) {
            Check(minimumEnvelope[packet]<0.32 && maximumEnvelope[packet]>0.98,
                  "Every short wave traverses both quiet and stronger wind patches");
        }
        for (int step = 0; step <= 1000; ++step) {
            const float phase = static_cast<float>(step) / 100;
            const float weight = island::OceanWaveFilter(phase);
            Check(weight >= 0 && weight <= previous && weight == island::OceanWaveFilter(-phase),
                  "Wave filter must be bounded, symmetric and monotonic");
            previous = weight;
        }
        Check(island::OceanWaveFilter(1.0f) == 1 && island::OceanWaveFilter(3.0f) == 0,
              "Resolved waves must survive; waves beyond Nyquist must vanish");
        for (float invalid : {std::numeric_limits<float>::infinity(),
                               std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::max()}) {
            const auto sample = island::SampleOceanWaves(invalid, invalid, invalid);
            Check(sample.height == 0 && sample.broadSlopeX == 0 && sample.detailSlopeZ == 0,
                  "Invalid/overflowing wave inputs must return finite calm water");
        }
        const auto noFootprint = island::SampleOceanWaves(7, 3, 2,
            {std::numeric_limits<float>::quiet_NaN(), 0, 0, 0});
        Check(noFootprint.height == 0, "Invalid footprint must fail closed");
        const auto heightGlsl = island::OceanDisplacementGlsl();
        const auto slopesGlsl = island::OceanSlopesGlsl();
        const auto oldLocale = std::locale();
        std::locale::global(std::locale(std::locale::classic(), new CommaDecimal));
        const bool stable = heightGlsl == island::OceanDisplacementGlsl()
            && slopesGlsl == island::OceanSlopesGlsl();
        std::locale::global(oldLocale);
        Check(stable, "GLSL decimals must not depend on the user's locale");
        const auto count = [](const std::string& source, const std::string& token) {
            std::size_t total = 0, offset = 0;
            while ((offset = source.find(token, offset)) != std::string::npos) {
                ++total;
                offset += token.size();
            }
            return total;
        };
        Check(count(heightGlsl, "height += sin(") == island::OceanDisplacementWaveCount
                  && count(slopesGlsl, "slopes.xy +=") == island::OceanBroadNormalWaveCount
                  && count(slopesGlsl, "slopes.zw +=") == 4,
              "Portable GLSL must contain exactly the configured displacement and normal bands");
        Check(count(slopesGlsl,"envelopeGradient*sin(phase)")==4
                  && count(slopesGlsl," + sideband")==4,
              "All packet bands must include their product-rule slope and sideband filtering");
        std::cout << "Ocean wave spectrum tests passed: " << samples
                  << " samples, analytic packet gradients, filtering, bounds and portable GLSL; slope bound "
                  << slopeBound << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ocean wave spectrum test failure: " << error.what() << '\n';
        return 1;
    }
}
