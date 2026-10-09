#include "IslandSimulator/BoarCoatTexture.hpp"
#include <algorithm>
#include <cstddef>

namespace island
{
    namespace
    {
        constexpr std::uint32_t Mix(std::uint32_t x) noexcept
        {
            x ^= x >> 16;
            x *= 0x7feb352dU;
            x ^= x >> 15;
            x *= 0x846ca68bU;
            return x ^ (x >> 16);
        }
    }

    BoarCoatTexture::Pixels BoarCoatTexture::Generate() noexcept
    {
        Pixels pixels{};
        pixels.fill(255);
        // Independent overlapping fibers, mostly along the body. Integer-only generation is
        // identical on all targets. The color is albedo, not a painted moving-light highlight.
        constexpr int WrapRows = FurRows - 1;
        for (int y = 0; y < WrapRows; ++y) {
            for (int x = 0; x < Size; ++x) {
                const auto h = Mix(static_cast<std::uint32_t>(y * Size + x));
                pixels[static_cast<std::size_t>(y * Size + x)] =
                    static_cast<std::uint8_t>(226U + h % 13U);
            }
        }
        for (std::uint32_t strand = 0; strand < 1600; ++strand) {
            const auto h = Mix(strand + 0x19a7U);
            const int startX = static_cast<int>(h % Size);
            const int startY = static_cast<int>((h >> 8) % WrapRows);
            const int length = 4 + static_cast<int>((h >> 16) % 9U);
            const int bend = static_cast<int>((h >> 24) % 3U) - 1;
            for (int step = 0; step < length; ++step) {
                const int x = (startX + step) % Size;
                const int y = (startY + bend * step / (length / 2) + WrapRows) % WrapRows;
                const auto index = static_cast<std::size_t>(y * Size + x);
                const int value = 174 + static_cast<int>((h >> 20) % 28U)
                    + 30 * step / length;
                pixels[index] = static_cast<std::uint8_t>(
                    (static_cast<int>(pixels[index]) + value) / 2);
                const auto edge = static_cast<std::size_t>(((y + 1) % WrapRows) * Size + x);
                pixels[edge] = static_cast<std::uint8_t>(std::min(247, pixels[edge] + 7));
            }
        }
        // Duplicate the first circumferential row at the UV seam. The remaining eight rows
        // stay white, with a four-texel margin around the UV used by untextured anatomy.
        std::copy_n(pixels.begin(), Size, pixels.begin() + WrapRows * Size);
        return pixels;
    }
}
