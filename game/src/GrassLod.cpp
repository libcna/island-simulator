#include "IslandSimulator/GrassLod.hpp"

namespace island
{
    GrassLodSplit GrassLod::Split(const int patchCount, const float distance) noexcept
    {
        if (patchCount <= 0) {
            return {};
        }
        if (std::isnan(distance) || distance <= NearOnlyDistance) {
            return GrassLodSplit{patchCount, 0};
        }
        if (distance >= FarOnlyDistance) {
            return GrassLodSplit{0, patchCount};
        }

        const float progress = std::clamp(
            (distance - NearOnlyDistance) / (FarOnlyDistance - NearOnlyDistance),
            0.0f, 1.0f);
        const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
        const int farCount = std::clamp(
            static_cast<int>(std::lround(static_cast<float>(patchCount) * smoothProgress)),
            0, patchCount);
        return GrassLodSplit{patchCount - farCount, farCount};
    }
}
