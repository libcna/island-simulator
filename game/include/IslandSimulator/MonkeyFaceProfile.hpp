#pragma once
#include <algorithm>
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
    inline constexpr float EyeRadiusX=.020f,EyeRadiusY=.012f,EyeRadiusZ=.009f;
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
    // Raised skin rims frame the dark cornea without painting a black circular eye.
    // t deliberately stops short of +/-1 in the mesh, keeping both bands nondegenerate.
    inline Point Eyelid(float side,float t,bool upper,bool outer)
    {
        t=std::clamp(t,-.92f,.92f);
        const float arc=std::sqrt(std::max(0.0f,1-t*t));
        const float extent=upper ? (outer ? .018f : .008f)
                                 : (outer ? .014f : .007f);
        return {side*.052f+t*.028f,.867f+(upper ? extent : -extent)*arc,
            .6445f+.0015f*(1-t*t)+(outer ? 0.0f : .0005f)};
    }
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
