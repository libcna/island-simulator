#pragma once

namespace island
{
    /** @brief Edge-triggered persistent walk/run selection for keyboard input. */
    class LocomotionPaceToggle final
    {
    public:
        /**
         * @brief Consumes the current Shift state and returns true when the selected pace changes.
         *
         * The physical key state is always tracked, including while toggling is disabled. This
         * prevents a Shift press used by the debug free camera from changing player pace when that
         * camera is deactivated before the key is released.
         */
        [[nodiscard]] bool Update(bool shiftDown, bool togglingEnabled = true) noexcept;

        /** @brief Returns whether the persistent running pace is selected. */
        [[nodiscard]] bool IsRunningSelected() const noexcept { return runningSelected_; }

        /** @brief Restores a persisted pace without synthesizing a key edge. */
        void SetRunningSelected(bool running) noexcept { runningSelected_ = running; }

    private:
        bool shiftWasDown_ = false;
        bool runningSelected_ = false;
    };
}
