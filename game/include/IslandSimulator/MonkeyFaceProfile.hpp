#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace island::MonkeyFaceProfile
{
    struct Point { float x,y,z; };
    struct Row { float y, halfWidth, front; };
    // A capuchin-like short face: broad brow and cheeks surround a projecting
    // but blunt muzzle instead of the former small rodent-like mask.
    inline constexpr std::array<Row, 12> Rows{{
        {.970f,.044f,.592f}, {.948f,.086f,.616f}, {.923f,.119f,.637f},
        {.894f,.137f,.648f}, {.870f,.142f,.646f}, {.847f,.134f,.651f},
        {.825f,.126f,.659f}, {.804f,.117f,.670f}, {.784f,.109f,.675f},
        {.764f,.091f,.665f}, {.746f,.055f,.635f}, {.735f,.020f,.595f}
    }};
    inline constexpr Point CraniumCentre{0,.885f,.440f};
    inline constexpr Point CraniumRadii{.190f,.210f,.160f};
    inline constexpr Point EarCentre{.176f,.866f,.430f};
    inline constexpr Point EarRadii{.052f,.064f,.023f};
    inline constexpr Point InnerEarCentre{.177f,.866f,.440f};
    inline constexpr Point InnerEarRadii{.040f,.051f,.017f};
    inline constexpr float EyeCentreX=.060f,EyeCentreY=.882f,EyeCentreZ=.644f;
    inline constexpr float EyeRadiusX=.024f,EyeRadiusY=.014f,EyeRadiusZ=.010f;
    inline constexpr Point MuzzlePadCentre{.036f,.794f,.666f};
    inline constexpr Point MuzzlePadRadii{.055f,.043f,.020f};
    inline constexpr Point NoseCentre{0,.817f,.675f};
    inline constexpr Point NoseRadii{.034f,.020f,.014f};
    inline float Bump(float value,float centre,float radius)
    {
        const float t=(value-centre)/radius;
        if(std::abs(t)>=1) return 0;
        const float q=1-t*t;return q*q;
    }
    inline float Orbit(float x,float y)
    {
        return Bump(std::abs(x),EyeCentreX,.038f)*Bump(y,EyeCentreY,.042f);
    }
    inline float Relief(float x,float y)
    {
        return -.008f*Orbit(x,y)
            +.011f*Bump(std::abs(x),EyeCentreX,.052f)*Bump(y,.907f,.027f)
            +.008f*Bump(x,0,.027f)*Bump(y,.850f,.038f)
            +.006f*Bump(std::abs(x),.082f,.044f)*Bump(y,.840f,.052f);
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
        return {side*EyeCentreX+t*.032f,EyeCentreY+(upper ? extent : -extent)*arc,
            .6510f+.0015f*(1-t*t)+(outer ? 0.0f : .0005f)};
    }
    inline Point Sample(std::size_t row, float across)
    {
        const auto& profile = Rows[row];
        // Rounded transverse sections, with narrower skin joining the hairy cheeks.
        const float arc = std::cos(across * 1.57079632679f);
        const float x=profile.halfWidth * std::sin(across * 1.57079632679f);
        return {x, profile.y,
                .565f + (profile.front - .565f) * arc + Relief(x,profile.y)*arc};
    }
}
