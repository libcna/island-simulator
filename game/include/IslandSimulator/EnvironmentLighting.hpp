#pragma once

namespace island::environment
{
    // Fixed pleasant-afternoon direction shared by terrain, ocean, and sky shading.
    inline constexpr float SunDirectionX = -0.45f;
    inline constexpr float SunDirectionY = 0.82f;
    inline constexpr float SunDirectionZ = -0.34f;
    // One aerial-perspective colour: the sky's horizon haze and every renderer's distance fog.
    // GLSL bodies repeat the same literal `vec3(0.53, 0.78, 0.88)`; keep them in step.
    inline constexpr float FogColorRed = 0.53f;
    inline constexpr float FogColorGreen = 0.78f;
    inline constexpr float FogColorBlue = 0.88f;
}
