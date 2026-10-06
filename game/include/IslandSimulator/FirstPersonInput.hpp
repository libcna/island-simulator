#pragma once

namespace island
{
    /** @brief Renderer-independent state of the desktop first-person movement controls. */
    struct FirstPersonDigitalInput
    {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool run = false;
    };

    /** @brief World-space movement intent produced from keyboard, touch, and camera yaw. */
    struct FirstPersonMovementIntent
    {
        float worldX = 0.0f;
        float worldZ = 0.0f;
        bool run = false;
    };

    /** @brief Converts first-person inputs to the game's camera-relative horizontal convention. */
    class FirstPersonInput final
    {
    public:
        [[nodiscard]] static FirstPersonMovementIntent Resolve(
            const FirstPersonDigitalInput& digital, float analogForward,
            float analogRight, float yawRadians) noexcept;
    };
}
