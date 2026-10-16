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
        {0.0f,0.040f,0.036f,0.0f}, {0.011f,0.0472f,0.0225f,0.0055f},
        {0.029f,0.0400f,0.0195f,0.0025f}, {0.043f,0.024f,0.0125f,-0.0005f}}};
    inline constexpr float SnakeNoseForward=0.049f;
    inline constexpr float SnakeEyeForward=0.017f;
    inline constexpr float SnakeEyeSide=0.036f;
    inline constexpr float SnakeEyeHeight=0.0085f;
    inline constexpr float SnakeEyeRadius=0.0048f;
}
