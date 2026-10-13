#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace island::MonkeyFaceProfile
{
    struct Point { float x,y,z; };
    struct Row { float y, halfWidth, front; };
    // A capuchin-like short face: broad brow and cheeks surround a restrained
    // muzzle instead of extending into the former narrow rodent/dog wedge.
    inline constexpr std::array<Row, 12> Rows{{
        {.972f,.008f,.560f}, {.942f,.070f,.587f}, {.915f,.104f,.610f},
        {.891f,.124f,.627f}, {.868f,.128f,.625f}, {.846f,.120f,.630f},
        {.824f,.111f,.637f}, {.803f,.102f,.646f}, {.784f,.096f,.648f},
        {.766f,.080f,.638f}, {.750f,.048f,.612f}, {.740f,.006f,.575f}
    }};
    inline constexpr Point CraniumCentre{0,.875f,.425f};
    inline constexpr Point CraniumRadii{.183f,.202f,.168f};
    inline constexpr Point EarCentre{.169f,.862f,.414f};
    inline constexpr Point EarRadii{.055f,.067f,.024f};
    inline constexpr Point InnerEarCentre{.170f,.862f,.424f};
    inline constexpr Point InnerEarRadii{.040f,.051f,.017f};
    inline constexpr float EyeCentreX=.055f,EyeCentreY=.878f,EyeCentreZ=.615f;
    inline constexpr float EyeRadiusX=.0215f,EyeRadiusY=.013f,EyeRadiusZ=.0095f;
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
        return {side*EyeCentreX+t*.030f,EyeCentreY+(upper ? extent : -extent)*arc,
            .6240f+.0015f*(1-t*t)+(outer ? 0.0f : .0005f)};
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
