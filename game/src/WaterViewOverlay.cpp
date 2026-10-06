#include "IslandSimulator/WaterViewOverlay.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int TextureWidth = 160;
        constexpr int TextureHeight = 90;

        [[nodiscard]] float SmoothStep(const float value) noexcept
        {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] std::unique_ptr<Texture2D> CreateVeilTexture(GraphicsDevice& device)
        {
            std::vector<Color> pixels(
                static_cast<std::size_t>(TextureWidth * TextureHeight));
            for (int y = 0; y < TextureHeight; ++y) {
                const float v = (static_cast<float>(y) + 0.5f)
                    / static_cast<float>(TextureHeight);
                for (int x = 0; x < TextureWidth; ++x) {
                    const float u = (static_cast<float>(x) + 0.5f)
                        / static_cast<float>(TextureWidth);
                    const float horizontalEdge = SmoothStep(
                        (std::abs(u - 0.5f) - 0.31f) / 0.19f);
                    const float verticalEdge = SmoothStep(
                        (std::abs(v - 0.5f) - 0.29f) / 0.21f);
                    const float edge = 1.0f
                        - (1.0f - horizontalEdge) * (1.0f - verticalEdge);
                    const float depthVeil = SmoothStep((v - 0.18f) / 0.82f);
                    const float surfaceLight = 1.0f - SmoothStep((v - 0.04f) / 0.52f);
                    const float firstRipple = 0.5f + 0.5f * std::sin(
                        u * 31.0f + v * 8.0f + std::sin(u * 8.0f) * 0.9f);
                    const float secondRipple = 0.5f + 0.5f * std::sin(
                        u * -19.0f + v * 13.0f + std::sin(v * 11.0f) * 0.7f);
                    const float surfaceScatter = SmoothStep(
                        (firstRipple * 0.68f + secondRipple * 0.32f - 0.63f) / 0.29f)
                        * surfaceLight;
                    const float rayField = 0.5f + 0.5f * std::sin(
                        u * 12.0f + v * 2.1f + std::sin(u * 3.7f) * 1.4f);
                    const float lightRay = SmoothStep((rayField - 0.70f) / 0.25f)
                        * surfaceLight * (0.45f + 0.55f * (1.0f - v));
                    const float alpha = std::clamp(
                        8.0f + edge * 25.0f + depthVeil * 15.0f
                            + surfaceScatter * 10.0f + lightRay * 5.0f,
                        0.0f, 63.0f);
                    const int alphaByte = static_cast<int>(std::lround(alpha));
                    const auto premultiply = [alphaByte](const float channel) {
                        return static_cast<int>(std::lround(
                            channel * static_cast<float>(alphaByte) / 255.0f));
                    };
                    const float lightLift = surfaceScatter * 34.0f + lightRay * 18.0f;
                    pixels[static_cast<std::size_t>(y * TextureWidth + x)] = Color(
                        premultiply(48.0f + lightLift),
                        premultiply(142.0f + depthVeil * 8.0f + lightLift * 0.72f),
                        premultiply(157.0f + depthVeil * 12.0f + lightLift * 0.84f),
                        alphaByte);
                }
            }

            auto texture = std::make_unique<Texture2D>(device, TextureWidth, TextureHeight);
            texture->SetData(pixels.data(), static_cast<int>(pixels.size()));
            return texture;
        }
    }

    WaterViewOverlay::WaterViewOverlay(GraphicsDevice& device)
        : spriteBatch_(std::make_unique<SpriteBatch>(device)),
          veilTexture_(CreateVeilTexture(device))
    {
    }

    WaterViewOverlay::~WaterViewOverlay() = default;

    void WaterViewOverlay::Draw(const float intensity, const float currentStrength,
                                const int viewportWidth, const int viewportHeight)
    {
        const float safeIntensity = std::clamp(intensity, 0.0f, 1.0f);
        const float safeCurrent = std::isfinite(currentStrength)
            ? std::clamp(currentStrength, 0.0f, 1.0f)
            : 0.0f;
        drawCallCount_ = 0;
        if (safeIntensity <= 0.001f || viewportWidth <= 0 || viewportHeight <= 0) return;

        const int alpha = static_cast<int>(std::lround(safeIntensity * 255.0f));
        const int red = static_cast<int>(std::lround(
            static_cast<float>(alpha) * (1.0f - safeCurrent * 0.12f)));
        const int green = static_cast<int>(std::lround(
            static_cast<float>(alpha) * (1.0f - safeCurrent * 0.03f)));
        spriteBatch_->Begin();
        spriteBatch_->Draw(*veilTexture_, Rectangle(0, 0, viewportWidth, viewportHeight),
                           Color(red, green, alpha, alpha));
        spriteBatch_->End();
        drawCallCount_ = 1;
    }
}
