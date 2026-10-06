#pragma once

namespace island
{
    /** @brief Smoothed player-motion strength for local grass bending. */
    class GrassInteractionModel final
    {
    public:
        static constexpr float MaximumStepSeconds = 0.1f;
        static constexpr float EntryRatePerSecond = 4.5f;
        static constexpr float ExitRatePerSecond = 2.8f;

        /** @brief Maps horizontal player speed to a normalized interaction target. */
        [[nodiscard]] static float TargetStrength(float horizontalSpeed) noexcept;

        /** @brief Snaps to a deterministic starting state for normal and QA placement. */
        void Reset(float horizontalSpeed) noexcept;

        /** @brief Advances the bounded interaction envelope. */
        [[nodiscard]] float Update(float horizontalSpeed, float deltaSeconds) noexcept;

        [[nodiscard]] float Strength() const noexcept { return strength_; }

    private:
        float strength_ = 0.0f;
    };
}
