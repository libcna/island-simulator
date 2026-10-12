#pragma once

#include <algorithm>
#include <cmath>

namespace island {
/** Shared authored clearing for the grounded helicopter and its stationary rotor disc. */
struct HelicopterSite final {
    static constexpr float X=-50.2571f,Z=48.7260f;
    static constexpr float ClearingRadius=6.20f;
    [[nodiscard]] static bool Reserves(float x,float z,float radius=0.0f) noexcept {
        return std::hypot(x-X,z-Z)<ClearingRadius+std::max(radius,0.0f);
    }
};
}
