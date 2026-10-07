#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace island::wildlife_render
{
    struct UnderwaterLightProfile
    {
        float waterTint = 0.0f;
        float brightness = 1.0f;
    };

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

    /** @brief One cheap shared sunlight/absorption sample for a complete aquatic animal. */
    [[nodiscard]] inline UnderwaterLightProfile UnderwaterLightAt(
        const Microsoft::Xna::Framework::Vector3& position,
        const float timeSeconds) noexcept
    {
        if (!std::isfinite(position.X) || !std::isfinite(position.Y)
            || !std::isfinite(position.Z) || !std::isfinite(timeSeconds)) {
            return {};
        }
        const float depth = std::clamp(-position.Y, 0.0f, 8.0f);
        const float normalizedDepth = std::clamp((depth - 0.35f) / 7.65f, 0.0f, 1.0f);
        const float depthBlend = normalizedDepth * normalizedDepth
            * (3.0f - 2.0f * normalizedDepth);
        const float crossingBands = 0.5f * (
            std::sin(position.X * 0.21f + position.Z * 0.13f + timeSeconds * 0.65f)
            + std::sin(position.Z * 0.27f - position.X * 0.09f - timeSeconds * 0.48f));
        const float crest = std::max(0.0f, crossingBands);
        const float caustic = crest * crest * (1.0f - depthBlend);
        return UnderwaterLightProfile{
            0.04f + depthBlend * 0.22f,
            0.98f - depthBlend * 0.10f + caustic * 0.14f,
        };
    }

    /** @brief Applies tropical-water absorption and the shared shallow sunlight sample. */
    [[nodiscard]] inline Microsoft::Xna::Framework::Color ApplyUnderwaterLight(
        const Microsoft::Xna::Framework::Color& color,
        const UnderwaterLightProfile& profile)
    {
        constexpr float waterR = 42.0f;
        constexpr float waterG = 121.0f;
        constexpr float waterB = 132.0f;
        const float tint = std::clamp(profile.waterTint, 0.0f, 1.0f);
        const auto blend = [tint](const std::uint8_t source, const float water) {
            return static_cast<int>(std::lround(
                static_cast<float>(source)
                + (water - static_cast<float>(source)) * tint));
        };
        return ScaleColor(
            Microsoft::Xna::Framework::Color(
                blend(color.getRProperty(), waterR),
                blend(color.getGProperty(), waterG),
                blend(color.getBProperty(), waterB),
                static_cast<int>(color.getAProperty())),
            profile.brightness);
    }
}
