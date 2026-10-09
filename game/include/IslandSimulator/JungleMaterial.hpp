#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace island::JungleMaterial
{
    inline constexpr float LeafFill = 0.48f, LeafSun = 0.44f, LeafSky = 0.08f;
    inline constexpr float WoodFill = 0.52f, WoodSun = 0.40f, WoodSky = 0.08f;
    inline constexpr float TrunkPigmentBase = 0.96f, TrunkPigmentTip = 1.02f;
    inline constexpr float FilterStart = 0.18f, FilterEnd = 0.50f;
    inline constexpr float TipStart = 0.62f, TipEnd = 0.98f;
    // Numerical mean of smoothstep(.82,.97,abs(sin(x))) over one period.
    inline constexpr float FissureStart = 0.82f, FissureEnd = 0.97f, FissureMean = 0.29043848f;
    inline constexpr std::array<float, 3> AgeShift{0.12f, -0.04f, -0.27f};

    inline float Smooth(float a, float b, float x)
    {
        const float t = std::clamp((x-a)/(b-a), 0.0f, 1.0f);
        return t*t*(3-2*t);
    }

    inline float LeafDaylight(float normalSun, float normalUp)
    {
        if (!std::isfinite(normalSun) || !std::isfinite(normalUp)
            || std::abs(normalSun) > 1 || std::abs(normalUp) > 1)
            throw std::invalid_argument("Leaf daylight requires finite unit-vector components");
        return LeafFill + LeafSun*std::abs(normalSun) + LeafSky*std::abs(normalUp);
    }

    inline float DetailWeight(float cyclesPerPixel)
    {
        if (!std::isfinite(cyclesPerPixel) || cyclesPerPixel < 0)
            throw std::invalid_argument("Material footprint must be finite and nonnegative");
        return 1-Smooth(FilterStart, FilterEnd, cyclesPerPixel);
    }

    inline float WoodDaylight(float normalSun, float normalUp)
    {
        if (!std::isfinite(normalSun) || !std::isfinite(normalUp)
            || std::abs(normalSun) > 1 || std::abs(normalUp) > 1)
            throw std::invalid_argument("Wood daylight requires finite unit-vector components");
        return WoodFill + WoodSun*std::max(normalSun, 0.0f) + WoodSky*std::max(normalUp, 0.0f);
    }

    // Linear in height so vertex interpolation does not depend on the number of trunk rings.
    // Byte colour packing introduces at most one channel value of interpolation difference.
    inline float TrunkPigment(float normalizedHeight)
    {
        if (!std::isfinite(normalizedHeight) || normalizedHeight < 0 || normalizedHeight > 1)
            throw std::invalid_argument("Trunk pigment requires normalized finite height");
        return std::lerp(TrunkPigmentBase, TrunkPigmentTip, normalizedHeight);
    }

    // Interpolate a circular pair, never a wrapped angle. Axial distance is in mesh-local metres.
    inline std::array<float, 3> WoodCoordinates(float angle, float along)
    {
        if (!std::isfinite(angle) || !std::isfinite(along) || along < 0)
            throw std::invalid_argument("Wood coordinates require a finite angle and nonnegative distance");
        return {std::cos(angle), std::sin(angle), along};
    }

    inline bool ValidWoodCoordinates(float x, float y, float along)
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(along) && along >= 0
            && std::abs(x*x + y*y - 1.0f) < 0.00001f;
    }

    // CPU reference for the shader's seam-independent radians/pixel footprint.
    inline float WoodAngularFootprint(std::array<float, 2> circle,
                                      std::array<float, 2> dx, std::array<float, 2> dy)
    {
        for (const auto& pair : {circle, dx, dy})
            for (float v : pair)
                if (!std::isfinite(v)) throw std::invalid_argument("Non-finite wood footprint input");
        const float squared = circle[0]*circle[0] + circle[1]*circle[1];
        return (std::abs(circle[0]*dx[1] - circle[1]*dx[0])
              + std::abs(circle[0]*dy[1] - circle[1]*dy[0])) / std::max(squared, 0.000001f);
    }

    inline std::array<float, 3> LeafAgeTint(float height, float phase)
    {
        if (!std::isfinite(height) || !std::isfinite(phase)
            || height < 0 || height > 1 || phase < 0 || phase > 1)
            throw std::invalid_argument("Leaf material coordinates outside normalized range");
        const float weight = Smooth(TipStart, TipEnd, height) * (1-Smooth(0.16f, 0.42f, phase));
        return {1+AgeShift[0]*weight, 1+AgeShift[1]*weight, 1+AgeShift[2]*weight};
    }

    inline std::string Shader()
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10)
            << "const vec3 leafDaylightTerms=vec3(" << LeafFill << ',' << LeafSun << ',' << LeafSky << ");\n"
            << "const vec3 woodDaylightTerms=vec3(" << WoodFill << ',' << WoodSun << ',' << WoodSky << ");\n"
            << "const vec2 materialFilterRange=vec2(" << FilterStart << ',' << FilterEnd << ");\n"
            << "const vec2 leafAgeRange=vec2(" << TipStart << ',' << TipEnd << ");\n"
            << "const vec3 barkFissureTerms=vec3(" << FissureStart << ',' << FissureEnd << ',' << FissureMean << ");\n"
            << "const vec3 leafAgeShift=vec3(" << AgeShift[0] << ',' << AgeShift[1] << ',' << AgeShift[2] << ");\n";
        out << R"(
float materialDetailWeight(float cyclesPerPixel) {
    return 1.0-smoothstep(materialFilterRange.x,materialFilterRange.y,cyclesPerPixel);
}
vec4 woodMaterialCoordinates(vec3 coordinate) {
    vec2 circle=coordinate.xy;
    float squared=dot(circle,circle);
    vec2 dx=dFdx(circle), dy=dFdy(circle);
    float angularFootprint=(abs(circle.x*dx.y-circle.y*dx.x)
        +abs(circle.x*dy.y-circle.y*dy.x))/max(squared,0.000001);
    // Root end caps can interpolate through the centre: suppress detail there and avoid atan(0,0).
    float angle=squared>0.000001 ? atan(circle.y,circle.x) : 0.0;
    return vec4(angle,coordinate.z,angularFootprint,fwidth(coordinate.z));
}
vec3 leafAgeTint(float t,float phase) {
    float age=smoothstep(leafAgeRange.x,leafAgeRange.y,t)*(1.0-smoothstep(0.16,0.42,phase));
    return vec3(1.0)+leafAgeShift*age;
}
)";
        return out.str();
    }
}
