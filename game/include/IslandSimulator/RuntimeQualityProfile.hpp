#pragma once

#include <cstddef>
#include <string_view>
#include "IslandSimulator/GeneratedTextureBudget.hpp"

namespace island
{
    /** @brief Centralized portable rendering and resource policy for one runtime class. */
    struct RuntimeQualityProfile final
    {
        std::string_view name;
        float grassMaximumDistance = 0.0f;
        float contactShadowDistanceScale = 0.0f;
        float wildlifeMaximumDistanceScale = 0.0f;
        int oceanQuadsPerAxis = 0;
        std::size_t textureByteBudget = 0U;
        int maximumPlayingAudioSources = 0;
        int preferredBackBufferSamples = 0;

        /** @brief Full portable quality used by Linux OpenGL and browser WebGL. */
        [[nodiscard]] static constexpr RuntimeQualityProfile Full() noexcept
        {
            return {"full", 135.0f, 1.0f, 1.0f, 160,
                    GeneratedTextureBudget::CanopyLightRgba8Bytes
                    + GeneratedTextureBudget::BoarCoatRgba8Bytes
                    + GeneratedTextureBudget::SnakeScaleRgba8Bytes
                    + GeneratedTextureBudget::GrassHeightRgba8Bytes
                    + forest_floor::RuntimeFullMipBytes + leaf_surface::FullMipBytes
                    + GeneratedTextureBudget::MonkeyFurMipBytes + GeneratedTextureBudget::HutThatchMipBytes
                    + GeneratedTextureBudget::GoatFurMipBytes
                    + GeneratedTextureBudget::SpiderSetaeMipBytes
                    + 160U * 1024U, 6, 4};
        }

        /**
         * @brief Balanced Android default that preserves close detail and the full soundscape.
         *
         * Distance-dependent work, the ocean grid and MSAA samples are reduced. One 4 MiB canopy field
         * is shared by terrain/grass; small UI/water textures and six audio voices are retained.
         */
        [[nodiscard]] static constexpr RuntimeQualityProfile Mobile() noexcept
        {
            return {"mobile", 112.0f, 0.72f, 0.82f, 128,
                    GeneratedTextureBudget::CanopyLightRgba8Bytes
                    + GeneratedTextureBudget::BoarCoatRgba8Bytes
                    + GeneratedTextureBudget::SnakeScaleRgba8Bytes
                    + GeneratedTextureBudget::GrassHeightRgba8Bytes
                    + forest_floor::RuntimeFullMipBytes + leaf_surface::FullMipBytes
                    + GeneratedTextureBudget::MonkeyFurMipBytes + GeneratedTextureBudget::HutThatchMipBytes
                    + GeneratedTextureBudget::GoatFurMipBytes
                    + GeneratedTextureBudget::SpiderSetaeMipBytes
                    + 160U * 1024U, 6, 2};
        }

        /** @brief Reports whether every field is safe for direct renderer consumption. */
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return !name.empty()
                && grassMaximumDistance > 0.0f
                && contactShadowDistanceScale > 0.0f
                && contactShadowDistanceScale <= 1.0f
                && wildlifeMaximumDistanceScale > 0.0f
                && wildlifeMaximumDistanceScale <= 1.0f
                && oceanQuadsPerAxis > 0
                && oceanQuadsPerAxis <= 255
                && textureByteBudget > 0U
                && maximumPlayingAudioSources > 0
                && (preferredBackBufferSamples == 0 || preferredBackBufferSamples == 2
                    || preferredBackBufferSamples == 4);
        }
    };

    static_assert(RuntimeQualityProfile::Full().IsValid());
    static_assert(RuntimeQualityProfile::Mobile().IsValid());
}
