#include "IslandSimulator/GrassVisibility.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>
#include <vector>

int TestGrassVisibility()
{
    using island::GrassVisibility;
    int failures = 0;
    const auto check = [&](bool valid, std::string_view message) {
        if (!valid) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    for (float end : {8.0f, 112.0f, 135.0f}) {
        const float start = GrassVisibility::FadeStart(end);
        check(GrassVisibility::Weight(0, end) == 1
            && GrassVisibility::Weight(start, end) == 1
            && GrassVisibility::Weight(end, end) == 0
            && GrassVisibility::Weight(end + 1, end) == 0,
            "fade preserves near geometry and collapses the complete cluster at the endpoint");
        check(std::abs(GrassVisibility::Weight((start + end) * 0.5f, end) - 0.5f) < 0.000001f,
              "fade midpoint follows smoothstep");
        float previous = 1;
        for (int step = 0; step <= 2000; ++step) {
            const float weight = GrassVisibility::Weight(end * step / 2000.0f, end);
            check(weight >= 0 && weight <= previous, "fade is bounded and monotonic");
            previous = weight;
        }
        check(GrassVisibility::Weight(start + 0.01f, end) > 0.99999f
            && GrassVisibility::Weight(end - 0.01f, end) < 0.00001f,
            "fade has flat endpoint slopes, not a last-frame visibility step");
    }
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    for (float invalid : {-1.0f, 0.0f, nan, infinity}) {
        bool rejected = false;
        try { (void)GrassVisibility::FadeStart(invalid); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid draw radius cannot reach shader uniforms");
    }
    for (float invalid : {-1.0f, nan, infinity}) {
        bool rejected = false;
        try { (void)GrassVisibility::Weight(invalid, 135); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid reference distance is rejected");
    }
    check(GrassVisibility::NearestDistanceSquared(0, 0, -1, -2, 1, 2) == 0
        && GrassVisibility::NearestDistanceSquared(4, 6, -1, -2, 1, 2) == 25
        && GrassVisibility::NearestDistanceSquared(1, 2, 1, 2, 1, 2) == 0,
        "rectangle distance handles interior, corner, and single-anchor cells");
    check(GrassVisibility::CellIndex(-300, -300) == 0
        && GrassVisibility::CellIndex(300, 300) == GrassVisibility::CellCount - 1
        && GrassVisibility::CellIndex(0, 0) == 136,
        "shared batching preserves original cell boundaries");

    const island::IslandTerrain terrain;
    const island::GrassLayout layout(terrain);
    std::array<std::vector<const island::GrassPlacement*>, GrassVisibility::CellCount> cells;
    for (const auto& patch : layout.Patches()) cells[GrassVisibility::CellIndex(patch.x, patch.z)].push_back(&patch);
    // Probe 16 approach directions just beyond each real cell's old centre cutoff. This is
    // a horizontal coverage audit, not a claim about terrain occlusion or image visibility.
    for (float end : {112.0f, 135.0f}) {
        std::size_t affectedViews = 0, prematureAnchors = 0, rejectedViews = 0;
        std::size_t maximumPrematureAnchors = 0;
        float largestPrematureDistance = 0;
        for (const auto& cell : cells) {
            if (cell.empty()) continue;
            float minX = cell.front()->x, maxX = minX, minZ = cell.front()->z, maxZ = minZ;
            for (const auto* patch : cell) {
                minX = std::min(minX, patch->x); maxX = std::max(maxX, patch->x);
                minZ = std::min(minZ, patch->z); maxZ = std::max(maxZ, patch->z);
            }
            const float centreX = (minX + maxX) * 0.5f, centreZ = (minZ + maxZ) * 0.5f;
            for (int direction = 0; direction < 16; ++direction) {
                const float angle = direction * std::numbers::pi_v<float> / 8;
                for (float offset : {0.01f, 27.0f}) {
                    const float x = centreX + std::cos(angle) * (end + offset);
                    const float z = centreZ + std::sin(angle) * (end + offset);
                    const float nearest = GrassVisibility::NearestDistanceSquared(x, z, minX, minZ, maxX, maxZ);
                    const bool rejected = nearest > end * end;
                    rejectedViews += rejected;
                    std::size_t live = 0;
                    for (const auto* patch : cell) {
                        const float distance = std::hypot(x - patch->x, z - patch->z);
                        check(nearest <= distance * distance + 0.02f,
                              "cell rejection is conservative for every actual anchor");
                        const float weight = GrassVisibility::Weight(distance, end);
                        check(!rejected || weight == 0,
                              "no rejected cell contains nonzero-weight geometry");
                        if (weight > 0) {
                            ++live;
                            largestPrematureDistance = std::max(largestPrematureDistance, end - distance);
                        }
                    }
                    if (offset < 1) {
                        affectedViews += live > 0;
                        prematureAnchors += live;
                        maximumPrematureAnchors = std::max(maximumPrematureAnchors, live);
                    }
                }
            }
        }
        check(affectedViews > 0 && rejectedViews > 0 && maximumPrematureAnchors > 1,
              "real layout audit exercises premature whole-cell drops and fully faded rejection");
        std::cout << "Grass distance audit radius=" << end << "m affected-cell-views=" << affectedViews
                  << " prematurely-dropped-anchor-observations=" << prematureAnchors
                  << " maximum-per-cell-view=" << maximumPrematureAnchors
                  << " maximum-premature-distance=" << largestPrematureDistance << "m\n";
    }
    return failures;
}
