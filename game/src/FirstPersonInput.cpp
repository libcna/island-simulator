#include "IslandSimulator/FirstPersonInput.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    FirstPersonMovementIntent FirstPersonInput::Resolve(
        const FirstPersonDigitalInput& digital, const float analogForward,
        const float analogRight, const float yawRadians) noexcept
    {
        const float safeYaw = std::isfinite(yawRadians) ? yawRadians : 0.0f;
        const float safeAnalogForward = std::isfinite(analogForward)
            ? std::clamp(analogForward, -1.0f, 1.0f) : 0.0f;
        const float safeAnalogRight = std::isfinite(analogRight)
            ? std::clamp(analogRight, -1.0f, 1.0f) : 0.0f;
        const float localForward = (digital.forward ? 1.0f : 0.0f)
            - (digital.backward ? 1.0f : 0.0f) + safeAnalogForward;
        const float localRight = (digital.right ? 1.0f : 0.0f)
            - (digital.left ? 1.0f : 0.0f) + safeAnalogRight;

        const float forwardX = std::sin(safeYaw);
        const float forwardZ = std::cos(safeYaw);
        // CreateLookAt's view convention places screen-right opposite the former +X basis at
        // yaw zero. This perpendicular therefore matches what the player sees on screen.
        const float rightX = -forwardZ;
        const float rightZ = forwardX;
        return FirstPersonMovementIntent{
            forwardX * localForward + rightX * localRight,
            forwardZ * localForward + rightZ * localRight,
            digital.run,
        };
    }
}
