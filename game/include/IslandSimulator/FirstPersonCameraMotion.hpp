#pragma once

#include "IslandSimulator/PlayerMotion.hpp"

namespace island
{
    /** @brief Local-space camera displacement used for restrained first-person motion. */
    struct CameraMotionOffset
    {
        float lateralMeters = 0.0f;
        float verticalMeters = 0.0f;
    };

    /**
     * @brief Renderer-independent walking and swimming camera-motion model.
     *
     * The output is deliberately positional only: look input remains authoritative, there is no
     * roll, and stopping or disabling the effect eases the camera back to its physical eye point.
     */
    class FirstPersonCameraMotion final
    {
    public:
        static constexpr float MaximumLateralOffset = 0.010f;
        static constexpr float MaximumVerticalOffset = 0.018f;

        /** @brief Advances the model and returns a local right/up displacement in metres. */
        [[nodiscard]] CameraMotionOffset Update(float horizontalSpeed, LocomotionMode mode,
                                                float deltaSeconds) noexcept;

        /** @brief Clears phase and displacement without changing the accessibility toggle. */
        void Reset() noexcept;

        /** @brief Enables or smoothly suppresses camera motion. */
        void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }

        /** @brief Returns whether locomotion-driven camera motion is requested. */
        [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

        /** @brief Returns the most recently evaluated displacement. */
        [[nodiscard]] const CameraMotionOffset& Offset() const noexcept { return offset_; }

        /** @brief Returns the smoothed zero-to-one movement contribution. */
        [[nodiscard]] float MotionStrength() const noexcept { return motionStrength_; }

        /** @brief Returns the smoothed zero-to-one swimming style blend. */
        [[nodiscard]] float SwimBlend() const noexcept { return swimBlend_; }

    private:
        float phaseRadians_ = 0.0f;
        float motionStrength_ = 0.0f;
        float swimBlend_ = 0.0f;
        CameraMotionOffset offset_;
        bool enabled_ = true;
    };
}
