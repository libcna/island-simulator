#include "IslandSimulator/ForestFloorSampling.hpp"
#include "IslandSimulator/ForestFloorPixels.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    struct Image { int width, height; std::vector<std::uint8_t> pixels; };

    Image ReadPpm(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("Cannot open P6 source image");
        const auto token = [&]() {
            input >> std::ws;
            while (input.peek() == '#') {
                input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                input >> std::ws;
            }
            std::string value;
            if (!(input >> value)) throw std::runtime_error("Truncated P6 header");
            return value;
        };
        if (token() != "P6") throw std::invalid_argument("Expected RGB8 P6 source");
        const auto dimension = [&]() {
            const auto value = token();
            std::size_t consumed = 0;
            const int result = std::stoi(value, &consumed);
            if (consumed != value.size() || result < 2 || result > 2048)
                throw std::invalid_argument("Source dimension must be in 2..2048");
            return result;
        };
        Image image{dimension(), dimension(), {}};
        if (token() != "255") throw std::invalid_argument("Expected 8-bit P6 channels");
        const int separator = input.get();
        if (separator != ' ' && separator != '\n' && separator != '\r' && separator != '\t')
            throw std::invalid_argument("Missing P6 payload separator");
        if (separator == '\r' && input.peek() == '\n') input.get();
        image.pixels.resize(static_cast<std::size_t>(image.width)*image.height*3);
        if (!input.read(reinterpret_cast<char*>(image.pixels.data()),
                        static_cast<std::streamsize>(image.pixels.size()))
            || input.peek() != std::char_traits<char>::eof())
            throw std::invalid_argument("Source P6 payload length mismatch");
        return image;
    }

    std::array<float, 3> Bilinear(const island::forest_floor::MipLevel& image, const float u, const float v)
    {
        const float x = u*image.size-0.5f, y = v*image.size-0.5f;
        const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
        const auto pixel = [&](int px, int py, int channel) {
            px = std::clamp(px, 0, image.size-1);
            py = std::clamp(py, 0, image.size-1);
            return static_cast<float>(image.pixels[static_cast<std::size_t>(py)*image.size+px][channel]);
        };
        std::array<float, 3> result{};
        for (int channel = 0; channel < 3; ++channel) {
            result[channel] = std::lerp(std::lerp(pixel(ix,iy,channel), pixel(ix+1,iy,channel), x-ix),
                std::lerp(pixel(ix,iy+1,channel), pixel(ix+1,iy+1,channel), x-ix), y-iy);
        }
        return result;
    }
}

int main(int argc, char** argv)
{
    try {
        const bool reduceOnly = argc == 5 && std::string(argv[1]) == "--reduce";
        if (argc != 3 && !reduceOnly) throw std::invalid_argument(
            "Usage: island-forest-floor-preview SOURCE.ppm OUTPUT.ppm | --reduce SOURCE.ppm OUTPUT.ppm SIZE");
        const char* sourcePath = argv[reduceOnly ? 2 : 1];
        const char* outputPath = argv[reduceOnly ? 3 : 2];
        int targetSize = island::forest_floor::RuntimeTextureSize;
        if (reduceOnly) {
            std::size_t consumed = 0;
            targetSize = std::stoi(argv[4], &consumed);
            if (consumed != std::string(argv[4]).size())
                throw std::invalid_argument("Reduction size must be an integer");
        }
        if (std::filesystem::weakly_canonical(sourcePath) == std::filesystem::weakly_canonical(outputPath))
            throw std::invalid_argument("Preview cannot overwrite its source");
        const auto source = ReadPpm(sourcePath);
        if (source.width != source.height) throw std::invalid_argument("Forest-floor source must be square");
        std::vector<island::forest_floor::Pixel> rgba(static_cast<std::size_t>(source.width)*source.height);
        for (std::size_t index = 0; index < rgba.size(); ++index)
            rgba[index] = {source.pixels[index*3], source.pixels[index*3+1], source.pixels[index*3+2], 255};
        const auto mips = island::forest_floor::PrepareTexture(rgba, source.width, targetSize);
        const int Size = reduceOnly ? targetSize : 1024;
        constexpr float Metres = 4.0f;
        const float lod = std::clamp(std::log2(island::forest_floor::UvPerMetre
            *island::forest_floor::RuntimeTextureSize*Metres/Size), 0.0f, static_cast<float>(mips.size()-1));
        const auto lower = static_cast<std::size_t>(std::floor(lod));
        const auto upper = std::min(lower+1,mips.size()-1);
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(Size)*Size*3);
        if (reduceOnly) {
            for (std::size_t i = 0; i < mips[0].pixels.size(); ++i)
                for (std::size_t c = 0; c < 3; ++c) pixels[i*3+c] = mips[0].pixels[i][c];
        } else
        for (int y = 0; y < Size; ++y) for (int x = 0; x < Size; ++x) {
            const float worldX = ((x+0.5f)/Size-0.5f)*Metres;
            const float worldZ = ((y+0.5f)/Size-0.5f)*Metres;
            const auto windows = island::forest_floor::SampleWindows(worldX, worldZ);
            std::array<float, 3> result{};
            for (const auto& window : windows) {
                const auto a = Bilinear(mips[lower], window.u, window.v);
                const auto b = Bilinear(mips[upper], window.u, window.v);
                for (int c = 0; c < 3; ++c)
                    result[c] += std::lerp(a[c], b[c], lod-static_cast<float>(lower))*window.weight;
            }
            for (int c = 0; c < 3; ++c) pixels[(static_cast<std::size_t>(y)*Size+x)*3+c] =
                static_cast<std::uint8_t>(std::lround(std::clamp(result[c], 0.0f, 255.0f)));
        }
        std::ofstream output(outputPath, std::ios::binary);
        if (!output) throw std::runtime_error("Cannot open material preview output");
        output << "P6\n" << Size << ' ' << Size << "\n255\n";
        output.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
        output.close();
        if (!output) throw std::runtime_error("Cannot finish material preview output");
        if (reduceOnly) {
            std::cout << "material reduction oracle: " << targetSize << " square; runtime filter unchanged\n";
            return 0;
        }
        std::cout << "forest-floor CPU preview: 4x4 metres, 1024 square, four interior source windows; "
                     "512-square area-filtered source, complete RGB-code-value mips, trilinear LOD="
                  << lod << "; offline material diagnostic, not lighting/runtime qualification\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "forest-floor preview: " << error.what() << '\n';
        return 1;
    }
}
