#include "IslandSimulator/AquaticLightingModel.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    AquaticLightProfile AquaticLightAt(const float x, const float y, const float z,
                                       const float timeSeconds) noexcept
    {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)
            || !std::isfinite(timeSeconds)) {
            return {};
        }
        const float depth = std::clamp(-y, 0.0f, 8.0f);
        const float normalizedDepth = std::clamp((depth - 0.35f) / 7.65f, 0.0f, 1.0f);
        const float depthBlend = normalizedDepth * normalizedDepth
            * (3.0f - 2.0f * normalizedDepth);
        const float crossingBands = 0.5f * (
            std::sin(x * 0.21f + z * 0.13f + timeSeconds * 0.65f)
            + std::sin(z * 0.27f - x * 0.09f - timeSeconds * 0.48f));
        const float crest = std::max(0.0f, crossingBands);
        const float caustic = crest * crest * (1.0f - depthBlend);
        return AquaticLightProfile{
            0.04f + depthBlend * 0.22f,
            0.98f - depthBlend * 0.10f + caustic * 0.14f,
        };
    }
}
