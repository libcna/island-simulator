#pragma once

#include <span>

namespace island
{
    /** @brief Platform-neutral lifecycle state for one touch contact. */
    enum class TouchPhase
    {
        Pressed,
        Moved,
        Released
    };

    /** @brief Minimal touch sample consumed by the mobile control interpreter. */
    struct TouchContact
    {
        int id = -1;
        float x = 0.0f;
        float y = 0.0f;
        TouchPhase phase = TouchPhase::Released;
    };

    /** @brief Runtime-selectable camera response for touch look gestures. */
    enum class TouchLookSensitivityLevel
    {
        Low,
        Standard,
        High
    };

    /** @brief Display-space hit target shared by touch interpretation and HUD presentation. */
    struct TouchSensitivitySelectorLayout
    {
        float x = 0.0f;
        float y = 0.0f;
        float radius = 0.0f;
    };

    /** @brief One-frame movement and look output plus presentation state. */
    struct TouchControlState
    {
        float moveRight = 0.0f;
        float moveForward = 0.0f;
        float lookDeltaX = 0.0f;
        float lookDeltaY = 0.0f;
        float movementAnchorX = 0.0f;
        float movementAnchorY = 0.0f;
        float movementTouchX = 0.0f;
        float movementTouchY = 0.0f;
        float lookTouchX = 0.0f;
        float lookTouchY = 0.0f;
        bool movementActive = false;
        bool lookActive = false;
        bool runRequested = false; ///< Full-stick land pace with hysteretic release.
        TouchLookSensitivityLevel lookSensitivity = TouchLookSensitivityLevel::Standard;
        bool sensitivitySelectorActive = false;
        bool lookSensitivityChanged = false;
    };

    /**
     * @brief Two-zone mobile controls plus a bounded sensitivity selector.
     *
     * A contact beginning in the left half controls a floating movement stick. A contact beginning
     * in the right half supplies relative look deltas. Contacts retain their role after crossing
     * the centre line; missing/released contacts and display-size changes cancel cleanly. A
     * separate top-right target cycles look response without acquiring either gameplay role.
     */
    class TouchControls final
    {
    public:
        static constexpr float MovementRadiusFraction = 0.14f;
        static constexpr float MovementDeadZone = 0.12f;
        static constexpr float RunEngageMagnitude = 0.92f; ///< Normalized stick travel.
        static constexpr float RunReleaseMagnitude = 0.78f; ///< Prevents threshold chatter.
        static constexpr float SensitivitySelectorRadiusFraction = 0.06f;

        /** @brief Returns the bounded multiplier used by the selected look-response level. */
        [[nodiscard]] static constexpr float LookSensitivityScale(
            const TouchLookSensitivityLevel sensitivity) noexcept
        {
            switch (sensitivity) {
            case TouchLookSensitivityLevel::Low: return 0.72f;
            case TouchLookSensitivityLevel::Standard: return 1.0f;
            case TouchLookSensitivityLevel::High: return 1.35f;
            }
            return 1.0f;
        }

        /** @brief Returns the shared top-right selector hit target for a display size. */
        [[nodiscard]] static TouchSensitivitySelectorLayout SensitivitySelectorLayout(
            float displayWidth, float displayHeight) noexcept;

        /** @brief Advances the interpreter from the complete current touch snapshot. */
        [[nodiscard]] TouchControlState Update(std::span<const TouchContact> contacts,
                                               float displayWidth,
                                               float displayHeight) noexcept;

        /** @brief Releases all pointer ownership and returns neutral output. */
        void Reset() noexcept;

        /** @brief Returns the most recent output without advancing input. */
        [[nodiscard]] const TouchControlState& State() const noexcept { return state_; }

        /** @brief Restores a persisted look-response level and releases active contacts. */
        void SetLookSensitivity(TouchLookSensitivityLevel sensitivity) noexcept;

    private:
        [[nodiscard]] static const TouchContact* FindContact(
            std::span<const TouchContact> contacts, int id) noexcept;
        void AssignNewContacts(std::span<const TouchContact> contacts,
                               float displayWidth, float displayHeight) noexcept;
        void UpdateSensitivitySelector(std::span<const TouchContact> contacts,
                                       float displayWidth, float displayHeight) noexcept;
        void UpdateMovement(const TouchContact& contact, float displayHeight) noexcept;

        TouchControlState state_;
        int movementId_ = -1;
        int lookId_ = -1;
        int sensitivitySelectorId_ = -1;
        float lastLookX_ = 0.0f;
        float lastLookY_ = 0.0f;
        float displayWidth_ = 0.0f;
        float displayHeight_ = 0.0f;
    };
}
