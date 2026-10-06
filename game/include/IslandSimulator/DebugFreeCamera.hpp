#pragma once

namespace island
{
    /** @brief Renderer-independent input sample for the opt-in development flight camera. */
    struct DebugFreeCameraInput
    {
        float forward = 0.0f;
        float right = 0.0f;
        float vertical = 0.0f;
        bool fast = false;
    };

    /** @brief Small position value used without introducing renderer dependencies into core tests. */
    struct DebugFreeCameraPosition
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    /** @brief Bounded debug-only flight state; activation remains owned by the application. */
    class DebugFreeCamera final
    {
    public:
        static constexpr float BaseSpeed = 12.0f;
        static constexpr float FastMultiplier = 3.0f;
        static constexpr float HorizontalExtent = 820.0f;
        static constexpr float MinimumHeight = -18.0f;
        static constexpr float MaximumHeight = 260.0f;

        /** @brief Activates flight at the current rendered camera position. */
        void Activate(float x, float y, float z) noexcept;

        /** @brief Deactivates flight without changing its retained inspection position. */
        void Deactivate() noexcept { active_ = false; }

        /** @brief Advances bounded yaw-relative movement with a clamped frame duration. */
        void Update(float yawRadians, const DebugFreeCameraInput& input,
                    float deltaSeconds) noexcept;

        [[nodiscard]] bool IsActive() const noexcept { return active_; }
        [[nodiscard]] const DebugFreeCameraPosition& Position() const noexcept
        {
            return position_;
        }

    private:
        DebugFreeCameraPosition position_;
        bool active_ = false;
    };
}
