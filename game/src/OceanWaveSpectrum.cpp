#include "IslandSimulator/OceanWaveSpectrum.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace island
{
    namespace
    {
        constexpr float FilterStart = 1.1f;
        constexpr float FilterEnd = 2.6f;

        std::ostringstream ShaderStream()
        {
            std::ostringstream stream;
            stream.imbue(std::locale::classic());
            stream << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10);
            return stream;
        }

        void WritePhase(std::ostream& stream, const OceanWave& wave, const char* point = "p")
        {
            stream << "dot(" << point << ", vec2(" << wave.waveX << ", " << wave.waveZ << ")) + time * "
                   << wave.angularSpeed << " + " << wave.phase;
        }
    }

    float OceanWaveFilter(const float phaseFootprint) noexcept
    {
        if (!std::isfinite(phaseFootprint)) return 0.0f;
        const float t = std::clamp((std::abs(phaseFootprint) - FilterStart)
                                   / (FilterEnd - FilterStart), 0.0f, 1.0f);
        return 1.0f - t * t * (3.0f - 2.0f * t);
    }

    OceanWaveSample SampleOceanWaves(const float x, const float z, const float timeSeconds,
                                      const OceanPixelFootprint footprint) noexcept
    {
        for (float value : {x, z, timeSeconds, footprint.dxX, footprint.dxZ,
                            footprint.dyX, footprint.dyZ}) {
            if (!std::isfinite(value)) return {};
        }
        OceanWaveSample sample;
        const float warpPhaseX = x * OceanWarpX.waveX + z * OceanWarpX.waveZ
            + timeSeconds * OceanWarpX.angularSpeed + OceanWarpX.phase;
        const float warpPhaseZ = x * OceanWarpZ.waveX + z * OceanWarpZ.waveZ
            + timeSeconds * OceanWarpZ.angularSpeed + OceanWarpZ.phase;
        if (!std::isfinite(warpPhaseX) || !std::isfinite(warpPhaseZ)) return {};
        const float sinX = std::sin(warpPhaseX), cosX = std::cos(warpPhaseX);
        const float sinZ = std::sin(warpPhaseZ), cosZ = std::cos(warpPhaseZ);
        const float detailX = x + OceanWarpX.amplitude * sinX;
        const float detailZ = z + OceanWarpZ.amplitude * sinZ;
        const float warpScaleX = OceanWarpX.amplitude * cosX;
        const float warpScaleZ = OceanWarpZ.amplitude * cosZ;
        for (std::size_t index = 0; index < OceanWaves.size(); ++index) {
            const auto& wave = OceanWaves[index];
            const bool detail = index >= OceanBroadNormalWaveCount;
            const float phase = (detail ? detailX : x) * wave.waveX
                + (detail ? detailZ : z) * wave.waveZ
                + timeSeconds * wave.angularSpeed + wave.phase;
            if (!std::isfinite(phase)) return {};
            if (index < OceanDisplacementWaveCount) {
                sample.height += std::sin(phase) * wave.amplitude;
            }
            const float gradientX = wave.waveX + (detail
                ? wave.waveX * warpScaleX * OceanWarpX.waveX
                    + wave.waveZ * warpScaleZ * OceanWarpZ.waveX : 0.0f);
            const float gradientZ = wave.waveZ + (detail
                ? wave.waveX * warpScaleX * OceanWarpX.waveZ
                    + wave.waveZ * warpScaleZ * OceanWarpZ.waveZ : 0.0f);
            const auto pixelPhaseAt = [&](float dx, float dz) {
                // Product envelopes create carrier +/- X +/- Z sidebands. Filter using their
                // conservative projected frequency, not the carrier alone.
                return std::abs(gradientX*dx + gradientZ*dz) + (detail
                    ? std::abs(OceanWarpX.waveX*dx + OceanWarpX.waveZ*dz)
                        + std::abs(OceanWarpZ.waveX*dx + OceanWarpZ.waveZ*dz) : 0.0f);
            };
            const float pixelPhase = std::max(pixelPhaseAt(footprint.dxX,footprint.dxZ),
                                               pixelPhaseAt(footprint.dyX,footprint.dyZ));
            const float scale = wave.amplitude * OceanWaveFilter(pixelPhase);
            if (index < OceanBroadNormalWaveCount) {
                sample.broadSlopeX += wave.waveX * scale * std::cos(phase);
                sample.broadSlopeZ += wave.waveZ * scale * std::cos(phase);
            } else {
                const auto& offset = OceanPacketPhases[index-OceanBroadNormalWaveCount];
                const float sx = sinX*std::cos(offset[0]) + cosX*std::sin(offset[0]);
                const float cx = cosX*std::cos(offset[0]) - sinX*std::sin(offset[0]);
                const float sz = sinZ*std::cos(offset[1]) + cosZ*std::sin(offset[1]);
                const float cz = cosZ*std::cos(offset[1]) - sinZ*std::sin(offset[1]);
                const float envelope = OceanPacketMean + OceanPacketVariation*sx*sz;
                const float envelopeX = OceanPacketVariation
                    * (cx*sz*OceanWarpX.waveX + sx*cz*OceanWarpZ.waveX);
                const float envelopeZ = OceanPacketVariation
                    * (cx*sz*OceanWarpX.waveZ + sx*cz*OceanWarpZ.waveZ);
                sample.detailSlopeX += scale * (std::cos(phase)*envelope*gradientX + std::sin(phase)*envelopeX);
                sample.detailSlopeZ += scale * (std::cos(phase)*envelope*gradientZ + std::sin(phase)*envelopeZ);
            }
        }
        return sample;
    }

    std::string OceanDisplacementGlsl()
    {
        auto stream = ShaderStream();
        stream << "float oceanHeight(vec2 p, float time) {\n    float height = 0.0;\n";
        for (std::size_t index = 0; index < OceanDisplacementWaveCount; ++index) {
            const auto& wave = OceanWaves[index];
            stream << "    height += sin(";
            WritePhase(stream, wave);
            stream << ") * " << wave.amplitude << ";\n";
        }
        stream << "    return height;\n}\n";
        return stream.str();
    }

    std::string OceanSlopesGlsl()
    {
        auto stream = ShaderStream();
        stream << "float oceanWaveFilter(float phaseFootprint) {\n"
               << "    return 1.0 - smoothstep(" << FilterStart << ", " << FilterEnd
               << ", phaseFootprint);\n}\n"
               << "vec4 oceanSlopes(vec2 p, float time, vec2 pixelDx, vec2 pixelDy) {\n"
               << "    vec4 slopes = vec4(0.0);\n";
        stream << "    float warpPhaseX = ";
        WritePhase(stream, OceanWarpX);
        stream << ";\n    float warpPhaseZ = ";
        WritePhase(stream, OceanWarpZ);
        stream << ";\n    vec2 warpS = sin(vec2(warpPhaseX, warpPhaseZ));\n"
               << "    vec2 warpC = cos(vec2(warpPhaseX, warpPhaseZ));\n"
               << "    vec2 warpKX = vec2(" << OceanWarpX.waveX << ", " << OceanWarpX.waveZ << ");\n"
               << "    vec2 warpKZ = vec2(" << OceanWarpZ.waveX << ", " << OceanWarpZ.waveZ << ");\n"
               << "    vec2 detailP = p + vec2(" << OceanWarpX.amplitude << ", " << OceanWarpZ.amplitude << ") * warpS;\n"
               << "    vec2 warpGradientX = warpKX * (" << OceanWarpX.amplitude << " * warpC.x);\n"
               << "    vec2 warpGradientZ = warpKZ * (" << OceanWarpZ.amplitude << " * warpC.y);\n"
               << "    vec2 sideband = abs(vec2(dot(warpKX,pixelDx),dot(warpKX,pixelDy)))\n"
               << "        + abs(vec2(dot(warpKZ,pixelDx),dot(warpKZ,pixelDy)));\n";
        for (std::size_t index = 0; index < OceanWaves.size(); ++index) {
            const auto& wave = OceanWaves[index];
            stream << "    { vec2 k = vec2(" << wave.waveX << ", " << wave.waveZ << ");\n";
            const bool detail = index >= OceanBroadNormalWaveCount;
            if (detail) {
                stream << "      k += k.x * warpGradientX + k.y * warpGradientZ;\n";
            }
            stream << "      vec2 footprint = abs(vec2(dot(k,pixelDx),dot(k,pixelDy)))"
                   << (detail ? " + sideband" : "") << ";\n"
                   << "      float weight = oceanWaveFilter(max(footprint.x,footprint.y));\n"
                   << "      float phase = ";
            WritePhase(stream, wave, detail ? "detailP" : "p");
            stream << ";\n";
            if (detail) {
                const auto& offset = OceanPacketPhases[index-OceanBroadNormalWaveCount];
                stream << "      vec2 offsetC = vec2(" << std::cos(offset[0]) << ", " << std::cos(offset[1]) << ");\n"
                       << "      vec2 offsetS = vec2(" << std::sin(offset[0]) << ", " << std::sin(offset[1]) << ");\n"
                       << "      vec2 packetS = warpS*offsetC + warpC*offsetS;\n"
                       << "      vec2 packetC = warpC*offsetC - warpS*offsetS;\n"
                       << "      float envelope = " << OceanPacketMean << " + " << OceanPacketVariation << "*packetS.x*packetS.y;\n"
                       << "      vec2 envelopeGradient = " << OceanPacketVariation
                       << " * (packetC.x*packetS.y*warpKX + packetS.x*packetC.y*warpKZ);\n"
                       << "      slopes.zw += " << wave.amplitude
                       << " * weight * (k*cos(phase)*envelope + envelopeGradient*sin(phase));\n";
            } else {
                stream << "      slopes.xy += k * (" << wave.amplitude << " * weight) * cos(phase);\n";
            }
            stream << "    }\n";
        }
        stream << "    return slopes;\n}\n";
        return stream.str();
    }
}
