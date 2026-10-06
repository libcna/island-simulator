#pragma once

namespace island
{
    /** @brief Smoothed movement-driven state for player ripples on the ocean surface. */
    class WaterRippleModel final
    {
    public:
        static constexpr float MaximumStepSeconds = 0.1f;
        static constexpr float EntryRatePerSecond = 3.6f;
        static constexpr float ExitRatePerSecond = 2.2f;

        /** @brief Returns the normalized ripple target for depth, locomotion, and speed. */
        [[nodiscard]] static float TargetIntensity(bool swimming, float groundHeight,
                                                   float horizontalSpeed) noexcept;

        /** @brief Snaps to a deterministic starting state for normal and QA placement. */
        void Reset(bool swimming, float groundHeight, float horizontalSpeed) noexcept;

        /** @brief Advances bounded intensity and wrapped radial phase. */
        [[nodiscard]] float Update(bool swimming, float groundHeight,
                                   float horizontalSpeed, float deltaSeconds) noexcept;

        [[nodiscard]] float Intensity() const noexcept { return intensity_; }
        [[nodiscard]] float Phase() const noexcept { return phase_; }

    private:
        float intensity_ = 0.0f;
        float phase_ = 0.0f;
    };
}
