#include "IslandSimulator/TouchOverlay.hpp"

#include "IslandSimulator/GeneratedTextureBudget.hpp"

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
        constexpr int TextureSize = GeneratedTextureBudget::TouchCircleSize;

        [[nodiscard]] Color PremultipliedColor(const int red, const int green,
                                               const int blue, const int alpha) noexcept
        {
            // SpriteBatch's default AlphaBlend state expects premultiplied source RGB. Keeping
            // transparent texels white would add an opaque rectangular quad on GLES as well as
            // producing bright fringes around the antialiased circles on every renderer.
            const auto premultiply = [alpha](const int channel) {
                return static_cast<int>(std::lround(
                    static_cast<float>(channel * alpha) / 255.0f));
            };
            return Color(premultiply(red), premultiply(green), premultiply(blue), alpha);
        }

        [[nodiscard]] std::unique_ptr<Texture2D> CreateCircleTexture(
            GraphicsDevice& device, const bool ring)
        {
            std::vector<Color> pixels(static_cast<std::size_t>(TextureSize * TextureSize),
                                      Color(0, 0, 0, 0));
            constexpr float centre = static_cast<float>(TextureSize - 1) * 0.5f;
            constexpr float outerRadius = centre - 1.0f;
            constexpr float innerRadius = outerRadius - 5.0f;
            for (int y = 0; y < TextureSize; ++y) {
                for (int x = 0; x < TextureSize; ++x) {
                    const float distance = std::hypot(static_cast<float>(x) - centre,
                                                      static_cast<float>(y) - centre);
                    const float outerCoverage = std::clamp(outerRadius + 1.0f - distance,
                                                           0.0f, 1.0f);
                    float coverage = outerCoverage;
                    if (ring) {
                        const float innerCoverage = std::clamp(distance - innerRadius,
                                                               0.0f, 1.0f);
                        coverage *= innerCoverage;
                    }
                    const int alpha = static_cast<int>(std::lround(coverage * 255.0f));
                    pixels[static_cast<std::size_t>(y * TextureSize + x)] =
                        PremultipliedColor(255, 255, 255, alpha);
                }
            }

            auto texture = std::make_unique<Texture2D>(device, TextureSize, TextureSize);
            texture->SetData(pixels.data(), static_cast<int>(pixels.size()));
            return texture;
        }

        [[nodiscard]] Rectangle CentredRectangle(const float x, const float y,
                                                  const float radius)
        {
            const int diameter = std::max(1, static_cast<int>(std::lround(radius * 2.0f)));
            return Rectangle(static_cast<int>(std::lround(x - radius)),
                             static_cast<int>(std::lround(y - radius)), diameter, diameter);
        }
    }

    TouchOverlay::TouchOverlay(GraphicsDevice& device)
        : spriteBatch_(std::make_unique<SpriteBatch>(device)),
          ringTexture_(CreateCircleTexture(device, true)),
          discTexture_(CreateCircleTexture(device, false))
    {
    }

    TouchOverlay::~TouchOverlay() = default;

    void TouchOverlay::Draw(const TouchControlState& state, const float inputWidth,
                            const float inputHeight, const int viewportWidth,
                            const int viewportHeight, const bool showIdleHints)
    {
        if (viewportWidth <= 0 || viewportHeight <= 0
            || (!showIdleHints && !state.movementActive && !state.lookActive)) {
            return;
        }

        const float safeInputWidth = std::max(1.0f, inputWidth);
        const float safeInputHeight = std::max(1.0f, inputHeight);
        const float scaleX = static_cast<float>(viewportWidth) / safeInputWidth;
        const float scaleY = static_cast<float>(viewportHeight) / safeInputHeight;
        const float radius = static_cast<float>(viewportHeight)
            * TouchControls::MovementRadiusFraction;
        const float knobRadius = radius * 0.34f;
        const float edgeInset = static_cast<float>(viewportHeight) * 0.04f;
        const float idleY = static_cast<float>(viewportHeight) - radius - edgeInset;
        const float idleLeftX = radius * 1.35f;
        const float idleRightX = static_cast<float>(viewportWidth) - radius * 1.35f;

        float movementCentreX = idleLeftX;
        float movementCentreY = idleY;
        float movementKnobX = movementCentreX;
        float movementKnobY = movementCentreY;
        if (state.movementActive) {
            const float rawCentreX = state.movementAnchorX * scaleX;
            const float rawCentreY = state.movementAnchorY * scaleY;
            movementCentreX = std::clamp(rawCentreX, radius + edgeInset,
                                         static_cast<float>(viewportWidth) - radius - edgeInset);
            movementCentreY = std::clamp(rawCentreY, radius + edgeInset,
                                         static_cast<float>(viewportHeight) - radius - edgeInset);
            float deltaX = state.movementTouchX * scaleX - rawCentreX;
            float deltaY = state.movementTouchY * scaleY - rawCentreY;
            const float distance = std::hypot(deltaX, deltaY);
            if (distance > radius) {
                deltaX *= radius / distance;
                deltaY *= radius / distance;
            }
            movementKnobX = movementCentreX + deltaX;
            movementKnobY = movementCentreY + deltaY;
        }

        float lookX = idleRightX;
        float lookY = idleY;
        if (state.lookActive) {
            const float lookRadius = radius * 0.82f;
            lookX = std::clamp(state.lookTouchX * scaleX, lookRadius + edgeInset,
                               static_cast<float>(viewportWidth) - lookRadius - edgeInset);
            lookY = std::clamp(state.lookTouchY * scaleY, lookRadius + edgeInset,
                               static_cast<float>(viewportHeight) - lookRadius - edgeInset);
        }

        const Color movementRingColor = state.runRequested
            ? PremultipliedColor(255, 208, 112, 168)
            : state.movementActive
                ? PremultipliedColor(99, 235, 190, 150)
                : PremultipliedColor(235, 250, 244, 62);
        const Color movementKnobColor = state.runRequested
            ? PremultipliedColor(255, 244, 205, 188)
            : state.movementActive
                ? PremultipliedColor(225, 255, 243, 170)
                : PremultipliedColor(235, 250, 244, 54);

        spriteBatch_->Begin();
        spriteBatch_->Draw(*ringTexture_, CentredRectangle(movementCentreX, movementCentreY, radius),
                           movementRingColor);
        spriteBatch_->Draw(*discTexture_, CentredRectangle(movementKnobX, movementKnobY, knobRadius),
                           movementKnobColor);
        spriteBatch_->Draw(*ringTexture_, CentredRectangle(lookX, lookY, radius * 0.82f),
                           state.lookActive
                               ? PremultipliedColor(110, 220, 255, 135)
                               : PremultipliedColor(235, 250, 255, 50));
        spriteBatch_->Draw(*discTexture_, CentredRectangle(lookX, lookY, knobRadius * 0.72f),
                           state.lookActive
                               ? PremultipliedColor(225, 250, 255, 150)
                               : PremultipliedColor(235, 250, 255, 40));
        spriteBatch_->End();
    }
}
