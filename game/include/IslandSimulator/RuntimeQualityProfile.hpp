#pragma once

#include <cstddef>
#include <string_view>

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
        std::size_t generatedTextureByteBudget = 0U;
        int maximumPlayingAudioSources = 0;

        /** @brief Full portable quality used by Linux OpenGL and browser WebGL. */
        [[nodiscard]] static constexpr RuntimeQualityProfile Full() noexcept
        {
            return {"full", 135.0f, 1.0f, 1.0f, 160, 160U * 1024U, 6};
        }

        /**
         * @brief Balanced Android default that preserves close detail and the full soundscape.
         *
         * Only distance-dependent work and the ocean grid are reduced. The generated textures
         * are already tiny, and six bounded audio voices cost less than removing atmosphere.
         */
        [[nodiscard]] static constexpr RuntimeQualityProfile Mobile() noexcept
        {
            return {"mobile", 112.0f, 0.72f, 0.82f, 128, 160U * 1024U, 6};
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
                && generatedTextureByteBudget > 0U
                && maximumPlayingAudioSources > 0;
        }
    };

    static_assert(RuntimeQualityProfile::Full().IsValid());
    static_assert(RuntimeQualityProfile::Mobile().IsValid());
}
