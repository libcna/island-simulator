#pragma once

#include <memory>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class SpriteBatch;
    class Texture2D;
}

namespace island
{
    /** @brief Procedural low-opacity edge veil shown only while near-surface swimming. */
    class WaterViewOverlay final
    {
    public:
        explicit WaterViewOverlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        ~WaterViewOverlay();

        WaterViewOverlay(const WaterViewOverlay&) = delete;
        WaterViewOverlay& operator=(const WaterViewOverlay&) = delete;
        WaterViewOverlay(WaterViewOverlay&&) = delete;
        WaterViewOverlay& operator=(WaterViewOverlay&&) = delete;

        /** @brief Draws one stretched procedural veil when intensity is nonzero. */
        void Draw(float intensity, int viewportWidth, int viewportHeight);

        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> veilTexture_;
        int drawCallCount_ = 0;
    };
}
