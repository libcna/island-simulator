#pragma once

namespace island
{
    /** @brief Smooth renderer-independent intensity for the near-surface water-view cue. */
    class WaterViewModel final
    {
    public:
        static constexpr float MaximumStepSeconds = 0.1f;
        static constexpr float EntryRatePerSecond = 2.4f;
        static constexpr float ExitRatePerSecond = 1.8f;

        /** @brief Returns the normalized target for locomotion, seabed depth, and current. */
        [[nodiscard]] static float TargetIntensity(bool swimming,
                                                   float groundHeight,
                                                   float currentStrength) noexcept;

        /** @brief Snaps to the current target, used when placing deterministic QA cameras. */
        void Reset(bool swimming, float groundHeight, float currentStrength) noexcept;

        /** @brief Advances the bounded entry/exit transition and returns its new intensity. */
        [[nodiscard]] float Update(bool swimming, float groundHeight, float currentStrength,
                                   float deltaSeconds) noexcept;

        [[nodiscard]] float Intensity() const noexcept { return intensity_; }

    private:
        float intensity_ = 0.0f;
    };
}
