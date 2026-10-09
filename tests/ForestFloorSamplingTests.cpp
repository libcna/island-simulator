#include "IslandSimulator/ForestFloorSampling.hpp"
#include "IslandSimulator/ForestFloorPixels.hpp"
#include "IslandSimulator/LeafSurfaceModel.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

int main() try
{
    using namespace island::forest_floor;
    const auto check = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    const auto value = [](const Windows& windows) {
        float total = 0;
        for (const auto& window : windows) {
            // Deliberately nonperiodic source with structure and unequal edges.
            total += window.weight * (window.u*0.3f + window.v*0.5f
                + 0.1f*std::sin(window.u*19.0f + window.v*7.0f));
        }
        return total;
    };
    const std::array<std::uint8_t, 24> pngHeader{
        137,80,78,71,13,10,26,10, 0,0,0,13, 73,72,68,82, 0,0,4,230, 0,0,4,230};
    check(ValidateSourceHeader(pngHeader) == 1254, "valid source PNG header rejected");
    for (int fault = 0; fault < 7; ++fault) {
        auto header = pngHeader;
        if (fault == 0) header[0] = 0;
        if (fault == 1) header[11] = 14;
        if (fault == 2) header[12] = 0;
        if (fault == 3) header[23] = 0;
        if (fault == 4) header[16] = header[20] = 255;
        if (fault == 5) { header[18] = header[22] = 0; header[19] = header[23] = 1; }
        bool rejected = false;
        try { (void)ValidateSourceHeader(std::span(header).first(fault == 6 ? 23 : 24)); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid or oversized PNG header accepted before decoding");
    }
    std::array<int, 4> rotations{};
    for (int x = -340; x <= 340; x += 2) for (int z = -340; z <= 340; z += 3) {
        const auto windows = SampleWindows(static_cast<float>(x)+0.173f,
                                          static_cast<float>(z)-0.291f);
        float sum = 0;
        for (const auto& window : windows) {
            check(std::isfinite(window.weight) && window.weight >= -1e-6f && window.weight <= 1.000001f,
                  "window weights must remain finite and bounded");
            check(window.u >= MinimumUv-0.0001f && window.u <= MaximumUv+0.0001f
                  && window.v >= MinimumUv-0.0001f && window.v <= MaximumUv+0.0001f,
                  "source sampling must remain away from all source edges");
            sum += window.weight;
            ++rotations[window.quarterTurns];
        }
        check(std::abs(sum-1.0f) < 0.000001f, "constant-colour reconstruction must preserve energy");
    }
    for (int count : rotations) check(count > 10000, "rotation variation missing");
    float maximumBoundaryDifference = 0;
    for (int edge = -1000; edge <= 1000; ++edge) {
        const float x = static_cast<float>(edge)*CellMetres;
        for (float t : {0.0f, 0.13f, 0.5f, 0.87f, 1.0f}) {
            const float z = (static_cast<float>(edge)+t)*CellMetres;
            for (bool transpose : {false, true}) {
                const auto sample = [&](float offset) {
                    return transpose ? SampleWindows(z, x+offset) : SampleWindows(x+offset, z);
                };
                const auto a = sample(-0.0001f), b = sample(0.0001f);
                const float difference = std::abs(value(a)-value(b));
                maximumBoundaryDifference = std::max(maximumBoundaryDifference, difference);
                check(difference < 0.0005f, "nonperiodic source jumps at lattice boundary");
                for (const auto& wa : a) if (wa.weight > 0.001f) {
                    const auto shared = std::find_if(b.begin(), b.end(), [&](const auto& wb) {
                        return wa.anchorX == wb.anchorX && wa.anchorZ == wb.anchorZ;
                    });
                    check(shared != b.end() && shared->quarterTurns == wa.quarterTurns,
                          "shared source identity changed across cells");
                }
            }
        }
    }
    for (const float bad : {std::numeric_limits<float>::quiet_NaN(),
                            std::numeric_limits<float>::infinity(), 1001.0f, -1001.0f}) {
        for (bool axis : {false, true}) {
            bool rejected = false;
            try { (void)SampleWindows(axis ? bad : 0, axis ? 0 : bad); }
            catch (const std::invalid_argument&) { rejected = true; }
            check(rejected, "invalid source-window query was accepted");
        }
    }
    const auto turn = Rotate(0.21f, -0.37f, 1);
    check(turn[0] == 0.37f && turn[1] == 0.21f, "quarter-turn convention changed");
    check(AnchorHash(0,0) == 3348245848U && AnchorHash(-19,37) == 810272989U
          && AnchorHash(19,-37) == 3446210204U && AnchorHash(-1000,-1000) == 804873385U,
          "stable signed-coordinate identity");
    const std::vector<Pixel> constant(9, Pixel{59,123,207,255});
    namespace leaf = island::leaf_surface;
    const leaf::Rgb mean{0.34f,0.49f,0.10f};
    check(leaf::Modulation(mean, mean) == leaf::Rgb{1,1,1},
          "last leaf mip must converge to neutral modulation");
    for (float along : {0.0f,0.1f,0.5f,0.9f,1.0f})
        for (float across : {-1.0f,-0.3f,0.0f,0.3f,1.0f}) {
            const auto a = leaf::Uv(along, across, 0.25f);
            const auto b = leaf::Uv(along, across, 0.75f);
            check(a[0] >= 0 && a[0] <= 1 && a[1] >= 0 && a[1] <= 1,
                  "leaf UV escaped the source square");
            check(std::abs(a[0]+b[0]-1) < 1e-6f && a[1] == b[1] && a[1] == 1-along,
                  "leaf mirror changed the midrib or root-tip orientation");
            if (across == 0) check(a[0] == 0.5f, "leaf midrib moved off centre");
        }
    for (float value : {0.0f,0.1f,0.5f,1.0f}) {
        const auto response = leaf::Modulation({value,value,value},mean);
        for (float c : response)
            check(c >= 1+leaf::Strength*(leaf::MinimumResponse-1)-1e-6f
                  && c <= 1+leaf::Strength*(leaf::MaximumResponse-1)+1e-6f,
                  "leaf colour response exceeds the declared bounds");
    }
    for (int fault = 0; fault < 9; ++fault) {
        bool rejected = false;
        const float nan = std::numeric_limits<float>::quiet_NaN();
        try {
            if (fault == 0) (void)leaf::Uv(nan,0,0);
            if (fault == 1) (void)leaf::Uv(-0.1f,0,0);
            if (fault == 2) (void)leaf::Uv(0,1.1f,0);
            if (fault == 3) (void)leaf::Uv(0,0,1.1f);
            if (fault == 4) (void)leaf::Modulation({nan,0,0},mean);
            if (fault == 5) (void)leaf::Modulation({1.1f,0,0},mean);
            if (fault == 6) (void)leaf::Modulation(mean,{0,0.5f,0.5f});
            if (fault == 7) (void)leaf::Modulation(mean,{0.5f,nan,0.5f});
            if (fault == 8) (void)leaf::Modulation(mean,{0.5f,0.5f,1.1f});
        } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid leaf material input accepted");
    }
    const auto leafMips = PrepareTexture(constant,3,leaf::TextureSize);
    std::size_t leafBytes = 0;
    for (const auto& mip : leafMips) {
        leafBytes += mip.pixels.size()*sizeof(Pixel);
        for (const auto& p : mip.pixels) check(p == constant[0], "leaf reduction changed constant RGB");
    }
    check(leafMips.size() == 9 && leafMips.back().size == 1 && leafBytes == leaf::FullMipBytes,
          "leaf material must upload exactly one complete 256-square mip chain");
    const auto mipChain = PrepareTexture(constant, 3);
    std::size_t bytes = 0;
    int size = RuntimeTextureSize;
    for (const auto& level : mipChain) {
        check(level.size == size && level.pixels.size() == static_cast<std::size_t>(size)*size,
              "complete floor-halved mip payload");
        for (const auto& pixel : level.pixels) check(pixel == constant[0], "constant source colour changed");
        bytes += level.pixels.size()*4;
        size = std::max(1,size/2);
    }
    check(mipChain.size() == 10 && mipChain.back().size == 1 && bytes == RuntimeFullMipBytes,
          "512-square full mip budget must be exact");
    // Odd source border: exact area reduction must not silently discard the last row/column.
    std::vector<Pixel> odd(9, Pixel{0,0,0,255});
    odd[8] = {180,90,45,255};
    const auto reduced = PrepareTexture(odd,3,2);
    check(reduced[0].pixels[3] == Pixel{80,40,20,255}
          && reduced[1].pixels[0] == Pixel{20,10,5,255}, "fractional source border lost energy");
    const std::vector<Pixel> checker{{0,0,0,255},{255,255,255,255},
                                     {255,255,255,255},{0,0,0,255}};
    check(PrepareTexture(checker,2,2)[1].pixels[0] == Pixel{128,128,128,255},
          "mip must use the declared RGB code-value convention");
    for (int fault = 0; fault < 5; ++fault) {
        auto pixels = odd;
        if (fault == 0) pixels.pop_back();
        if (fault == 1) pixels[0][3] = 254;
        bool rejected = false;
        try { (void)PrepareTexture(pixels, fault == 2 ? 2049 : 3,
                                  fault == 3 ? 3 : fault == 4 ? 2048 : 2); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid or transparent source accepted");
    }
    std::cout << "forest-floor source windows: bounded UVs, normalized weights, stable identities; "
              << "maximum boundary delta=" << maximumBoundaryDifference << '\n';
}
catch (const std::exception& error)
{
    std::cerr << "forest-floor reference test failed: " << error.what() << '\n';
    return 1;
}
