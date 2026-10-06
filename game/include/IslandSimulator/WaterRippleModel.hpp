#pragma once

namespace island
{
    /** @brief Smoothed movement ripples and bounded shoreline-crossing bursts. */
    class WaterRippleModel final
    {
    public:
        static constexpr float MaximumStepSeconds = 0.1f;
        static constexpr float EntryRatePerSecond = 3.6f;
        static constexpr float ExitRatePerSecond = 2.2f;
        static constexpr float SplashEntryDepth = 0.075f;
        static constexpr float SplashExitDepth = 0.025f;
        static constexpr float SplashMinimumSpeed = 0.12f;
        static constexpr float SplashDurationSeconds = 1.6f;
        static constexpr float SplashCooldownSeconds = 1.6f;

        /** @brief Returns the normalized ripple target for depth, locomotion, and speed. */
        [[nodiscard]] static float TargetIntensity(bool swimming, float groundHeight,
                                                   float horizontalSpeed) noexcept;

        /** @brief Snaps to a deterministic starting state for normal and QA placement. */
        void Reset(bool swimming, float groundHeight, float horizontalSpeed) noexcept;

        /** @brief Advances bounded intensity and wrapped radial phase. */
        [[nodiscard]] float Update(bool swimming, float groundHeight,
                                   float horizontalSpeed, float deltaSeconds) noexcept;

        /** @brief Starts one bounded entry/exit burst without changing tracked water state. */
        void TriggerSplash(bool entering, float horizontalSpeed) noexcept;

        [[nodiscard]] float Intensity() const noexcept { return intensity_; }
        [[nodiscard]] float Phase() const noexcept { return phase_; }
        [[nodiscard]] float SplashIntensity() const noexcept { return splashIntensity_; }
        [[nodiscard]] float SplashPhase() const noexcept { return splashPhase_; }

    private:
        float intensity_ = 0.0f;
        float phase_ = 0.0f;
        float splashIntensity_ = 0.0f;
        float splashPhase_ = 1.0f;
        float splashCooldownSeconds_ = 0.0f;
        bool inWater_ = false;
    };
}
