#include "IslandSimulator/GrassLod.hpp"

namespace island
{
    GrassLodSplit GrassLod::Split(const int patchCount, const float distance) noexcept
    {
        if (patchCount <= 0) {
            return {};
        }
        if (std::isnan(distance) || distance <= NearOnlyDistance) {
            return GrassLodSplit{patchCount, 0, 0};
        }
        if (distance < MiddleOnlyStartDistance) {
            const float progress = std::clamp(
                (distance - NearOnlyDistance)
                    / (MiddleOnlyStartDistance - NearOnlyDistance),
                0.0f, 1.0f);
            const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
            const int middleCount = std::clamp(
                static_cast<int>(std::lround(
                    static_cast<float>(patchCount) * smoothProgress)),
                0, patchCount);
            return GrassLodSplit{patchCount - middleCount, middleCount, 0};
        }
        if (distance <= MiddleOnlyEndDistance) {
            return GrassLodSplit{0, patchCount, 0};
        }
        if (distance < FarOnlyDistance) {
            const float progress = std::clamp(
                (distance - MiddleOnlyEndDistance)
                    / (FarOnlyDistance - MiddleOnlyEndDistance),
                0.0f, 1.0f);
            const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
            const int farCount = std::clamp(
                static_cast<int>(std::lround(
                    static_cast<float>(patchCount) * smoothProgress)),
                0, patchCount);
            return GrassLodSplit{0, patchCount - farCount, farCount};
        }
        return GrassLodSplit{0, 0, patchCount};
    }
}
