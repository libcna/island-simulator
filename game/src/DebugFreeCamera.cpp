#include "IslandSimulator/DebugFreeCamera.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    void DebugFreeCamera::Activate(const float x, const float y, const float z) noexcept
    {
        position_.x = std::clamp(x, -HorizontalExtent, HorizontalExtent);
        position_.y = std::clamp(y, MinimumHeight, MaximumHeight);
        position_.z = std::clamp(z, -HorizontalExtent, HorizontalExtent);
        active_ = true;
    }

    void DebugFreeCamera::Update(const float yawRadians,
                                 const DebugFreeCameraInput& input,
                                 const float deltaSeconds) noexcept
    {
        if (!active_) return;

        float localForward = input.forward;
        float localRight = input.right;
        float localVertical = input.vertical;
        const float inputLength = std::hypot(
            localVertical, std::hypot(localForward, localRight));
        if (inputLength > 1.0f) {
            localForward /= inputLength;
            localRight /= inputLength;
            localVertical /= inputLength;
        }

        const float forwardX = std::sin(yawRadians);
        const float forwardZ = std::cos(yawRadians);
        const float rightX = -forwardZ;
        const float rightZ = forwardX;
        const float speed = BaseSpeed * (input.fast ? FastMultiplier : 1.0f);
        const float step = speed * std::clamp(deltaSeconds, 0.0f, 0.1f);
        position_.x = std::clamp(
            position_.x + (forwardX * localForward + rightX * localRight) * step,
            -HorizontalExtent, HorizontalExtent);
        position_.y = std::clamp(
            position_.y + localVertical * step, MinimumHeight, MaximumHeight);
        position_.z = std::clamp(
            position_.z + (forwardZ * localForward + rightZ * localRight) * step,
            -HorizontalExtent, HorizontalExtent);
    }
}
