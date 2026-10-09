#include "IslandSimulator/ForestFloorPixels.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace island::forest_floor
{
    int ValidateSourceHeader(const std::span<const std::uint8_t> header)
    {
        constexpr std::array<std::uint8_t, 16> prefix{
            137,80,78,71,13,10,26,10, 0,0,0,13, 73,72,68,82};
        if (header.size() < 24 || !std::equal(prefix.begin(), prefix.end(), header.begin()))
            throw std::invalid_argument("Opaque image source requires a PNG IHDR header");
        const auto word = [&](std::size_t at) {
            return (std::uint32_t{header[at]} << 24U) | (std::uint32_t{header[at+1]} << 16U)
                | (std::uint32_t{header[at+2]} << 8U) | std::uint32_t{header[at+3]};
        };
        const auto width = word(16), height = word(20);
        if (width < 2 || width > 2048 || height != width)
            throw std::invalid_argument("Opaque PNG must be square and 2..2048 pixels wide");
        return static_cast<int>(width);
    }

    std::vector<MipLevel> PrepareTexture(const std::span<const Pixel> source, const int sourceSize,
                                         const int targetSize)
    {
        if (sourceSize < 2 || sourceSize > 2048
            || source.size() != static_cast<std::size_t>(sourceSize)*sourceSize
            || targetSize < 2 || targetSize > 1024
            || !std::has_single_bit(static_cast<unsigned>(targetSize)))
            throw std::invalid_argument("Invalid bounded opaque texture dimensions");
        if (std::any_of(source.begin(), source.end(), [](const Pixel& p) { return p[3] != 255; }))
            throw std::invalid_argument("Image source must be opaque RGBA8");
        std::vector<MipLevel> result;
        result.reserve(std::bit_width(static_cast<unsigned>(targetSize)));
        result.push_back({targetSize, std::vector<Pixel>(static_cast<std::size_t>(targetSize)*targetSize)});
        const double scale = static_cast<double>(sourceSize)/targetSize;
        // Integrate exact source-texel overlap, including the fractional last row/column. A
        // bilinear shrink can alias the fine litter into the permanent base-level texture.
        for (int y = 0; y < targetSize; ++y) for (int x = 0; x < targetSize; ++x) {
            const double left = x*scale, right = (x+1)*scale;
            const double top = y*scale, bottom = (y+1)*scale;
            std::array<double, 3> sum{};
            for (int sy = static_cast<int>(std::floor(top)); sy < std::min(sourceSize, static_cast<int>(std::ceil(bottom))); ++sy) {
                const double dy = std::min(bottom, sy+1.0)-std::max(top, static_cast<double>(sy));
                for (int sx = static_cast<int>(std::floor(left)); sx < std::min(sourceSize, static_cast<int>(std::ceil(right))); ++sx) {
                    const double dx = std::min(right, sx+1.0)-std::max(left, static_cast<double>(sx));
                    const auto& pixel = source[static_cast<std::size_t>(sy)*sourceSize+sx];
                    for (int c = 0; c < 3; ++c) sum[c] += pixel[c]*dx*dy;
                }
            }
            auto& pixel = result[0].pixels[static_cast<std::size_t>(y)*targetSize+x];
            for (int c = 0; c < 3; ++c)
                pixel[c] = static_cast<std::uint8_t>(std::lround(std::clamp(sum[c]/(scale*scale), 0.0, 255.0)));
            pixel[3] = 255;
        }
        while (result.back().size > 1) {
            const auto& previous = result.back();
            const int size = previous.size/2;
            MipLevel next{size, std::vector<Pixel>(static_cast<std::size_t>(size)*size)};
            for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
                auto& pixel = next.pixels[static_cast<std::size_t>(y)*size+x];
                for (int c = 0; c < 3; ++c) {
                    unsigned sum = 0;
                    for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx)
                        sum += previous.pixels[static_cast<std::size_t>(y*2+dy)*previous.size+x*2+dx][c];
                    pixel[c] = static_cast<std::uint8_t>((sum+2)/4);
                }
                pixel[3] = 255;
            }
            result.push_back(std::move(next));
        }
        return result;
    }
}
