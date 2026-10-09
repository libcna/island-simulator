#pragma once

#include <array>
#include <cstdint>

namespace island
{
    /** @brief Original opaque albedo detail; rows 0..119 wrap around the body. */
    struct BoarCoatTexture final
    {
        static constexpr int Size = 128;
        static constexpr int FurRows = 120;
        static constexpr float WhiteU = 0.5f / Size;
        static constexpr float WhiteV = 124.5f / Size;
        using Pixels = std::array<std::uint8_t, Size * Size>;

        /** @brief Grayscale multiplier, expanded to RGBA8 once during content loading. */
        [[nodiscard]] static Pixels Generate() noexcept;
        [[nodiscard]] static constexpr float BodyU(const float along) noexcept
        {
            return (0.5f + along * (Size - 1)) / Size;
        }
        [[nodiscard]] static constexpr float BodyV(const float around) noexcept
        {
            return (0.5f + around * (FurRows - 1)) / Size;
        }
    };
}
