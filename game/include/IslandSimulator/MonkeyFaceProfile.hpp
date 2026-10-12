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
        {.817f,.098f,.665f}, {.797f,.089f,.678f}, {.780f,.085f,.674f},
        {.762f,.069f,.663f}, {.746f,.041f,.634f}, {.736f,.005f,.592f}
    }};
    struct Point { float x,y,z; };
    inline constexpr float EyeCentreZ=.635f;
    inline float Bump(float value,float centre,float radius)
    {
        const float t=(value-centre)/radius;
        if(std::abs(t)>=1) return 0;
        const float q=1-t*t;return q*q;
    }
    inline float Orbit(float x,float y)
    {
        return Bump(std::abs(x),.052f,.035f)*Bump(y,.865f,.040f);
    }
    inline float Relief(float x,float y)
    {
        return -.008f*Orbit(x,y)
            +.011f*Bump(std::abs(x),.052f,.050f)*Bump(y,.893f,.025f)
            +.008f*Bump(x,0,.025f)*Bump(y,.844f,.035f)
            +.006f*Bump(std::abs(x),.078f,.040f)*Bump(y,.832f,.050f);
    }
    // Restrained ambient occlusion in the eye sockets, not a painted sun highlight.
    inline float AmbientRetention(float x,float y) { return 1-.20f*Orbit(x,y); }
    inline Point Sample(std::size_t row, float across)
    {
        const auto& profile = Rows[row];
        // Rounded transverse sections, with narrower skin joining the hairy cheeks.
        const float arc = std::cos(across * 1.57079632679f);
        const float x=profile.halfWidth * std::sin(across * 1.57079632679f);
        return {x, profile.y,
                .55f + (profile.front - .55f) * arc + Relief(x,profile.y)*arc};
    }
}
