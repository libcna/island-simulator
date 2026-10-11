#pragma once
#include <array>
#include <cmath>

namespace island::MonkeyFaceProfile
{
    struct Row { float y, halfWidth, front; };
    // One connected brow/cheek/muzzle/chin surface, recessed into fur at its perimeter.
    inline constexpr std::array<Row, 12> Rows{{
        {.958f,.008f,.572f}, {.928f,.064f,.604f}, {.903f,.096f,.632f},
        {.880f,.113f,.651f}, {.859f,.115f,.650f}, {.837f,.105f,.658f},
        {.817f,.089f,.685f}, {.797f,.077f,.704f}, {.780f,.072f,.700f},
        {.762f,.059f,.687f}, {.746f,.033f,.650f}, {.736f,.005f,.592f}
    }};
    struct Point { float x,y,z; };
    inline Point Sample(std::size_t row, float across)
    {
        const auto& profile = Rows[row];
        // Rounded transverse sections, with narrower skin joining the hairy cheeks.
        const float arc = std::cos(across * 1.57079632679f);
        return {profile.halfWidth * std::sin(across * 1.57079632679f), profile.y,
                .55f + (profile.front - .55f) * arc};
    }
}
