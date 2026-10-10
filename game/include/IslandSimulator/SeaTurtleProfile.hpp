#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace island::sea_turtle
{
    /**
     * @brief Renderer-independent green-turtle shape in mesh-local metres at scale 1.
     *
     * Local axes: x right, y up, z forward. u is normalized carapace length (-1 rear, +1 front),
     * s is normalized lateral position (-1 left rim, +1 right rim). Every function is bounded and
     * deterministic so the renderer, its bounding sphere and the tests share one reference.
     */
    inline constexpr float CarapaceRear = -0.56f, CarapaceFront = 0.50f;
    inline constexpr float CarapaceHalfWidth = 0.40f, CarapaceDome = 0.215f, CarapaceRimY = 0.0f;
    inline constexpr float PlastronDepth = 0.085f;
    inline constexpr float ReachLimit = 1.20f; // Every point stays inside scale * ReachLimit.

    // Column boundaries align with scute rows: marginal, two costal and one vertebral band.
    inline constexpr std::array<float, 9> CarapaceColumns{-1.0f, -0.78f, -0.50f, -0.22f, 0.0f,
                                                          0.22f, 0.50f, 0.78f, 1.0f};
    inline constexpr int CarapaceStations = 11; // Five vertebral scutes, two rows each.
    inline constexpr std::array<float, 5> PlastronColumns{-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};

    enum class Scute : unsigned char { Vertebral, Costal, Marginal };

    inline void RequireUnit(const float value, const char* what)
    {
        if (!std::isfinite(value) || value < -1.0f || value > 1.0f)
            throw std::invalid_argument(what);
    }

    /** @brief Teardrop outline: widest ahead of the middle, narrowing to a rounded rear. */
    inline float HalfWidth(const float u)
    {
        RequireUnit(u, "Carapace length coordinate must be normalized");
        const float ellipse = std::sqrt(std::max(0.0f, 1.0f - u * u));
        return CarapaceHalfWidth * std::pow(ellipse, 0.72f) * (1.0f + 0.10f * u);
    }

    inline float LengthToZ(const float u)
    {
        RequireUnit(u, "Carapace length coordinate must be normalized");
        return CarapaceRear + (CarapaceFront - CarapaceRear) * (u + 1.0f) * 0.5f;
    }

    /** @brief Domed carapace height above the rim; the ridge sits slightly ahead of centre. */
    inline float CarapaceHeight(const float u, const float s)
    {
        RequireUnit(u, "Carapace length coordinate must be normalized");
        RequireUnit(s, "Carapace lateral coordinate must be normalized");
        const float along = std::sqrt(std::max(0.0f, 1.0f - u * u));
        const float across = std::sqrt(std::max(0.0f, 1.0f - s * s));
        const float lift = 1.0f + 0.12f * u * (1.0f - u * u);
        return CarapaceRimY + CarapaceDome * std::pow(along, 0.85f) * std::pow(across, 0.78f) * lift;
    }

    /** @brief Shallow convex plastron below the rim. */
    inline float PlastronY(const float u, const float s)
    {
        RequireUnit(u, "Plastron length coordinate must be normalized");
        RequireUnit(s, "Plastron lateral coordinate must be normalized");
        const float along = std::sqrt(std::max(0.0f, 1.0f - u * u));
        return CarapaceRimY - PlastronDepth * std::pow(along, 0.6f) * (1.0f - 0.55f * s * s);
    }

    inline Scute ScuteAt(const float s)
    {
        RequireUnit(s, "Scute lateral coordinate must be normalized");
        const float a = std::abs(s);
        return a < 0.22f ? Scute::Vertebral : (a < 0.78f ? Scute::Costal : Scute::Marginal);
    }

    /** @brief Scute index along the shell for a facet centred at u (five vertebral rows). */
    inline int ScuteRow(const float u)
    {
        RequireUnit(u, "Scute length coordinate must be normalized");
        return std::clamp(static_cast<int>((u + 1.0f) * 2.5f), 0, 4);
    }

    struct FlipperPoint { float x, y, z; };

    /**
     * @brief Front paddle centreline and half-width at fraction t of its length.
     *
     * stroke is the simulation's sine in [-1, 1]: positive lifts the paddle and sweeps it
     * forward, negative is the power stroke down and back. The tip lags the root so the paddle
     * bends through the stroke instead of rotating rigidly.
     */
    inline FlipperPoint FrontFlipper(const float side, const float t, const float stroke,
                                     float& halfWidth)
    {
        if (!std::isfinite(t) || t < 0.0f || t > 1.0f) throw std::invalid_argument("Flipper fraction");
        RequireUnit(stroke, "Flipper stroke must be normalized");
        if (side != 1.0f && side != -1.0f) throw std::invalid_argument("Flipper side");
        constexpr float length = 0.74f;
        const float lag = std::pow(t, 1.6f);
        const float flap = stroke * 0.55f - stroke * 0.30f * lag;      // radians about forward axis
        // Power stroke (negative) sweeps back, recovery (positive) brings the paddle forward.
        const float sweep = -0.32f + stroke * 0.22f * (0.4f + 0.6f * lag); // radians, negative = rear
        const float reach = length * t;
        const float outward = reach * std::cos(sweep) * std::cos(flap);
        const float back = reach * std::sin(sweep);
        const float rise = reach * std::sin(flap) * std::cos(sweep);
        halfWidth = 0.095f * (0.95f + 0.75f * std::sin(3.14159265f * std::min(1.0f, t * 1.1f + 0.1f)))
            * (1.0f - 0.55f * t * t);
        return {side * (0.36f + outward), -0.01f + rise, 0.30f + back};
    }

    /** @brief Small rear paddle; stroke is a reduced, delayed echo of the front stroke. */
    inline FlipperPoint RearFlipper(const float side, const float t, const float stroke,
                                    float& halfWidth)
    {
        if (!std::isfinite(t) || t < 0.0f || t > 1.0f) throw std::invalid_argument("Flipper fraction");
        RequireUnit(stroke, "Flipper stroke must be normalized");
        if (side != 1.0f && side != -1.0f) throw std::invalid_argument("Flipper side");
        constexpr float length = 0.30f;
        const float flap = -stroke * 0.18f * t;
        const float reach = length * t;
        halfWidth = 0.075f * (0.7f + 0.6f * std::sin(3.14159265f * std::min(1.0f, t + 0.15f)));
        return {side * (0.28f + reach * 0.62f), -0.045f + reach * std::sin(flap),
                -0.40f - reach * 0.78f};
    }
}
