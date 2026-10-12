#pragma once

#include <algorithm>
#include <cmath>

namespace island {
// Vertical swimming state only. The caller supplies a conservative rendered
// seabed support height and must resolve the complete horizontal swept path.
// This is not a substitute for horizontal collision or underwater presentation.
class SwimDepth final {
public:
    static constexpr float SurfaceEyeHeight = .58f;
    static constexpr float BottomClearance = .45f;
    static constexpr float VerticalSpeed = 1.6f;

    void Reset() noexcept { eyeHeight_ = SurfaceEyeHeight; }
    [[nodiscard]] float EyeHeight() const noexcept { return eyeHeight_; }
    [[nodiscard]] float SubmergedDepth() const noexcept { return std::max(0.0f,-eyeHeight_); }

    // False means invalid ground: caller must reject motion, not invent a seabed.
    // Dry/shallow support returns to the surface; land eye height belongs to walking.
    bool Update(float renderedSupport, bool ascend, bool descend, float seconds) noexcept {
        if (!std::isfinite(renderedSupport)) return false;
        const float floor=std::min(SurfaceEyeHeight,renderedSupport+BottomClearance);
        const float dt=std::isfinite(seconds)?std::clamp(seconds,0.0f,.1f):0.0f;
        const float direction=static_cast<float>(ascend)-static_cast<float>(descend);
        eyeHeight_=std::clamp(eyeHeight_+direction*VerticalSpeed*dt,floor,SurfaceEyeHeight);
        return true;
    }
private:
    float eyeHeight_=SurfaceEyeHeight;
};
}
