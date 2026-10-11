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
    // First section exactly joins the first body ring. Rounded flattened muzzle,
    // entirely inside the population's 50 mm horizontal clearance envelope.
    inline constexpr std::array<SnakeHeadSection,4> SnakeHeadSections{{
        {0.0f,0.043f,0.043f,0.0f}, {0.018f,0.037f,0.030f,-0.008f},
        {0.035f,0.019f,0.022f,-0.010f}, {0.045f,0.007f,0.011f,-0.007f}}};
    inline constexpr float SnakeNoseForward=0.049f;
    inline constexpr float SnakeEyeForward=0.013f;
    inline constexpr float SnakeEyeSide=0.036f;
    inline constexpr float SnakeEyeHeight=0.013f;
    inline constexpr float SnakeEyeRadius=0.007f;
}
