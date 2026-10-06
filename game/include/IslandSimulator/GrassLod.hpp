#pragma once

#include <algorithm>
#include <cmath>

namespace island
{
    struct GrassLodSplit
    {
        int nearCount = 0;
        int middleCount = 0;
        int farCount = 0;
    };

    /** @brief Deterministic population split used to stagger grass mesh LOD changes. */
    class GrassLod final
    {
    public:
        static constexpr float NearOnlyDistance = 62.0f;
        static constexpr float MiddleOnlyStartDistance = 78.0f;
        static constexpr float MiddleOnlyEndDistance = 86.0f;
        static constexpr float FarOnlyDistance = 102.0f;

        [[nodiscard]] static GrassLodSplit Split(int patchCount, float distance) noexcept;
    };
}
