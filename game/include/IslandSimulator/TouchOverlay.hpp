#pragma once

#include <memory>

#include "IslandSimulator/TouchControls.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class SpriteBatch;
    class Texture2D;
}

namespace island
{
    /** @brief Asset-free presentation for movement, look, and look-sensitivity touch controls. */
    class TouchOverlay final
    {
    public:
        explicit TouchOverlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~TouchOverlay();

        TouchOverlay(const TouchOverlay&) = delete;
        TouchOverlay& operator=(const TouchOverlay&) = delete;
        TouchOverlay(TouchOverlay&&) = delete;
        TouchOverlay& operator=(TouchOverlay&&) = delete;

        /** @brief Draws movement and look affordances over the completed 3D scene. */
        void Draw(const TouchControlState& state, float inputWidth, float inputHeight,
                  int viewportWidth, int viewportHeight, bool showIdleHints);

    private:
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ringTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> discTexture_;
    };
}
