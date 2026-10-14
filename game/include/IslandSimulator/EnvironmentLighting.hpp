#pragma once

// Keep the C++ channels and the portable shader declaration generated from the same tokens.
#define ISLAND_FOG_RED 0.53
#define ISLAND_FOG_GREEN 0.78
#define ISLAND_FOG_BLUE 0.88
#define ISLAND_STRINGIFY_DETAIL(value) #value
#define ISLAND_STRINGIFY(value) ISLAND_STRINGIFY_DETAIL(value)

namespace island::environment
{
    // Fixed pleasant-afternoon direction shared by terrain, ocean, and sky shading.
    inline constexpr float SunDirectionX = -0.45f;
    inline constexpr float SunDirectionY = 0.82f;
    inline constexpr float SunDirectionZ = -0.34f;
    // One aerial-perspective colour: stock effects consume the channels while every custom
    // shader consumes this generated declaration. The numeric tokens therefore have one owner.
    inline constexpr float FogColorRed = ISLAND_FOG_RED;
    inline constexpr float FogColorGreen = ISLAND_FOG_GREEN;
    inline constexpr float FogColorBlue = ISLAND_FOG_BLUE;
    inline constexpr const char* FogShaderDefinition =
        "const vec3 islandAtmosphereFog=vec3("
        ISLAND_STRINGIFY(ISLAND_FOG_RED) ","
        ISLAND_STRINGIFY(ISLAND_FOG_GREEN) ","
        ISLAND_STRINGIFY(ISLAND_FOG_BLUE) ");\n";
}

#undef ISLAND_STRINGIFY
#undef ISLAND_STRINGIFY_DETAIL
#undef ISLAND_FOG_BLUE
#undef ISLAND_FOG_GREEN
#undef ISLAND_FOG_RED
