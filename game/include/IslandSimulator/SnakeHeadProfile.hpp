#pragma once
#include <array>
namespace island {
    inline constexpr float SnakeSkinCircumferenceRepeats=4.0f;
    inline constexpr float SnakeSkinStationRepeats=1.5f;
    // Signed longitudinal distance: positive toward the tail, negative over the muzzle.
    [[nodiscard]] constexpr float SnakeSkinV(float distanceBehindHead,float stationSpacing) noexcept {
        return distanceBehindHead*(SnakeSkinStationRepeats/stationSpacing);
    }
    [[nodiscard]] constexpr float SnakeSkinU(float sector,float sides) noexcept {
        return sector*SnakeSkinCircumferenceRepeats/sides;
    }
    struct SnakeHeadSection { float forward, width, height, centreY; };
    // First section exactly joins the first body ring. The following broad temple and
    // tapered cheek make a low snake head rather than continuing the body as a cone;
    // every section remains inside the population's 50 mm clearance envelope.
    inline constexpr std::array<SnakeHeadSection,4> SnakeHeadSections{{
        {0.0f,0.043f,0.043f,0.0f}, {0.012f,0.0478f,0.029f,0.005f},
        {0.030f,0.039f,0.024f,0.002f}, {0.044f,0.021f,0.014f,-0.001f}}};
    inline constexpr float SnakeNoseForward=0.050f;
    inline constexpr float SnakeEyeForward=0.018f;
    inline constexpr float SnakeEyeSide=0.038f;
    inline constexpr float SnakeEyeHeight=0.011f;
    inline constexpr float SnakeEyeRadius=0.006f;
}
