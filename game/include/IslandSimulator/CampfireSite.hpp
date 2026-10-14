#pragma once
#include <algorithm>
#include <cmath>
namespace island {
// Authored on a broad, level dry-sand beach, well away from the stream channel.
struct CampfireSite final {
    static constexpr float X=-6.52f,Z=-223.35f;
    static constexpr float WoodX=-.62f,WoodZ=-223.45f;
    static constexpr float PitRadius=1.25f,WoodRadius=1.40f;
    static constexpr float VegetationClearance=5.5f,WoodClearance=2.0f;
    [[nodiscard]] static bool Reserves(float x,float z,float margin=0) noexcept {
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(margin))return false;
        margin=std::max(0.0f,margin);
        return std::hypot(x-X,z-Z)<VegetationClearance+margin
            ||std::hypot(x-WoodX,z-WoodZ)<WoodClearance+margin;
    }
    [[nodiscard]] static float SoundPresence(float x,float y,float z,float fireY) noexcept {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(fireY))return 0;
        const float t=std::clamp((std::hypot(x-X,y-fireY,z-Z)-3.0f)/21.0f,0.0f,1.0f);
        return 1-t*t*(3-2*t);
    }
};
}
