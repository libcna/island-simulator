#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace island::wildlife_render
{
    inline const Microsoft::Xna::Framework::Vector3 FixedSunDirection =
        Microsoft::Xna::Framework::Vector3::Normalize(
            Microsoft::Xna::Framework::Vector3(
                environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ));

    [[nodiscard]] inline Microsoft::Xna::Framework::Color ScaleColor(
        const Microsoft::Xna::Framework::Color& color, const float brightness)
    {
        const float scale = std::clamp(brightness, 0.0f, 1.12f);
        const auto channel = [scale](const std::uint8_t value) {
            return static_cast<int>(std::lround(std::clamp(
                static_cast<float>(value) * scale, 0.0f, 255.0f)));
        };
        return Microsoft::Xna::Framework::Color(
            channel(color.getRProperty()), channel(color.getGProperty()),
            channel(color.getBProperty()), static_cast<int>(color.getAProperty()));
    }

    [[nodiscard]] inline Microsoft::Xna::Framework::Color FacetColor(
        const Microsoft::Xna::Framework::Vector3& a,
        const Microsoft::Xna::Framework::Vector3& b,
        const Microsoft::Xna::Framework::Vector3& c,
        const Microsoft::Xna::Framework::Color& color, const bool twoSided = false)
    {
        using Microsoft::Xna::Framework::Vector3;
        const Vector3 normal = Vector3::Normalize(Vector3::Cross(b - a, c - a));
        const float normalDotSun = Vector3::Dot(normal, FixedSunDirection);
        const float directLight = twoSided ? std::abs(normalDotSun)
                                           : std::max(0.0f, normalDotSun);
        const float skyFill = twoSided ? std::abs(normal.Y) : std::max(0.0f, normal.Y);
        return ScaleColor(color, 0.72f + directLight * 0.30f + skyFill * 0.06f);
    }
}
