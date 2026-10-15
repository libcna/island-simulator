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
        {.950f,.026f,.585f}, {.936f,.060f,.602f}, {.916f,.092f,.617f},
        {.893f,.112f,.626f}, {.870f,.114f,.626f}, {.848f,.108f,.632f},
        {.827f,.101f,.639f}, {.807f,.094f,.646f}, {.788f,.086f,.649f},
        {.770f,.071f,.641f}, {.755f,.043f,.615f}, {.746f,.015f,.586f}
    }};
    inline constexpr Point CraniumCentre{0,.877f,.446f};
    inline constexpr Point CraniumRadii{.155f,.170f,.140f};
    inline constexpr Point EarCentre{.163f,.866f,.438f};
    inline constexpr Point EarRadii{.042f,.052f,.019f};
    inline constexpr Point InnerEarCentre{.164f,.866f,.447f};
    inline constexpr Point InnerEarRadii{.031f,.039f,.014f};
    inline constexpr float EyeCentreX=.049f,EyeCentreY=.880f,EyeCentreZ=.625f;
    inline constexpr float EyeRadiusX=.020f,EyeRadiusY=.012f,EyeRadiusZ=.013f;
    inline constexpr Point MuzzlePadCentre{.026f,.797f,.647f};
    inline constexpr Point MuzzlePadRadii{.039f,.030f,.015f};
    inline constexpr Point NoseCentre{0,.817f,.657f};
    inline constexpr Point NoseRadii{.024f,.013f,.009f};
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
        return -.012f*Orbit(x,y)
            +.017f*Bump(std::abs(x),EyeCentreX,.043f)*Bump(y,.905f,.025f)
            +.013f*Bump(x,0,.026f)*Bump(y,.850f,.038f)
            +.009f*Bump(std::abs(x),.074f,.040f)*Bump(y,.842f,.050f);
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
        return {side*EyeCentreX+t*.025f,EyeCentreY+(upper ? extent : -extent)*arc,
            .6190f+.0015f*(1-t*t)+(outer ? 0.0f : .0005f)};
    }
    inline Point Sample(std::size_t row, float across)
    {
        const auto& profile = Rows[row];
        // Rounded transverse sections, with narrower skin joining the hairy cheeks.
        const float arc = std::cos(across * 1.57079632679f);
        const float x=profile.halfWidth * std::sin(across * 1.57079632679f);
        return {x, profile.y,
                .555f + (profile.front - .555f) * arc + Relief(x,profile.y)*arc};
    }
}
