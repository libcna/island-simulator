#include "IslandSimulator/BoarCoatTexture.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

int main()
{
    using Coat = island::BoarCoatTexture;
    const auto pixels = Coat::Generate();
    const auto check = [](const bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    check(pixels == Coat::Generate(), "coat generation must be deterministic");
    long horizontal = 0, vertical = 0;
    for (int y = 0; y < Coat::Size; ++y) {
        for (int x = 0; x < Coat::Size; ++x) {
            const int value = pixels[static_cast<std::size_t>(y * Coat::Size + x)];
            if (y >= Coat::FurRows) check(value == 255, "anatomy padding must stay white");
            else check(value >= 174 && value <= 247, "coat albedo outside subtle detail range");
            if (y < Coat::FurRows - 1 && x < Coat::Size - 1) {
                horizontal += std::abs(value - pixels[static_cast<std::size_t>(y * Coat::Size + x + 1)]);
                vertical += std::abs(value - pixels[static_cast<std::size_t>((y + 1) * Coat::Size + x)]);
            }
        }
    }
    for (int x = 0; x < Coat::Size; ++x) {
        check(pixels[static_cast<std::size_t>(x)] ==
              pixels[static_cast<std::size_t>((Coat::FurRows - 1) * Coat::Size + x)],
              "body seam texels must match");
    }
    check(horizontal > 0 && vertical > horizontal * 3 / 2,
          "coat must have longitudinal strands, not isotropic noise");
    check(Coat::BodyU(0) > 0 && Coat::BodyU(1) < 1, "body U stays inside texel centres");
    check(Coat::BodyV(1) < static_cast<float>(Coat::FurRows) / Coat::Size,
          "fur UV must not sample white anatomy padding");
    check(Coat::WhiteV > static_cast<float>(Coat::FurRows + 2) / Coat::Size,
          "white sample requires filter padding");
    std::cout << "boar coat deterministic, padded and directional: "
              << horizontal << " / " << vertical << '\n';
}
