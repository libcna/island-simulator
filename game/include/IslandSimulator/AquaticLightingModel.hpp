#pragma once

namespace island
{
    /** @brief Portable per-animal water-column colour controls. */
    struct AquaticLightProfile
    {
        float waterTint = 0.0f;
        float brightness = 1.0f;
    };

    /**
     * @brief Samples restrained depth absorption and moving shallow sunlight.
     *
     * Non-finite inputs fail closed to the neutral profile so renderer colour data cannot be
     * contaminated by an invalid wildlife pose or clock value.
     */
    [[nodiscard]] AquaticLightProfile AquaticLightAt(float x, float y, float z,
                                                     float timeSeconds) noexcept;
}
