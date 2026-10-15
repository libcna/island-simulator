#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace island::MonkeyFaceProfile
{
    struct Point { float x,y,z; };
    struct Row { float y, halfWidth, front; };
    // A capuchin-like short face.  The outline is deliberately heart-shaped rather
    // than circular: a compact brow flows into narrower cheeks and a blunt muzzle.
    inline constexpr std::array<Row, 12> Rows{{
        {.958f,.034f,.592f}, {.941f,.072f,.611f}, {.918f,.105f,.632f},
        {.892f,.122f,.643f}, {.868f,.124f,.643f}, {.846f,.116f,.649f},
        {.824f,.108f,.657f}, {.803f,.100f,.667f}, {.783f,.094f,.671f},
        {.765f,.078f,.661f}, {.749f,.048f,.629f}, {.739f,.018f,.595f}
    }};
    inline constexpr Point CraniumCentre{0,.877f,.446f};
    inline constexpr Point CraniumRadii{.171f,.184f,.150f};
    inline constexpr Point EarCentre{.163f,.866f,.438f};
    inline constexpr Point EarRadii{.042f,.052f,.019f};
    inline constexpr Point InnerEarCentre{.164f,.866f,.447f};
    inline constexpr Point InnerEarRadii{.031f,.039f,.014f};
    inline constexpr float EyeCentreX=.052f,EyeCentreY=.880f,EyeCentreZ=.643f;
    inline constexpr float EyeRadiusX=.022f,EyeRadiusY=.013f,EyeRadiusZ=.014f;
    inline constexpr Point MuzzlePadCentre{.029f,.794f,.668f};
    inline constexpr Point MuzzlePadRadii{.043f,.034f,.016f};
    inline constexpr Point NoseCentre{0,.814f,.679f};
    inline constexpr Point NoseRadii{.027f,.015f,.008f};
    inline float Bump(float value,float centre,float radius)
    {
        const float t=(value-centre)/radius;
        if(std::abs(t)>=1) return 0;
        const float q=1-t*t;return q*q;
    }
    inline float Orbit(float x,float y)
    {
        return Bump(std::abs(x),EyeCentreX,.034f)*Bump(y,EyeCentreY,.038f);
    }
    inline float Relief(float x,float y)
    {
        return -.008f*Orbit(x,y)
            +.011f*Bump(std::abs(x),EyeCentreX,.046f)*Bump(y,.905f,.025f)
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
        const float extent=upper ? (outer ? .016f : .007f)
                                 : (outer ? .012f : .006f);
        return {side*EyeCentreX+t*.028f,EyeCentreY+(upper ? extent : -extent)*arc,
            .6370f+.0015f*(1-t*t)+(outer ? 0.0f : .0005f)};
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
